#include "storage/allocator/allocator.h"

#ifdef USE_MMAP

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
# define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_FAILED
#define MAP_FAILED (void *)-1
#endif

typedef struct  {
    zend_shared_segment common;
    size_t size;
} zend_shared_segment_mmap;

static int create_segments(size_t requested_size, zend_shared_segment_mmap ***shared_segments_p, int *shared_segments_count, char **error_in)
{
	zend_shared_segment_mmap *shared_segment;

	*shared_segments_count = 1;
	*shared_segments_p = (zend_shared_segment_mmap **) calloc(1, sizeof(zend_shared_segment_mmap) + sizeof(void *));
	if (!*shared_segments_p) {
		*error_in = "calloc";
		return ALLOC_FAILURE;
	}
	shared_segment = (zend_shared_segment_mmap *)((char *)(*shared_segments_p) + sizeof(void *));
	(*shared_segments_p)[0] = shared_segment;

#ifdef MAP_HUGETLB
	/* Try to allocate huge pages first to reduce dTLB misses.
	 * OS has to be configured properly
	 * (e.g. https://wiki.debian.org/Hugepages#Enabling_HugeTlbPage)
	 * You may verify huge page usage with the following command:
	 * `grep "Huge" /proc/meminfo`
	 */
	shared_segment->common.p = mmap(0, requested_size, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);
	if (shared_segment->common.p != MAP_FAILED) {
		shared_segment->common.pos = 0;
		shared_segment->common.size = requested_size;
		shared_segment->size = requested_size;

		return ALLOC_SUCCESS;
	}
#endif

	shared_segment->common.p = mmap(0, requested_size, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if (shared_segment->common.p == MAP_FAILED) {
		*error_in = "mmap";
		return ALLOC_FAILURE;
	}

	shared_segment->common.pos = 0;
	shared_segment->common.size = requested_size;
	shared_segment->size = requested_size;

	return ALLOC_SUCCESS;
}

static int detach_segment(zend_shared_segment *shared_segment)
{
    if (shared_segment->size > 0)
	    munmap(shared_segment->p, shared_segment->size);
	return 0;
}

static size_t segment_type_size(void)
{
	return sizeof(zend_shared_segment_mmap);
}

zend_shared_memory_handlers zend_alloc_mmap_handlers = {
	create_segments,
	detach_segment,
	segment_type_size
};

#endif /* USE_MMAP */
