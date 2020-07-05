//
// Created by zhenkai on 2020/6/27.
//

#ifndef DISCOVERY_VINTAGE_H
#define DISCOVERY_VINTAGE_H

#include "http/http.h"
#include "storage/storage.h"
#include "common/json.h"

#include <mutex>
#include <string>
#include <memory>
#include <vector>

#include "msq/msq.h"

#define MSQ_FILE "/msq"
#define MSG_TYPE_CONFIG_SERVICE 0x10c
#define MSG_TYPE_NAMING_SERVICE 0x10d


class Vintage
{
private:
    const char *host_;
    std::shared_ptr<Http> http_;

protected:
    static std::string gen_hash_key(const char *key1, const char*key2);
    static bool apart_hash_key(const std::string &gen_key, std::string &key1, std::string &key2);
    static void build_key(const char *key, char *build_key);
    bool lookup(const char *group, const char *key, rapidjson::Document &root);
    bool lookup(const char *service, const char *cluster, const char *sign, rapidjson::Document &root);

public:
    explicit Vintage(const char *host);
    bool get_group(std::vector<std::string> &result);
    bool get_service(std::vector<std::string> &result);

};


#endif //DISCOVERY_VINTAGE_H
