#include "storage/storage.h"
#include "vintage/configservice.h"
#include "vintage/namingservice.h"
#include <iostream>

#include "msq/msq.h"

int msg_test ()
{
    int msqid = create_msq("/data1/apache2/htdocs/msq");

    std::cout << msqid << std::endl;

    const char *msg = "hello world";
    int ret = send_msg(msqid, msg, strlen(msg), (void *)MSG_TYPE);

    std::cout << ret << std::endl;
    std::cout << msqid << std::endl;

    char recv[MAX_MSG_LEN]  = {0};

    init_msq("/data1/apache2/htdocs/msq");

    int nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, (void *) MSG_TYPE);

    std::cout << nrecv << std::endl;
    std::cout << recv << std::endl;

    return 0;
}


int main(int argc, char **argv)
{

    char *err = NULL;
    HashTable *ht = hash_startup(4 * 1024 * 1024, 4*1024*1024, &err);
    if (!ht) {
        printf("error %s", err);
        return 1;
    }

    hash_dump(ht);

    ConfigService configservice("register.kailash.weibo.com", ht);
    configservice.watch();

    NamingService namingservice("register.kailash.weibo.com", ht);
    namingservice.add_watch("for-test", "test.for");
    namingservice.add_watch("ks-search", "search.ks");
    namingservice.add_watch("tc-search-huatiapi", "com.weibo.search.huati.i");
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
    while (true) {
        configservice.find("ks", value);
        std::cout << "find value : " << value << std::endl;

        configservice.find("ks", "elk_whitelist", value);
        std::cout << "find value : " << value << std::endl;

        namingservice.find("for-test", "test.for", value);
        std::cout << "find value : " << value << std::endl;

        namingservice.find("tc-search-huatiapi", "com.weibo.search.huati.i", value);
        std::cout << "find value : " << value << std::endl;

        sleep(1);
    }


    get_mem_info();

//    hash_dump(ht);

    hash_destory(ht);

    return 0;
}