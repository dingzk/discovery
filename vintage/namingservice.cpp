//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/namingservice.h"
#include "serializer/MD5/md5.h"
#include "serializer/rapidjson/document.h"
#include "serializer/rapidjson/writer.h"
#include "serializer/rapidjson/stringbuffer.h"

static const char* kNamingServicePath = "/naming/service";
static const char* kNamingAdminPath = "/naming/admin";
static const char* kLookup = "?action=lookup";
static const char* kLookupForUpdate = "?action=lookupforupdate";
static const char* kGetService = "?action=getservice";
static const int kDefaultTimeOut = 1000;

NamingService::NamingService(const char *host, HashTable *ht): host_(host), http_(new Http), ht_(ht) {}

bool NamingService::lookup(const char *service, const char *cluster, std::string &result)
{
    return lookupforupdate(service, cluster, nullptr, result);
}

bool NamingService::lookupforupdate(const char *service, const char *cluster, const char *sign, std::string &result)
{
    if (service == nullptr) {
        return false;
    }
    std::map<uint64_t, Response> resp;
    std::string url("http://");
    url += host_;
    url += kNamingServicePath;
    if (sign != nullptr) {
        url += kLookupForUpdate;
        url += "&sign=";
        url += sign;
    } else {
        url += kLookup;
    }
    url += "&service=";
    url += service;
    url += "&cluster=";
    url += cluster;
    url += "%2Fservice";
    http_->add_url(url, "GET", kDefaultTimeOut);
    int ret = http_->do_call(resp);
    if (ret <= 0) {
        return false;
    }
    if (resp.begin()->second.ret_code == 200) {
        rapidjson::Document root;
        if (root.Parse(resp.begin()->second.body.c_str()).HasParseError()) {
            std::cout << "parse json error " << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        if (atoi(root["code"].GetString()) != 200) {
            return false;
        }
        result.assign(root["body"].GetString());
        return true;
    } else if (resp.begin()->second.ret_code == 304) {
        result.clear();
        return true;
    }

    return false;
}

bool NamingService::get_service(std::vector<std::string> &result)
{
    std::string url("http://");
    url += host_;
    url += kNamingAdminPath;
    url += kGetService;

    std::map<uint64_t, Response> resp;
    http_->add_url(url, "GET", kDefaultTimeOut);
    int ret = http_->do_call(resp);
    if (ret <= 0) {
        return false;
    }
    if (resp.begin()->second.ret_code == 200) {
        rapidjson::Document root;
        if (root.Parse(resp.begin()->second.body.c_str()).HasParseError()) {
            std::cout << "parse json error " << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        if (atoi(root["code"].GetString()) != 200) {
            return false;
        }
        assert(root.HasMember("body"));
        rapidjson::Document doc;
        doc.Parse(root["body"].GetString());
        const rapidjson::Value& v = doc["services"];
        for (rapidjson::Value::ConstValueIterator itr = v.Begin(); itr != v.End(); ++itr) {
            const rapidjson::Value &tmp = *itr;
            result.push_back(tmp["name"].GetString());
        }
        return true;
    }

    return false;
}

void NamingService::build_key(const char *key, char *build_key)
{
    if (strlen(key) >= MAX_KEY_LEN) {
        gen_md5(key, build_key);
    } else {
        strcpy(build_key, key);
    }
}

bool NamingService::fetch()
{
    std::lock_guard<std::mutex> guard(lock_);
    if (service_clusters.empty()) {
        return false;
    }
    std::string serv_clu, service, cluster, result;
    rapidjson::Document root;
    for (auto iter = service_clusters.begin(); iter != service_clusters.end(); ++iter) {
        char key[MAX_KEY_LEN] = {0};
        serv_clu = iter->c_str();
        int pos = serv_clu.find_first_of("_");
        service = serv_clu.substr(0, pos);
        cluster = serv_clu.substr(pos + 1);

        result.clear();
        bool ret = lookup(service.c_str(), cluster.c_str(), result);
        if (!ret || result.empty()) {
            continue;
        }
        if (root.Parse(result.c_str()).HasParseError()) {
            std::cout << "parse json error " << std::endl;
            continue;
        }
        assert(root.HasMember("sign"));
        assert(root.HasMember("nodes"));
        const char *sign = root["sign"].GetString();

        rapidjson::Value v(root["nodes"].GetObject());
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        v.Accept(writer);
        const char *value = buffer.GetString();

        build_key(serv_clu.c_str(), key);

        Bucket *b = hash_find_bucket(ht_, key, strlen(key));
        if (b && memcmp(sign, b->sign, strlen(sign)) == 0) {
            continue;
        }

        std::cout << "lookup : " << value << std::endl;

        hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value, strlen(value));
    }

    return true;
}

bool NamingService::fetchforupdate()
{
    std::lock_guard<std::mutex> guard(lock_);
    if (service_clusters.empty()) {
        return false;
    }
    std::string serv_clu, service, cluster, result;
    rapidjson::Document root;
    for (auto iter = service_clusters.begin(); iter != service_clusters.end(); ++iter) {
        char key[MAX_KEY_LEN] = {0};
        serv_clu = iter->c_str();
        int pos = serv_clu.find_first_of("_");
        service = serv_clu.substr(0, pos);
        cluster = serv_clu.substr(pos + 1);

        build_key(serv_clu.c_str(), key);
        Bucket *b = hash_find_bucket(ht_, key, strlen(key));
        if (!b) {
            continue;
        }
        result.clear();
        bool ret = lookupforupdate(service.c_str(), cluster.c_str(), b->sign, result);
        if (!ret || result.empty()) {
            std::cout << "not modified!" << std::endl;
            continue;
        }
        if (root.Parse(result.c_str()).HasParseError()) {
            std::cout << "parse json error!" << std::endl;
            continue;
        }
        assert(root.HasMember("sign"));
        assert(root.HasMember("nodes"));
        const char *sign = root["sign"].GetString();

        rapidjson::Value v(root["nodes"].GetObject());
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        v.Accept(writer);
        const char *value = buffer.GetString();

        std::cout << "lookupforupdate : " << value << std::endl;

        hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value, strlen(value));

        return true;
    }

    return true;
}

bool NamingService::add_watch(const char *service, const char *cluster)
{
    std::lock_guard<std::mutex> guard(lock_);
    std::string serv(service);
    std::string clu(cluster);
    service_clusters.push_back(serv + "_" + clu);

    return true;
}

static void *timer_do(void *arg)
{
    pthread_detach(pthread_self());

    NamingService *nameservice = (NamingService *)arg;

    while (true) {
        nameservice->fetchforupdate();
        sleep(5);
        std::cout << "timer fetch naming ..." << std::endl;
    }

    return nullptr;
}

bool NamingService::watch()
{
    pthread_t pid;
    fetch();
    pthread_create(&pid, NULL, timer_do, (void *)this);

    return true;
}

bool NamingService::find(const char *service, const char *cluster, std::string &result)
{
    result.clear();
    if (service == nullptr || cluster == nullptr) {
        return false;
    }

    std::string serv(service);
    std::string clu(cluster);
    std::string ori_key = serv + "_" + clu;

    char key[MAX_KEY_LEN] = {0};
    build_key(ori_key.c_str(), key);
    Bucket *b = hash_find_bucket(ht_, key, strlen(key));
    if (b) {
        result.assign(b->val->data, b->val->len);
        return true;
    }

    return false;
}