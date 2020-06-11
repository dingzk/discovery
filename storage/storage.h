//
// Created by zhenkai on 2020/6/10.
//

#ifndef DISCOVERY_STORAGE_H
#define DISCOVERY_STORAGE_H

#include <stdint.h>
#include "storage/allocator/allocator.h"

#define MAX_KEY_LEN		(48)

typedef struct {
    uint32_t        next;      /* hash collision chain */
    uint32_t        len;
    char            data[1];
} String;

/*
 * HashTable Data Layout
 * =====================
 *
 *                 +=============================+
 *                 | HT_HASH(ht, ht->nTableMask) |
 *                 | ...                         |
 *                 | HT_HASH(ht, -1)             |
 *                 +-----------------------------+
 * ht->arData ---> | Bucket[0]                   |
 *                 | ...                         |
 *                 | Bucket[ht->nTableSize-1]    |
 *                 +=============================+
 */

typedef struct _Bucket {
    uint64_t        h;
    String          *val;
    uint32_t        crc;
    uint32_t        len;
    uint32_t        flag;
    uint32_t        size;
    uint32_t        mutex;
    char            sign[33];
    char            key[MAX_KEY_LEN];
} Bucket;

typedef struct {
    uint32_t          nTableMask;
    Bucket           *arData;
    uint32_t          nNumUsed;
    uint32_t          nNumOfElements;
    uint32_t          nTableSize;
    uint32_t          nInternalPointer;
    zend_long         nNextFreeElement;
} HashTable;

#define HT_INVALID_IDX ((uint32_t) -1)

#define HT_HASH_EX(data, idx) \
    ((uint32_t*)(data))[(int32_t)(idx)]
#define HT_HASH(ht, idx) \
    HT_HASH_EX((ht)->arData, idx)


HashTable *hash_startup(uint32_t k_size, uint32_t v_size, char **err);
void hash_destory(const HashTable *ht);
void hash_dump(const HashTable *ht);

Bucket *hash_find_bucket(const HashTable *ht, char *key, uint32_t len);
int hash_delete_bucket(const HashTable *ht, char *key, uint32_t len);
int hash_add_or_update_bucket(const HashTable *ht, const char *key, uint32_t len, char *data, uint32_t size);

char* get_hash_info(const HashTable *ht);



#endif //DISCOVERY_STORAGE_H
