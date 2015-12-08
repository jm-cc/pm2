
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008-2012 "the PM2 team" (see AUTHORS file)
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

#ifdef PIOMAN_MARCEL

/* ** blocking cond wait for marcel ************************ */
static inline void piom_cond_wait_blocking(piom_cond_t*cond, piom_cond_value_t mask)
{
  /* set highest priority so that the thread 
     is scheduled (almost) immediatly when done */
  struct marcel_sched_param sched_param = { .sched_priority = MA_MAX_SYS_RT_PRIO };
  struct marcel_sched_param old_param;
  marcel_sched_getparam(PIOM_SELF, &old_param);
  marcel_sched_setparam(PIOM_SELF, &sched_param);
  if(ma_in_atomic())
    {
      PIOM_FATAL("trying to wait while in scheduling hook.\n");
    }

  while(!(piom_cond_test(cond, mask)))
    {
      piom_sem_P(&cond->sem);
    }

  marcel_sched_setparam(PIOM_SELF, &old_param);
}
#endif /* PIOMAN_MARCEL */

#ifdef PIOMAN_PTHREAD
/* ** blocking cond wait *********************************** */

static inline void piom_cond_wait_blocking(piom_cond_t*cond, piom_cond_value_t mask)
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

  while(!(piom_cond_test(cond, mask)))
    {
      piom_sem_P(&cond->sem);
    }

  pthread_setschedprio(pthread_self(), old_param.sched_priority);
}
#endif /* PIOMAN_PTHREAD */

/** adaptive wait on piom cond */
void piom_cond_wait(piom_cond_t *cond, piom_cond_value_t mask)
{
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
	      /* consume the semaphore */
	      piom_sem_P(&cond->sem);
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
  while(busy_wait);
  piom_cond_wait_blocking(cond, mask);
}

#endif /* PIOMAN_MULTITHREAD */
