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
        const char *groupId = body["groupId"].GetString();
        const char *sign = body["sign"].GetString();

        const rapidjson::Value &nodes = body["nodes"];
        for (rapidjson::Value::ConstValueIterator itr = nodes.Begin(); itr != nodes.End(); ++itr) {
            const rapidjson::Value &tmp = *itr;
            const char *k = tmp["key"].GetString();
            const char *v = tmp["value"].GetString();

            std::string gen_key  = gen_hash_key(groupId, k);
            char key[MAX_KEY_LEN] = {0};
            build_key(gen_key.c_str(), key);

            Bucket *b = hash_find_bucket(ht_, key, strlen(key));
            if (b && memcmp(sign, b->sign, strlen(sign)) == 0) {
                continue;
            }
            LOG_INFO("lookup: %s_%s value: %s", groupId, k, v);
            hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), v, strlen(v));
        }

    }

    return true;
}

bool ConfigService::add_watch(const char *group)
{
    std::lock_guard<std::mutex> guard(lock_);
    auto ret = groups.insert(group);
    if (ret.second) {
        LOG_INFO("add watch config: %s", group);
    }
}

bool ConfigService::add_watch(const char *group, const char *key)
{
    add_watch(group);
}

static void *timer_do(void *arg)
{
    pthread_detach(pthread_self());

    auto *configservice = (ConfigService *)arg;

    while (true) {
        configservice->fetch();
        sleep(5);
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
            continue;
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
    if (b && b->val != nullptr) {
        result.assign(b->val->data, b->val->len);
        return true;
    }

    int msqid = init_msq(MSQ_FILE);
    send_msg(msqid, group, strlen(group), (void *) MSG_TYPE_CONFIG_SERVICE);

    return false;
}

bool ConfigService::find(const char *group, const char *raw_key, std::string &result)
{
    result.clear();
    if (group == nullptr || raw_key == nullptr) {
        return false;
    }
    std::string gen_key = gen_hash_key(group, raw_key);
    char key[MAX_KEY_LEN] = {0};
    build_key(gen_key.c_str(), key);

    Bucket *b = hash_find_bucket(ht_, key, strlen(key));
    if (!b) {
        int msqid = init_msq(MSQ_FILE);
        send_msg(msqid, group, strlen(group), (void *) MSG_TYPE_CONFIG_SERVICE);
        return false;
    }
    if (b->val != nullptr) {
        result.assign(b->val->data, b->val->len);
        return true;
    }

    return false;
}

