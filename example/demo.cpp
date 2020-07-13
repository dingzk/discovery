#include "vintage/configservice.h"
#include "vintage/namingservice.h"
#include <iostream>

int main(int argc, char **argv)
{

    char *err = NULL;
    HashTable *ht = hash_startup(4 * 1024 * 1024, 4*1024*1024, &err);
    if (!ht) {
        printf("error %s", err);
        return 1;
    }

    hash_dump_timer(ht);
    log_init("./discovery.log", LEVEL_INFO);

    ConfigService configservice("register.kailash.weibo.com", ht);
    configservice.add_watch("ks");
    configservice.add_watch("mi");
    configservice.add_watch("searcher_sdk");
    configservice.add_watch("searcher_sdk1");
    configservice.watch();

    NamingService namingservice("register.kailash.weibo.com", ht);
    namingservice.add_watch("for-test", "test.for");
    namingservice.add_watch("aliyun-search-huatiapi", "com.weibo.search.huati");
    namingservice.add_watch("unify-search-weibo-com", "com.weibo.search.unify.v2");
    namingservice.watch();

    // watch careful for lock
//    std::vector<std::string> res;
//    namingservice.get_service(res);
//
//    for (auto & re : res) {
//        std::cout << re << std::endl;
//    }



    std::string value;
    int i = 100;
    while (i--) {
        configservice.find("ks", "elk_whitelist", value);
        std::cout << "find value : " << value << std::endl;

        configservice.find("ks2", "elk_whitelist", value);
        std::cout << "find value : " << value << std::endl;

        namingservice.find("for-test", "test.for", value);
        std::cout << "find value : " << value << std::endl;

        namingservice.find("tc-search-huatiapi", "com.weibo.search.huati.i", value);
        std::cout << "find value : " << value << std::endl;

        sleep(1);
    }

    get_mem_info();

    hash_dump_debug(ht);

    hash_destory(ht);

    log_destroy();

    return 0;
}