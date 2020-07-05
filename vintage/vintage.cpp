//
// Created by zhenkai on 2020/6/27.
//

#include "vintage/vintage.h"
#include "serializer/MD5/md5.h"

#include <string.h>

// configservice
static const char* kConfigServicePath = "/1/config/service";
static const char* kLookupGroup = "?action=lookup&group=";
static const char* kGetGroup = "?action=getgroup";

// namingservice
static const char* kNamingServicePath = "/naming/service";
static const char* kNamingAdminPath = "/naming/admin";
static const char* kLookupService = "?action=lookup";
static const char* kLookupServiceForUpdate = "?action=lookupforupdate";
static const char* kGetService = "?action=getservice";

static const int kDefaultTimeOut = 1000;

Vintage::Vintage(const char *host): host_(host), http_(new Http) {}

std::string Vintage::gen_hash_key(const char *key1, const char *key2)
{
    return std::string(key1) + "_" + key2;
}

void Vintage::build_key(const char *key, char *build_key)
{
    if (strlen(key) >= MAX_KEY_LEN) {
        gen_md5(key, build_key);
    } else {
        strcpy(build_key, key);
    }
}

bool Vintage::lookup(const char *service, const char *cluster, const char *sign, rapidjson::Document &root)
{
    if (service == nullptr) {
        return false;
    }
    std::map<uint64_t, Response> resp;
    std::string url("http://");
    url += host_;
    url += kNamingServicePath;
    if (sign != nullptr) {
        url += kLookupServiceForUpdate;
        url += "&sign=";
        url += sign;
    } else {
        url += kLookupService;
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
        if(!Json::decode(resp.begin()->second.body, root)) {
            std::cout << "parse json error " << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        return atoi(root["code"].GetString()) == 200;
    } else if (resp.begin()->second.ret_code == 304) {
        return false;
    }

    return false;
}

bool Vintage::lookup(const char *group, const char *key, rapidjson::Document &root)
{
    if (group == nullptr) {
        return false;
    }
    std::map<uint64_t, Response> resp;
    std::string url("http://");
    url += host_;
    url += kConfigServicePath;
    url += kLookupGroup;
    url += group;
    if (key != nullptr) {
        url += "&key=";
        url += key;
    }
    http_->add_url(url, "GET", kDefaultTimeOut);
    int ret = http_->do_call(resp);
    if (ret <= 0 || resp.empty()) {
        return false;
    }
    if (resp.begin()->second.ret_code == 200) {
        if (!Json::decode(resp.begin()->second.body, root)) {
            std::cout << "parse json error!" << resp.begin()->second.ret_code << resp.begin()->second.body << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        return atoi(root["code"].GetString()) == 200;
    }

    return false;
}

bool Vintage::get_group(std::vector<std::string> &result)
{
    std::string url("http://");
    url += host_;
    url += kConfigServicePath;
    url += kGetGroup;

    std::map<uint64_t, Response> resp;
    http_->add_url(url, "GET", kDefaultTimeOut);
    int ret = http_->do_call(resp);
    if (ret <= 0 || resp.empty()) {
        return false;
    }
    if (resp.begin()->second.ret_code == 200) {
        rapidjson::Document root;
        if (!Json::decode(resp.begin()->second.body, root)) {
            std::cout << "parse json error! msg: " << resp.begin()->second.ret_code << resp.begin()->second.body << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        if (atoi(root["code"].GetString()) != 200) {
            return false;
        }
        assert(root.HasMember("body"));
        const rapidjson::Value& v = root["body"]["groups"];
        for (rapidjson::Value::ConstValueIterator itr = v.Begin(); itr != v.End(); ++itr) {
            result.emplace_back(itr->GetString());
        }
        return true;
    }

    return false;
}

bool Vintage::get_service(std::vector<std::string> &result)
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
        rapidjson::Document root;
        if (!Json::decode(resp.begin()->second.body, root)) {
            std::cout << "parse json error " << std::endl;
            return false;
        }
        assert(root.HasMember("code"));
        if (atoi(root["code"].GetString()) != 200) {
            return false;
        }
        assert(root.HasMember("body"));
        rapidjson::Document doc;
        Json::decode(root["body"].GetString(), doc);
        const rapidjson::Value& v = doc["services"];
        for (rapidjson::Value::ConstValueIterator itr = v.Begin(); itr != v.End(); ++itr) {
            result.emplace_back((*itr)["name"].GetString());
        }
        return true;
    }

    return false;
}
