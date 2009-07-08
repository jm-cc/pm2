
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
#include <stdint.h>
#ifndef __MINGW32__
#  include <sys/mman.h>
#endif
#if defined(MA__LWPS)
#  include <topology.h>
#endif

/*
 * How to bind a thread on a given processor
 */

#if defined(MA__LWPS)
void ma_bind_on_processor(unsigned target) {
	topo_cpuset_t set;

	topo_cpuset_zero(&set);
	topo_cpuset_set(&set, target);
	if (topo_set_cpubind(topology, &set, TOPO_CPUBIND_THREAD)) {
		fprintf(stderr,"while binding on CPU%d\n", target);
		perror("topo_set_cpubind");
		exit(1);
	}
}
void ma_unbind_from_processor(void) {
	topo_cpuset_t set;

	topo_cpuset_fill(&set);
	if (topo_set_cpubind(topology, &set, TOPO_CPUBIND_THREAD)) {
		fprintf(stderr,"while unbinding\n");
		perror("topo_set_cpubind");
		exit(1);
	}
}
#endif


/*
 * How to allocate memory on specific nodes
 */

#ifdef MA__NUMA
int ma_numa_not_available = 0;
#endif

#ifdef MM_HEAP_ENABLED
#ifdef LINUX_SYS
#define HAS_NUMA
#include <numa.h>
#include <mm_helper.h>

void *ma_malloc_node(size_t size, int node, char *file, unsigned line) {
	void *p;
	if (ma_numa_not_available || node == -1 || marcel_use_fake_topology)
		return ma_malloc_nonuma(size,file,line);

	marcel_malloc_protect();
	p = numa_alloc_onnode(size, node);
	marcel_malloc_unprotect();

	if (p == NULL)
		return ma_malloc_nonuma(size,file,line);
	return p;
}
void ma_free_node(void *data, size_t size, char * __restrict file, unsigned line) {
	if (ma_numa_not_available || marcel_use_fake_topology)
		return ma_free_nonuma(data,file,line);

	marcel_malloc_protect();
	numa_free(data, size);
	marcel_malloc_unprotect();
}

int ma_is_numa_available(void) {
	return (ma_numa_not_available == 0) && (numa_available() != -1);
}

#include <numaif.h>
void ma_migrate_mem(void *ptr, size_t size, int node) {
#if defined(MPOL_BIND) && defined(MPOL_MF_MOVE) && defined(MPOL_MF_MOVE_ALL)
	unsigned long mask = 1<<node;
	uintptr_t addr = ((uintptr_t)ptr) & ~(getpagesize()-1);
	size += ((uintptr_t)ptr) - addr;
	if (node < 0 || ma_numa_not_available)
		return;
	if (_mm_mbind((void*)addr, size, MPOL_BIND, &mask, sizeof(mask)*CHAR_BIT, MPOL_MF_MOVE_ALL) == -1 && errno == EPERM)
                _mm_mbind((void*)addr, size, MPOL_BIND, &mask, sizeof(mask)*CHAR_BIT, MPOL_MF_MOVE);
#endif
}
#elif defined(OSF_SYS)
#define HAS_NUMA
#include <numa.h>
void *ma_malloc_node(size_t size, int node, char *file, unsigned line) {
	/* TODO: utiliser acreate/amalloc plutôt ? */
	if (node == -1)
		return ma_malloc_nonuma(size,file,line);
	memalloc_attr_t mattr;
	memset(&mattr, 0, sizeof(mattr));
	mattr.mattr_policy = MPOL_DIRECTED;
	mattr.mattr_rad = node;
	radsetcreate(&mattr.mattr_radset);
	rademptyset(mattr.mattr_radset);
	radaddset(mattr.mattr_radset,node);
	return nmmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0, &mattr);
}
void ma_free_node(void *ptr, size_t size, char * __restrict file, unsigned line) {
	munmap(ptr, size);
}

int ma_is_numa_available(void) {
	return 0;//TODO
}

void ma_migrate_mem(void *ptr, size_t size, int node) {
}
#elif defined(AIX_SYS)
#include <sys/rset.h>
#ifdef P_DEFAULT
#define HAS_NUMA
void *ma_malloc_node(size_t size, int node, char *file, unsigned line) {
	rsethandle_t rset, rad;
	int MCMlevel = rs_getinfo(NULL, R_MCMSDL, 0);
	void *ret;
	rsid_t rsid;

	if (node == -1)
		return ma_malloc_nonuma(size,file,line);
	rset = rs_alloc(RS_PARTITION);
	rad = rs_alloc(RS_EMPTY);
	rs_getrad(rset, rad, MCMlevel, node, 0);
	rsid.at_rset = rad;
	ret = ra_mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0, R_RSET, rsid, P_DEFAULT);
	rs_free(rset);
	rs_free(rad);
	return ret;
}
void ma_free_node(void *ptr, size_t size, char * __restrict file, unsigned line) {
	munmap(ptr, size);
}

int ma_is_numa_available(void) {
	return 0;//TODO
}

void ma_migrate_mem(void *ptr, size_t size, int node) {
}
#else
#warning "Only AIX >= 5.3 has the NUMA API"
#endif

#else

/* TODO: SOLARIS_SYS, WIN_SYS, GNU_SYS, FREEBSD_SYS, DARWIN_SYS, IRIX_SYS */
#ifdef WIN_SYS
#warning TODO: use AllocateUserPhysicalPagesNuma or VirtualAllocExNuma
#endif

#endif

#ifndef HAS_NUMA
#warning "don't know how to allocate memory on specific nodes"
void *ma_malloc_node(size_t size, int node, char *file, unsigned line) {
	return ma_malloc_nonuma(size,file,line);
}

void ma_free_node(void *data, size_t size, char * __restrict file, unsigned line) {
	ma_free_nonuma(data,file,line);
}

int ma_is_numa_available(void) {
	return -1;//TODO
}

void ma_migrate_mem(void *ptr, size_t size, int node) {
}
#endif

#endif /* MM_HEAP_ENABLED */
