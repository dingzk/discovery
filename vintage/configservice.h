//
// Created by zhenkai on 2020/6/23.
//

#ifndef DISCOVERY_CONFIGSERVICE_H
#define DISCOVERY_CONFIGSERVICE_H

#include "vintage/vintage.h"

#include <set>
#include <mutex>

class ConfigService : public Vintage
{
private:
    HashTable *ht_;
    std::mutex lock_{};
    std::set<std::string> groups;

public:
    ConfigService(const char *host, HashTable *ht);
    bool fetch();
    bool watch();
    bool add_watch(const char *group);
    bool add_watch(const char *group, const char *key);
    bool find(const char *group, std::string &result);
    bool find(const char *group, const char *raw_key, std::string &result);
};


#endif //DISCOVERY_CONFIGSERVICE_H
