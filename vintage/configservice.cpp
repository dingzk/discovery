//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/configservice.h"
#include "common/json.h"

#include <pthread.h>

ConfigService::ConfigService(const char *host, HashTable *ht): Vintage(host), ht_(ht) {}

bool ConfigService::fetch()
{
    std::lock_guard<std::mutex> guard(lock_);
    std::vector<std::string> groups;
    bool ret = get_group(groups);
    if (!ret || groups.empty()) {
        return false;
    }
    for (auto & group : groups) {
        rapidjson::Document root;
        ret = lookup(group.c_str(), nullptr, root);
        if (!ret) {
            continue;
        }
        const rapidjson::Value &body = root["body"];
        if (body.IsNull()) {
            continue;
        }

        assert(body.HasMember("groupId"));
        assert(body.HasMember("sign"));
        assert(body.HasMember("nodes"));
        const char *ori_key = body["groupId"].GetString();
        const char *sign = body["sign"].GetString();

        std::string value;
        Json::encode(body["nodes"], value);

        char key[MAX_KEY_LEN] = {0};
        build_key(ori_key, key);

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

    auto *configservice = (ConfigService *)arg;

    while (true) {
        configservice->fetch();
        sleep(5);
        std::cout << "timer fetch config ..." << std::endl;
    }

}

bool ConfigService::watch()
{
    pthread_t pid;
    fetch();
    pthread_create(&pid, nullptr, timer_do, (void *)this);

    return true;
}

bool ConfigService::find(const char *group, std::string &result)
{
    result.clear();
    if (group == nullptr) {
        return false;
    }
    char key[MAX_KEY_LEN] = {0};
    build_key(group, key);

    Bucket *b = hash_find_bucket(ht_, key, strlen(key));
    if (b) {
        result.assign(b->val->data, b->val->len);
        return true;
    }

    return false;
}

bool ConfigService::find(const char *group, const char *raw_key, std::string &result)
{
    result.clear();
    if (group == nullptr || raw_key == nullptr) {
        return false;
    }
    char key[MAX_KEY_LEN] = {0};
    build_key(raw_key, key);

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

