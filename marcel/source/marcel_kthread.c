
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

#define clone(...) insufficient_clone(__VA_ARGS__)
#include "marcel.h"
#include <errno.h>

#ifdef MA__LWPS
#ifdef LINUX_SYS
#include <linux/unistd.h>
int marcel_gettid(void) {
#ifdef SYS_gettid
	/* 2.6 linux kernels and above have tids */
	int ret = syscall(SYS_gettid);
	if (ret>=0 || ret != -ENOSYS)
		return ret;
	else
		return getpid();
#else
	return getpid();
#endif
}
#endif
#endif

#ifdef MA__SMP

#ifdef MARCEL_DONT_USE_POSIX_THREADS

#ifdef LINUX_SYS

// XXX: for getting rid of u32/__user of debian sarge's futex.h
#define sys_futex(...) sys_futex()
#include <linux/futex.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/syscall.h>



static int dummy;

#ifdef IA64_ARCH
int  __clone2(int (*fn) (void *arg), void *child_stack_base,
              size_t child_stack_size, int flags, void *arg,
              pid_t *parent_tid, void *tls, pid_t *child_tid);
#else
#undef clone
int  clone(int (*fn) (void *arg), void *child_stack_base,
              int flags, void *arg,
              pid_t *parent_tid, void *tls, pid_t *child_tid);
#endif


// WARNING: stack is never freed. That's not a problem since kernel
// threads only terminate at the end of the program, but...
void marcel_kthread_create(marcel_kthread_t *pid, void *sp,
			   void* stack_base,
			   marcel_kthread_func_t func, void *arg)
{
#ifdef IA64_ARCH
  size_t stack_size;
#endif

  LOG_IN();
  if (!sp) {
	  stack_base = marcel_slot_alloc();
	  sp = stack_base + THREAD_SLOT_SIZE;
	  mdebug("Allocating stack for kthread at %p\n", stack_base);
  }
  sp -= 64;
  mdebug("Stack for kthread set at %p\n", sp);

#ifdef IA64_ARCH
  stack_base=(void*)(((unsigned long int)stack_base+15)&(~0xF));
  stack_size=(sp-stack_base)&(~0xF);
  int ret = __clone2((int (*)(void *))func, 
	       stack_base, stack_size,
	       CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
	       CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID |
	       CLONE_THREAD,
	       arg, pid, &dummy, pid);
  MA_BUG_ON(ret == -1);
#else
  int ret = clone((int (*)(void *))func, 
	       sp,
	       CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
	       CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID |
	       CLONE_THREAD,
	       arg, pid, &dummy, pid);
  MA_BUG_ON(ret == -1);
#endif
  LOG_OUT();
}

void marcel_kthread_join(marcel_kthread_t *pid)
{
	LOG_IN();
	pid_t the_pid = *pid;
	if (the_pid)
		/* not dead yet, wait for it */
		while (syscall(SYS_futex, pid, FUTEX_WAIT, the_pid, NULL) == -1 && errno == EINTR);
	LOG_OUT();
}

void marcel_kthread_exit(void *retval)
{
	LOG_IN();
	syscall(SYS_exit,(int)(long)retval);
	LOG_OUT();
}

marcel_kthread_t marcel_kthread_self(void)
{
	return marcel_gettid();
}

void marcel_kthread_sigmask(int how, sigset_t *newmask, sigset_t *oldmask)
{
	sigprocmask(how, newmask, oldmask);
}

void marcel_kthread_kill(marcel_kthread_t pid, int sig)
{
	if (syscall(SYS_tkill, pid, sig) == -1 && errno == ENOSYS)
		kill(pid, sig);
}

void marcel_kthread_sem_init(marcel_kthread_sem_t *sem, int pshared, unsigned int value)
{
	ma_atomic_set(sem,value);
}

void marcel_kthread_mutex_init(marcel_kthread_mutex_t *lock)
{
	marcel_kthread_sem_init(lock,0,1);
}

void marcel_kthread_sem_wait(marcel_kthread_sem_t *sem)
{
	int newval;
	if ((newval = ma_atomic_dec_return(sem)) >= 0)
		return;
	while(1) {
		if (syscall(SYS_futex, sem, FUTEX_WAIT, newval, NULL) == 0)
			return;
		if (errno == EWOULDBLOCK)
			newval = ma_atomic_read(sem);
		else if (errno != EINTR)
			MA_BUG();
	}
}
TBX_FUN_ALIAS(void, marcel_kthread_mutex_lock, marcel_kthread_sem_wait, (marcel_kthread_mutex_t *lock), (lock));

void marcel_kthread_sem_post(marcel_kthread_sem_t *sem)
{
	if (ma_atomic_inc_return(sem) <= 0)
		syscall(SYS_futex, sem, FUTEX_WAKE, 1, NULL);
}
TBX_FUN_ALIAS(void, marcel_kthread_mutex_unlock, marcel_kthread_sem_post, (marcel_kthread_mutex_t *lock), (lock));

int marcel_kthread_sem_trywait(marcel_kthread_sem_t *sem)
{
	int oldval = ma_atomic_read(sem);
	int newval;
	if (oldval <= 0)
		return EBUSY;
	if ((newval = ma_atomic_xchg(oldval,oldval-1,sem)) == oldval)
		return 0;
	return EBUSY;
}
TBX_FUN_ALIAS(void, marcel_kthread_mutex_trylock, marcel_kthread_sem_trywait, (marcel_kthread_mutex_t *lock), (lock));

void marcel_kthread_cond_init(marcel_kthread_cond_t *cond)
{
	*cond = 0;
}

