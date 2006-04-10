
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

#define _GNU_SOURCE /* pour sched_setaffinity sous linux */
#include "marcel.h"

#if defined(MA__SMP) && defined(MA__BIND_LWP_ON_PROCESSOR)
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

#if defined(MA__SMP) && defined(MA__BIND_LWP_ON_PROCESSOR)
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
#warning "don't know how to bind on processors on this system, please disable smp_bind_proc in flavor"
#endif
}
#endif
