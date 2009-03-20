
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

#include "pm2_common.h"
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>

DEF_POSIX(pmarcel_t, self, (void), (), {
    LOG_IN(); LOG_RETURN((pmarcel_t)(marcel_self()));}

)

#ifdef MA__LIBPTHREAD
#include <sys/syscall.h>

int __pthread_create_2_1(pthread_t * thread, const pthread_attr_t * attr,
    void *(*start_routine) (void *), void *arg)
{
	LOG_IN();
	marcel_attr_t new_attr;

	if (__builtin_expect(marcel_activity, tbx_true) == tbx_false) {
		marcel_init(NULL, NULL);
  		// TODO: call libc_pthread_init
	}

	if (attr != NULL) {
		int policy;
		pmarcel_attr_getschedpolicy((pmarcel_attr_t *) attr, &policy);

		/* The ATTR attribute is not really of type `pthread_attr_t *'.  It has
		   the old size and access to the new members might crash the program.
		   We convert the struct now.  */

		new_attr = marcel_attr_default;

		memcpy(&new_attr, attr, tbx_offset_of(marcel_attr_t, __stacksize) + sizeof(new_attr.__stacksize));

		if (new_attr.__flags & MA_ATTR_FLAG_INHERITSCHED) {
			/* détermination des attributs du thread courant 
			   (marcel priority, scope, marcel policy) */

			marcel_t cthread = marcel_self();

			/* marcel policy, marcel priority : qu'on stocke dans new_attr */
			marcel_getschedparam(cthread, &new_attr.__schedpolicy,
			    &new_attr.__schedparam);
			/* and scope */
			marcel_vpset_t vpset;
			marcel_get_vpset(cthread, &vpset);
			if (marcel_vpset_isfull(&vpset))
				marcel_attr_setscope(&new_attr,
				    PTHREAD_SCOPE_PROCESS);
			else
				marcel_attr_setscope(&new_attr,
				    PTHREAD_SCOPE_SYSTEM);

			/* détermination de la policy POSIX du thread courant
			   d'après la priorité Marcel et la préemption */
			int mprio = cthread->as_entity.prio;
			if (mprio >= MA_DEF_PRIO)
				policy = SCHED_OTHER;
			else if (mprio <= MA_RT_PRIO) {
				if (marcel_some_thread_is_preemption_disabled
				    (cthread))
					policy = SCHED_FIFO;
				else
					policy = SCHED_RR;
			}
			/* policy et priority POSIX déterminées */
		}

		/* on ajoute les caractéristiques du thread courant au nouvel attribut :
		   préemption, scope (policy et priority marcel déja fait) */

		/* désactivation de la préemption */
		if (policy == SCHED_FIFO)
			marcel_attr_setpreemptible(&new_attr, 0);

		if (new_attr.__flags & MA_ATTR_FLAG_SCOPESYSTEM) {
			unsigned lwp = marcel_lwp_add_vp();
			marcel_attr_setvpset(&new_attr,
			    MARCEL_VPSET_VP(lwp));
		}
		attr = (pthread_attr_t *) (void *) &new_attr;
	}
	LOG_OUT();
	return marcel_create((marcel_t *) thread, (marcel_attr_t *) attr,
	    start_routine, arg);
}

versioned_symbol(libpthread, __pthread_create_2_1, pthread_create, GLIBC_2_1);

/*********************pthread_self***************************/

lpt_t LPT_NAME(self)(void)
{
	LOG_IN();
	LOG_RETURN((lpt_t) pmarcel_self());
}

DEF_PTHREAD(pthread_t, self, (void), ())
    DEF___PTHREAD(pthread_t, self, (void), ())


#ifdef STATIC_BUILD
#pragma weak __libc_setup_tls
extern void __libc_setup_tls (size_t tcbsize, size_t tcbalign);
#endif

//static void pthread_initialize() __attribute__((constructor));
/* Appelé par la libc pour initialiser de manière basique. */
void __pthread_initialize_minimal(void)
{
#ifdef STATIC_BUILD
	/* Unlike in the dynamically linked case the dynamic linker has not
	   taken care of initializing the TLS data structures.  */
	if (__libc_setup_tls)
		__libc_setup_tls (sizeof(lpt_tcb_t), __alignof__(lpt_tcb_t));

	/* We must prevent gcc from being clever and move any of the
	   following code ahead of the __libc_setup_tls call.	This function
	   will initialize the thread register which is subsequently
	   used.	*/
	__asm __volatile ("");
#endif

	mdebug("Initialisation mini libpthread marcel-based\n");
}

//static void pthread_finalize() __attribute__((destructor));
/*
static void pthread_finalize()
{
	printf("Ben c'est fini...\n");
	marcel_end();
}
*/

int pthread_equal(pthread_t thread1, pthread_t thread2)
{
	LOG_IN();
	LOG_RETURN(thread1 == thread2);
}

#ifdef PM2_DEV
#warning _pthread_cleanup_push,restore à écrire
#endif
#if 0
strong_alias(_pthread_cleanup_push_defer, _pthread_cleanup_push);
strong_alias(_pthread_cleanup_pop_restore, _pthread_cleanup_pop);

