
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

#include <string.h>
#include <errno.h>

/* Déclaré non statique car utilisé dans marcel.c : */
marcel_attr_t marcel_attr_default = MARCEL_ATTR_INITIALIZER;

/* Déclaré dans marcel.c : */
extern volatile unsigned default_stack;

#ifdef MA__POSIX_BEHAVIOUR
#include <sys/shm.h>
#endif

DEF_MARCEL_POSIX(int, attr_init, (marcel_attr_t *attr), (attr))
{
    *attr = marcel_attr_default;
#ifdef MA__POSIX_BEHAVIOUR
    attr->__guardsize = MARCEL_STACKSGUARD;
#endif
   return 0;
}
DEF_PTHREAD(int, attr_init, (pthread_attr_t *attr), (attr))
DEF___PTHREAD(int, attr_init, (pthread_attr_t *attr), (attr))

DEF_MARCEL_POSIX(int, attr_setstacksize, (marcel_attr_t *attr, size_t stack), (attr, stack))
{
   if (stack > THREAD_SLOT_SIZE) {
      errno = EINVAL;
      return 1;
   }
   attr->__stacksize = stack;
   return 0;
}
DEF_PTHREAD(int, attr_setstacksize, (pthread_attr_t *attr, size_t stack), (attr, stack))
DEF___PTHREAD(int, attr_setstacksize, (pthread_attr_t *attr, size_t stack), (attr, stack))

DEF_MARCEL_POSIX(int, attr_getstacksize, (__const marcel_attr_t *attr, size_t *stack), (stack))
{
  *stack = attr->__stacksize;
  return 0;
}
DEF_PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))
DEF___PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))

DEF_MARCEL_POSIX(int, attr_setstackaddr, (marcel_attr_t *attr, void *addr), (attr, addr))
{
   attr->__stackaddr_set = 1;
   attr->__stackaddr = addr;
   return 0;
}
DEF_PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))
DEF___PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))

DEF_MARCEL_POSIX(int, attr_getstackaddr, (__const marcel_attr_t *attr, void **addr), (attr, addr))
{
   *addr = attr->__stackaddr;
   return 0;
}
DEF_PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))
DEF___PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))

DEF_MARCEL_POSIX(int, attr_setdetachstate, (marcel_attr_t *attr, boolean detached), (attr, detached))
{
   attr->__detachstate = detached;
   return 0;
}
DEF_PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, boolean detached), (attr, detached))
DEF___PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, boolean detached), (attr, detached))

DEF_MARCEL_POSIX(int, attr_getdetachstate, (__const marcel_attr_t *attr, boolean *detached), (attr, detached))
{
   *detached = attr->__detachstate;
   return 0;
}
DEF_PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, boolean *detached), (attr, detached))
DEF___PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, boolean *detached), (attr, detached))

#ifdef MA__PTHREAD_FUNCTIONS
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize)
{
   return marcel_attr_setguardsize(attr, guardsize);
}
int pthread_attr_getguardsize(__const pthread_attr_t *attr, size_t *guardsize)
{
   return marcel_attr_getguardsize(attr, guardsize);
}
#endif

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space)
{
   attr->user_space = space;
   return 0;
}

int marcel_attr_getuserspace(__const marcel_attr_t *attr, unsigned *space)
{
   *space = attr->user_space;
   return 0;
}

int marcel_attr_setactivation(marcel_attr_t *attr, boolean immediate)
{
  attr->immediate_activation = immediate;
  return 0;
}

int marcel_attr_getactivation(__const marcel_attr_t *attr, boolean *immediate)
{
  *immediate = attr->immediate_activation;
  return 0;
}

int marcel_attr_setmigrationstate(marcel_attr_t *attr, boolean migratable)
{
   attr->not_migratable = (migratable ? 0 : 1);
   return 0;
}

int marcel_attr_getmigrationstate(__const marcel_attr_t *attr, boolean *migratable)
{
   *migratable = (attr->not_migratable ? FALSE : TRUE);
   return 0;
}

int marcel_attr_setdeviationstate(marcel_attr_t *attr, boolean deviatable)
{
   attr->not_deviatable = (deviatable ? 0 : 1);
   return 0;
}

int marcel_attr_getdeviationstate(__const marcel_attr_t *attr, boolean *deviatable)
{
   *deviatable = (attr->not_deviatable ? FALSE : TRUE);
   return 0;
}

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy)
{
  attr->__schedpolicy = policy;
  return 0;
}

int marcel_attr_getschedpolicy(__const marcel_attr_t *attr, int *policy)
{
  *policy = attr->__schedpolicy;
  return 0;
}

int marcel_attr_setrealtime(marcel_attr_t *attr, boolean realtime)
{
  return marcel_attr_setprio(attr, realtime ? MA_RT_PRIO : MA_DEF_PRIO);
}

int marcel_attr_getrealtime(__const marcel_attr_t *attr, boolean *realtime)
{
  int prio;
  marcel_attr_getprio(attr, &prio);
  *realtime = prio < MA_DEF_PRIO;
  return 0;
}

int marcel_attr_setvpmask(marcel_attr_t *attr, marcel_vpmask_t mask)
{
  attr->vpmask = mask;
  return 0;
}

int marcel_attr_getvpmask(__const marcel_attr_t *attr, marcel_vpmask_t *mask)
{
  *mask = attr->vpmask;
  return 0;
}

int marcel_attr_setname(marcel_attr_t *attr, const char *name)
{
  strncpy(attr->name,name,MARCEL_MAXNAMESIZE-1);
  attr->name[MARCEL_MAXNAMESIZE-1]='\0';
  return 0;
}

int marcel_attr_getname(__const marcel_attr_t *attr, char *name, size_t n)
{
  strncpy(name,attr->name,n);
  if (MARCEL_MAXNAMESIZE>n)
    name[n-1]='\0';
  return 0;
}

int marcel_attr_setflags(marcel_attr_t *attr, int flags)
{
  attr->flags = flags;
  return 0;
}

int marcel_attr_getflags(__const marcel_attr_t *attr, int *flags)
{
  *flags = attr->flags;
  return 0;
}

int marcel_attr_setpreemptible(marcel_attr_t *attr, int preemptible)
{
  attr->not_preemptible = preemptible?0:1;
  return 0;
}

int marcel_attr_getpreemptible(__const marcel_attr_t *attr, int *preemptible)
{
  *preemptible = !attr->not_preemptible;
  return 0;
}
