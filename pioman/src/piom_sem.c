
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008-2016 "the PM2 team" (see AUTHORS file)
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

#include "piom_private.h"


#if defined(PIOMAN_MULTITHREAD)

/** wait all conds */
static inline void piom_cond_wait_all_blocking(void**pp_conds, int n, uintptr_t offset, piom_cond_value_t mask)
{
  struct piom_waitsem_s waitsem;
  piom_sem_init(&waitsem.sem, 1 - n);
  waitsem.mask = mask;
  int ready = 0;
  int i;
  for(i = 0; i < n; i++)
    {
      piom_cond_t*p_cond = (pp_conds[i] + offset);
      piom_spin_lock(&p_cond->lock);
      if(piom_cond_test(p_cond, mask))
	{
	  ready++;
	}
      if(p_cond->p_waitsem != NULL)
	{
	  PIOM_FATAL("waiting on cond- sem = %p", p_cond->p_waitsem);
	  abort();
	}
      p_cond->p_waitsem = &waitsem;
      piom_spin_unlock(&p_cond->lock);
    }
  if(ready < n)
    {
      piom_sem_P(&waitsem.sem);
    }
  for(i = 0; i < n; i++)
    {
      piom_cond_t*p_cond = (pp_conds[i] + offset);
      assert(piom_cond_test(p_cond, mask));
      p_cond->p_waitsem = NULL;
    }
  return;
  piom_sem_destroy(&waitsem.sem);
}

static inline void piom_cond_wait_blocking(piom_cond_t*cond, piom_cond_value_t mask)
{
  struct piom_waitsem_s waitsem;
  piom_sem_init(&waitsem.sem, 0);
  waitsem.mask = mask;
  piom_spin_lock(&cond->lock);
  if(piom_cond_test(cond, mask))
    {
      /* early exit- signal fired between last test and actual locking */
      piom_sem_destroy(&waitsem.sem);
      piom_spin_unlock(&cond->lock);
      return;
    }
#ifdef DEBUG
  if(cond->p_waitsem != NULL)
    {
      PIOM_FATAL("another sempahore is already waiting on cond- sem = %p", cond->p_waitsem);
    }
#endif
  cond->p_waitsem = &waitsem;
  piom_spin_unlock(&cond->lock);
  
  piom_sem_P(&waitsem.sem);
  assert(piom_cond_test(cond, mask));
  cond->p_waitsem = NULL;
  piom_sem_destroy(&waitsem.sem);
}

#ifdef PIOMAN_MARCEL
#define PIOM_BLOCKING_PRIO
/* ** blocking cond wait for marcel ************************ */
static inline void piom_cond_wait_blocking_prio(piom_cond_t*cond, piom_cond_value_t mask)
{
  /* set highest priority so that the thread 
     is scheduled (almost) immediatly when done */
  struct marcel_sched_param sched_param = { .sched_priority = MA_MAX_SYS_RT_PRIO };
  struct marcel_sched_param old_param;
  marcel_sched_getparam(PIOM_THREAD_SELF, &old_param);
  marcel_sched_setparam(PIOM_THREAD_SELF, &sched_param);
  if(ma_in_atomic())
    {
      PIOM_FATAL("trying to wait while in scheduling hook.\n");
    }
  piom_cond_wait_blocking(cond, mask);
  marcel_sched_setparam(PIOM_THREAD_SELF, &old_param);
}
#endif /* PIOMAN_MARCEL */

#ifdef PIOMAN_PTHREAD
#define PIOM_BLOCKING_PRIO
/* ** blocking cond wait *********************************** */
static inline void piom_cond_wait_blocking_prio(piom_cond_t*cond, piom_cond_value_t mask)
{
  struct sched_param old_param;
  int policy = -1;
  pthread_getschedparam(pthread_self(), &policy, &old_param);
  const int prio = sched_get_priority_max(policy);
  int rc = pthread_setschedprio(pthread_self(), prio);
  if(rc != 0)
    {
      PIOM_FATAL("cannot set sched prio %d.\n", prio);
    }
  piom_cond_wait_blocking(cond, mask);
  pthread_setschedprio(pthread_self(), old_param.sched_priority);
}
#endif /* PIOMAN_PTHREAD */

/** adaptive wait on piom cond */
void piom_cond_wait(piom_cond_t*cond, piom_cond_value_t mask)
{
#if defined(DEBUG)
#  if defined(PIOMAN_PTHREAD)
  piom_pthread_check_wait();
#elif defined(PIOMAN_NOTHREAD)
  piom_nothread_check_wait();
#  endif
#endif /* DEBUG */
  /* First, let's poll for a while before blocking */
  tbx_tick_t t1;
  int busy_wait = 1;
  do
    {
      int i;
      for(i = 0; (piom_parameters.busy_wait_usec < 0) || (i < piom_parameters.busy_wait_granularity) ; i++)
	{
	  if(piom_cond_test(cond, mask))
	    {
	      return;
	    }
	  piom_ltask_schedule(PIOM_POLL_POINT_BUSY);
	}
      /* amortize cost of TBX_GET_TICK() */
      if(busy_wait == 1)
	{
	  TBX_GET_TICK(t1);
	  busy_wait = 2;
	}
      else
	{
	  tbx_tick_t t2;
	  TBX_GET_TICK(t2);
	  if(TBX_TIMING_DELAY(t1, t2) > piom_parameters.busy_wait_usec)
	    busy_wait = 0;
	}
    }
  while(busy_wait || !piom_parameters.enable_progression);
#ifdef PIOM_BLOCKING_PRIO
  piom_cond_wait_blocking_prio(cond, mask);
#else
  piom_cond_wait_blocking(cond, mask);
#endif
}

/** wait all conds */
void piom_cond_wait_all(void**pp_conds, int n, uintptr_t offset, piom_cond_value_t mask)
{
  tbx_tick_t t1;
  int busy_wait = 1;
  do
    {
      int i;
      for(i = 0; (piom_parameters.busy_wait_usec < 0) || (i < piom_parameters.busy_wait_granularity) ; i++)
	{
	  int done = 0;
	  for(i = 0; i < n; i++)
	    {
	      piom_cond_t*p_cond = (pp_conds[i] + offset);
	      if(piom_cond_test(p_cond, mask))
		{
		  done++;
		}
	    }
	  if(done == n)
	    return;
	  piom_ltask_schedule(PIOM_POLL_POINT_BUSY);
	}
      /* amortize cost of TBX_GET_TICK() */
      if(busy_wait == 1)
	{
	  TBX_GET_TICK(t1);
	  busy_wait = 2;
	}
      else
	{
	  tbx_tick_t t2;
	  TBX_GET_TICK(t2);
	  if(TBX_TIMING_DELAY(t1, t2) > piom_parameters.busy_wait_usec)
	    busy_wait = 0;
	}
    }
  while(busy_wait || !piom_parameters.enable_progression);
#if 1
  int i;
  for(i = 0; i < n; i++)
    {
      piom_cond_t*p_cond = (pp_conds[i] + offset);
#ifdef PIOM_BLOCKING_PRIO
      piom_cond_wait_blocking_prio(p_cond, mask);
#else
      piom_cond_wait_blocking(p_cond, mask);
#endif
    }
#else

  /* disabled by default */
  
  /* blocking wait on all conds */
  piom_cond_wait_all_blocking(pp_conds, n, offset, mask);
  
#endif


  
}

#endif /* PIOMAN_MULTITHREAD */
