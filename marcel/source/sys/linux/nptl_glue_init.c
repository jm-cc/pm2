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
#include "marcel_pmarcel.h"


#if defined(MARCEL_LIBPTHREAD) && defined(__GLIBC__)


struct xid_command {
	int syscall_no;
	long int id[3];
	volatile int cntr;
};

static int __ma_setxid(struct xid_command *cmdp)
{
	fprintf(stderr, "TODO: execute setxid on all LWPs\n");
	return syscall(cmdp->syscall_no, cmdp->id[0], cmdp->id[1], cmdp->id[2]);
}


#ifndef __PIC__
#pragma weak __libc_setup_tls
extern void __libc_setup_tls(size_t tcbsize, size_t tcbalign);
#endif
/*
 * In principle, our pthread_* symbols already override glibc's, so
 * we can just fill the pthread_functions table with the plain symbols.
 *
 * A few cases are not handled (compatibility symbols notably). Let's MA_BUG in
 * these cases.
 */
__attribute__((noreturn)) static void error_stub(void)
{
	MA_BUG();
}

static void null_stub(void)
{
}

/*
 * Glibc uses this to know whether when canceling the main thread it should
 * kill the process (because there are no threads any more) or just kill the
 * main thread and let others orphaned.  We already handle that ourselves, so
 * just expose a safe value which reminds glibc that there is concurrency.
 */
static unsigned int nthreads = 2;

