
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
#ifdef MA__LIBPTHREAD

#include "pm2_common.h"
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <semaphore.h>


int __pthread_create_2_1(pthread_t *thread, const pthread_attr_t *attr,
                         void * (*start_routine)(void *), void *arg)
{
  static int _launched=0;
  marcel_attr_t new_attr;

  if (__builtin_expect(_launched, 1)==0) {
    marcel_start_sched(NULL, NULL);
  }

  if (attr != NULL)
    {
		if (attr->__stacksize == -1)
		{
		   fprintf(stderr,"pthread_create : attr non initialis�\n");
         return EINVAL;
		}
   /* The ATTR attribute is not really of type `pthread_attr_t *'.  It has
     the old size and access to the new members might crash the program.
     We convert the struct now.  */
      new_attr = marcel_attr_default;
      memcpy (&new_attr, attr,
              (size_t) &(((marcel_attr_t*)NULL)->user_space));
      // Le cast par (void*) est demand� par gcc pour respecter la norme C
      // sinon, on a:
      // warning: dereferencing type-punned pointer will break strict-aliasing rules
      attr = (pthread_attr_t*)(void*)&new_attr;
    }
  return marcel_create ((marcel_t*)thread, (marcel_attr_t*)attr,
			start_routine, arg);
}

versioned_symbol (libpthread, __pthread_create_2_1, pthread_create, GLIBC_2_1);

/*********************pthread_self***************************/

DEF_POSIX(pmarcel_t, self, (void), (), 
{
   return (pmarcel_t)marcel_self();
})

lpt_t lpt_self(void)
{
   return (lpt_t)pmarcel_self();
}

DEF_PTHREAD(pthread_t, self, (void), ())
DEF___PTHREAD(pthread_t, self, (void), ())



//static void pthread_initialize() __attribute__((constructor));

/* Appel� par la libc. Non utilis�e par marcel */
void __pthread_initialize_minimal(void)
{
	//marcel_init_section(MA_INIT_MAIN_LWP);
#ifdef PM2DEBUG
	printf("Initialisation mini libpthread marcel-based\n");
#endif
}

/* Appel� par la libc. Non utilis�e par marcel */
void __pthread_initialize(int* argc, char**argv)
{
	//marcel_init_section(MA_INIT_MAIN_LWP);
#ifdef PM2DEBUG
	printf("__Initialisation libpthread marcel-based\n");
#endif
	
}

//strong_alias(pthread_initialize,__pthread_initialize);
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
  return thread1 == thread2;
}

#ifdef PM2_DEV
#warning _pthread_cleanup_push,restore � �crire
#endif
#if 0
strong_alias(_pthread_cleanup_push_defer,_pthread_cleanup_push);
strong_alias(_pthread_cleanup_pop_restore,_pthread_cleanup_pop);

#warning _pthread_cleanup_push,restore � v�rifier
void _pthread_cleanup_push(struct _pthread_cleanup_buffer * buffer,
                           void (*routine)(void *), void * arg)
{
	return marcel_cleanup_push(routine, arg);
}

void _pthread_cleanup_pop(struct _pthread_cleanup_buffer * buffer,
                          int execute)
{
	return marcel_cleanup_pop(execute);
}
#endif

int __pthread_clock_settime(clockid_t clock_id, const struct timespec *tp) {
	LOG_IN();
	/* Extension Real-Time non support�e */
	errno = ENOTSUP;
	LOG_RETURN(-1);
}

int __pthread_clock_gettime(clockid_t clock_id, struct timespec *tp) {
	LOG_IN();
	/* Extension Real-Time non support�e */
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

#define cancellable_call_ext(ret, name, sysnr, ver, proto, ...) \
ret lpt_##name proto {\
	if (tbx_unlikely(!marcel_createdthreads())) { \
		return syscall(sysnr, ##__VA_ARGS__); \
	} else { \
		int old = __pmarcel_enable_asynccancel(); \
		ret res; \
		res = syscall(sysnr, ##__VA_ARGS__); \
		__pmarcel_disable_asynccancel(old); \
		return res; \
	} \
} \
versioned_symbol(libpthread, LPT_NAME(name), LIBC_NAME(name), ver); \
DEF___LIBC(ret, name, proto, (args))

#define cancellable_call(ret, name, proto, ...) \
	cancellable_call_ext(ret, name, SYS_##name, GLIBC_2_0, proto, ##__VA_ARGS__)

/*********************lseek***************************/

#if MA_BITS_PER_LONG < 64
off64_t __lseek64(int fd, off64_t offset, int whence) {
	off64_t res;
	int old = __pmarcel_enable_asynccancel();
	int ret = syscall(SYS__llseek, fd, (off_t)(offset >> 32), (off_t)(offset & 0xffffffff), &res, whence);
	__pmarcel_disable_asynccancel(old);
	return ret ? ret : res;
}
strong_alias(__lseek64, lseek64)
loff_t __llseek(int fd, loff_t offset, int whence) {
	loff_t res;
	int old = __pmarcel_enable_asynccancel();
	int ret = syscall(SYS__llseek, fd, (off_t)(offset >> 32), (off_t)(offset & 0xffffffff), &res, whence);
	__pmarcel_disable_asynccancel(old);
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
cancellable_call_ext(ssize_t, pread64, SYS_pwrite64, GLIBC_2_2, (int fd, void *buf, size_t count, off64_t pos), fd, buf, count, pos)
cancellable_call_ext(ssize_t, pwrite64, SYS_pwrite64, GLIBC_2_2, (int fd, const void *buf, size_t count, off64_t pos), fd, buf, count, pos)
cancellable_call(ssize_t, read, (int fd, void *buf, size_t count), fd, buf, count)
cancellable_call(pid_t, waitpid, (pid_t pid, int *status, int options), pid, status, options)
cancellable_call(ssize_t, write, (int fd, const void *buf, size_t count), fd, buf, count)

#endif /* MA__LIBPTHREAD */
