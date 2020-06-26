//
// Created by zhenkai on 2020/6/23.
//

#include "namingservice.h"
#include "serializer/cJSON/cJSON.h"

static const char* kNamingServicePath = "/naming/service";
static const char* kNamingAdminPath = "/naming/admin";
static const char* kLookup = "?action=lookup";
static const char* kLookupForUpdate = "?action=lookupforupdate";
static const char* kGetService = "?action=getservice";
static const int kDefaultTimeOut = 1000;

NamingService::NamingService(const char *host): host_(host), http_(new Http) {}

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
            return false;
        }
        cJSON* body_json = cJSON_GetObjectItem(root, "body");

        char *body = cJSON_Print(body_json);
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


}