
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
  s->lock = MARCEL_LOCK_INIT;
}

void marcel_sem_P(marcel_sem_t *s)
{
  semcell c;

  LOG_IN();

  lock_task();

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
    marcel_lock_release(&s->lock);
    marcel_give_hand(&c.blocked);
  } else {
    marcel_lock_release(&s->lock);
    unlock_task();
  }

  LOG_OUT();
}

int marcel_sem_try_P(marcel_sem_t *s)
{
  int result = 0;

  LOG_IN();

  lock_task();

  marcel_lock_acquire(&s->lock);

  result = ((s->value - 1) < 0);

  if(result) {
    marcel_lock_release(&s->lock);
    unlock_task();
  } else {
    s->value--;
    marcel_lock_release(&s->lock);
    unlock_task();
  }

  LOG_OUT();
  return !result;
}

void marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout)
{
  semcell c;

  LOG_IN();

  lock_task();

  marcel_lock_acquire(&s->lock);

  if(--(s->value) < 0) {
    if(timeout == 0) {
      s->value++;
      marcel_lock_release(&s->lock);
      unlock_task();
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
    __marcel_tempo_give_hand(timeout, &c.blocked, s);
  } else {
    marcel_lock_release(&s->lock);
    unlock_task();
  }

  LOG_OUT();
}

void marcel_sem_V(marcel_sem_t *s)
{
  semcell *c;

  LOG_IN();

  lock_task();

  marcel_lock_acquire(&s->lock);

  if(++(s->value) <= 0) {
    c = s->first;
    s->first = c->next;
    marcel_wake_task(c->task, &c->blocked);
  }

  marcel_lock_release(&s->lock);
  unlock_task();

  LOG_OUT();
}

void marcel_sem_VP(marcel_sem_t *s1, marcel_sem_t *s2)
{
  semcell c, *ce;

/* 
   V(s1)  suivi de
   P(s2)  en ne perdant pas la main entre les 2
*/

  LOG_IN();

  lock_task();

  marcel_lock_acquire(&s1->lock);

  if(++(s1->value) <= 0) {
    ce = s1->first;
    s1->first = ce->next;
    marcel_wake_task(ce->task, &ce->blocked);
  }

  marcel_lock_acquire(&s2->lock);
  marcel_lock_release(&s1->lock);

  if(--(s2->value) < 0) {
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(s2->first == NULL)
      s2->first = s2->last = &c;
    else {
      s2->last->next = &c;
      s2->last = &c;
    }
    marcel_lock_release(&s2->lock);
    marcel_give_hand(&c.blocked);
  } else {

    marcel_lock_release(&s2->lock);
    unlock_task();
  }

  LOG_OUT();
}

void marcel_sem_unlock_all(marcel_sem_t *s)
{
  semcell *cur;

  LOG_IN();

  lock_task();

  marcel_lock_acquire(&s->lock);

  if((cur = s->first) == NULL) {
    marcel_lock_release(&s->lock);
    unlock_task();
    return;
  }
  do {
    marcel_wake_task(cur->task, &cur->blocked);
    (s->value)++;
    cur = cur->next;
  } while (cur != NULL);
  s->first = NULL;
  marcel_lock_release(&s->lock);
  unlock_task();

  LOG_OUT();
}

