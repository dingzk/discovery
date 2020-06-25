//
// Created by zhenkai on 2020/6/23.
//

#ifndef DISCOVERY_CONFIGSERVICE_H
#define DISCOVERY_CONFIGSERVICE_H

#include "http/http.h"

#include <string>
#include <memory>
#include <vector>

class ConfigService
{
private:
    const char *host_;
    std::shared_ptr<Http> http_;

public:
    ConfigService(const char *host);
    bool lookup(const char *group, std::string &result);
    bool lookup(const char *group, const char *key, std::string &result);
    bool get_group(std::vector<std::string> &result);
};


#endif //DISCOVERY_CONFIGSERVICE_H
