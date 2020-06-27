//
// Created by zhenkai on 2020/6/23.
//

#ifndef DISCOVERY_CONFIGSERVICE_H
#define DISCOVERY_CONFIGSERVICE_H

#include "http/http.h"
#include "storage/storage.h"

#include <string>
#include <memory>
#include <vector>

class ConfigService
{
private:
    const char *host_;
    std::shared_ptr<Http> http_;
    HashTable *ht_;
    bool lookup(const char *group, std::string &result);
    bool lookup(const char *group, const char *key, std::string &result);
    bool get_group(std::vector<std::string> &result);

public:
    ConfigService(const char *host, HashTable *ht);
    bool fetch();
    bool watch(void);
    bool find(const char *group, std::string &result);
    bool find(const char *group, const char *key, std::string &result);
};


#endif //DISCOVERY_CONFIGSERVICE_H
