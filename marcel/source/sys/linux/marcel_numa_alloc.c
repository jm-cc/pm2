
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "marcel.h"
#include <errno.h>
#include <limits.h>
#include <sys/mman.h>
#include "sys/marcel_hwlocapi.h"


/*
 * How to allocate memory on specific nodes
 */
#ifdef MA__NUMA
int ma_numa_not_available = 0;
#endif

#ifdef MM_HEAP_ENABLED
#  define HAS_NUMA
#  include <numa.h>
#  include <mm_helper.h>
#  include <numaif.h>

void *ma_malloc_node(size_t size, int node)
{
	void *p;
	if (ma_numa_not_available || node == -1 || marcel_use_fake_topology)
		return ma_malloc_nonuma(size);

	marcel_extlib_protect();
	p = numa_alloc_onnode(size, node);
	marcel_extlib_unprotect();

	if (p == NULL)
		return ma_malloc_nonuma(size);
	return p;
}

void ma_free_node(void *data, size_t size)
{
	if (ma_numa_not_available || marcel_use_fake_topology)
		return ma_free_nonuma(data);

	marcel_extlib_protect();
	numa_free(data, size);
	marcel_extlib_unprotect();
}

int ma_is_numa_available(void)
{
	return (ma_numa_not_available == 0) && (numa_available() != -1);
}

void ma_migrate_mem(void *ptr, size_t size, int node)
{
#  if defined(MPOL_BIND) && defined(MPOL_MF_MOVE) && defined(MPOL_MF_MOVE_ALL)
	unsigned long mask = 1 << node;
	uintptr_t addr = ((uintptr_t) ptr) & ~(getpagesize() - 1);
	size += ((uintptr_t) ptr) - addr;
	if (node < 0 || ma_numa_not_available)
		return;
	// Don't even try MF_MOVE_ALL, that's root-only and probably won't change.
	//if (_mm_mbind((void*)addr, size, MPOL_BIND, &mask, sizeof(mask)*CHAR_BIT, MPOL_MF_MOVE_ALL) == -1 && errno == EPERM)
	_mm_mbind((void *) addr, size, MPOL_BIND, &mask, sizeof(mask) * CHAR_BIT,
		  MPOL_MF_MOVE);
#  endif
}


#endif				/* MM_HEAP_ENABLED */