#warning _pthread_cleanup_push,restore à vérifier
void _pthread_cleanup_push(struct _pthread_cleanup_buffer *buffer,
    void (*routine) (void *), void *arg)
{
	LOG_IN();
	LOG_RETURN(marcel_cleanup_push(routine, arg));
}

void _pthread_cleanup_pop(struct _pthread_cleanup_buffer *buffer, int execute)
{
	LOG_IN();
	LOG_RETURN(marcel_cleanup_pop(execute));
}
#endif
int __pthread_clock_settime(clockid_t clock_id, const struct timespec *tp) {
	LOG_IN();
	/* Extension Real-Time non supportée */
	errno = ENOTSUP;
	LOG_RETURN(-1);
}

int __pthread_clock_gettime(clockid_t clock_id, struct timespec *tp) {
	LOG_IN();
	/* Extension Real-Time non supportée */
	errno = ENOTSUP;
	LOG_RETURN(-1);
}

/********************************************************/

void ma_check_lpt_sizes(void) {
	MA_BUG_ON(sizeof(pmarcel_sem_t) > sizeof(sem_t));
	MA_BUG_ON(sizeof(lpt_attr_t) > sizeof(pthread_attr_t));
	MA_BUG_ON(sizeof(lpt_mutex_t) > sizeof(pthread_mutex_t));
	MA_BUG_ON(sizeof(lpt_mutexattr_t) > sizeof(pthread_mutexattr_t));
	MA_BUG_ON(sizeof(lpt_cond_t) > sizeof(pthread_cond_t));
	MA_BUG_ON(sizeof(lpt_condattr_t) > sizeof(pthread_condattr_t));
	MA_BUG_ON(sizeof(lpt_key_t) > sizeof(pthread_key_t));
	MA_BUG_ON(sizeof(lpt_once_t) > sizeof(pthread_once_t));
	MA_BUG_ON(sizeof(lpt_rwlock_t) > sizeof(pthread_rwlock_t));
	MA_BUG_ON(sizeof(lpt_rwlockattr_t) > sizeof(pthread_rwlockattr_t));
	MA_BUG_ON(sizeof(lpt_spinlock_t) > sizeof(pthread_spinlock_t));
	MA_BUG_ON(sizeof(lpt_barrier_t) > sizeof(pthread_barrier_t));
	MA_BUG_ON(sizeof(lpt_barrierattr_t) > sizeof(pthread_barrierattr_t));
}

#define cancellable_call_generic(ret, name, invocation, ver, proto, ...) \
ret LPT_NAME(name) proto { \
	if (tbx_unlikely(!marcel_createdthreads())) { \
		return invocation; \
	} else { \
		int old = __pmarcel_enable_asynccancel(); \
		ret res; \
		res = invocation; \
		__pmarcel_disable_asynccancel(old); \
		return res; \
	} \
} \
versioned_symbol(libpthread, LPT_NAME(name), LIBC_NAME(name), ver); \
DEF___LIBC(ret, name, proto, (args))