static struct pthread_functions {
	/* Taken from NPTL.  */
	int (*ptr_pthread_attr_destroy) (pthread_attr_t *);
	int (*ptr___pthread_attr_init_2_0) (pthread_attr_t *);
	int (*ptr___pthread_attr_init_2_1) (pthread_attr_t *);
	int (*ptr_pthread_attr_getdetachstate) (const pthread_attr_t *, int *);
	int (*ptr_pthread_attr_setdetachstate) (pthread_attr_t *, int);
	int (*ptr_pthread_attr_getinheritsched) (const pthread_attr_t *, int *);
	int (*ptr_pthread_attr_setinheritsched) (pthread_attr_t *, int);
	int (*ptr_pthread_attr_getschedparam) (const pthread_attr_t *,
					       struct sched_param *);
	int (*ptr_pthread_attr_setschedparam) (pthread_attr_t *,
					       const struct sched_param *);
	int (*ptr_pthread_attr_getschedpolicy) (const pthread_attr_t *, int *);
	int (*ptr_pthread_attr_setschedpolicy) (pthread_attr_t *, int);
	int (*ptr_pthread_attr_getscope) (const pthread_attr_t *, int *);
	int (*ptr_pthread_attr_setscope) (pthread_attr_t *, int);
	int (*ptr_pthread_condattr_destroy) (pthread_condattr_t *);
	int (*ptr_pthread_condattr_init) (pthread_condattr_t *);
	int (*ptr___pthread_cond_broadcast) (pthread_cond_t *);
	int (*ptr___pthread_cond_destroy) (pthread_cond_t *);
	int (*ptr___pthread_cond_init) (pthread_cond_t *, const pthread_condattr_t *);
	int (*ptr___pthread_cond_signal) (pthread_cond_t *);
	int (*ptr___pthread_cond_wait) (pthread_cond_t *, pthread_mutex_t *);
	int (*ptr___pthread_cond_timedwait) (pthread_cond_t *, pthread_mutex_t *,
					     const struct timespec *);
	/* Let's be lazy and not try to emulate glibc 2.0 ABI.  */
	int (*ptr___pthread_cond_broadcast_2_0) (void /*pthread_cond_2_0_t */ *);
	int (*ptr___pthread_cond_destroy_2_0) (void /*pthread_cond_2_0_t */ *);
	int (*ptr___pthread_cond_init_2_0) (void /*pthread_cond_2_0_t */ *,
					    const pthread_condattr_t *);
	int (*ptr___pthread_cond_signal_2_0) (void /*pthread_cond_2_0_t */ *);
	int (*ptr___pthread_cond_wait_2_0) (void /*pthread_cond_2_0_t */ *,
					    pthread_mutex_t *);
	int (*ptr___pthread_cond_timedwait_2_0) (void /*pthread_cond_2_0_t */ *,
						 pthread_mutex_t *,
						 const struct timespec *);
	int (*ptr_pthread_equal) (pthread_t, pthread_t);
	void (*ptr___pthread_exit) (void *);
	int (*ptr_pthread_getschedparam) (pthread_t, int *, struct sched_param *);
	int (*ptr_pthread_setschedparam) (pthread_t, int, const struct sched_param *);
	int (*ptr_pthread_mutex_destroy) (pthread_mutex_t *);
	int (*ptr_pthread_mutex_init) (pthread_mutex_t *, const pthread_mutexattr_t *);
	int (*ptr_pthread_mutex_lock) (pthread_mutex_t *);
	int (*ptr_pthread_mutex_unlock) (pthread_mutex_t *);
	 pthread_t(*ptr_pthread_self) (void);
	int (*ptr_pthread_setcancelstate) (int, int *);
	int (*ptr_pthread_setcanceltype) (int, int *);
	void (*ptr___pthread_cleanup_upto) (__jmp_buf, char *);
	int (*ptr___pthread_once) (pthread_once_t *, void (*)(void));
	int (*ptr___pthread_rwlock_rdlock) (pthread_rwlock_t *);
	int (*ptr___pthread_rwlock_wrlock) (pthread_rwlock_t *);
	int (*ptr___pthread_rwlock_unlock) (pthread_rwlock_t *);
	int (*ptr___pthread_key_create) (pthread_key_t *, void (*)(void *));
	void *(*ptr___pthread_getspecific) (pthread_key_t);
	int (*ptr___pthread_setspecific) (pthread_key_t, const void *);
	void (*ptr__pthread_cleanup_push_defer) (struct _pthread_cleanup_buffer *,
						 void (*)(void *), void *);
	void (*ptr__pthread_cleanup_pop_restore) (struct _pthread_cleanup_buffer *, int);
#define HAVE_PTR_NTHREADS
	unsigned int *ptr_nthreads;
	void (*ptr___pthread_unwind) (__pthread_unwind_buf_t *);
	void (*ptr__nptl_deallocate_tsd) (void);
	int (*ptr__nptl_setxid) (struct xid_command *);
	void (*ptr_freeres) (void);
} const ptr_pthread_functions = {
	.ptr_pthread_attr_destroy = pthread_attr_destroy,
	.ptr___pthread_attr_init_2_0 = (int (*)(pthread_attr_t *))error_stub,
	.ptr___pthread_attr_init_2_1 = pthread_attr_init,
	.ptr_pthread_attr_getdetachstate = pthread_attr_getdetachstate,
	.ptr_pthread_attr_setdetachstate = pthread_attr_setdetachstate,
	.ptr_pthread_attr_getinheritsched = pthread_attr_getinheritsched,
	.ptr_pthread_attr_setinheritsched = pthread_attr_setinheritsched,
	.ptr_pthread_attr_getschedparam = pthread_attr_getschedparam,
	.ptr_pthread_attr_setschedparam = pthread_attr_setschedparam,
	.ptr_pthread_attr_getschedpolicy = pthread_attr_getschedpolicy,
	.ptr_pthread_attr_setschedpolicy = pthread_attr_setschedpolicy,
	.ptr_pthread_attr_getscope = pthread_attr_getscope,
	.ptr_pthread_attr_setscope = pthread_attr_setscope,
	.ptr_pthread_condattr_destroy = pthread_condattr_destroy,
	.ptr_pthread_condattr_init = pthread_condattr_init,
	.ptr___pthread_cond_broadcast = pthread_cond_broadcast,
	.ptr___pthread_cond_destroy = pthread_cond_destroy,
	.ptr___pthread_cond_init = pthread_cond_init,
	.ptr___pthread_cond_signal = pthread_cond_signal,
	.ptr___pthread_cond_wait = pthread_cond_wait,
	.ptr___pthread_cond_timedwait = pthread_cond_timedwait,
	.ptr___pthread_cond_broadcast_2_0 = (int (*)(void *))error_stub,
	.ptr___pthread_cond_destroy_2_0 = (int (*)(void *))error_stub,
	.ptr___pthread_cond_init_2_0 = (int (*)(void *, const pthread_condattr_t *))error_stub,
	.ptr___pthread_cond_signal_2_0 = (int (*)(void *))error_stub,
	.ptr___pthread_cond_wait_2_0 = (int (*)(void *, pthread_mutex_t *))error_stub,
	.ptr___pthread_cond_timedwait_2_0 = (int (*)(void *, pthread_mutex_t *, const struct timespec *))error_stub,
	.ptr_pthread_equal = pthread_equal,
	.ptr___pthread_exit = pthread_exit,
	.ptr_pthread_getschedparam = pthread_getschedparam,
	.ptr_pthread_setschedparam = pthread_setschedparam,
	.ptr_pthread_mutex_destroy = pthread_mutex_destroy,
	.ptr_pthread_mutex_init = pthread_mutex_init,
	.ptr_pthread_mutex_lock = pthread_mutex_lock,
	.ptr_pthread_mutex_unlock = pthread_mutex_unlock,
	.ptr_pthread_self = pthread_self,
	.ptr_pthread_setcancelstate = pthread_setcancelstate,
	.ptr_pthread_setcanceltype = pthread_setcanceltype,
	.ptr___pthread_cleanup_upto = (void (*)(__jmp_buf, char *))null_stub,
	.ptr___pthread_once = pthread_once,
	.ptr___pthread_rwlock_rdlock = pthread_rwlock_rdlock,
	.ptr___pthread_rwlock_wrlock = pthread_rwlock_wrlock,
	.ptr___pthread_rwlock_unlock = pthread_rwlock_unlock,
	.ptr___pthread_key_create = pthread_key_create,
	.ptr___pthread_getspecific = pthread_getspecific,
	.ptr___pthread_setspecific = pthread_setspecific,
#ifdef MARCEL_CLEANUP_ENABLED
	.ptr__pthread_cleanup_push_defer = _pthread_cleanup_push_defer,
	.ptr__pthread_cleanup_pop_restore = _pthread_cleanup_pop_restore,
#else
	.ptr__pthread_cleanup_push_defer = 
 	       (void (*)(struct _pthread_cleanup_buffer *, void (*)(void *), void *))error_stub,
	.ptr__pthread_cleanup_pop_restore = 
	       (void (*)(struct _pthread_cleanup_buffer *, int))error_stub,
#endif
	.ptr_nthreads = &nthreads,
	.ptr___pthread_unwind = (void (*) (__pthread_unwind_buf_t *))error_stub,
	.ptr__nptl_deallocate_tsd = error_stub,
	.ptr__nptl_setxid = __ma_setxid,
	.ptr_freeres = error_stub,
};

