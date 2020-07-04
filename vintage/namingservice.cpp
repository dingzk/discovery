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
    if (service_clusters.empty()) {
        return false;
    }
    std::string service, cluster;
    rapidjson::Document root;
    for (const auto & servClu : service_clusters) {
        char key[MAX_KEY_LEN] = {0};
        int pos = servClu.find_first_of("_");
        service = servClu.substr(0, pos);
        cluster = servClu.substr(pos + 1);

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

        build_key(servClu.c_str(), key);
        Bucket *b = hash_find_bucket(ht_, key, strlen(key));
        if (b && memcmp(sign, b->sign, strlen(sign)) == 0) {
            continue;
        }

        std::cout << "lookup : " << value << std::endl;

        hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value.c_str(), value.size());
    }

    return true;
}

bool NamingService::fetchforupdate()
{
    std::lock_guard<std::mutex> guard(lock_);
    if (service_clusters.empty()) {
        return false;
    }
    std::string service, cluster;
    rapidjson::Document root;
    bool ret;
    for (const auto & servClu : service_clusters) {
        char key[MAX_KEY_LEN] = {0};
        int pos = servClu.find_first_of("_");
        service = servClu.substr(0, pos);
        cluster = servClu.substr(pos + 1);

        build_key(servClu.c_str(), key);
        Bucket *b = hash_find_bucket(ht_, key, strlen(key));
        root.SetNull();
        if (!b) {
            ret = lookup(service.c_str(), cluster.c_str(), nullptr, root);
            std::cout << "add new naming !" << std::endl;
        } else {
            ret = lookup(service.c_str(), cluster.c_str(), b->sign, root);
        }
        if (!ret) {
            std::cout << "not modified!" << std::endl;
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

        std::cout << "lookupforupdate : " << value << std::endl;

        hash_add_or_update_bucket(ht_, sign, strlen(sign), key, strlen(key), value.c_str(), value.size());
    }

    return true;
}

bool NamingService::add_watch(std::string service, std::string cluster)
{
    std::lock_guard<std::mutex> guard(lock_);
    service_clusters.insert(service + "_" + cluster);

    return true;
}

bool NamingService::add_watch(const char *service_cluster)
{
    std::lock_guard<std::mutex> guard(lock_);
    service_clusters.insert(service_cluster);

    return true;
}

static void *timer_do(void *arg)
{
    pthread_detach(pthread_self());

    auto *nameservice = (NamingService *)arg;

    while (true) {
        nameservice->fetchforupdate();
        sleep(5);
        std::cout << "timer fetch naming ..." << std::endl;
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
            std::cout << "add naming ..." << recv << std::endl;
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

bool NamingService::find(std::string service, std::string cluster, std::string &result)
{
    result.clear();
    if (service.empty() || cluster.empty()) {
        return false;
    }

    std::string ori_key = service + "_" + cluster;

    char key[MAX_KEY_LEN] = {0};
    build_key(ori_key.c_str(), key);
    Bucket *b = hash_find_bucket(ht_, key, strlen(key));
    if (b) {
        result.assign(b->val->data, b->val->len);
        return true;
    }

    int msqid = init_msq(MSQ_FILE);
    send_msg(msqid, ori_key.c_str(), ori_key.size(), (void *) MSG_TYPE_NAMING_SERVICE);

    return false;
}