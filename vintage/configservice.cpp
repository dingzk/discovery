//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/configservice.h"
#include "serializer/cJSON/cJSON.h"

static const char* kConfigServicePath = "/1/config/service";
static const char* kLookup = "?action=lookup&group=";
static const char* kGetGroup = "?action=getgroup";
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

        char *body = cJSON_Print(body_json);
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


