//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/namingservice.h"
#include "serializer/cJSON/cJSON.h"
#include "serializer/MD5/md5.h"


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
        cJSON* root = cJSON_Parse(resp.begin()->second.body.c_str());
        if (root == nullptr) {
            return false;
        }
        cJSON* code_c = cJSON_GetObjectItem(root, "code");
        char *code = cJSON_GetStringValue(code_c);
        if (atoi(code) != 200) {
            cJSON_Delete(root);
            return false;
        }
        cJSON* body_json = cJSON_GetObjectItem(root, "body");

        char *body = cJSON_PrintUnformatted(body_json);
        result = body;
        free(body);

        cJSON_Delete(root);
        return true;
    } else if (resp.begin()->second.ret_code == 304) {
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
        cJSON* root = cJSON_Parse(resp.begin()->second.body.c_str());
        if (root == nullptr) {
            return false;
        }
        cJSON* code_c = cJSON_GetObjectItem(root, "code");
        char *code = cJSON_GetStringValue(code_c);
        if (atoi(code) != 200) {
            cJSON_Delete(root);
            return false;
        }
        cJSON* body_json = cJSON_GetObjectItem(root, "body");
        cJSON *services_json = cJSON_GetObjectItem(body_json, "services");

        cJSON *element = nullptr;
        cJSON_ArrayForEach(element, services_json) {
            result.push_back(cJSON_GetStringValue(element));
        }
        cJSON_Delete(root);
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
    for (auto iter = service_clusters.begin(); iter != service_clusters.end(); ++iter) {
        char key[MAX_KEY_LEN] = {0};
        std::string serv_clu = iter->c_str();
        int pos = serv_clu.find_first_of("_");
        std::string service = serv_clu.substr(0, pos);
        std::string cluster = serv_clu.substr(pos + 1);

        std::string result;
        bool ret = lookup(service.c_str(), cluster.c_str(), result);
        if (!ret) {
            continue;
        }
        std::cout << "lookup: " << result << std::endl;
        cJSON* root = cJSON_Parse(result.c_str());
        if (root == nullptr) {
            continue;
        }
        cJSON *sign_json = cJSON_GetObjectItemCaseSensitive(root, "sign");
        cJSON *nodes_json = cJSON_GetObjectItem(root, "nodes");
        char *value = cJSON_PrintUnformatted(nodes_json);
        char *sign = cJSON_GetStringValue(sign_json);
        if (!sign) {
            free(value);
            cJSON_Delete(root);
            continue;
        }
        std::cout << "lookup: sign" << sign << std::endl;

        build_key(serv_clu.c_str(), key);
        Bucket *b = hash_find_bucket(ht_, key, strlen(key));
        if (b && memcmp(sign, b->sign, strlen(sign)) == 0) {
            free(value);
            cJSON_Delete(root);
            continue;
        }
        hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value, strlen(value));

        free(value);
        cJSON_Delete(root);
    }

    return true;
}

bool NamingService::fetchforupdate()
{
    std::lock_guard<std::mutex> guard(lock_);
    if (service_clusters.empty()) {
        return false;
    }
    for (auto iter = service_clusters.begin(); iter != service_clusters.end(); ++iter) {
        char key[MAX_KEY_LEN] = {0};
        std::string serv_clu = iter->c_str();
        int pos = serv_clu.find_first_of("_");
        std::string service = serv_clu.substr(0, pos);
        std::string cluster = serv_clu.substr(pos + 1);

        build_key(serv_clu.c_str(), key);
        Bucket *b = hash_find_bucket(ht_, key, strlen(key));
        if (!b) {
            continue;
        }
        std::string result;
        bool ret = lookupforupdate(service.c_str(), cluster.c_str(), b->sign, result);
        if (ret && !result.empty()) {
            cJSON* root = cJSON_Parse(result.c_str());
            if (root == nullptr) {
                continue;
            }

            std::cout << "lookupforupdate:" << result << std::endl;

            cJSON *sign_json = cJSON_GetObjectItemCaseSensitive(root, "sign");
            cJSON *nodes_json = cJSON_GetObjectItem(root, "nodes");
            char *value = cJSON_PrintUnformatted(nodes_json);

            char *sign = cJSON_GetStringValue(sign_json);
            if (!sign) {
                free(value);
                cJSON_Delete(root);
                continue;
            }
            hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value, strlen(value));

            free(value);
            cJSON_Delete(root);
        }
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