//
// Created by zhenkai on 2020/6/23.
//

#ifndef DISCOVERY_NAMINGSERVICE_H
#define DISCOVERY_NAMINGSERVICE_H

#include "vintage/vintage.h"

#include <set>
#include <mutex>
#include <unordered_map>

class NamingService : public Vintage
{
private:
    HashTable *ht_;
    std::mutex lock_{};
    std::unordered_map<std::string, std::set<std::string>> serv_clusters;

public:
    NamingService(const char *host, HashTable *ht);
    bool fetch();
    bool fetchforupdate();
    bool watch();
    bool add_watch(std::string service, std::string cluster);
    bool add_watch(std::string servCluster);
    bool find(std::string service, std::string cluster, std::string &result);
    bool find(const char *service, const char *cluster, std::string &result);

};


#endif //DISCOVERY_NAMINGSERVICE_H
