
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

#ifndef MARCEL_ATTR_EST_DEF
#define MARCEL_ATTR_EST_DEF

#include "sys/marcel_privatedefs.h"

/* in "bits/marcel_pthreadtypes.h now"
_PRIVATE_ typedef struct __marcel_attr_s {
  unsigned stack_size;
  char *stack_base;
  boolean detached;
  unsigned user_space;
  boolean immediate_activation;
  unsigned not_migratable;
  unsigned not_deviatable;
  int sched_policy;
  boolean rt_thread;
  marcel_vpmask_t vpmask;
  int flags;
} marcel_attr_t;
*/

/* detachstate */
/*#define MARCEL_CREATE_JOINABLE    FALSE
#define MARCEL_CREATE_DETACHED    TRUE
*/

/* realtime */
#define MARCEL_CLASS_REGULAR      FALSE
#define MARCEL_CLASS_REALTIME     TRUE

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

// only for internal use
int marcel_attr_setflags(marcel_attr_t *attr, int flags);
int marcel_attr_getflags(__const marcel_attr_t *attr, int *flags);

_PRIVATE_ extern marcel_attr_t marcel_attr_default;

#endif
