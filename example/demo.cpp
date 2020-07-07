#include "vintage/configservice.h"
#include "vintage/namingservice.h"
#include <iostream>

#include "msq/msq.h"

int hashtable_test()
{
    char *err = NULL;
    HashTable *ht = hash_startup(4 * 1024 * 1024, 4*1024*1024, &err);
    if (!ht) {
        printf("error %s", err);
        return 1;
    }

    char *sign1 = "c21f969b5f03d33d43e04f8f136e7682";
    char *key1 = "test1";
    char *value1 = "[{\\\"key\\\":\\\"elk\\\",\\\"value\\\":\\\"2771122141,2008783705,1838150241,2367539940,1287349807, 2447986777\\\"},{\\\"key\\\":\\\"degrade.level\\\",\\\"value\\\":\\\"0\\\"}]";

    char *sign2 = "c21f969b5f03d33d43e04f8f136e7683";
    char *key2 = "test2";
    char *value2 = "value2";

    char *sign3 = "c21f969b5f03d33d43e04f8f136e7684";
    char *key3 = "test3";
    char *value3 = "value3";

    int ret1 = hash_add_or_update_bucket(ht, sign1, strlen(sign1), key1, strlen(key1), value1, strlen(value1));

    int ret2 = hash_add_or_update_bucket(ht, sign1, strlen(sign1), key2, strlen(key2), value2, strlen(value2));

    int ret3 = hash_add_or_update_bucket(ht, sign1, strlen(sign1), key3, strlen(key3), value3, strlen(value3));

    hash_dump(ht);

//    Bucket *hash_find_bucket(const HashTable *ht, const char *key, uint32_t len);

    Bucket *b = hash_find_bucket(ht, key1, strlen(key1));
    if (b) {
        printf("find bucket.val.len = %d\n", b->val->len);
    }
    int a = hash_delete_bucket(ht, key1, strlen(key1));

    printf("del ret %d\n", a);
    if (a == SUCCESS) {
        Bucket *b = hash_find_bucket(ht, key1, strlen(key1));
        if (b) {
            printf("find bucket.val.len = %d\n", b->val->len);
        } else {
            printf("find none\n");
        }
    }

    get_mem_info();

    hash_destory(ht);

    return 0;
}

int msg_test ()
{
    int msqid = create_msq("/msq");

    std::cout << msqid << std::endl;

    const char *msg = "hello world";
    const char *msg2 = "hello world 1212";
    int ret = send_msg(msqid, msg, strlen(msg), (void *)1);
    send_msg(msqid, msg2, strlen(msg2), (void *)2);
//    send_msg(msqid, msg2, strlen(msg2), (void *)(MSG_TYPE + 1));

    std::cout << ret << std::endl;
    std::cout << msqid << std::endl;

    char recv[MAX_MSG_LEN]  = {0};
    int nrecv;

    msqid = init_msq("/msq");

//    int nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, (void *) MSG_TYPE);
//    int prio;
//    int nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, &prio);
//    std::cout << nrecv << std::endl;
//    std::cout << recv << std::endl;
//    std::cout << prio << std::endl;
//    nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, &prio);
//    std::cout << nrecv << std::endl;
//    std::cout << prio << std::endl;
//    nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, &prio);
//    std::cout << nrecv << std::endl;
//    std::cout << prio << std::endl;
    nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, (void *) 2);

    std::cout << nrecv << std::endl;
    std::cout << recv << std::endl;


    destory_msq("/msq");

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
    log_init("./discovery.log", LEVEL_INFO);

    ConfigService configservice("register.kailash.weibo.com", ht);
    configservice.add_watch("ks");
    configservice.add_watch("mi");
    configservice.add_watch("searcher_sdk");
    configservice.watch();

    NamingService namingservice("register.kailash.weibo.com", ht);
    namingservice.add_watch("for-test", "test.for");
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
        configservice.find("ks", "elk_whitelist", value);
        std::cout << "find value : " << value << std::endl;

        namingservice.find("for-test", "test.for", value);
        std::cout << "find value : " << value << std::endl;

        namingservice.find("tc-search-huatiapi", "com.weibo.search.huati.i", value);
        std::cout << "find value : " << value << std::endl;

        sleep(1);
    }

    get_mem_info();

    hash_dump(ht);

    hash_destory(ht);

    log_destroy();

    return 0;
}