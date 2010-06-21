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


/* POSIX a revoir :
 *
 * create
 * exit
 * detach
 * setspecific
 * getspecific
 * join
 * cancel
*/

static int TBX_UNUSED infile = 0;

#define NOSYM(returntype, symbol, prototype, errval)			\
returntype symbol prototype					        \
{									\
	if (! infile) {							\
		infile = 1;						\
		printf("libpthread(marcel): %s not yet implemented\n", __func__); \
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

// ssize_t recvmsg(int socket, struct msghdr *message, int flags)
NOSYM(ssize_t, recvmsg, (int socket TBX_UNUSED, struct msghdr * message TBX_UNUSED, int flags TBX_UNUSED), -1)

// ssize_t sendmsg(int socket, const struct msghdr *message, int flags)
NOSYM(ssize_t, sendmsg, (int socket TBX_UNUSED, const struct msghdr * message TBX_UNUSED, int flags TBX_UNUSED), -1)

// int tcdrain(int fd)
NOSYM(int, tcdrain, (int fd TBX_UNUSED), -1)

// int msync(void *addr, size_t len, int flags)
NOSYM(int, msync, (void *addr TBX_UNUSED, size_t len TBX_UNUSED, int flags TBX_UNUSED), -1)

// void pthread_kill_other_threads_np(void)
#ifndef HAVE_DECL__PTHREAD_KILL_OTHER_THREADS_NP
void pthread_kill_other_threads_np(void);
#endif
NOSYM(void, pthread_kill_other_threads_np, (void),)

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

// int pthread_mutex_consistent(pthread_mutex_t *mutex)
#ifndef HAVE__PTHREAD_MUTEX_CONSISTENT
int pthread_mutex_consistent(pthread_mutex_t *mutex TBX_UNUSED);
#endif
NOSYM(int, pthread_mutex_consistent, (pthread_mutex_t * mutex TBX_UNUSED), -1)

// int pthread_mutexattr_getprotocol(__const pthread_mutexattr_t *__restrict __attr, int *__restrict protocol)
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
#ifndef HAVE_DECL__PTHREAD_UNWIND
void __pthread_unwind(__pthread_unwind_buf_t *buf);
#endif
NOSYM(void, __pthread_unwind, (__pthread_unwind_buf_t * buf TBX_UNUSED),)

// int pthread_sigqueue(pthread_t __threadid, int __signo, const union sigval __value)
#ifndef HAVE_DECL__PTHREAD_SIGQUEUE
int pthread_sigqueue(pthread_t __threadid, int __signo, const union sigval __value);
#endif
NOSYM(int, pthread_sigqueue, (pthread_t __threadid TBX_UNUSED, int __signo TBX_UNUSED, const union sigval __value TBX_UNUSED), -1)

#ifdef HAVE__PTHREAD_CONDATTR_GETCLOCK
// int pthread_condattr_getclock(__const pthread_condattr_t *__restrict attr, __clockid_t *__restrict clock_id)
NOSYM(int, pthread_condattr_getclock, (__const pthread_condattr_t * __restrict attr TBX_UNUSED, __clockid_t * __restrict clock_id TBX_UNUSED), -1)

// int pthread_condattr_setclock(pthread_condattr_t *attr, __clockid_t clock_id)
NOSYM(int, pthread_condattr_setclock, (pthread_condattr_t * attr TBX_UNUSED, __clockid_t clock_id TBX_UNUSED), -1)
#endif

#endif


#ifdef MA__IFACE_PMARCEL

// void pmarcel_kill_other_threads_np(void)
PMCL_NOSYM(void, pmarcel_kill_other_threads_np, (void),)

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

// int pmarcel_sigqueue(pmarcel_t __threadid, int __signo, const union sigval __value)
PMCL_NOSYM(int, pmarcel_sigqueue, (pmarcel_t __threadid TBX_UNUSED, int __signo TBX_UNUSED, const union sigval __value TBX_UNUSED), -1)

#ifdef HAVE__PTHREAD_CONDATTR_GETCLOCK
// int pmarcel_condattr_getclock(__const pmarcel_condattr_t *__restrict attr, __clockid_t *__restrict clock_id)
PMCL_NOSYM(int, pmarcel_condattr_getclock, (__const pmarcel_condattr_t * __restrict attr TBX_UNUSED, __clockid_t * __restrict clock_id TBX_UNUSED), -1)

// int pmarcel_condattr_setclock(pmarcel_condattr_t *attr, __clockid_t clock_id)
PMCL_NOSYM(int, pmarcel_condattr_setclock, (pmarcel_condattr_t * attr TBX_UNUSED, __clockid_t clock_id TBX_UNUSED), -1)
#endif

#endif
