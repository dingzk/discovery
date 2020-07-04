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
    if (groups.empty()) {
        return false;
    }
    bool ret;
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

bool ConfigService::add_watch(const char *group)
{
    std::lock_guard<std::mutex> guard(lock_);
    groups.insert(group);
}

bool ConfigService::add_watch(const char *group, const char *key)
{

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

static void *scan_msq(void *arg)
{
    pthread_detach(pthread_self());

    auto *configservice = (ConfigService *)arg;
    int msqid = create_msq(MSQ_FILE);
    int nrecv;
    while (true) {
        msqid = init_msq(MSQ_FILE);
        char recv[MAX_MSG_LEN]  = {0};
        nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, (void *) MSG_TYPE_CONFIG_SERVICE);
        if (nrecv > 0) {
            configservice->add_watch(recv);
            std::cout << "add config ..." << recv << std::endl;
        }
        sleep(1);
    }

}

bool ConfigService::watch()
{
    pthread_t pid, pid_msg;
    fetch();
    pthread_create(&pid, nullptr, timer_do, (void *)this);
    pthread_create(&pid_msg, nullptr, scan_msq, (void *)this);

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

