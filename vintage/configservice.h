//
// Created by zhenkai on 2020/6/23.
//

#ifndef DISCOVERY_CONFIGSERVICE_H
#define DISCOVERY_CONFIGSERVICE_H

#include "http/http.h"

#include <string>
#include <memory>

class ConfigService
{
private:
    const char *host_;
    std::shared_ptr<Http> http_;
public:
    ConfigService(const char *host);
    std::string lookup(std::string &group, std::string key = "");
    std::string get_group(void);
};


#endif //DISCOVERY_CONFIGSERVICE_H