void marcel_kthread_cond_signal(marcel_kthread_cond_t *cond)
{
	syscall(SYS_futex, cond, FUTEX_WAKE, 1, NULL);
}

void marcel_kthread_cond_broadcast(marcel_kthread_cond_t *cond)
{
	syscall(SYS_futex, cond, FUTEX_WAKE, *cond, NULL);
}

void marcel_kthread_cond_wait(marcel_kthread_cond_t *cond, marcel_kthread_mutex_t *mutex)
{
	int val;
	val = ++(*cond);
	marcel_kthread_mutex_unlock(mutex);
	while(1) {
		if (syscall(SYS_futex, cond, FUTEX_WAIT, val, NULL) == 0)
			break;
		if (errno == EWOULDBLOCK)
			val = *cond;
		else if (errno != EINTR)
			MA_BUG();
	}
	marcel_kthread_mutex_lock(mutex);
	(*cond)--;
}
#else

#error NOT YET IMPLEMENTED

#endif

#else /* On utilise les pthread */

#include <errno.h>

#ifdef PTHREAD_STACK_MIN
#if ((((ASM_THREAD_SLOT_SIZE)-1)/2) < (PTHREAD_STACK_MIN))
#error please increase THREAD_SLOT_SIZE in marcel/include/sys/isomalloc_archdep.h for this system, see ia64 for an example
#endif
#endif

void marcel_kthread_create(marcel_kthread_t *pid, void *sp,
			   void *stack_base,
			   marcel_kthread_func_t func, void *arg)
{
	pthread_attr_t attr;
	int err;
	size_t stack_size;

	LOG_IN();
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	/* Contraintes d'alignement (16 pour l'IA64) */
	stack_base=(void*)(((unsigned long int)stack_base+15)&(~0xF));
	sp=(void*)(((unsigned long int)sp)&(~0xF));
	/* On ne peut pas savoir combien la libpthread veut que cela soit
	 * align�, donc on aligne au maximum */
	stack_size=1<<(ma_fls(sp-stack_base)-1);
#ifdef SOLARIS_SYS
	if ((err=pthread_attr_setstackaddr (&attr, stack_base))
	  ||(err=pthread_attr_setstacksize (&attr, stack_size))
			)
#else
	if ((err=pthread_attr_setstack (&attr, 
#ifdef AIX_SYS
					/* violation claire de posix, hum... */
					sp,
#else
					stack_base, 
#endif
					stack_size)))
#endif
	{
#ifdef SOLARIS_SYS
		char *s;
		s = strerror(err);
#else
		char s[256];
		strerror_r(err,s,256);
#endif
		fprintf(stderr, "Error: pthread_attr_setstack(%p, %p-%p (%#zx)):"
			" (%d) %s\n", &attr, stack_base, sp, stack_size, err, s);
#ifdef PTHREAD_STACK_MIN
		fprintf(stderr, "PTHREAD_STACK_MIN: %#x\n", PTHREAD_STACK_MIN);
#endif
		abort();
	}
	pthread_create(pid, &attr, func, arg);
	LOG_OUT();
}

void marcel_kthread_join(marcel_kthread_t *pid)
{
	int cc;
	LOG_IN();

	do {
		cc = pthread_join(*pid, NULL);
	} while(cc == -1 && errno == EINTR);
	LOG_OUT();

}

void marcel_kthread_exit(void *retval)
{
	LOG_IN();
	pthread_exit(retval);
	LOG_OUT();
}

marcel_kthread_t marcel_kthread_self(void)
{
	return pthread_self();
}

void marcel_kthread_sigmask(int how, sigset_t *newmask, sigset_t *oldmask)
{
	pthread_sigmask(how, newmask, oldmask);
}

void marcel_kthread_kill(marcel_kthread_t pid, int sig)
{
	pthread_kill(pid, sig);
}

void marcel_kthread_mutex_init(marcel_kthread_mutex_t *lock)
{
	pthread_mutex_init(lock,NULL);
}

void marcel_kthread_mutex_lock(marcel_kthread_mutex_t *lock)
{
	pthread_mutex_lock(lock);
}

void marcel_kthread_mutex_unlock(marcel_kthread_mutex_t *lock)
{
	pthread_mutex_unlock(lock);
}

int marcel_kthread_mutex_trylock(marcel_kthread_mutex_t *lock)
{
	return pthread_mutex_trylock(lock);
}

void marcel_kthread_sem_init(marcel_kthread_sem_t *sem, int pshared, unsigned int value)
{
	sem_init(sem,pshared,value);
}

void marcel_kthread_sem_wait(marcel_kthread_sem_t *sem)
{
	while(sem_wait(sem) == -1 && errno == EINTR);
}

void marcel_kthread_sem_post(marcel_kthread_sem_t *sem)
{
	sem_post(sem);
}

int marcel_kthread_sem_trywait(marcel_kthread_sem_t *sem)
{
	return sem_trywait(sem);
}

void marcel_kthread_cond_init(marcel_kthread_cond_t *cond)
{
	pthread_cond_init(cond, NULL);
}

void marcel_kthread_cond_signal(marcel_kthread_cond_t *cond)
{
	pthread_cond_signal(cond);
}

void marcel_kthread_cond_broadcast(marcel_kthread_cond_t *cond)
{
	pthread_cond_broadcast(cond);
}

void marcel_kthread_cond_wait(marcel_kthread_cond_t *cond, marcel_kthread_mutex_t *mutex)
{
	while (pthread_cond_wait(cond, mutex) == EINTR);
}
#endif

#endif // MA__SMP
