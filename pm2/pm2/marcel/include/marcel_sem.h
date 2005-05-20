
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

#section types
#section functions
#section inline
#section marcel_types
#section marcel_structures


#section types
typedef struct semcell_struct semcell;
#section marcel_structures
struct semcell_struct {
  marcel_t task;
  struct semcell_struct *next;
  struct semcell_struct *last; /* Valide uniquement dans la cellule de tête */
  boolean blocked;
};

#section structures
struct semaphor_struct {
  int value;
  struct semcell_struct *first,*last;
  ma_spinlock_t lock;  /* For preventing concurrent access from multiple LWPs */
};

#section types
typedef struct semaphor_struct marcel_sem_t;

#section functions
void marcel_sem_init(marcel_sem_t *s, int initial);
void marcel_sem_P(marcel_sem_t *s);
int marcel_sem_try_P(marcel_sem_t *s);
void marcel_sem_V(marcel_sem_t *s);
void marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout);

static __tbx_inline__ int marcel_sem_destroy(marcel_sem_t* s);
#section inline
static __tbx_inline__ int marcel_sem_destroy(marcel_sem_t* s)
{
  return 0;
}

