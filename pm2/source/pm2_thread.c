
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

/* #define DEBUG */

#include "pm2.h"
#include "madeleine.h"

#define MAX_PARAMS   128

#define MAX_THREADS  512

// Also used in netserver.h
marcel_vpset_t __pm2_global_vpset = MARCEL_VPSET_FULL;

extern marcel_key_t pm2_mad_send_key;
extern marcel_key_t pm2_mad_recv_key;

typedef enum {
  PM2_THREAD_REGULAR,
  PM2_THREAD_SERVICE
} pm2_thread_class_t;

struct pm2_thread_arg {
  pm2_thread_class_t class;
  marcel_attr_t attr;
  pm2_func_t func;
  void *arg;
  struct pm2_thread_arg *next;
  void *mad_specific;
  void *mad_sem;
} thread_param[MAX_PARAMS];

static struct pm2_thread_arg *next_free = NULL;
static unsigned next_unalloc = 0;

static size_t the_granted_size;

static __inline__ void *pm2_thread_stack_alloc(void)
{
  return slot_general_alloc(NULL, 0, &the_granted_size, NULL, NULL);
}

static __inline__ void pm2_thread_stack_free(void *stack)
{
  slot_free(NULL, stack);
}

static struct pm2_thread_arg *__thread_alloc(void)
{
  struct pm2_thread_arg *res;

  lock_task();
  if(next_free != NULL) {
    res = next_free;
    next_free = next_free->next;
    unlock_task();

    pm2debug("Params allocated in cache\n");

  } else {
    if(next_unalloc == MAX_PARAMS)
      MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
    res = &thread_param[next_unalloc];
    next_unalloc++;
    unlock_task();

    marcel_attr_init(&res->attr);
    marcel_attr_setdetachstate(&res->attr, tbx_true);
    marcel_attr_setuserspace(&res->attr, sizeof(block_descr_t));
    marcel_attr_setactivation(&res->attr, tbx_true);
  }

  marcel_attr_setstackaddr(&res->attr, pm2_thread_stack_alloc());
  marcel_attr_setstacksize(&res->attr, the_granted_size);

  return res;
}

static __inline__ struct pm2_thread_arg *pm2_thread_alloc(pm2_thread_class_t class)
{
  struct pm2_thread_arg *res;

  res = __thread_alloc();

  res->class = class;

  if(class == PM2_THREAD_REGULAR) {
    marcel_attr_setvpset(&res->attr, __pm2_global_vpset);
    marcel_attr_setrealtime(&res->attr, MARCEL_CLASS_REGULAR);
  } else {
    marcel_attr_setvpset(&res->attr, marcel_self()->sched.lwps_allowed);
    marcel_attr_setrealtime(&res->attr,
			    MA_TASK_REAL_TIME(marcel_self()) ?
			       MARCEL_CLASS_REALTIME : MARCEL_CLASS_REGULAR);
  }

  return res;
}

static __inline__ void pm2_thread_free(struct pm2_thread_arg *ta)
{
  lock_task();
  ta->next = next_free;
  next_free = ta;
  unlock_task();
}

static void pm2_thread_term_func(void *arg)
{
  block_flush_list((block_descr_t *)(*marcel_specificdatalocation((marcel_t)arg,
								  _pm2_block_key)));

  pm2_thread_stack_free(marcel_stackbase((marcel_t)arg));
}

static any_t pm2_thread_starter(any_t arg)
{
  struct pm2_thread_arg *ta = (struct pm2_thread_arg *)arg;
  pm2_func_t _func;
  void *_arg;
  block_descr_t *bdesc;

  _func = ta->func;
  _arg = ta->arg;

  // TO BE FIXED: pour des raisons de compatibilité des codes
  // existants, le test suivant est biaisé pour l'instant...
  if(1 || ta->class == PM2_THREAD_SERVICE) {
    // On hérite des capacités d'accès au réseau que si l'on est un
    // 'thread de service'.
    marcel_setspecific(pm2_mad_recv_key, ta->mad_specific);
    marcel_setspecific(_pm2_mad_key, ta->mad_sem);
  }

  pm2_thread_free(ta);

  marcel_getuserspace(marcel_self(), (void *)&bdesc);
  block_init_list(bdesc);
  marcel_setspecific(_pm2_block_key, (any_t)bdesc);

  marcel_postexit(pm2_thread_term_func, marcel_self());

  (*_func)(_arg);

  return NULL;
}

static __inline__ marcel_t __pm2_thread_create(pm2_func_t func, void *arg,
					       pm2_thread_class_t class)
{
  struct pm2_thread_arg *ta;
  marcel_t pid;

  ta = pm2_thread_alloc(class);
  ta->func = func;
  ta->arg = arg;

  // TO BE FIXED: pour des raisons de compatibilité des codes
  // existants, le test suivant est biaisé pour l'instant...
  if(1 || class == PM2_THREAD_SERVICE) {
    ta->mad_specific = marcel_getspecific(pm2_mad_recv_key);
    ta->mad_sem = marcel_getspecific(_pm2_mad_key);
  }

  marcel_create(&pid, &ta->attr, (marcel_func_t)pm2_thread_starter, ta);

  return pid;
}

marcel_t pm2_thread_create(pm2_func_t func, void *arg)
{
  return __pm2_thread_create(func, arg, PM2_THREAD_REGULAR);
}

marcel_t pm2_service_thread_create(pm2_func_t func, void *arg)
{
  return __pm2_thread_create(func, arg, PM2_THREAD_SERVICE);
}

void pm2_thread_init(void)
{
}

void pm2_thread_exit(void)
{
}