#ifdef MA_TLS_MULTIPLE_THREADS_IN_TCB
extern void __libc_pthread_init(unsigned long int *ptr,
				void (*reclaim) (void),
				const struct pthread_functions *functions)
    ma_libc_internal_function;
#else
extern int *__libc_pthread_init(unsigned long int *ptr,
				void (*reclaim) (void),
				const struct pthread_functions *functions)
    ma_libc_internal_function;
#endif

void __pthread_initialize_minimal(void)
{
#ifndef MA_TLS_MULTIPLE_THREADS_IN_TCB
/* Pointer to the libc variable set to a nonzero value if more than one thread runs or ran. */
	int *libc_multiple_threads_ptr;
#endif

#ifndef __PIC__
	/* Unlike in the dynamically linked case the dynamic linker has not
	   taken care of initializing the TLS data structures.  */
	if (__libc_setup_tls)
		__libc_setup_tls(sizeof(lpt_tcb_t), __alignof__(lpt_tcb_t));

	/* We must prevent gcc from being clever and move any of the
	   following code ahead of the __libc_setup_tls call.   This function
	   will initialize the thread register which is subsequently
	   used.        */
	__asm __volatile("");
#endif

#ifndef MA_TLS_MULTIPLE_THREADS_IN_TCB
	libc_multiple_threads_ptr =
#endif
	    /* Like NPTL does
	     * - give glibc a pointer to our generation counter.
	     * - we do not give a reclaiming function, to let pthread_ptfork.c do
	     *   everything.
	     * - provide pthread_* functions
	     */
	    __libc_pthread_init(&ma_fork_generation, NULL, &ptr_pthread_functions);

#ifndef MA_TLS_MULTIPLE_THREADS_IN_TCB
	*libc_multiple_threads_ptr = 1;
#endif

	MARCEL_LOG("Initialisation mini libpthread marcel-based\n");
}


#endif /** MA__LIBPTHREAD & GLIBC **/
