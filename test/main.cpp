#include "storage/storage.h"
#include "vintage/configservice.h"
#include "vintage/namingservice.h"

#include "serializer/rapidjson/document.h"
#include "serializer/rapidjson/writer.h"
#include "serializer/rapidjson/stringbuffer.h"
#include <iostream>
using namespace rapidjson;


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

int json_test()
{
    // 1. 把 JSON 解析至 DOM。
    const char* json =  "{\"service\":\"for-test\",\"cluster\":\"test.for/service\",\"sign\":\"3bb444b2aae425d4e31dc04f717134fd\",\"nodes\":{\"working\":[{\"host\":\"127.0.0.1:80\",\"extInfo\":\"\"},{\"host\":\"172.16.139.72:80\",\"extInfo\":\"\"},{\"host\":\"172.16.139.71:80\",\"extInfo\":\"{\\\"ancd\\\":\\\"cc\\\",\\\"weight\\\":122,\\\"falcon_tag\\\":\\\"static\\\"}\"}],\"unreachable\":[]}}";
    Document d;
    d.Parse(json);

    // 2. 利用 DOM 作出修改。
//    Value& s = d["stars"];
//    s.SetInt(s.GetInt() + 1);

    assert(d.IsObject());
    assert(d.HasMember("sign"));
    rapidjson::Value& s = d["sign"];
    std::cout << "lookup: sign : " << d["sign"].GetString() << std::endl;


    // 3. 把 DOM 转换（stringify）成 JSON。
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;
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
    namingservice.watch();

    std::vector<std::string> res;
    namingservice.get_service(res);

    for (auto i = res.begin(); i != res.end(); i++) {
        std::cout << *i << std::endl;
    }


    std::string value;
    int i = 100;
    while (i-- > 0) {
        configservice.find("ks", value);
        std::cout << "find value : " << value << std::endl;

        configservice.find("ks", "elk_whitelist", value);
        std::cout << "find value : " << value << std::endl;

        namingservice.find("for-test", "test.for", value);
        std::cout << "find value : " << value << std::endl;

        sleep(1);
    }


    get_mem_info();

//    hash_dump(ht);

    hash_destory(ht);

    return 0;
}