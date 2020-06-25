//
// Created by zhenkai on 2020/6/10.
//

#ifndef DISCOVERY_ALLOCATOR_H
#define DISCOVERY_ALLOCATOR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <stdio.h>

#define USE_MMAP      1

#define ALLOC_FAILURE               0
#define ALLOC_SUCCESS               1
#define FAILED_REATTACHED           2
#define SUCCESSFULLY_REATTACHED     4
#define ALLOC_FAIL_MAPPING          8
#define ALLOC_FALLBACK              9

#define MEM_SEGMENT_MIN_SIZE        (4*1024*1024)

#define STORAGE_FACTOR              (1.25)
#define MEM_ALIGNMENT               8
#define MEM_ALIGNMENT_MASK          ~(MEM_ALIGNMENT - 1)
#define MEM_ALIGNED_SIZE(x)         ((int)((x) + MEM_ALIGNMENT - 1) & (MEM_ALIGNMENT_MASK))
#define MEM_MIN_BLOCK_SIZE          128
#define MEM_TRUE_SIZE(x)            ((x < MEM_MIN_BLOCK_SIZE)? (MEM_MIN_BLOCK_SIZE) : (MEM_ALIGNED_SIZE(x)))

typedef struct _zend_shared_segment
{
    size_t size;
    volatile size_t pos;
    void *p;
} zend_shared_segment;

typedef struct _mem_shared_globals
{
    /* Shared Memory Manager */
    zend_shared_segment **shared_segments;
    /* Number of allocated shared segments */
    int shared_segments_count;
    /* Amount of free shared memory */
    size_t shared_free;
    /* Amount of shared memory allocated by garbage */
    size_t wasted_shared_memory;
    /* No more shared memory flag */
    unsigned char memory_exhausted;
    /* Pointer to the application's shared data structures */
    void *app_shared_globals;
} mem_shared_globals;

typedef int (*create_segments_t)(size_t requested_size, zend_shared_segment ***shared_segments,
                                 int *shared_segment_count, char **error_in);

typedef int (*detach_segment_t)(zend_shared_segment *shared_segment);

typedef struct
{
    create_segments_t create_segments;
    detach_segment_t detach_segment;

    size_t (*segment_type_size)(void);
} zend_shared_memory_handlers;

typedef struct _handler_entry
{
    const char *name;
    zend_shared_memory_handlers *handler;
} zend_shared_memory_handler_entry;

int shared_alloc_startup(size_t requested_size);

void *zend_shared_alloc(size_t size);

void *zend_shared_raw_alloc(size_t size);

size_t alloc_real_size(size_t size);

//void zend_shared_clean(void);
void shared_alloc_shutdown(void);

void shared_alloc_protect(int mode);

int shared_in_shm(void *ptr);

#if defined(USE_MMAP)
extern zend_shared_memory_handlers zend_alloc_mmap_handlers;
#elif defined(USE_SHM)
extern zend_shared_memory_handlers zend_alloc_shm_handlers;
#elif defined(USE_SHM_OPEN)
extern zend_shared_memory_handlers zend_alloc_posix_handlers;
#else
#error(no defined shared memory handlers supported)
#endif

extern mem_shared_globals *shared_globals;

#define SMH(s) (handler_entry.handler->s)
#define MMSG(s) (shared_globals->s)

#ifdef __cplusplus
}
#endif

#endif //DISCOVERY_ALLOCATOR_H
