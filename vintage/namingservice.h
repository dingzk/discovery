//
// Created by zhenkai on 2020/6/23.
//

#ifndef DISCOVERY_NAMINGSERVICE_H
#define DISCOVERY_NAMINGSERVICE_H

#include "http/http.h"

#include <string>
#include <memory>
#include <vector>

class NamingService
{
private:
    const char *host_;
    std::shared_ptr<Http> http_;

public:
    NamingService(const char *host);
    bool lookup(const char *service, const char *cluster, std::string &result);
    bool lookupforupdate(const char *service, const char *cluster, const char *sign, std::string &result);
    bool get_service(std::vector<std::string> &result);
};


#endif //DISCOVERY_NAMINGSERVICE_H
