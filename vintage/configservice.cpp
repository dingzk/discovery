//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/configservice.h"
#include "serializer/cJSON/cJSON.h"

static const char* PATH_CONFIG_SERVICE = "/1/config/service";
static const char* CONFIG_LOOKUP = "?action=lookup&group=%s";
static const char* CONFIG_LOOKUP_KEY = "?action=lookup&group=%s&key=%s";
static const char* CONFIG_GET_GROUP = "?action=getgroup";

static const int kDefaultTimeOut = 1000;

ConfigService::ConfigService(const char *host): host_(host), http_(new Http) {}

bool ConfigService::lookup(const char *group, const char *key, std::string &result)
{
    if (group == nullptr) {
        return false;
    }
    std::map<uint64_t, Response> resp;
    std::string url("http://");
    url += host_;
    url += PATH_CONFIG_SERVICE;
    url += "?action=lookup&group=";
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
            std::cout << "get status code not 200" << std::endl;
            return false;
        }

        cJSON* body_c = cJSON_GetObjectItem(root, "body");

        cJSON *groupid_c = cJSON_GetObjectItem(body_c, "groupId");
        const char *groupid = cJSON_GetStringValue(groupid_c);

        cJSON *sign_c = cJSON_GetObjectItem(body_c, "sign");
        const char *sign = cJSON_GetStringValue(sign_c);
        //char *sign = cJSON_Print(sign_c); // free(sign);

        cJSON *nodes_c = cJSON_GetObjectItem(body_c, "nodes");

        char *nodes = cJSON_Print(nodes_c);
        result = nodes;
        free(nodes);

        cJSON *element = NULL;
        cJSON_ArrayForEach(element, nodes_c) {
            cJSON *key_c = cJSON_GetObjectItem(element, "key");
            cJSON *value_c = cJSON_GetObjectItem(element, "value");
            std::cout << "key: " << cJSON_GetStringValue(key_c) << " value: " << cJSON_GetStringValue(value_c) << std::endl;
        }
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
    url += PATH_CONFIG_SERVICE;
    url += "?action=getgroup";

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
            std::cout << "get status code not 200" << std::endl;
            return false;
        }
        cJSON* body_c = cJSON_GetObjectItem(root, "body");
        cJSON *groups_c = cJSON_GetObjectItem(body_c, "groups");

        cJSON *element = nullptr;
        cJSON_ArrayForEach(element, groups_c) {
            result.push_back(cJSON_GetStringValue(element));
        }
        cJSON_Delete(root);
        return true;
    }

    return false;
}


