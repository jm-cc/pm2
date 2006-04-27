
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
#include <semaphore.h>
#include <errno.h>

void marcel_sem_init(marcel_sem_t *s, int initial)
{
  s->value = initial;
  s->first = NULL;
  ma_spin_lock_init(&s->lock);
}

DEF_POSIX(int, sem_init, (pmarcel_sem_t *s, int pshared, unsigned int initial),
		(pshared, initial),
{
  if (pshared) {
    errno = ENOSYS;
    return -1;
  }
  marcel_sem_init(s, initial);
  return 0;
})
DEF_STRONG_T(int, POSIX_NAME(sem_init), sem_init, (sem_t *s, int pshared, unsigned int initial), (pshared, initial))

DEF_POSIX(int, sem_destroy, (pmarcel_sem_t *s), (s),
{
  int res = 0;
  ma_spin_lock_bh(&s->lock);
  if (s->first) {
    errno = EBUSY;
    res = -1;
  }
  ma_spin_unlock_bh(&s->lock);
  return res;
})
DEF_STRONG_T(int, POSIX_NAME(sem_destroy), sem_destroy, (sem_t *s), (s))

void marcel_sem_P(marcel_sem_t *s)
{
  semcell c;

  LOG_IN();
  LOG_PTR("semaphore",s);

  ma_spin_lock_bh(&s->lock);

  if(--(s->value) < 0) {
    c.next = NULL;
    c.blocked = tbx_true;
    c.task = marcel_self();
    if(s->first == NULL)
      s->first = s->last = &c;
    else {
      s->last->next = &c;
      s->last = &c;
    }
    
    INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
	    c.blocked, 
	    ma_spin_unlock_bh(&s->lock),
	    ma_spin_lock_bh(&s->lock));
  }
  ma_spin_unlock_bh(&s->lock);

  LOG_OUT();
}

DEF_POSIX(int, sem_wait, (pmarcel_sem_t *s), (s),
{
  marcel_sem_P(s);
  return 0;
})
DEF_STRONG_T(int, POSIX_NAME(sem_wait), sem_wait, (sem_t *s), (s))

int marcel_sem_try_P(marcel_sem_t *s)
{
  int result = 0;

  LOG_IN();
  LOG_PTR("semaphore",s);

  ma_spin_lock_bh(&s->lock);

  result = ((s->value - 1) < 0);

  if(result) {
    ma_spin_unlock_bh(&s->lock);
  } else {
    s->value--;
    ma_spin_unlock_bh(&s->lock);
  }

  LOG_OUT();
  return !result;
}

DEF_POSIX(int, sem_trywait, (pmarcel_sem_t *s), (s),
{
  if (!marcel_sem_try_P(s)) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
})
DEF_STRONG_T(int, POSIX_NAME(sem_trywait), sem_trywait, (sem_t *s), (s))

void marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout)
{
  semcell c;

  LOG_IN();
  LOG_PTR("semaphore",s);

  ma_spin_lock_bh(&s->lock);

  if(--(s->value) < 0) {
    if(timeout == 0) {
      s->value++;
      ma_spin_unlock_bh(&s->lock);
      LOG_OUT();
      RAISE(TIME_OUT);
    }
    c.next = NULL;
    c.blocked = tbx_true;
    c.task = marcel_self();
    if(s->first == NULL)
      s->first = s->last = &c;
    else {
      s->last->next = &c;
      s->last = &c;
    }
    ma_spin_unlock_bh(&s->lock);
#ifdef PM2_DEV
#warning timeout to manage
#endif
    RAISE(NOT_IMPLEMENTED);
    //__marcel_tempo_give_hand(timeout, &c.blocked, s);
  } else {
    ma_spin_unlock_bh(&s->lock);
  }

  LOG_OUT();
}
// TODO: pmarcel_sem_timedwait

void marcel_sem_V(marcel_sem_t *s)
{
  semcell *c;

  LOG_IN();
  LOG_PTR("semaphore",s);

  ma_spin_lock_bh(&s->lock);

  if(++(s->value) <= 0) {
    c = s->first;
    s->first = c->next;
    ma_spin_unlock_bh(&s->lock);
    c->blocked = 0;
    ma_smp_wmb();
    ma_wake_up_thread(c->task);
  } else {
    ma_spin_unlock_bh(&s->lock);
  }
    
  LOG_OUT();
}

DEF_POSIX(int, sem_post, (pmarcel_sem_t *s), (s),
{
  // TODO: vérifier tout overflow de value
  marcel_sem_V(s);
  return 0;
})
DEF_STRONG_T(int, POSIX_NAME(sem_post), sem_post, (sem_t *s), (s))

DEF_POSIX(int, sem_getvalue, (pmarcel_sem_t * __restrict s, int * __restrict sval), (s, sval),
{
  ma_spin_lock_bh(&s->lock);
  *sval = s->value;
  ma_spin_unlock_bh(&s->lock);
  return 0;
})
DEF_STRONG_T(int, POSIX_NAME(sem_getvalue), sem_getvalue, (sem_t * __restrict s, int * __restrict sval), (s, sval))

void marcel_sem_VP(marcel_sem_t *s1, marcel_sem_t *s2)
{
	marcel_printf("Use marcel_cond_wait/marcel_cond_signal"
		      " instead of marcel_sem_VP/marcel_sem_unlock_all\n");
	RAISE(NOT_IMPLEMENTED);
}

void marcel_sem_unlock_all(marcel_sem_t *s)
{
	marcel_printf("Use marcel_cond_wait/marcel_cond_signal"
		      " instead of marcel_sem_VP/marcel_sem_unlock_all\n");
	RAISE(NOT_IMPLEMENTED);
}

