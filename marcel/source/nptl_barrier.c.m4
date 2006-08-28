dnl -*- linux-c -*-
include(scripts/marcel.m4)
dnl /***************************
dnl  * This is the original file
dnl  * =========================
dnl  ***************************/
/* This file has been autogenerated from __file__ */
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

#include <errno.h>
#if defined(MA__IFACE_PMARCEL) || defined(MA__IFACE_LPT) || defined(MA__LIBPTHREAD)
#include <pthread.h>
#endif

#include "marcel_fastlock.h"

/****************************************************************
 * BARRIERS
 */
 
/*****************/
/* barrierattr_init */
/*****************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, barrierattr_init,
	  (pthread_barrierattr_t * attr),
	  (attr))
DEF___LIBPTHREAD(int, barrierattr_init,
	  (pthread_barrierattr_t * attr),
	  (attr))
]])

REPLICATE_CODE([[dnl
int prefix_barrierattr_init(prefix_barrierattr_t *attr)
{
  ((struct prefix_barrierattr *)attr)->pshared = PREFIX_PROCESS_PRIVATE;

  return 0; 
}
]])

/********************/
/* barrierattr_destroy */
/********************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, barrierattr_destroy,
	  (pthread_barrierattr_t * attr),
	  (attr))
DEF___LIBPTHREAD(int, barrierattr_destroy,
	  (pthread_barrierattr_t * attr),
	  (attr))
]])

REPLICATE_CODE([[dnl
int prefix_barrierattr_destroy (prefix_barrierattr_t *attr)
{
        /* Nothing to do.  */
        return 0;
}
]])

/***********************/
/* barrierattr_getpshared */
/***********************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, barrierattr_getpshared,
	  (pthread_barrierattr_t * __restrict attr, int* __restrict pshared),
	  (attr, pshared))
]])

REPLICATE_CODE([[dnl
int prefix_barrierattr_getpshared(__const prefix_barrierattr_t *attr, int *pshared)
{
  *pshared = ((__const struct prefix_barrierattr *)attr)->pshared;

  return 0;
}
]])

/***********************/
/* barrierattr_setpshared */
/***********************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, barrierattr_setpshared,
	  (pthread_barrierattr_t * attr, int pshared),
	  (attr, pshared))
]])

REPLICATE_CODE([[dnl
int prefix_barrierattr_setpshared (prefix_barrierattr_t *attr, int pshared)
{
  struct prefix_barrierattr *iattr;

  LOG_IN();

  if (pshared != PREFIX_PROCESS_PRIVATE
      && __builtin_expect (pshared != PREFIX_PROCESS_SHARED, 0))
    LOG_RETURN(EINVAL);

  if (pshared == PREFIX_PROCESS_SHARED)
	 LOG_RETURN(ENOTSUP);   
  
  iattr = (struct prefix_barrierattr *)attr;

  iattr->pshared = pshared;

  LOG_RETURN(0);
}
]])

/***************************/
/* marcel_barrier_setcount */
/***************************/

int marcel_barrier_setcount(marcel_barrier_t *barrier, unsigned int count)
{
  struct marcel_barrier *ibarrier;

  LOG_IN();
  
  ibarrier = (struct marcel_barrier *)barrier;

  marcel_lock_acquire(&ibarrier->lock.__spinlock);
  
  ibarrier->init_count = count;
  ibarrier->leftB = count;
  ibarrier->leftE = 0;

  marcel_lock_release(&ibarrier->lock.__spinlock);

  LOG_RETURN(0);
}

/***************************/
/* marcel_barrier_getcount */
/***************************/

int marcel_barrier_getcount(const marcel_barrier_t *barrier, unsigned int *count)
{
  LOG_IN();
  
  *count = ((__const struct marcel_barrier *)barrier)->init_count;
  
  LOG_RETURN(0);
}

/***************************/
/* marcel_barrier_addcount */
/***************************/

int marcel_barrier_addcount(marcel_barrier_t *barrier, int addcount)
{
  struct marcel_barrier *ibarrier;

  LOG_IN();
  
  ibarrier = (struct marcel_barrier *)barrier;

  marcel_lock_acquire(&ibarrier->lock.__spinlock);
  
  ibarrier->init_count += addcount;
  ibarrier->leftB += addcount;

  marcel_lock_release(&ibarrier->lock.__spinlock);

  LOG_RETURN(0);
}

