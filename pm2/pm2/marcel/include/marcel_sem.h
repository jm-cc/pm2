
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

#ifndef MARCEL_SEM_EST_DEF
#define MARCEL_SEM_EST_DEF

_PRIVATE_ typedef struct cell_struct {
  marcel_t task;
  struct cell_struct *next;
  boolean blocked;
} cell;

_PRIVATE_ struct semaphor_struct {
  int value;
  struct cell_struct *first,*last;
  marcel_lock_t lock;  /* For preventing concurrent access from multiple LWPs */
};

typedef struct semaphor_struct marcel_sem_t;

void marcel_sem_init(marcel_sem_t *s, int initial);
void marcel_sem_P(marcel_sem_t *s);
int marcel_sem_try_P(marcel_sem_t *s);
void marcel_sem_V(marcel_sem_t *s);
void marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout);
_PRIVATE_ void marcel_sem_VP(marcel_sem_t *s1, marcel_sem_t *s2);
_PRIVATE_ void marcel_sem_unlock_all(marcel_sem_t *s);

static __inline__ int marcel_sem_destroy(marcel_sem_t* s) __attribute__ ((unused));
static __inline__ int marcel_sem_destroy(marcel_sem_t* s)
{
  return 0;
}

#endif
