/*
 * PM2 Parallel Multithreaded Machine
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


#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include "marcel.h"
#include "marcel_pmarcel.h"


static int TBX_UNUSED infile = 0;

#define NOSYM(returntype, symbol, prototype, errval)			\
returntype symbol prototype					        \
{									\
	if (! infile) {							\
		infile = 1;						\
		MA_NOT_IMPLEMENTED(__func__);				\
		infile = 0;						\
	}								\
	errno = ENOSYS;							\
	return errval ;							\
}

// declare and define the fake pmarcel symbol
#define PMCL_NOSYM(returntype, symbol, prototype, errval)	        \
	returntype pmarcel_##symbol prototype ;				\
	NOSYM(returntype, pmarcel_##symbol, prototype, errval)


#ifdef MA__LIBPTHREAD

#ifdef __GLIBC__
// int pthread_mutexattr_getrobust_np(__const pthread_mutexattr_t *attr, int *robustness)
NOSYM(int, pthread_mutexattr_getrobust_np, (__const pthread_mutexattr_t * attr TBX_UNUSED, int *robustness TBX_UNUSED), -1)

// int pthread_mutexattr_getrobust(__const pthread_mutexattr_t *attr, int *robustness)
NOSYM(int, pthread_mutexattr_getrobust, (__const pthread_mutexattr_t * attr TBX_UNUSED, int *robustness TBX_UNUSED), -1)

// int pthread_mutexattr_setrobust_np(pthread_mutexattr_t *attr, int robustness)
NOSYM(int, pthread_mutexattr_setrobust_np, (pthread_mutexattr_t * attr TBX_UNUSED, int robustness TBX_UNUSED), -1)

// int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robustness)
NOSYM(int, pthread_mutexattr_setrobust, (pthread_mutexattr_t * attr TBX_UNUSED, int robustness TBX_UNUSED), -1)

// int pthread_mutex_consistent_np(pthread_mutex_t *mutex)
NOSYM(int, pthread_mutex_consistent_np, (pthread_mutex_t * mutex TBX_UNUSED), -1)
#endif

// int pthread_mutex_consistent(pthread_mutex_t *mutex)
#if !HAVE_DECL_PTHREAD_MUTEX_CONSISTENT
int pthread_mutex_consistent(pthread_mutex_t *mutex TBX_UNUSED);
#endif
NOSYM(int, pthread_mutex_consistent, (pthread_mutex_t * mutex TBX_UNUSED), -1)

// int pthread_mutexattr_getprotocol(__const pthread_mutexattr_t *__restrict __attr, int *__restrict protocol)
#ifdef __GLIBC__
NOSYM(int, pthread_mutexattr_getprotocol, (__const pthread_mutexattr_t * __restrict __attr TBX_UNUSED, int *__restrict protocol TBX_UNUSED), -1)

// int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
NOSYM(int, pthread_mutexattr_setprotocol, (pthread_mutexattr_t * attr TBX_UNUSED, int protocol TBX_UNUSED), -1)

// int pthread_mutexattr_getprioceiling(__const pthread_mutexattr_t *__restrict attr, int *__restrict prioceiling)
NOSYM(int, pthread_mutexattr_getprioceiling, (__const pthread_mutexattr_t * __restrict attr TBX_UNUSED, int *__restrict prioceiling TBX_UNUSED), -1)

// int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
NOSYM(int, pthread_mutexattr_setprioceiling, (pthread_mutexattr_t * attr TBX_UNUSED, int prioceiling TBX_UNUSED), -1)

// int pthread_mutex_getprioceiling(__const pthread_mutex_t *__restrict mutex, int *__restrict prioceiling)
NOSYM(int, pthread_mutex_getprioceiling, (__const pthread_mutex_t * __restrict mutex TBX_UNUSED, int *__restrict prioceiling TBX_UNUSED), -1)

// int pthread_mutex_setprioceiling(pthread_mutex_t *__restrict mutex, int prioceiling, int *__restrict old_ceiling)
NOSYM(int, pthread_mutex_setprioceiling, (pthread_mutex_t * __restrict mutex TBX_UNUSED, int prioceiling TBX_UNUSED, int *__restrict old_ceiling TBX_UNUSED), -1)

// void __pthread_unwind(__pthread_unwind_buf_t *buf)
#if !HAVE_DECL___PTHREAD_UNWIND
void __pthread_unwind(__pthread_unwind_buf_t *buf);
#endif
NOSYM(void, __pthread_unwind, (__pthread_unwind_buf_t * buf TBX_UNUSED),)
#endif

#endif


#ifdef MA__IFACE_PMARCEL

// int pmarcel_mutexattr_getrobust_np(__const pmarcel_mutexattr_t *attr, int *robustness)
PMCL_NOSYM(int, pmarcel_mutexattr_getrobust_np, (__const pmarcel_mutexattr_t * attr TBX_UNUSED, int *robustness TBX_UNUSED), -1)

// int pmarcel_mutexattr_getrobust(__const pmarcel_mutexattr_t *attr, int *robustness)
PMCL_NOSYM(int, pmarcel_mutexattr_getrobust, (__const pmarcel_mutexattr_t * attr TBX_UNUSED, int *robustness TBX_UNUSED), -1)

// int pmarcel_mutexattr_setrobust(pmarcel_mutexattr_t *attr, int robustness)
PMCL_NOSYM(int, pmarcel_mutexattr_setrobust, (pmarcel_mutexattr_t * attr TBX_UNUSED, int robustness TBX_UNUSED), -1)

// int pmarcel_mutex_consistent_np(pmarcel_mutex_t *mutex)
PMCL_NOSYM(int, pmarcel_mutex_consistent_np, (pmarcel_mutex_t * mutex TBX_UNUSED), -1)

// int pmarcel_mutex_consistent(pmarcel_mutex_t *mutex)
PMCL_NOSYM(int, pmarcel_mutex_consistent, (pmarcel_mutex_t * mutex TBX_UNUSED), -1)

// int pmarcel_mutexattr_getprotocol(__const pmarcel_mutexattr_t *__restrict __attr, int *__restrict protocol)
PMCL_NOSYM(int, pmarcel_mutexattr_getprotocol, (__const pmarcel_mutexattr_t * __restrict __attr TBX_UNUSED, int *__restrict protocol TBX_UNUSED), -1)

// int pmarcel_mutexattr_setprotocol(pmarcel_mutexattr_t *attr, int protocol)
PMCL_NOSYM(int, pmarcel_mutexattr_setprotocol, (pmarcel_mutexattr_t * attr TBX_UNUSED, int protocol TBX_UNUSED), -1)

// int pmarcel_mutexattr_getprioceiling(__const pmarcel_mutexattr_t *__restrict attr, int *__restrict prioceiling)
PMCL_NOSYM(int, pmarcel_mutexattr_getprioceiling, (__const pmarcel_mutexattr_t * __restrict attr TBX_UNUSED, int *__restrict prioceiling TBX_UNUSED), -1)

// int pmarcel_mutexattr_setprioceiling(pmarcel_mutexattr_t *attr, int prioceiling)
PMCL_NOSYM(int, pmarcel_mutexattr_setprioceiling, (pmarcel_mutexattr_t * attr TBX_UNUSED, int prioceiling TBX_UNUSED), -1)

// int pmarcel_mutex_getprioceiling(__const pmarcel_mutex_t *__restrict mutex, int *__restrict prioceiling)
PMCL_NOSYM(int, pmarcel_mutex_getprioceiling, (__const pmarcel_mutex_t * __restrict mutex TBX_UNUSED, int *__restrict prioceiling TBX_UNUSED), -1)

// int pmarcel_mutex_setprioceiling(pmarcel_mutex_t *__restrict mutex, int prioceiling, int *__restrict old_ceiling)
PMCL_NOSYM(int, pmarcel_mutex_setprioceiling, (pmarcel_mutex_t * __restrict mutex TBX_UNUSED, int prioceiling TBX_UNUSED, int *__restrict old_ceiling TBX_UNUSED), -1)

// void __pmarcel_unwind(__pmarcel_unwind_buf_t *buf)
PMCL_NOSYM(void, __pmarcel_unwind, (void *buf TBX_UNUSED),)

#endif
