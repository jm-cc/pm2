
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

/* for clone() and gettid() */
#define _GNU_SOURCE
#include <sched.h>

#include "marcel.h"

#ifdef LINUX_SYS
#include <linux/unistd.h>
#endif
#ifdef MA__LWPS
#ifdef __NR_gettid
#include <errno.h>
#  ifdef _syscall0
_syscall0(pid_t,gettid)
#  else
#    define gettid() syscall(__NR_gettid)
#  endif
#endif
int marcel_gettid(void) {
#ifdef __NR_gettid
	int ret;
/* 2.6 linux kernels and above have tids */
	if ((ret=gettid())!=-1 || errno != ENOSYS)
		return ret;
	else
		return getpid();
#else
	return getpid();
#endif
}
#endif

#ifdef MA__SMP

#ifdef MARCEL_DONT_USE_POSIX_THREADS

#ifdef LINUX_SYS

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#define __STACK_SIZE  (1024 * 1024)

// WARNING: stack is never freed. That's not a problem since kernel
// threads only terminate at the end of the program, but...
void marcel_kthread_create(marcel_kthread_t *pid, void *sp,
			   void* stack_base,
			   marcel_kthread_func_t func, void *arg)
{
  void *stack;

  LOG_IN();
  //stack = mmap(NULL, __STACK_SIZE, PROT_READ | PROT_WRITE,
  //	       MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS | MAP_GROWSDOWN,
  //	       -1, 0);
  if (!sp) {
	  stack = malloc(__STACK_SIZE);
	  sp = stack + __STACK_SIZE;
	  mdebug("Allocating stack for kthread at %p\n", stack);
  }
  sp -= 64;
  mdebug("Stack for kthread set at %p\n", sp);

  *pid = clone((int (*)(void *))func, 
	       sp,
	       CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
	       CLONE_THREAD | CLONE_PARENT |
	       SIGCHLD, arg);
  LOG_OUT();
}

void marcel_kthread_join(marcel_kthread_t pid)
{
	LOG_IN();
	waitpid(pid, NULL, 0);
	LOG_OUT();
}

void marcel_kthread_exit(void *retval)
{
	LOG_IN();
	_exit((int)retval);
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
	LOG_IN();
	kill(pid, sig);
	LOG_OUT();	
}

#else

#error NOT YET IMPLEMENTED

#endif

#else /* On utilise les pthread */

#include <errno.h>

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
	/* On ne peut pas savoir combien la libpthread veut que cela soit
	 * aligné, donc on aligne au maximum */
	stack_size=1<<(ma_fls(sp-stack_base)-1);
	if ((err=pthread_attr_setstack (&attr, stack_base, stack_size))) {
		char s[256];
		strerror_r(err,s,256);
		fprintf(stderr, "Error: pthread_attr_setstack(%p, %p, %#zx):"
			" (%d)%s\n", &attr, stack_base, stack_size, err, s);
		fprintf(stderr, "PTHREAD_STACK_MIN: %#x\n", PTHREAD_STACK_MIN);
		abort();
	}
	pthread_create(pid, &attr, func, arg);
	LOG_OUT();
}

void marcel_kthread_join(marcel_kthread_t pid)
{
	int cc;
	LOG_IN();

	do {
		cc = pthread_join(pid, NULL);
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
	LOG_IN();
	pthread_kill(pid, sig);
	LOG_OUT();
}

#endif

#endif // MA__SMP
