//
// Created by zhenkai on 2020/6/10.
//

#ifndef DISCOVERY_STORAGE_H
#define DISCOVERY_STORAGE_H

#include <stdint.h>
#include "storage/allocator/allocator.h"

#define MAX_SIGN_LEN            (33)
#define MAX_KEY_LEN             (48)

typedef enum {
    SUCCESS =  0,
    FAILURE = -1,
} RESULT_CODE;

typedef enum {
    FLAG_OK =  0,
    FLAG_DELETE = 1,
} FLAG_CODE;

typedef struct {
    uint64_t        atime;
    uint32_t        next;      /* hash collision chain */
    uint32_t        len;
    uint32_t        real_size;
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
    uint32_t        mutex;
    char            sign[MAX_SIGN_LEN];
    char            key[MAX_KEY_LEN];
} Bucket;

typedef struct {
    uint32_t        nTableMask;
    Bucket          *arData;
    uint32_t        nNumUsed;
    uint32_t        nNumOfElements;
    uint32_t        nTableSize;
    uint32_t        nInternalPointer;
    uint32_t        nNextFreeElement;
} HashTable;

#define HT_INVALID_IDX ((uint32_t) -1)

#define HT_HASH_EX(data, idx) \
    ((uint32_t*)(data))[(int32_t)(idx)]
#define HT_HASH(ht, idx) \
    HT_HASH_EX((ht)->arData, idx)


HashTable *hash_startup(uint32_t k_size, uint32_t v_size, char **err);
void hash_destory(HashTable *ht);
void hash_dump(const HashTable *ht);

Bucket *hash_find_bucket(const HashTable *ht, const char *key, uint32_t len);
int hash_delete_bucket(HashTable *ht, char *key, uint32_t len);
int hash_add_or_update_bucket(HashTable *ht, const char *sign, uint32_t sign_len, const char *key, uint32_t len, const char *data, uint32_t size);

void get_hash_info(const HashTable *ht);



#endif //DISCOVERY_STORAGE_H
