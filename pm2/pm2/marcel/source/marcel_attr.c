
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

/* Déclaré non statique car utilisé dans marcel.c : */
marcel_attr_t marcel_attr_default = {
  DEFAULT_STACK,           /* stack size */
  NULL,                    /* stack base */
  MARCEL_CREATE_JOINABLE,  /* detached */
  0,                       /* user space */
  FALSE,                   /* immediate activation */
  1,                       /* not_migratable */
  0,                       /* not_deviatable */
  MARCEL_SCHED_OTHER,      /* scheduling policy */
  MARCEL_CLASS_REGULAR     /* scheduling class */
};

/* Déclaré dans marcel.c : */
extern volatile unsigned default_stack;


int marcel_attr_init(marcel_attr_t *attr)
{
   *attr = marcel_attr_default;
   return 0;
}

int marcel_attr_setstacksize(marcel_attr_t *attr, size_t stack)
{
   attr->stack_size = stack;
   return 0;
}

int marcel_attr_getstacksize(marcel_attr_t *attr, size_t *stack)
{
  *stack = attr->stack_size;
  return 0;
}

int marcel_attr_setstackaddr(marcel_attr_t *attr, void *addr)
{
   attr->stack_base = addr;
   return 0;
}

int marcel_attr_getstackaddr(marcel_attr_t *attr, void **addr)
{
   *addr = attr->stack_base;
   return 0;
}

int marcel_attr_setdetachstate(marcel_attr_t *attr, boolean detached)
{
   attr->detached = detached;
   return 0;
}

int marcel_attr_getdetachstate(marcel_attr_t *attr, boolean *detached)
{
   *detached = attr->detached;
   return 0;
}

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space)
{
   attr->user_space = space;
   return 0;
}

int marcel_attr_getuserspace(marcel_attr_t *attr, unsigned *space)
{
   *space = attr->user_space;
   return 0;
}

int marcel_attr_setactivation(marcel_attr_t *attr, boolean immediate)
{
  attr->immediate_activation = immediate;
  return 0;
}

int marcel_attr_getactivation(marcel_attr_t *attr, boolean *immediate)
{
  *immediate = attr->immediate_activation;
  return 0;
}

int marcel_attr_setmigrationstate(marcel_attr_t *attr, boolean migratable)
{
   attr->not_migratable = (migratable ? 0 : 1 );
   return 0;
}

int marcel_attr_getmigrationstate(marcel_attr_t *attr, boolean *migratable)
{
   *migratable = (attr->not_migratable ? FALSE : TRUE);
   return 0;
}

int marcel_attr_setdeviationstate(marcel_attr_t *attr, boolean deviatable)
{
   attr->not_deviatable = (deviatable ? 0 : 1);
   return 0;
}

int marcel_attr_getdeviationstate(marcel_attr_t *attr, boolean *deviatable)
{
   *deviatable = (attr->not_deviatable ? FALSE : TRUE);
   return 0;
}

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy)
{
  attr->sched_policy = policy;
  return 0;
}

int marcel_attr_getschedpolicy(marcel_attr_t *attr, int *policy)
{
  *policy = attr->sched_policy;
  return 0;
}

int marcel_attr_setrealtime(marcel_attr_t *attr, boolean realtime)
{
  attr->rt_thread = realtime;
  return 0;
}

int marcel_attr_getrealtime(marcel_attr_t *attr, boolean *realtime)
{
  *realtime = attr->rt_thread;
  return 0;
}

