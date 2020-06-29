//
// Created by zhenkai on 2020/6/23.
//

#ifndef DISCOVERY_NAMINGSERVICE_H
#define DISCOVERY_NAMINGSERVICE_H

#include "http/http.h"
#include "storage/storage.h"

#include <string>
#include <memory>
#include <set>
#include <mutex>

class NamingService
{
private:
    const char *host_;
    std::shared_ptr<Http> http_;
    HashTable *ht_;
    std::mutex lock_{};
    std::set<std::string> service_clusters;
    bool lookup(const char *service, const char *cluster, std::string &result);
    bool lookupforupdate(const char *service, const char *cluster, const char *sign, std::string &result);

    void build_key(const char *key, char *build_key);

public:
    bool get_service(std::vector<std::string> &result);
    NamingService(const char *host, HashTable *ht);
    bool fetch(void);
    bool fetchforupdate(void);
    bool watch(void);
    bool add_watch(const char *service, const char *cluster);
    bool find(const char *service, const char *cluster, std::string &result);

};


#endif //DISCOVERY_NAMINGSERVICE_H
