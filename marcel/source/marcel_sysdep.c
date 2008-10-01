
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
#include <sys/mman.h>
#endif
#ifdef DARWIN_SYS
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#endif

/*
 * How to bind a thread on a given processor
 */

#if defined(MA__LWPS)
#ifdef LINUX_SYS
#include <sched.h>
#ifndef CPU_SET
	/* libc doesn't have support for sched_setaffinity,
	 * build system call ourselves: */
#include <linux/unistd.h>
#ifndef __NR_sched_setaffinity
	/* not even syscall number... */
#if defined(X86_ARCH)
#define __NR_sched_setaffinity 241
#elif defined(IA64_ARCH)
#define __NR_sched_setaffinity 1231
#define __ia64_syscall(arg1,arg2,arg3,arg4,arg5,nr) syscall(nr,arg1,arg2,arg3,arg4,arg5)
#else
#error "don't know the syscall number for sched_setaffinity on this architecture"
#endif
	/* declare system call */
_syscall3(int, sched_setaffinity, pid_t, pid, unsigned int, lg,
		unsigned long *, mask);
#endif
#endif
#endif
#ifdef WIN_SYS
#include <windows.h>
#endif
#ifdef OSF_SYS
#include <radset.h>
#include <numa.h>
#endif
#ifdef AIX_SYS
#include <sys/processor.h>
#endif
#endif

#if defined(MA__LWPS)
void ma_bind_on_processor(unsigned target) {
#if defined(SOLARIS_SYS)
	if(processor_bind(P_LWPID, P_MYID,
			  (processorid_t)(target),
			  NULL) != 0) {
		perror("processor_bind");
		exit(1);
	}
#elif defined(LINUX_SYS)

#ifndef CPU_SET
	/* no libc support, use direct system call */
	unsigned long mask = 1UL<<target;

	if (sched_setaffinity(0,sizeof(mask),&mask)<0) {
		perror("sched_setaffinity");
		exit(1);
	}
#else
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(target, &mask);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
	if(sched_setaffinity(0,&mask)<0) {
#else /* HAVE_OLD_SCHED_SETAFFINITY */
	if(sched_setaffinity(0,sizeof(mask),&mask)<0) {
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
		perror("sched_setaffinity");
		exit(1);
	}
#endif
#elif defined(WIN_SYS)
	DWORD mask = 1UL<<target;
	SetThreadAffinityMask(GetCurrentThread(), mask);
#elif defined(OSF_SYS)
	radset_t radset;
	radsetcreate(&radset);
	rademptyset(radset);
	radaddset(radset, target);
	if (pthread_rad_bind(pthread_self(), radset, RAD_INSIST)) {
		perror("pthread_rad_attach");
		exit(1);
	}
	radsetdestroy(&radset);
#elif defined(AIX_SYS)
	bindprocessor(BINDTHREAD, thread_self(), target);
#else
	/* TODO: GNU_SYS, FREEBSD_SYS, DARWIN_SYS, IRIX_SYS */
	/* IRIX: voir _DSM_MUSTRUN */
#warning "don't know how to bind on processors on this system, please disable smp_bind_proc in flavor"
#endif
}
void ma_unbind_from_processor() {
#if defined(SOLARIS_SYS)
	if(processor_bind(P_LWPID, P_MYID, PBIND_NONE, NULL) != 0) {
		perror("processor_bind");
		exit(1);
	}
#elif defined(LINUX_SYS)
#ifndef CPU_SET
	/* no libc support, use direct system call */
	unsigned long mask = ~0;

	if (sched_setaffinity(0,sizeof(mask),&mask)<0) {
		perror("sched_setaffinity");
		exit(1);
	}
#else
	cpu_set_t mask;
	unsigned long target;

	CPU_ZERO(&mask);
	for (target = 0; target < marcel_nbprocessors; target++)
		CPU_SET(target, &mask);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
	if(sched_setaffinity(0,&mask)<0) {
#else /* HAVE_OLD_SCHED_SETAFFINITY */
	if(sched_setaffinity(0,sizeof(mask),&mask)<0) {
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
		perror("sched_setaffinity");
		exit(1);
	}
#endif
#elif defined(WIN_SYS)
#warning TODO unbind thread
#elif defined(OSF_SYS)
	unsigned long target;
	radset_t radset;
	radsetcreate(&radset);
	rademptyset(radset);
	for (target = 0; target < marcel_nbprocessors; target++)
		radaddset(radset, target);
	if (pthread_rad_bind(pthread_self(), radset, RAD_INSIST)) {
		perror("pthread_rad_attach");
		exit(1);
	}
	radsetdestroy(&radset);
#elif defined(AIX_SYS)
#warning TODO unbind thread
#else
	/* TODO: GNU_SYS, FREEBSD_SYS, DARWIN_SYS, IRIX_SYS */
	/* IRIX: voir _DSM_MUSTRUN */
#warning "don't know how to unbind from processors on this system"
#endif
}
#endif

/*
 * How to get the number of processors
 */

#ifdef MA__LWPS
unsigned ma_nbprocessors(void) {
#if defined(_SC_NPROCESSORS_ONLN)
	return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
	return sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF) || defined(IRIX_SYS)
	return sysconf(_SC_NPROC_CONF);
#elif defined(DARWIN_SYS)
	struct host_basic_info info;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	host_info(mach_host_self(), HOST_BASIC_INFO, (integer_t*) &info, &count);
	return info.avail_cpus;
#else
#warning No known way to discover number of available processors on this system
#warning ma_nbprocessors will default to 1
#warning use the --marcel-nvp option to set it by hand when running your program
	return 1;
#endif
}
#endif

/*
 * How to allocate memory on specific nodes
 */

#ifdef MA__NUMA
int ma_numa_not_available = 0;
#endif

#ifdef MA__NUMA_MEMORY
#ifdef LINUX_SYS
#define HAS_NUMA
#include <numa.h>

void *ma_malloc_node(size_t size, int node, char *file, unsigned line) {
	void *p;
	if (ma_numa_not_available || node == -1 || ma_use_synthetic_topology)
		return ma_malloc_nonuma(size,file,line);
#ifndef MA__HAS_GNU_MALLOC_HOOKS
	marcel_extlib_protect();
#endif
	p = numa_alloc_onnode(size, node);
#ifndef MA__HAS_GNU_MALLOC_HOOKS
	marcel_extlib_unprotect();
#endif
	if (p == NULL)
		return ma_malloc_nonuma(size,file,line);
	return p;
}
void ma_free_node(void *data, size_t size, char * __restrict file, unsigned line) {
	if (ma_numa_not_available || ma_use_synthetic_topology)
		return ma_free_nonuma(data,file,line);
#ifndef MA__HAS_GNU_MALLOC_HOOKS
	marcel_extlib_protect();
#endif
	numa_free(data, size);
#ifndef MA__HAS_GNU_MALLOC_HOOKS
	marcel_extlib_unprotect();
#endif
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
	if (mbind((void*)addr, size, MPOL_BIND, &mask, sizeof(mask)*CHAR_BIT, MPOL_MF_MOVE_ALL) == -1 && errno == EPERM)
		mbind((void*)addr, size, MPOL_BIND, &mask, sizeof(mask)*CHAR_BIT, MPOL_MF_MOVE);
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

#endif /* MA__NUMA_MEMORY */
