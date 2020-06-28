//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/configservice.h"
#include "serializer/cJSON/cJSON.h"
#include <pthread.h>

static const char* kConfigServicePath = "/1/config/service";
static const char* kLookup = "?action=lookup&group=";
static const char* kGetGroup = "?action=getgroup";
static const int kDefaultTimeOut = 1000;

ConfigService::ConfigService(const char *host, HashTable *ht): host_(host), http_(new Http), ht_(ht) {}

bool ConfigService::lookup(const char *group, const char *key, std::string &result)
{
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
            return false;
        }
        cJSON* body_json = cJSON_GetObjectItem(root, "body");

        char *body = cJSON_PrintUnformatted(body_json);
        result = body;
        free(body);

        cJSON_Delete(root);
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
        cJSON *groups_json = cJSON_GetObjectItem(body_json, "groups");

        cJSON *element = nullptr;
        cJSON_ArrayForEach(element, groups_json) {
            result.push_back(cJSON_GetStringValue(element));
        }
        cJSON_Delete(root);
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
    for (auto iter = groups.begin(); iter != groups.end(); ++ iter) {
        std::string result;
        ret = lookup(iter->c_str(), result);
        if (!ret) {
            continue;
        }
        cJSON* root = cJSON_Parse(result.c_str());
        if (root == nullptr) {
            continue;
        }
        cJSON *groupid_json = cJSON_GetObjectItemCaseSensitive(root, "groupId");
        cJSON *sign_json = cJSON_GetObjectItemCaseSensitive(root, "sign");
        cJSON *nodes_json = cJSON_GetObjectItem(root, "nodes");
        char *value = cJSON_PrintUnformatted(nodes_json);

        char *key = cJSON_GetStringValue(groupid_json);
        char *sign = cJSON_GetStringValue(sign_json);

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
    bool flag = false;
    Bucket *b = hash_find_bucket(ht_, group, strlen(group));
    if (b) {
        cJSON* root = cJSON_ParseWithLength(b->val->data, b->val->len);

        cJSON *element = nullptr;
        cJSON_ArrayForEach(element, root) {
            cJSON *k_json = cJSON_GetObjectItemCaseSensitive(element, "key");
            cJSON *v_json = cJSON_GetObjectItemCaseSensitive(element, "value");
            char *k = cJSON_GetStringValue(k_json);
            if (k != nullptr && memcmp(k, key, strlen(key)) == 0) {
                char *v = cJSON_GetStringValue(v_json);
                result.assign(v);
                flag = true;
                break;
            }
        }
        cJSON_Delete(root);
    }

    return flag;
}

