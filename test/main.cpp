#include "storage/storage.h"
#include "vintage/configservice.h"
#include "vintage/namingservice.h"
#include "serializer/cJSON/cJSON.h"

#include <stdio.h>
#include <string.h>

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
    char *value1 = "value1";

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

int config_test()
{
    ConfigService configservice("register.kailash.weibo.com");
    std::string result;
    configservice.lookup("ks", result);
    std::cout << result << std::endl;

    std::string value;
    configservice.lookup("ks", "test", value);
    std::cout << value << std::endl;

    std::vector<std::string> groups;
    configservice.get_group(groups);
    for (auto iter = groups.begin(); iter != groups.end(); ++ iter) {
        std::cout << *iter << std::endl;
    }

    return 0;
}

int naming_test()
{
    NamingService namingservice("register.kailash.weibo.com");

    std::string result2;
    namingservice.lookup("for-test", "test.for", result2);
    std::cout << result2 << std::endl;

    std::string val;
    namingservice.lookupforupdate("for-test", "test.for", "3bb444b2aae425d4e31dc04f717134fd", val);
    std::cout << val << std::endl;

    std::vector<std::string> services;
    namingservice.get_service(services);
    for (auto iter = services.begin(); iter != services.end(); ++ iter) {
        std::cout << *iter << std::endl;
    }

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
    ConfigService configservice("register.kailash.weibo.com");
    std::vector<std::string> groups;
    configservice.get_group(groups);

    for (auto iter = groups.begin(); iter != groups.end(); ++ iter) {
        std::string result;
        bool ret = configservice.lookup(iter->c_str(), result);
        if (!ret) {
            continue;
        }
        cJSON* root = cJSON_Parse(result.c_str());
        if (root == nullptr) {
            continue;
        }
        cJSON *groupid_json = cJSON_GetObjectItemCaseSensitive(root, "groupId");
        cJSON *sign_json = cJSON_GetObjectItemCaseSensitive(root, "sign");
        cJSON *nodes_json = cJSON_GetObjectItem(root, "nodes");
        char *value = cJSON_PrintUnformatted(nodes_json);

        char *key = cJSON_GetStringValue(groupid_json);
        char *sign = cJSON_GetStringValue(sign_json);

        hash_add_or_update_bucket(ht, sign, strlen(sign), key, strlen(key), value, strlen(value));

        free(value);
    }

    hash_dump(ht);

    for (auto iter = groups.begin(); iter != groups.end(); ++ iter) {
        Bucket *b = hash_find_bucket(ht, iter->c_str(), strlen(iter->c_str()));
        if (b) {
            printf("find bucket.val %s, bucket.val.len = %d\n", strndup(b->val->data, b->val->len), b->val->len);
        }
    }

    return 0;
}