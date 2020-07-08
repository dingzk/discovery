//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/namingservice.h"
#include "common/json.h"
#include "msq/msq.h"
#include <pthread.h>

NamingService::NamingService(const char *host, HashTable *ht): Vintage(host), ht_(ht) {}

bool NamingService::fetch()
{
    std::lock_guard<std::mutex> guard(lock_);
    if (serv_clusters.empty()) {
        return false;
    }
    std::string service;
    rapidjson::Document root;
    for (const auto & servClu : serv_clusters) {
        service = servClu.first;
        for (const auto &cluster : servClu.second) {
            root.SetNull();
            bool ret = lookup(service.c_str(), cluster.c_str(), nullptr, root);
            if (!ret) {
                continue;
            }
            const char *body_str = root["body"].GetString();

            rapidjson::Document body;
            Json::decode(body_str, body);
            assert(body.HasMember("sign"));
            assert(body.HasMember("nodes"));
            const char *sign = body["sign"].GetString();

            std::string value;
            Json::encode(body["nodes"], value);

            char key[MAX_KEY_LEN] = {0};
            std::string gen_key = gen_hash_key(service.c_str(), cluster.c_str());
            build_key(gen_key.c_str(), key);
            Bucket *b = hash_find_bucket(ht_, key, strlen(key));
            if (b && memcmp(sign, b->sign, strlen(sign)) == 0) {
                continue;
            }
            LOG_INFO("lookup: %s_%s value: %s sign: %s", service.c_str(), cluster.c_str(), value.c_str(), sign);
            hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value.c_str(), value.size());
        }
    }

    return true;
}

bool NamingService::fetchforupdate()
{
    std::lock_guard<std::mutex> guard(lock_);
    if (serv_clusters.empty()) {
        return false;
    }
    std::string service;
    rapidjson::Document root;
    bool ret;
    for (const auto & servClu : serv_clusters) {
        service = servClu.first;
        for (const auto &cluster : servClu.second) {
            char key[MAX_KEY_LEN] = {0};
            std::string gen_key = gen_hash_key(service.c_str(), cluster.c_str());
            build_key(gen_key.c_str(), key);
            Bucket *b = hash_find_bucket(ht_, key, strlen(key));
            root.SetNull();
            if (!b) {
                ret = lookup(service.c_str(), cluster.c_str(), nullptr, root);
                LOG_INFO("add watch naming: %s_%s", service.c_str(), cluster.c_str());
            } else {
                ret = lookup(service.c_str(), cluster.c_str(), b->sign, root);
            }
            if (!ret) {
                continue;
            }
            rapidjson::Document body;
            const char *body_str = root["body"].GetString();
            Json::decode(body_str, body);
            assert(body.HasMember("sign"));
            assert(body.HasMember("nodes"));
            const char *sign = body["sign"].GetString();

            std::string value;
            Json::encode(body["nodes"], value);

            LOG_INFO("lookupforupdate: %s_%s value: %s sign: %s", service.c_str(), cluster.c_str(), value.c_str(), sign);
            hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value.c_str(), value.size());
        }

    }

    return true;
}

bool NamingService::add_watch(std::string service, std::string cluster)
{
    std::lock_guard<std::mutex> guard(lock_);
    auto ret = serv_clusters.insert({service, {cluster}});
    if (ret.second) {
        LOG_INFO("add watch naming: %s_%s", service.c_str(), cluster.c_str());
    }

    return true;
}

bool NamingService::add_watch(std::string servCluster)
{
    std::lock_guard<std::mutex> guard(lock_);
    std::string service;
    std::string cluster;
    apart_hash_key(servCluster, service, cluster);
    auto ret = serv_clusters.insert({service, {cluster}});
    if (ret.second) {
        LOG_INFO("add watch naming: %s_%s", service.c_str(), cluster.c_str());
    }

    return true;
}

static void *timer_do(void *arg)
{
    pthread_detach(pthread_self());

    auto *nameservice = (NamingService *)arg;

    while (true) {
        nameservice->fetchforupdate();
        sleep(5);
    }

}

static void *scan_msq(void *arg)
{
    pthread_detach(pthread_self());

    auto *nameservice = (NamingService *)arg;
    int msqid = create_msq(MSQ_FILE);
    int nrecv;
    while (true) {
        msqid = init_msq(MSQ_FILE);
        char recv[MAX_MSG_LEN]  = {0};
        nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, (void *) MSG_TYPE_NAMING_SERVICE);
        if (nrecv > 0) {
            nameservice->add_watch(recv);
            continue;
        }
        sleep(1);
    }

}

bool NamingService::watch()
{
    pthread_t pid, pid_msg;
    fetch();
    pthread_create(&pid, nullptr, timer_do, (void *)this);
    pthread_create(&pid_msg, nullptr, scan_msq, (void *)this);

    return true;
}

bool NamingService::find(const char *service, const char *cluster, std::string &result)
{
    result.clear();
    if (service == nullptr || cluster == nullptr) {
        return false;
    }
    std::string gen_key = gen_hash_key(service, cluster);
    char key[MAX_KEY_LEN] = {0};
    build_key(gen_key.c_str(), key);
    Bucket *b = hash_find_bucket(ht_, key, strlen(key));
    if (b && b->val) {
        result.assign(b->val->data, b->val->len);
        return true;
    }

    int msqid = init_msq(MSQ_FILE);
    send_msg(msqid, gen_key.c_str(), gen_key.size(), (void *) MSG_TYPE_NAMING_SERVICE);

    return false;
}

bool NamingService::find(std::string service, std::string cluster, std::string &result)
{
    result.clear();
    if (service.empty() || cluster.empty()) {
        return false;
    }

    return find(service.c_str(), cluster.c_str(), result);
}