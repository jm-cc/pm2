
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

#include "pioman.h"

__tbx_inline__ void piom_sem_P(piom_sem_t *sem)
{
#if defined (PIOMAN_LOCK_MARCEL)
  marcel_sem_P(sem);
#elif defined (PIOMAN_LOCK_PTHREAD)
  while(sem_wait(sem) == -1)
    ;
#else /* PIOMAN_LOCK_NONE */
  (*sem)--;
  while((*sem) < 0)
    {
      piom_check_polling(PIOM_POLL_AT_IDLE);
    }
#endif /* PIOMAN_LOCK_* */
}

__tbx_inline__ void piom_sem_V(piom_sem_t *sem)
{
#if defined (PIOMAN_LOCK_MARCEL)
  marcel_sem_V(sem);
#elif defined (PIOMAN_LOCK_PTHREAD)
  sem_post(sem);
#else /* PIOMAN_LOCK_NONE */
  (*sem)++;
#endif /* PIOMAN_LOCK_* */
}

__tbx_inline__ void piom_sem_init(piom_sem_t *sem, int initial)
{
#if defined (PIOMAN_LOCK_MARCEL)
  marcel_sem_init(sem, initial);
#elif defined (PIOMAN_LOCK_PTHREAD)
  sem_init(sem, 0, initial);
#else /* PIOMAN_LOCK_NONE */
  (*sem) = initial;
#endif /* PIOMAN_LOCK_* */
}

#if defined(PIOMAN_MULTITHREAD)

/* todo: get a dynamic value here !
 * it could be based on:
 * - application hints
 * - the history of previous request
 * - compiler hints
 */
/** time to do a busy wait before blocking, in usec. */
#define PIOMAN_BUSY_WAIT_USEC 50
/** number of loops between timestamps (to amortize cost of clock_gettime- ~100ns per call) */
#define PIOMAN_BUSY_WAIT_LOOP 100

__tbx_inline__ void piom_cond_wait(piom_cond_t *cond, piom_cond_value_t mask)
{
  PIOM_LOG_IN();

  /* First, let's poll for a while before blocking */
  tbx_tick_t t1;
  int busy_wait = 1;
  do
    {
      int i;
      for(i = 0; i < PIOMAN_BUSY_WAIT_LOOP ; i++)
	{
	  if(cond->value & mask)
	    {
	      /* consume the semaphore */
	      piom_sem_P(&cond->sem);
	      return;
	    }
	  piom_check_polling(PIOM_POLL_AT_IDLE);
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
	  if(TBX_TIMING_DELAY(t1, t2) > PIOMAN_BUSY_WAIT_USEC)
	    busy_wait = 0;
	}
    }
  while(busy_wait);
#ifdef PIOMAN_MARCEL
  /* set highest priority so that the thread 
     is scheduled (almost) immediatly when done */
  struct marcel_sched_param sched_param = { .sched_priority = MA_MAX_SYS_RT_PRIO };
  struct marcel_sched_param old_param;
  marcel_sched_getparam(PIOM_SELF, &old_param);
  marcel_sched_setparam(PIOM_SELF, &sched_param);
  if(ma_in_atomic())
    {
      fprintf(stderr, "pioman: FATAL- trying to wait while in scheduling hook.\n");
      abort();
    }
#endif /* PIOMAN_MARCEL */

  while(!(piom_cond_test(cond, mask)))
    {
      piom_sem_P(&cond->sem);
    }

#ifdef PIOMAN_MARCEL
  marcel_sched_setparam(PIOM_SELF, &old_param);
#endif /* PIOMAN_MARCEL */
  
  PIOM_LOG_OUT();
}

__tbx_inline__ void piom_cond_signal(piom_cond_t *cond, piom_cond_value_t mask)
{
  PIOM_LOG_IN();
  cond->value |= mask;
  piom_sem_V(&cond->sem);
  PIOM_LOG_OUT();
}

__tbx_inline__ int piom_cond_test(piom_cond_t *cond, piom_cond_value_t mask)
{
  return cond->value & mask;
}

__tbx_inline__ void piom_cond_init(piom_cond_t *cond, piom_cond_value_t initial)
{
  cond->value = initial;
  piom_sem_init(&cond->sem, 0);
}

__tbx_inline__ void piom_cond_mask(piom_cond_t *cond, piom_cond_value_t mask)
{
  cond->value &= mask;
}

#else  /* PIOMAN_MULTITHREADED */

/* Warning: we assume that Marcel is not running and there is no thread here
 * these functions are not reentrant
 */

__tbx_inline__ void piom_cond_wait(piom_cond_t *cond, piom_cond_value_t mask)
{
  while(! (*cond & mask))
    piom_check_polling(PIOM_POLL_AT_IDLE);		
}
__tbx_inline__ void piom_cond_signal(piom_cond_t *cond, piom_cond_value_t mask)
{
  *cond |= mask;
}
__tbx_inline__ int piom_cond_test(piom_cond_t *cond, piom_cond_value_t mask)
{
  return *cond & mask;
}
__tbx_inline__ void piom_cond_init(piom_cond_t *cond, piom_cond_value_t initial)
{
  *cond = initial;
}

__tbx_inline__ void piom_cond_mask(piom_cond_t *cond, piom_cond_value_t mask)
{
  *cond &= mask;
}

#endif /* PIOMAN_MULTITHREAD */
