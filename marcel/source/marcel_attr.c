
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

DEF_MARCEL_POSIX(int, attr_init, (marcel_attr_t *attr), (attr),
{
    *attr = marcel_attr_default;
    return 0;
})
#ifdef MA__LIBPTHREAD
int __pthread_attr_init_2_1 (pthread_attr_t *__attr)
{
	int ret;
	marcel_attr_t attr;
	ret=marcel_attr_init(&attr);
	memcpy(__attr, &attr, sizeof(pthread_attr_t));
	return ret;
}
versioned_symbol (libpthread, __pthread_attr_init_2_1, pthread_attr_init, GLIBC_2_1);
#endif

DEF_MARCEL_POSIX(int, attr_setstacksize, (marcel_attr_t *attr, size_t stack), (attr, stack),
{
   if (stack > THREAD_SLOT_SIZE) {
      errno = EINVAL;
      return 1;
   }
   attr->__stacksize = stack;
   return 0;
})
DEF_PTHREAD(int, attr_setstacksize, (pthread_attr_t *attr, size_t stack), (attr, stack))
DEF___PTHREAD(int, attr_setstacksize, (pthread_attr_t *attr, size_t stack), (attr, stack))

DEF_MARCEL_POSIX(int, attr_getstacksize, (__const marcel_attr_t * __restrict attr, size_t * __restrict stack), (attr, stack),
{
  *stack = attr->__stacksize;
  return 0;
})
DEF_PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))
DEF___PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))

DEF_MARCEL_POSIX(int, attr_setstackaddr, (marcel_attr_t *attr, void *addr), (attr, addr),
{
   attr->__stackaddr_set = 1;
   attr->__stackaddr = addr;
   return 0;
})
DEF_PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))
DEF___PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))

DEF_MARCEL_POSIX(int, attr_getstackaddr, (__const marcel_attr_t * __restrict attr, void ** __restrict addr), (attr, addr),
{
   *addr = attr->__stackaddr;
   return 0;
})
DEF_PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))
DEF___PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))

DEF_MARCEL_POSIX(int, attr_setdetachstate, (marcel_attr_t *attr, int detached), (attr, detached),
{
   attr->__detachstate = detached;
   return 0;
})
DEF_PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, int detached), (attr, detached))
DEF___PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, int detached), (attr, detached))

DEF_MARCEL_POSIX(int, attr_getdetachstate, (__const marcel_attr_t *attr, int *detached), (attr, detached),
{
   *detached = attr->__detachstate;
   return 0;
})
DEF_PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, int *detached), (attr, detached))
DEF___PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, int *detached), (attr, detached))

#ifdef MA__LIBPTHREAD
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize)
{
   return marcel_attr_setguardsize(attr, guardsize);
}
int pthread_attr_getguardsize(__const pthread_attr_t * __restrict attr,
		size_t * __restrict guardsize)
{
   return marcel_attr_getguardsize(attr, guardsize);
}
#endif

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space)
{
   attr->user_space = space;
   return 0;
}

int marcel_attr_getuserspace(__const marcel_attr_t * __restrict attr,
		unsigned * __restrict space)
{
   *space = attr->user_space;
   return 0;
}

int marcel_attr_setactivation(marcel_attr_t *attr, tbx_bool_t immediate)
{
  attr->immediate_activation = immediate;
  return 0;
}

int marcel_attr_getactivation(__const marcel_attr_t * __restrict attr,
		tbx_bool_t * __restrict immediate)
{
  *immediate = attr->immediate_activation;
  return 0;
}

int marcel_attr_setmigrationstate(marcel_attr_t *attr, tbx_bool_t migratable)
{
   attr->not_migratable = (migratable ? 0 : 1);
   return 0;
}

int marcel_attr_getmigrationstate(__const marcel_attr_t * __restrict attr,
		tbx_bool_t * __restrict migratable)
{
   *migratable = (attr->not_migratable ? tbx_false : tbx_true);
   return 0;
}

int marcel_attr_setdeviationstate(marcel_attr_t *attr, tbx_bool_t deviatable)
{
   attr->not_deviatable = (deviatable ? 0 : 1);
   return 0;
}

int marcel_attr_getdeviationstate(__const marcel_attr_t * __restrict attr,
		tbx_bool_t *__restrict deviatable)
{
   *deviatable = (attr->not_deviatable ? tbx_false : tbx_true);
   return 0;
}

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy)
{
  attr->__schedpolicy = policy;
  return 0;
}

int marcel_attr_getschedpolicy(__const marcel_attr_t * __restrict attr,
		int * __restrict policy)
{
  *policy = attr->__schedpolicy;
  return 0;
}

int marcel_attr_setrealtime(marcel_attr_t *attr, tbx_bool_t realtime)
{
  return marcel_attr_setprio(attr, realtime ? MA_RT_PRIO : MA_DEF_PRIO);
}

int marcel_attr_getrealtime(__const marcel_attr_t * __restrict attr,
		tbx_bool_t * __restrict realtime)
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

int marcel_attr_getvpmask(__const marcel_attr_t * __restrict attr,
		marcel_vpmask_t * __restrict mask)
{
  *mask = attr->vpmask;
  return 0;
}

static void setname(char * __restrict dest, const char * __restrict src)
{
  strncpy(dest,src,MARCEL_MAXNAMESIZE-1);
  dest[MARCEL_MAXNAMESIZE-1]='\0';
}

static void getname(char * __restrict dest, const char * __restrict src, size_t n)
{
  strncpy(dest, src, n);
  if (MARCEL_MAXNAMESIZE>n)
    dest[n-1]='0';
}

int marcel_attr_setname(marcel_attr_t * __restrict attr,
		const char * __restrict name)
{
  setname(attr->name, name);
  return 0;
}

int marcel_attr_getname(__const marcel_attr_t * __restrict attr,
		char * __restrict name, size_t n)
{
  getname(name, attr->name, n);
  return 0;
}

int marcel_setname(marcel_t __restrict pid, const char * __restrict name)
{
  setname(pid->name, name);
  PROF_SET_THREAD_NAME(pid);
  return 0;
}

int marcel_getname(marcel_t __restrict pid, char * __restrict name, size_t n)
{
  getname(name, pid->name, n);
  return 0;
}

int marcel_attr_setid(marcel_attr_t * attr, int id)
{
  attr->id = id;
  return 0;
}

int marcel_attr_getid(__const marcel_attr_t * __restrict attr, int * __restrict id)
{
  *id = attr->id;
  return 0;
}

int marcel_attr_setpreemptible(marcel_attr_t *attr, int preemptible)
{
  attr->not_preemptible = preemptible?0:1;
  return 0;
}

int marcel_attr_getpreemptible(__const marcel_attr_t * __restrict attr,
		int * __restrict preemptible)
{
  *preemptible = !attr->not_preemptible;
  return 0;
}

int marcel_attr_setflags(marcel_attr_t *attr, int flags)
{
  attr->flags = flags;
  return 0;
}

int marcel_attr_getflags(__const marcel_attr_t * __restrict attr,
		int * __restrict flags)
{
  *flags = attr->flags;
  return 0;
}
