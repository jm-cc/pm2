
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

extern marcel_key_t mad2_send_key, mad2_recv_key;

struct pm2_thread_arg {
  marcel_attr_t attr;
  pm2_func_t func;
  void *arg;
  struct pm2_thread_arg *next;
#ifdef MAD2
  void *mad2_specific;
#endif
  void *mad_sem;
} thread_param[MAX_PARAMS];

static struct pm2_thread_arg *next_free = NULL;
static unsigned next_unalloc = 0;

static unsigned the_granted_size;

static __inline__ void *pm2_thread_stack_alloc(void)
{
  return slot_general_alloc(NULL, 0, &the_granted_size, NULL, NULL);
}

static __inline__ void pm2_thread_stack_free(void *stack)
{
  slot_free(NULL, stack);
}

static __inline__ struct pm2_thread_arg *pm2_thread_alloc(void)
{
  struct pm2_thread_arg *res;

  lock_task();
  if(next_free != NULL) {
    res = next_free;
    next_free = next_free->next;
    unlock_task();

    mdebug("Params allocated in cache\n");

    marcel_attr_setstackaddr(&res->attr, pm2_thread_stack_alloc());
  } else {
    if(next_unalloc == MAX_PARAMS)
      RAISE(CONSTRAINT_ERROR);
    res = &thread_param[next_unalloc];
    next_unalloc++;
    unlock_task();

    marcel_attr_init(&res->attr);
    marcel_attr_setdetachstate(&res->attr, TRUE);
    marcel_attr_setuserspace(&res->attr, sizeof(block_descr_t));
    marcel_attr_setactivation(&res->attr, TRUE);
    marcel_attr_setstackaddr(&res->attr, pm2_thread_stack_alloc());
    marcel_attr_setstacksize(&res->attr, the_granted_size);
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

#ifdef MAD2
  marcel_setspecific(mad2_recv_key, ta->mad2_specific);
#endif
  marcel_setspecific(_pm2_mad_key, ta->mad_sem);

  pm2_thread_free(ta);

  marcel_getuserspace(marcel_self(), (void **)&bdesc);
  block_init_list(bdesc);
  marcel_setspecific(_pm2_block_key, (any_t)bdesc);

  marcel_cleanup_push(pm2_thread_term_func, marcel_self());

  (*_func)(_arg);

  return NULL;
}

marcel_t pm2_thread_create(pm2_func_t func, void *arg)
{
  struct pm2_thread_arg *ta;
  marcel_t pid;

  ta = pm2_thread_alloc();
  ta->func = func;
  ta->arg = arg;
#ifdef MAD2
  ta->mad2_specific = marcel_getspecific(mad2_recv_key);
#endif
  ta->mad_sem = marcel_getspecific(_pm2_mad_key);

  marcel_create(&pid, &ta->attr, (marcel_func_t)pm2_thread_starter, ta);

  return pid;
}

void pm2_thread_init(void)
{
}

void pm2_thread_exit(void)
{
}
