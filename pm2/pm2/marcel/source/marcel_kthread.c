
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

#include "sys/marcel_kthread.h"

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

#define __STACK_SIZE  (1024 * 1024)

// WARNING: stack is never freed. That's not a problem since kernel
// threads only terminate at the end of the program, but...
void marcel_kthread_create(marcel_kthread_t *pid,
			   marcel_kthread_func_t func, void *arg)
{
  void *stack;

  //stack = mmap(NULL, __STACK_SIZE, PROT_READ | PROT_WRITE,
  //	       MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS | MAP_GROWSDOWN,
  //	       -1, 0);
  stack = malloc(__STACK_SIZE);

  *pid = clone((int (*)(void *))func, (stack + __STACK_SIZE - 64),
	       CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
	       SIGCHLD, arg);
}

void marcel_kthread_join(marcel_kthread_t pid)
{
  waitpid(pid, NULL, 0);
}

void marcel_kthread_exit(void *retval)
{
  _exit((int)retval);
}

marcel_kthread_t marcel_kthread_self(void)
{
  return getpid();
}

void marcel_kthread_sigmask(int how, sigset_t *newmask, sigset_t *oldmask)
{
  sigprocmask(how, newmask, oldmask);
}

#else

#error NOT YET IMPLEMENTED

#endif

#else

#include <errno.h>

void marcel_kthread_create(marcel_kthread_t *pid,
			   marcel_kthread_func_t func, void *arg)
{
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_create(pid, &attr, func, arg);
}

void marcel_kthread_join(marcel_kthread_t pid)
{
  int cc;

  do {
    cc = pthread_join(pid, NULL);
  } while(cc == -1 && errno == EINTR);
}

void marcel_kthread_exit(void *retval)
{
  pthread_exit(retval);
}

marcel_kthread_t marcel_kthread_self(void)
{
  return pthread_self();
}

void marcel_kthread_sigmask(int how, sigset_t *newmask, sigset_t *oldmask)
{
  pthread_sigmask(how, newmask, oldmask);
}

#endif

#endif // MA__SMP
