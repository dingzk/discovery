//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/configservice.h"
#include "common/json.h"
#include <pthread.h>

static const char* kConfigServicePath = "/1/config/service";
static const char* kLookup = "?action=lookup&group=";
static const char* kGetGroup = "?action=getgroup";
static const int kDefaultTimeOut = 1000;

ConfigService::ConfigService(const char *host, HashTable *ht): host_(host), http_(new Http), ht_(ht) {}

bool ConfigService::lookup(const char *group, const char *key, std::string &result)
{
    result.clear();
    if (group == nullptr) {
        return false;
    }
    std::map<uint64_t, Response> resp;
    std::string url("http://");
    url += host_;
    url += kConfigServicePath;
    url += kLookup;
    url += group;
    if (key != nullptr) {
        url += "&key=";
        url += key;
    }
    http_->add_url(url, "GET", kDefaultTimeOut);
    int ret = http_->do_call(resp);
    if (ret <= 0 || resp.empty()) {
        return false;
    }
    if (resp.begin()->second.ret_code == 200) {
        rapidjson::Document root;
        if (!Json::decode(resp.begin()->second.body, root)) {
            std::cout << "parse json error!" << resp.begin()->second.ret_code << resp.begin()->second.body << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        if (atoi(root["code"].GetString()) != 200) {
            return false;
        }
        assert(root.HasMember("body"));
        Json::encode(root["body"], result);
        return true;
    }

    return false;
}

bool ConfigService::lookup(const char *group, std::string &result)
{
    return lookup(group, nullptr, result);
}

bool ConfigService::get_group(std::vector<std::string> &result)
{
    std::string url("http://");
    url += host_;
    url += kConfigServicePath;
    url += kGetGroup;

    std::map<uint64_t, Response> resp;
    http_->add_url(url, "GET", kDefaultTimeOut);
    int ret = http_->do_call(resp);
    if (ret <= 0 || resp.empty()) {
        return false;
    }
    if (resp.begin()->second.ret_code == 200) {
        rapidjson::Document root;
        if (!Json::decode(resp.begin()->second.body, root)) {
            std::cout << "parse json error! msg: " << resp.begin()->second.ret_code << resp.begin()->second.body << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        if (atoi(root["code"].GetString()) != 200) {
            return false;
        }
        assert(root.HasMember("body"));
        const rapidjson::Value& v = root["body"]["groups"];
        for (rapidjson::Value::ConstValueIterator itr = v.Begin(); itr != v.End(); ++itr) {
            result.push_back(itr->GetString());
        }
        return true;
    }

    return false;
}

bool ConfigService::fetch()
{
    std::lock_guard<std::mutex> guard(lock_);
    std::vector<std::string> groups;
    bool ret = get_group(groups);
    if (!ret || groups.empty()) {
        return false;
    }
    std::string result;
    for (auto iter = groups.begin(); iter != groups.end(); ++ iter) {
        ret = lookup(iter->c_str(), result);
        if (!ret || result.empty()) {
            continue;
        }
        rapidjson::Document root;
        if (!Json::decode(result, root)) {
            std::cout << "parse json error! msg: " << result << std::endl;
            continue;
        }
        assert(root.HasMember("groupId"));
        assert(root.HasMember("sign"));
        const char *key = root["groupId"].GetString();
        const char *sign = root["sign"].GetString();

        std::string value;
        Json::encode(root["nodes"], value);

        Bucket *b = hash_find_bucket(ht_, key, strlen(key));
        if (b && memcmp(sign, b->sign, strlen(sign)) == 0) {
            continue;
        }
        hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value.c_str(), value.size());
    }

    return true;
}

static void *timer_do(void *arg)
{
    pthread_detach(pthread_self());

    ConfigService *configservice = (ConfigService *)arg;

    while (true) {
        configservice->fetch();
        sleep(5);
        std::cout << "timer fetch config ..." << std::endl;
    }

    return nullptr;
}

bool ConfigService::watch()
{
    pthread_t pid;
    fetch();
    pthread_create(&pid, NULL, timer_do, (void *)this);

    return true;
}

bool ConfigService::find(const char *group, std::string &result)
{
    result.clear();
    if (group == nullptr) {
        return false;
    }
    Bucket *b = hash_find_bucket(ht_, group, strlen(group));
    if (b) {
        result.assign(b->val->data, b->val->len);
        return true;
    }

    return false;
}

bool ConfigService::find(const char *group, const char *key, std::string &result)
{
    result.clear();
    if (group == nullptr || key == nullptr) {
        return false;
    }
    Bucket *b = hash_find_bucket(ht_, group, strlen(group));
    if (!b) {
        return false;
    }
    rapidjson::Document root;
    if (!Json::decode(b->val->data, b->val->len, root)) {
        std::cout << "parse json error! msg: " << std::endl;
        return false;
    }
    for (rapidjson::Value::ConstValueIterator itr = root.Begin(); itr != root.End(); ++itr) {
        const rapidjson::Value &tmp = *itr;
        const char *k = tmp["key"].GetString();
        const char *v = tmp["value"].GetString();
        if (k != nullptr && memcmp(k, key, strlen(key)) == 0) {
            result.assign(v);
            return true;
        }
    }

    return false;
}