#define cancellable_call_ext(ret, name, sysnr, ver, proto, ...)					\
  cancellable_call_generic(ret, name, syscall(sysnr, ##__VA_ARGS__), ver, proto, ##__VA_ARGS__)

#define cancellable_call(ret, name, proto, ...) \
	cancellable_call_ext(ret, name, SYS_##name, GLIBC_2_0, proto, ##__VA_ARGS__)

/*********************lseek***************************/

#if MA_BITS_PER_LONG < 64
off64_t __lseek64(int fd, off64_t offset, int whence) {
	off64_t res;
	int ret;
	if (tbx_unlikely(!marcel_createdthreads())) {
		ret = syscall(SYS__llseek, fd, (off_t)(offset >> 32), (off_t)(offset & 0xffffffff), &res, whence);
	} else {
		int old = __pmarcel_enable_asynccancel();
		ret = syscall(SYS__llseek, fd, (off_t)(offset >> 32), (off_t)(offset & 0xffffffff), &res, whence);
		__pmarcel_disable_asynccancel(old);
	}
	return ret ? ret : res;
}
strong_alias(__lseek64, lseek64)

loff_t __llseek(int fd, loff_t offset, int whence) {
	loff_t res;
	int ret;
	if (tbx_unlikely(!marcel_createdthreads())) {
		ret = syscall(SYS__llseek, fd, (off_t)(offset >> 32), (off_t)(offset & 0xffffffff), &res, whence);
	} else {
		int old = __pmarcel_enable_asynccancel();
		ret = syscall(SYS__llseek, fd, (off_t)(offset >> 32), (off_t)(offset & 0xffffffff), &res, whence);
		__pmarcel_disable_asynccancel(old);
	}
	return ret ? ret : res;
}
strong_alias(__llseek, llseek)
#endif

cancellable_call(int, close, (int fd), (fd))
cancellable_call(int, fcntl, (int fd, int cmd, long arg), fd, cmd, arg)
cancellable_call(int, fsync, (int fd), (fd))
cancellable_call(off_t, lseek, (int fd, off_t offset, int whence), fd, offset, whence)
cancellable_call(int, open, (const char *path, int flags, mode_t mode), path, flags, mode)
cancellable_call_ext(int, open64, SYS_open, GLIBC_2_2, (const char *path, int flags, mode_t mode), path, flags, mode)
#ifdef SYS_pread
cancellable_call_ext(ssize_t, pread, SYS_pread, GLIBC_2_2, (int fd, void *buf, size_t count, off_t pos), fd, buf, count, pos)
#endif
#ifdef SYS_pread64
cancellable_call_ext(ssize_t, pread64, SYS_pread64, GLIBC_2_2, (int fd, void *buf, size_t count, off64_t pos), fd, buf, count, pos)
#endif
#ifdef SYS_pwrite
cancellable_call_ext(ssize_t, pwrite, SYS_pwrite, GLIBC_2_2, (int fd, const void *buf, size_t count, off_t pos), fd, buf, count, pos)
#endif
#ifdef SYS_pwrite64
cancellable_call_ext(ssize_t, pwrite64, SYS_pwrite64, GLIBC_2_2, (int fd, const void *buf, size_t count, off64_t pos), fd, buf, count, pos)
#endif
cancellable_call(ssize_t, read, (int fd, void *buf, size_t count), fd, buf, count)
#ifdef SYS_waitpid
cancellable_call(pid_t, waitpid, (pid_t pid, int *status, int options), pid, status, options)
#elif SYS_wait4
/* On `x86_64-unknown-linux-gnu', waitpid(2) is implemented in terms of
 * wait4(2).  */
cancellable_call_generic (pid_t, waitpid,
				syscall (SYS_wait4, pid, status, options, NULL),
				GLIBC_2_2_5,
				(pid_t pid, int *status, int options),
				pid, status, options)
#endif

cancellable_call(ssize_t, write, (int fd, const void *buf, size_t count), fd, buf, count)

#ifdef SYS_connect
cancellable_call(int, connect, (int sockfd, const struct sockaddr *serv_addr,
				socklen_t addrlen), sockfd, serv_addr, addrlen)
#endif
#ifdef SYS_accept
cancellable_call(int, accept, (int sockfd, struct sockaddr *addr, 
				socklen_t *addrlen), sockfd, addr, addrlen)
#endif
#ifdef SYS_sendto
cancellable_call(ssize_t, sendto, (int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen), s, buf, len, flags, to, tolen)
#endif
#ifdef SYS_recvfrom
cancellable_call(ssize_t, recvfrom, (int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen), s, buf, len, flags, from, fromlen)
#endif

ssize_t send (int s, const void *buf, size_t len, int flags) {
	return sendto(s, buf, len, flags, NULL, 0);
}
ssize_t recv (int s, void *buf, size_t len, int flags) {
	return recvfrom(s, buf, len, flags, NULL, NULL);
}

int marcel_cpuset2vpset(size_t cpusetsize, const cpu_set_t *cpuset, marcel_vpset_t *vpset)
{
	int i;
	marcel_vpset_zero(vpset);
	for (i = 0; i < cpusetsize * CHAR_BIT; i++) {
		if (CPU_ISSET(i, cpuset)) {
			if (i >= MARCEL_NBMAXCPUS) {
#ifdef MA__DEBUG
				mdebug("cpuset2vpset: invalid VP %d\n", i);
#endif
				return EINVAL;
			}
			marcel_vpset_set(vpset, i);
		} else {
			if (i < MARCEL_NBMAXCPUS)
				marcel_vpset_clr(vpset, i);
		}
	}
	return 0;
}

int marcel_vpset2cpuset(const marcel_vpset_t *vpset, size_t cpusetsize, cpu_set_t *cpuset)
{
	int i;
	CPU_ZERO_S(cpusetsize, cpuset);
	for (i = 0; i < MARCEL_NBMAXCPUS; i++) {
		if (marcel_vpset_isset(vpset, i)) {
			fprintf(stderr,"%d is set\n", i);
			if (i >= cpusetsize * CHAR_BIT) {
#ifdef MA__DEBUG
				mdebug("cpuset2vpset: VP %d beyond user-provided buffer\n", i);
#endif
				return EINVAL;
			}
			CPU_SET(i, cpuset);
		} else {
			if (i < cpusetsize * CHAR_BIT)
				CPU_CLR(i, cpuset);
		}
	}
	return 0;
}


#ifdef __GLIBC__

/* Glibc/NPTL cleanup function stubs.  These symbols are required, e.g., by
   `librt.so' from Glibc 2.7, with version `GLIBC_2.3.3'.

   FIXME: Implement them.  */

void __cleanup_fct_attribute
__pthread_register_cancel(__pthread_unwind_buf_t *buf)
{ MA_BUG(); }

void __cleanup_fct_attribute
__pthread_unregister_cancel(__pthread_unwind_buf_t *buf)
{ MA_BUG(); }

void __cleanup_fct_attribute
__pthread_unwind_next(__pthread_unwind_buf_t *buf)
{ MA_BUG(); }

#endif

#endif /* MA__LIBPTHREAD */