/*************/
/* barrier_init */
/*************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_barrier_init,
                  pthread_barrier_init, GLIBC_2_2);
]])

REPLICATE_CODE([[dnl
int prefix_barrier_init(prefix_barrier_t *barrier,
                        __const prefix_barrierattr_t *attr,
                        unsigned int count)
{
  struct prefix_barrier *ibarrier;

  LOG_IN();

  if (__builtin_expect (count == 0, 0))
    LOG_RETURN(EINVAL);

  if (attr != NULL)
  {
    struct prefix_barrierattr *iattr;

    iattr = (struct prefix_barrierattr *) attr;

    if (iattr->pshared != PREFIX_PROCESS_PRIVATE
   	 && __builtin_expect (iattr->pshared != PREFIX_PROCESS_SHARED, 0))
	    /* Invalid attribute.  */
	    LOG_RETURN(EINVAL);
  
	 if (iattr->pshared == PREFIX_PROCESS_SHARED)
	    LOG_RETURN(ENOTSUP);
  }
   
  ibarrier = (struct prefix_barrier *) barrier;

  /* Initialize the individual fields.  */
  ibarrier->lock = (struct _prefix_fastlock) MA_PREFIX_FASTLOCK_UNLOCKED;
  ibarrier->init_count = count;
  ibarrier->leftB = count;
  ibarrier->leftE = 0;

  LOG_RETURN(0);
}
]])

/****************/
/* barrier_destroy */
/****************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_barrier_destroy,
                  pthread_barrier_destroy, GLIBC_2_2);
]])

REPLICATE_CODE([[dnl
int prefix_barrier_destroy (prefix_barrier_t *barrier)
{
  struct prefix_barrier *ibarrier;
  int result = EBUSY;

  ibarrier = (struct prefix_barrier *) barrier;

  prefix_lock_acquire(&ibarrier->lock.__spinlock);
 
  if (__builtin_expect (ibarrier->leftE == 0, 1))
  {  /* The barrier is not used anymore.  */
     ibarrier->leftB = ibarrier->init_count;
     ibarrier->leftE = 0;
     result = 0;
  }
  else
  {  /* Still used, return with an error.  */
    prefix_lock_release(&ibarrier->lock.__spinlock);
  }  
  return result;
}
]])

/*************/
/* barrier_wait */
/*************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_barrier_wait,
                  pthread_barrier_wait, GLIBC_2_2);
]])

REPLICATE_CODE([[dnl
int prefix_barrier_wait(prefix_barrier_t *barrier)
{
  int result = 0;
  prefix_barrier_wait_begin(barrier);

  if (!prefix_barrier_wait_end(barrier))
     result = PREFIX_BARRIER_SERIAL_THREAD;
  return result; 
}
]])


/*****************/
/* barrier_wait_begin */
/*****************/

REPLICATE_CODE([[dnl
int prefix_barrier_wait_begin(prefix_barrier_t *barrier)
{
  struct prefix_barrier *ibarrier = (struct prefix_barrier *) barrier;

  prefix_lock_acquire(&ibarrier->lock.__spinlock);

  ibarrier->leftB --;
  int ret = ibarrier->leftB; 

  if (!ibarrier->leftB)
  {
     //fprintf(stderr,"le thread %p arrive a la barrière begin : on en a assez\n",marcel_self());
      
     /* Yes. Increment the event counter to avoid invalid wake-ups and
	     tell the current waiters that it is their turn.  */
     //++ibarrier->curr_event;

     /* Wake up everybody.  */
     do 
     {} 
     while (__prefix_unlock_spinlocked(&ibarrier->lock));

     ibarrier->leftB = ibarrier->init_count;
     ibarrier->leftE = ibarrier->init_count;
  }

  prefix_lock_release(&ibarrier->lock.__spinlock);

  return ret;
}
]])

/*****************/
/* barrier_wait_end */
/*****************/

REPLICATE_CODE([[dnl
int prefix_barrier_wait_end(prefix_barrier_t *barrier)
{
  struct prefix_barrier *ibarrier = (struct prefix_barrier *) barrier;

  /* Make sure we are alone.  */
  prefix_lock_acquire(&ibarrier->lock.__spinlock);

  /* Are these all?  */
  while (!ibarrier->leftE)
  {

     //fprintf(stderr,"le thread %p arrive a la barrier_end : il en reste %d\n",marcel_self(),ibarrier->leftE);
      
     /* The number of the event we are waiting for.  The barrier's event
	  number must be bumped before we continue.  */
     //unsigned int event = ibarrier->curr_event;

     /* Before suspending, make the barrier available to others.  */
     //prefix_lock_release(&ibarrier->lock.__spinlock);

     /* Wait for the event counter of the barrier to change.  */
     /* voir si cest bon, je pense que ça termine la boucle si on a l'événement */
     INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
        (!ibarrier->leftE),
        prefix_lock_release(&ibarrier->lock.__spinlock),
	     prefix_lock_acquire(&ibarrier->lock.__spinlock));
	
    }
    
  /* If this was the last woken thread, unlock.  */
  
  ibarrier->leftE --;
  int ret = ibarrier->leftE;
  /* We are done.  */
  prefix_lock_release(&ibarrier->lock.__spinlock);

  return ret;
}
]])
