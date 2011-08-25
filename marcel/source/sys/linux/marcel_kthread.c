
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
#ifdef MA__LWPS
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#endif


#ifdef MA__LWPS

int marcel_gettid(void)
{
#ifdef SYS_gettid
	/* 2.6 linux kernels and above have tids */
	int ret = syscall(SYS_gettid);
	if (ret >= 0 || ret != -ENOSYS)
		return ret;
	else
		return getpid();
#else
	return getpid();
#endif
}


#ifdef MARCEL_DONT_USE_POSIX_THREADS
#include <linux/futex.h>
#ifndef FUTEX_WAIT_PRIVATE
#define FUTEX_WAIT_PRIVATE FUTEX_WAIT
#define FUTEX_WAKE_PRIVATE FUTEX_WAKE
#endif

#ifdef IA64_ARCH
int __clone2(int (*fn) (void *arg), void *child_stack_base,
	     size_t child_stack_size, int flags, void *arg,
	     pid_t * parent_tid, void *tls, pid_t * child_tid);
#endif


static int dummy;

/** Todo: stack is never freed. That s not a problem since kernel threads 
    only terminate at the end of the program, but... **/
void marcel_kthread_create(marcel_kthread_t * pid, void *sp,
			   void *stack_base, marcel_kthread_func_t func, void *arg)
{
	int TBX_UNUSED ret;

	MARCEL_LOG_IN();
	if (!sp) {
		stack_base = marcel_slot_alloc(NULL);
		sp = (char *) stack_base + THREAD_SLOT_SIZE;
		MARCEL_LOG("Allocating stack for kthread at %p\n", stack_base);
	}
	sp = (char *) sp - 64;
	MARCEL_LOG("Stack for kthread set at %p\n", sp);

#ifdef IA64_ARCH
	stack_base = (void *) (((unsigned long int) stack_base + 15) & (~0xF));
	size_t stack_size = (sp - stack_base) & (~0xF);
	ret = __clone2((int (*)(void *)) func,
			   stack_base, stack_size,
			   CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
			   CLONE_DETACHED | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID |
			   CLONE_THREAD,
			   arg, pid, &dummy, pid);
	MA_BUG_ON(ret == -1);
#else
	ret = clone((int (*)(void *)) func,
		    sp,
		    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_DETACHED |
		    CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_THREAD,
		    arg, pid, &dummy, pid);
	MA_BUG_ON(ret == -1);
#endif
	MARCEL_LOG_OUT();
}

void marcel_kthread_join(marcel_kthread_t * pid)
{
	pid_t the_pid;
	MARCEL_LOG_IN();
	the_pid = *pid;
	if (the_pid)
		/* not dead yet, wait for it (Linux wakes that futex because of the CLONE_CHILD_CLEARTID flag) */
		while (syscall(SYS_futex, pid, FUTEX_WAIT, the_pid, NULL) == -1) {
			if (errno == EWOULDBLOCK) {
				/* done */
				break;
			} else if (errno != EINTR && errno != EAGAIN)
				MA_BUG();
		}
	MARCEL_LOG_OUT();
}

void marcel_kthread_exit(void *retval)
{
	MARCEL_LOG_IN();
	syscall(SYS_exit, (int) (long) retval);
	MARCEL_LOG_OUT();
}

marcel_kthread_t marcel_kthread_self(void)
{
	return marcel_gettid();
}

void marcel_kthread_kill(marcel_kthread_t pid, int sig)
{
	if (syscall(SYS_tkill, pid, sig) == -1) {
		int err TBX_UNUSED;
		MA_BUG_ON(errno != ENOSYS);
		err = ma_kill(pid, sig);
		MA_BUG_ON(err);
	}
}

void marcel_kthread_sem_init(marcel_kthread_sem_t * sem, int pshared TBX_UNUSED,
			     unsigned int value)
{
	ma_atomic_init(sem, value);
}

void marcel_kthread_mutex_init(marcel_kthread_mutex_t * lock)
{
	marcel_kthread_sem_init(lock, 0, 1);
}

void marcel_kthread_sem_wait(marcel_kthread_sem_t * sem)
{
	int newval;
	if ((newval = ma_atomic_dec_return(sem)) >= 0)
		return;
	while (syscall(SYS_futex, sem, FUTEX_WAIT_PRIVATE, newval, NULL) == -1) {
		if (errno == EWOULDBLOCK)
			newval = ma_atomic_read(sem);
		else if (errno != EINTR && errno != EAGAIN) {
			printf("%d\n", errno);
			MA_BUG();
		}
	}
}

TBX_FUN_ALIAS(void, marcel_kthread_sem_wait, marcel_kthread_mutex_lock,
	      (marcel_kthread_mutex_t * lock), (lock));

