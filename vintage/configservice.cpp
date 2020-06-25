//
// Created by zhenkai on 2020/6/23.
//

#include "vintage/configservice.h"
#include "serializer/cJSON/cJSON.h"

static const char* PATH_CONFIG_SERVICE = "/1/config/service";
static const char* CONFIG_LOOKUP = "?action=lookup&group=%s";
static const char* CONFIG_LOOKUP_KEY = "?action=lookup&group=%s&key=%s";
static const char* CONFIG_GET_GROUP = "?action=getgroup";

ConfigService::ConfigService(const char *host): host_(host), http_(new Http) {}

std::string ConfigService::lookup(std::string &group, std::string key)
{
    if (group.empty()) {
        return "";
    }
    std::map<uint64_t, Response> resp;
    std::string url("http://");
    url += host_;
    url += PATH_CONFIG_SERVICE;
    url += "?action=lookup&group=";
    url += group;
    if (!key.empty()) {
        url += "&key=";
        url += key;
    }
    http_->add_url(url, "GET", 1000);
    int ret = http_->do_call(resp);
    if (ret > 0 && resp.begin()->second.ret_code == 200) {
        cJSON* root = cJSON_Parse(resp.begin()->second.body.c_str());
        if (root == nullptr) {
            return "";
        }
        cJSON* code_c = cJSON_GetObjectItem(root, "code");
        char *code = cJSON_GetStringValue(code_c);
        if (atoi(code) != 200) {
            std::cout << "get status code not 200" << std::endl;
            return "";
        }

        cJSON* body_c = cJSON_GetObjectItem(root, "body");

        cJSON *groupid_c = cJSON_GetObjectItem(body_c, "groupId");
        const char *groupid = cJSON_GetStringValue(groupid_c);

        cJSON *sign_c = cJSON_GetObjectItem(body_c, "sign");
        const char *sign = cJSON_GetStringValue(sign_c);
        //char *sign = cJSON_Print(sign_c); // free(sign);

        cJSON *nodes_c = cJSON_GetObjectItem(body_c, "nodes");

        char *nodes = cJSON_Print(nodes_c);
        std::cout << nodes << std::endl;
        free(nodes);

        cJSON *element = NULL;
        cJSON_ArrayForEach(element, nodes_c) {
            cJSON *key_c = cJSON_GetObjectItem(element, "key");
            cJSON *value_c = cJSON_GetObjectItem(element, "value");
            std::cout << "key: " << cJSON_GetStringValue(key_c) << " value: " << cJSON_GetStringValue(value_c) << std::endl;
        }
        cJSON_Delete(root);
    }

    return "";

}

std::string ConfigService::get_group()
{
    std::map<uint64_t, Response> resp;

    std::string url("http://");
    url += host_;
    url += PATH_CONFIG_SERVICE;
    url += "?action=getgroup";

    http_->add_url(url, "GET", 1000);
    http_->do_call(resp);


}

