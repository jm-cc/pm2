
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

#include <stdlib.h>

#section types
typedef struct __marcel_attr_s marcel_attr_t;

#section macros
#define MARCEL_MAXNAMESIZE 16

#section structures

#ifdef LINUX_SYS
#define __need_schedparam
#include <bits/sched.h>
#else // AD: fake Linux-like structures on other systems
#include <sys/signal.h>
struct __sched_param
{
  int __sched_priority;
};
typedef sigset_t __sigset_t;
#endif

#include <sys/types.h> /* pour size_t */
#depend "marcel_sched_generic.h[]"

/* Attributes for threads.  */
struct __marcel_attr_s
{
  /* begin of pthread */
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
  /* end of pthread */

  /* marcel attributes */
  //unsigned stack_size;
  //char *stack_base;
  //int /*boolean*/ detached;
  unsigned user_space;
  /*boolean*/int immediate_activation;
  unsigned not_migratable;
  unsigned not_deviatable;
  //int sched_policy;
  /*boolean*/int rt_thread;
  marcel_vpmask_t vpmask;
  int flags;
  char name[MARCEL_MAXNAMESIZE];
  marcel_sched_attr_t sched;
};



/* detachstate */
/*#define MARCEL_CREATE_JOINABLE    FALSE
#define MARCEL_CREATE_DETACHED    TRUE
*/

/* realtime */
#define MARCEL_CLASS_REGULAR      FALSE
#define MARCEL_CLASS_REALTIME     TRUE

#section functions
#depend "marcel_utils.h[types]"

int marcel_attr_init(marcel_attr_t *attr) __THROW;
#define marcel_attr_destroy(attr_ptr)	0

int marcel_attr_setstacksize(marcel_attr_t *attr, size_t stack) __THROW;
int marcel_attr_getstacksize(__const marcel_attr_t *attr, size_t *stack) __THROW;

int marcel_attr_setstackaddr(marcel_attr_t *attr, void *addr) __THROW;
int marcel_attr_getstackaddr(__const marcel_attr_t *attr, void **addr) __THROW;

int marcel_attr_setdetachstate(marcel_attr_t *attr, boolean detached) __THROW;
int marcel_attr_getdetachstate(__const marcel_attr_t *attr, boolean *detached) __THROW;

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space);
int marcel_attr_getuserspace(__const marcel_attr_t *attr, unsigned *space);

int marcel_attr_setactivation(marcel_attr_t *attr, boolean immediate);
int marcel_attr_getactivation(__const marcel_attr_t *attr, boolean *immediate);

int marcel_attr_setmigrationstate(marcel_attr_t *attr, boolean migratable);
int marcel_attr_getmigrationstate(__const marcel_attr_t *attr, boolean *migratable);

int marcel_attr_setdeviationstate(marcel_attr_t *attr, boolean deviatable);
int marcel_attr_getdeviationstate(__const marcel_attr_t *attr, boolean *deviatable);

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy) __THROW;
int marcel_attr_getschedpolicy(__const marcel_attr_t *attr, int *policy) __THROW;

int marcel_attr_setrealtime(marcel_attr_t *attr, boolean realtime);
int marcel_attr_getrealtime(__const marcel_attr_t *attr, boolean *realtime);

int marcel_attr_setvpmask(marcel_attr_t *attr, marcel_vpmask_t mask);
int marcel_attr_getvpmask(__const marcel_attr_t *attr, marcel_vpmask_t *mask);

int marcel_attr_setname(marcel_attr_t *attr, const char name[MARCEL_MAXNAMESIZE]);
int marcel_attr_getname(__const marcel_attr_t *attr, char name[MARCEL_MAXNAMESIZE], size_t n);

// only for internal use
int marcel_attr_setflags(marcel_attr_t *attr, int flags);
int marcel_attr_getflags(__const marcel_attr_t *attr, int *flags);

#section marcel_variables
extern marcel_attr_t marcel_attr_default;

