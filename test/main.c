#include "storage/storage.h"
#include <stdio.h>
#include <string.h>


int main(int argc, char **argv)
{
    char *err;
    HashTable *ht = hash_startup(4 * 1024 * 1024, 4*1024*1024, &err);
    if (!ht) {
        printf("error %s", err);
        return 1;
    }

    char *sign1 = "c21f969b5f03d33d43e04f8f136e7682";
    char *key1 = "test1";
    char *value1 = "value1";

    char *sign2 = "c21f969b5f03d33d43e04f8f136e7683";
    char *key2 = "test1";
    char *value2 = "value1";

    char *sign3 = "c21f969b5f03d33d43e04f8f136e7684";
    char *key3 = "test1";
    char *value3 = "value1";

    int ret1 = hash_add_or_update_bucket(ht, sign1, strlen(sign1), key1, strlen(key1), value1, strlen(value1));

    int ret2 = hash_add_or_update_bucket(ht, sign1, strlen(sign1), key2, strlen(key2), value2, strlen(value2));

    int ret3 = hash_add_or_update_bucket(ht, sign1, strlen(sign1), key3, strlen(key3), value3, strlen(value3));

    hash_dump(ht);

    hash_destory(ht);

    return 0;
}