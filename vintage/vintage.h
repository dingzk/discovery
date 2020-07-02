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

class Vintage
{
private:
    const char *host_;
    std::shared_ptr<Http> http_;

protected:
    static void build_key(const char *key, char *build_key);
    bool lookup(const char *group, const char *key, rapidjson::Document &root);
    bool lookup(const char *service, const char *cluster, const char *sign, rapidjson::Document &root);

public:
    explicit Vintage(const char *host);
    bool get_group(std::vector<std::string> &result);
    bool get_service(std::vector<std::string> &result);

};


#endif //DISCOVERY_VINTAGE_H