void marcel_kthread_sem_post(marcel_kthread_sem_t * sem)
{
	if (ma_atomic_inc_return(sem) <= 0)
		while (1) {
			switch (syscall(SYS_futex, sem, FUTEX_WAKE_PRIVATE, 1, NULL)) {
			case 1:
				return;
			case -1:
				perror("futex(WAKE) in kthread_sem_post");
				MA_BUG();
				break;
			case 0:
				continue;
			default:
				MA_BUG();
				break;
			}
		}
}

TBX_FUN_ALIAS(void, marcel_kthread_sem_post, marcel_kthread_mutex_unlock,
	      (marcel_kthread_mutex_t * lock), (lock));

int marcel_kthread_sem_trywait(marcel_kthread_sem_t * sem)
{
	int oldval = ma_atomic_read(sem);
	int newval TBX_UNUSED;

	if (oldval <= 0)
		return EBUSY;
	if ((newval = ma_atomic_xchg(oldval, oldval - 1, sem)) == oldval)
		return 0;
	return EBUSY;
}

TBX_FUN_ALIAS(void, marcel_kthread_sem_trywait, marcel_kthread_mutex_trylock,
	     (marcel_kthread_mutex_t * lock), (lock));

void marcel_kthread_cond_init(marcel_kthread_cond_t * cond)
{
	*cond = 0;
}

void marcel_kthread_cond_signal(marcel_kthread_cond_t * cond)
{
	if (*cond) {
		while (1) {
			switch (syscall(SYS_futex, cond, FUTEX_WAKE_PRIVATE, 1, NULL)) {
			case -1:
				perror("futex(WAKE) in kthread_cond_signal");
				MA_BUG();
			case 0:
				/* we know from the first 'if' that at                                                                               
				 * least one kthread is waiting but                                                                                  
				 * none was waken up, thus let's try                                                                                 
				 * again */
				continue;
			case 1:
				/* one waiting kthread was waken up,                                                                                 
				 * update cond state and leave */
				(*cond)--;
				return;
			default:
				/* should not be there since we expect                                                                               
				 * 0 or 1 waiting kthread to be waken                                                                                
				 * */
				MA_BUG();
			}
		}
	}
}

void marcel_kthread_cond_broadcast(marcel_kthread_cond_t * cond)
{
	while (*cond) {
		int res;
		switch ((res = syscall(SYS_futex, cond, FUTEX_WAKE_PRIVATE, *cond, NULL))) {
		case -1:
			perror("futex(WAKE) in kthread_cond_broadcast");
			MA_BUG();
		case 0:
			continue;
		default:
			if (res > 0 && res <= *cond) {
				(*cond) -= res;
			} else
				MA_BUG();
		}
	}
}

void marcel_kthread_cond_wait(marcel_kthread_cond_t * cond,
			      marcel_kthread_mutex_t * mutex)
{
	int val = ++(*cond);

	marcel_kthread_mutex_unlock(mutex);
	while (syscall(SYS_futex, cond, FUTEX_WAIT_PRIVATE, val, NULL) == -1) {
		if (errno == EWOULDBLOCK) {
			val = *cond;
		} else if (errno != EINTR && errno != EAGAIN)
			MA_BUG();
	}
	marcel_kthread_mutex_lock(mutex);
}


#else /* !MARCEL_DONT_USE_POSIX_THREADS */


void marcel_kthread_create(marcel_kthread_t * pid, void *sp,
			   void *stack_base, marcel_kthread_func_t func, void *arg)
{
	pthread_attr_t attr;
	int err;
	size_t stack_size;

	MARCEL_LOG_IN();
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	/* Contraintes d'alignement (16 pour l'IA64) */
	stack_base = (void *) (((unsigned long int) stack_base + 15) & (~0xF));
	sp = (void *) (((unsigned long int) sp) & (~0xF));
	/* On ne peut pas savoir combien la libpthread veut que cela soit
	 * aligné, donc on aligne au maximum */
	stack_size = 1 << (ma_fls((char *)sp - (char *)stack_base) - 1);
	if ((err = pthread_attr_setstack(&attr, stack_base, stack_size))) {
		char s[256];
		strerror_r(err, s, 256);
		MA_WARN_USER("error, pthread_attr_setstack(%p, %p-%p (%#zx)):"
			     " (%d) %s\n", &attr, stack_base, sp, stack_size, err, s);
#ifdef PTHREAD_STACK_MIN
		MA_WARN_USER("PTHREAD_STACK_MIN: %#x\n", PTHREAD_STACK_MIN);
#endif
		abort();
	}
	err = pthread_create(pid, &attr, func, arg);
	MA_BUG_ON(err);
	MARCEL_LOG_OUT();
}

#endif /** !DONT_USE_POSIX_THREADS **/
#endif /** MA_LWPS **/
