
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

#ifdef MA__SMP

#ifdef MARCEL_DONT_USE_POSIX_THREADS

#ifdef LINUX_SYS

#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <linux/unistd.h>

#define __STACK_SIZE  (1024 * 1024)

#ifdef __NR_gettid
_syscall0(pid_t,gettid)
#endif

// WARNING: stack is never freed. That's not a problem since kernel
// threads only terminate at the end of the program, but...
void marcel_kthread_create(marcel_kthread_t *pid, void *sp,
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
  pid_t pid;
#ifdef __NR_gettid
/* 2.6 kernels and above have tids */
  if ((pid=gettid())==-1 && errno==ENOSYS)
#endif
    pid=getpid();
  return pid;
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

#else

#include <errno.h>

void marcel_kthread_create(marcel_kthread_t *pid, void *sp,
			   marcel_kthread_func_t func, void *arg)
{
	pthread_attr_t attr;

	LOG_IN();
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setstackaddr (&attr, sp);
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
	LOG_IN();
	pthread_sigmask(how, newmask, oldmask);
	LOG_OUT();
}

void marcel_kthread_kill(marcel_kthread_t pid, int sig)
{
	LOG_IN();
	pthread_kill(pid, sig);
	LOG_OUT();
}

#endif

#endif // MA__SMP
