//
// Created by zhenkai on 2020/6/10.
//

#include "allocator.h"
#include <string.h>

/*
 * zend_shared_segment**
 * --------------------------------------------------------------------------
 * | p1 | p2  | p3  | shared_segment1  | shared_segment2  | shared_segment3  |
 * --------------------------------------------------------------------------
 */

mem_shared_globals *shared_globals = NULL;

#if defined(USE_MMAP)
extern zend_shared_memory_handlers zend_alloc_mmap_handlers;
static const zend_shared_memory_handler_entry handler_entry = { "mmap", &zend_alloc_mmap_handlers };
#elif defined(USE_SHM)
extern zend_shared_memory_handlers zend_alloc_shm_handlers;
static const zend_shared_memory_handler_entry handler_entry = { "shm", &zend_alloc_shm_handlers };
#elif defined(USE_SHM_OPEN)
extern zend_shared_memory_handlers zend_alloc_posix_handlers;
static const zend_shared_memory_handler_entry handler_entry = { "posix", &zend_alloc_posix_handlers };
#else
#error(no defined shared memory supported)
#endif

// 内存初始化
int shared_alloc_startup(size_t requested_size)
{
    int res;
    requested_size = MEM_ALIGNED_SIZE(requested_size);
    requested_size = requested_size < MEM_SEGMENT_MIN_SIZE ? MEM_SEGMENT_MIN_SIZE : requested_size;
    zend_shared_segment **shared_segments = NULL;
    int shared_segments_count = 0;
    char *error_in = NULL;
    res = SMH(create_segments)(requested_size, &shared_segments, &shared_segments_count, &error_in);
    if (res == ALLOC_FAILURE && shared_segments) {
        int i;
        for (i = 0; i < shared_segments_count; i++) {
            if (shared_segments[i]->p && shared_segments[i]->p != (void *)-1) {
                SMH(detach_segment)(shared_segments[i]);
            }
        }
        free(shared_segments);
        shared_segments = NULL;

        return ALLOC_FAILURE;
    }

    int segment_array_size = shared_segments_count * ( sizeof(void *) + SMH(segment_type_size)() );

    if (shared_segments[0]->size <= MEM_ALIGNED_SIZE(sizeof(mem_shared_globals) + segment_array_size)) {
        return ALLOC_FAILURE;
    }

    shared_globals = malloc(sizeof(mem_shared_globals));

    MMSG(shared_free) = requested_size;
    MMSG(shared_segments_count) = shared_segments_count;
    MMSG(shared_segments) = shared_segments;

    mem_shared_globals *shared_globals_tmp = (mem_shared_globals *) zend_shared_alloc(sizeof(mem_shared_globals));
    if (shared_globals == NULL) {
        free(shared_globals);
        free(shared_segments);
        return ALLOC_FAILURE;
    }
    memcpy(shared_globals_tmp, shared_globals, sizeof(mem_shared_globals));
    free(shared_globals);
    shared_globals = shared_globals_tmp;

    MMSG(shared_segments) = (zend_shared_segment **) zend_shared_alloc(segment_array_size);
    if (MMSG(shared_segments) == NULL) {
        free(shared_segments);
        return ALLOC_FAILURE;
    }
    // copy real struct value to shared memory
    memcpy(MMSG(shared_segments), shared_segments,  segment_array_size);

    // copy pointer to shared memory
    int i = 0;
    for (i = 0; i < shared_segments_count; i++) {
        MMSG(shared_segments)[i] = (char *)MMSG(shared_segments) + sizeof(void *) * shared_segments_count + i * SMH(segment_type_size)();
    }

    MMSG(wasted_shared_memory) = 0;
    MMSG(memory_exhausted) = 0;

    MMSG(app_shared_globals) = MMSG(shared_segments)[0]->p + MMSG(shared_segments)[0]->pos;

    free(shared_segments);

    return ALLOC_SUCCESS;
}

// 内存分配
void *zend_shared_alloc(size_t size)
{
    return zend_shared_raw_alloc(MEM_ALIGNED_SIZE(size));
}

void *zend_shared_raw_alloc(size_t size)
{
    int i;

    if (MMSG(shared_free) < size || size == 0) {
        return NULL;
    }
    // lock ?
    for (i = 0; i < MMSG(shared_segments_count); ++i) {
        if (MMSG(shared_segments)[i]->pos + size < MMSG(shared_segments)[i]->size) {
            void *retp = (void *)(MMSG(shared_segments)[i]->pos + (char *)(MMSG(shared_segments)[i]->p));
            MMSG(shared_free) -= size;
            MMSG(shared_segments)[i]->pos += size;
            memset(retp, 0, size);

            return retp;
        }
    }

    return NULL;
}

size_t alloc_real_size(size_t size)
{
    size_t real_size = MEM_TRUE_SIZE(size * STORAGE_FACTOR);

    if (MMSG(shared_free) < real_size) {
        return 0;
    }

    return real_size;
}

// 内存释放
void shared_alloc_shutdown(void)
{
    int segment_array_size = MMSG(shared_segments_count) * sizeof(void *) + SMH(segment_type_size)() * MMSG(shared_segments_count);
    zend_shared_segment **shared_segments_tmp = (zend_shared_segment **) malloc(segment_array_size);
    if (shared_segments_tmp == NULL) {
        return ;
    }
    int shared_segments_count = MMSG(shared_segments_count);
    memcpy(shared_segments_tmp, MMSG(shared_segments), segment_array_size);
    int i;
    for (i = 0; i < shared_segments_count; i++) {
        if (shared_segments_tmp[i]->p && shared_segments_tmp[i]->p != (void *)-1) {
            SMH(detach_segment)(shared_segments_tmp[i]);
        }
    }

    free(shared_segments_tmp);
}

void shared_alloc_protect(int mode)
{
#ifdef HAVE_MPROTECT
    int i;

	if (!shared_globals) {
		return;
	}

	if (mode) {
		mode = PROT_READ;
	} else {
		mode = PROT_READ|PROT_WRITE;
	}

	for (i = 0; i < MMSG(shared_segments_count); i++) {
		mprotect(MMSG(shared_segments)[i]->p, ZSMMG(shared_segments)[i]->size, mode);
	}
#endif
}

int shared_in_shm(void *ptr)
{
    int i;
    if (!shared_globals) {
        return 0;
    }
    for (i = 0; i < MMSG(shared_segments_count); i++) {
        if ((char*)ptr >= (char*)MMSG(shared_segments)[i]->p &&
            (char*)ptr < (char*)MMSG(shared_segments)[i]->p + MMSG(shared_segments)[i]->size) {
            return 1;
        }
    }
    return 0;
}

