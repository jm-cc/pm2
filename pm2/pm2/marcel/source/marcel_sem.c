
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

void marcel_sem_init(marcel_sem_t *s, int initial)
{
  s->value = initial;
  s->first = NULL;
  ma_spin_lock_init(&s->lock);
}

void marcel_sem_P(marcel_sem_t *s)
{
  semcell c;

  LOG_IN();
  LOG_PTR("semaphore",s);

  marcel_lock_acquire(&s->lock);

  if(--(s->value) < 0) {
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(s->first == NULL)
      s->first = s->last = &c;
    else {
      s->last->next = &c;
      s->last = &c;
    }
    
    INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
	    c.blocked, 
	    marcel_lock_release(&s->lock),
	    marcel_lock_acquire(&s->lock));
  }
  marcel_lock_release(&s->lock);

  LOG_OUT();
}

int marcel_sem_try_P(marcel_sem_t *s)
{
  int result = 0;

  LOG_IN();
  LOG_PTR("semaphore",s);

  marcel_lock_acquire(&s->lock);

  result = ((s->value - 1) < 0);

  if(result) {
    marcel_lock_release(&s->lock);
  } else {
    s->value--;
    marcel_lock_release(&s->lock);
  }

  LOG_OUT();
  return !result;
}

void marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout)
{
  semcell c;

  LOG_IN();
  LOG_PTR("semaphore",s);

  marcel_lock_acquire(&s->lock);

  if(--(s->value) < 0) {
    if(timeout == 0) {
      s->value++;
      marcel_lock_release(&s->lock);
      LOG_OUT();
      RAISE(TIME_OUT);
    }
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(s->first == NULL)
      s->first = s->last = &c;
    else {
      s->last->next = &c;
      s->last = &c;
    }
    marcel_lock_release(&s->lock);
#warning timeout to manage
    RAISE(NOT_IMPLEMENTED);
    //__marcel_tempo_give_hand(timeout, &c.blocked, s);
  } else {
    marcel_lock_release(&s->lock);
  }

  LOG_OUT();
}

void marcel_sem_V(marcel_sem_t *s)
{
  semcell *c;

  LOG_IN();
  LOG_PTR("semaphore",s);

  marcel_lock_acquire(&s->lock);

  if(++(s->value) <= 0) {
    c = s->first;
    s->first = c->next;
    c->blocked = 0;
    ma_wmb();
    ma_wake_up_thread(c->task);
  }

  marcel_lock_release(&s->lock);

  LOG_OUT();
}

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

