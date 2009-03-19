
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

#section common
#include "tbx_compiler.h"
#include <semaphore.h>

#section types
typedef struct semcell_struct semcell;
#section marcel_structures
struct semcell_struct {
	marcel_t task;
	struct semcell_struct *next;
	struct semcell_struct *last;	/* Valide uniquement dans la cellule de tête */
	volatile tbx_bool_t blocked;
};

#section structures
struct semaphor_struct {
	int value;
	struct semcell_struct *first, *last;
	ma_spinlock_t lock;	/* For preventing concurrent access from multiple LWPs */
};

#section macros
#define MARCEL_SEM_INITIALIZER(initial) { \
	.value = initial, \
	.first = NULL, \
	.lock = MA_SPIN_LOCK_UNLOCKED \
}

#define PMARCEL_SEM_FAILED ((pmarcel_sem_t *) 0)

#section types
typedef struct semaphor_struct marcel_sem_t;
typedef marcel_sem_t pmarcel_sem_t;

#section functions
void marcel_sem_init(marcel_sem_t *s, int initial);
void marcel_sem_P(marcel_sem_t *s);
int marcel_sem_try_P(marcel_sem_t *s);
void marcel_sem_V(marcel_sem_t *s);
int marcel_sem_try_V(marcel_sem_t *s);
int marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout);

DEC_POSIX(int, sem_init, (pmarcel_sem_t *s, int pshared, unsigned int initial) __THROW);
DEC_POSIX(int, sem_destroy, (pmarcel_sem_t *s) __THROW);
DEC_POSIX(int, sem_wait, (pmarcel_sem_t *s) __THROW);
DEC_POSIX(int, sem_trywait, (pmarcel_sem_t *s) __THROW);
DEC_POSIX(int, sem_timedwait, (pmarcel_sem_t *__restrict sem, 
                          const struct timespec *__restrict abs_timeout) __THROW);
DEC_POSIX(int, sem_post, (pmarcel_sem_t *s) __THROW);
DEC_MARCEL_POSIX(int, sem_getvalue, (pmarcel_sem_t * __restrict s, int * __restrict sval) __THROW);

static __tbx_inline__ int marcel_sem_destroy(marcel_sem_t* s);
#section inline
static __tbx_inline__ int marcel_sem_destroy(marcel_sem_t* s)
{
	(void) s;
	return 0;
}

#section functions
DEC_POSIX(int, sem_close, (pmarcel_sem_t *sem));
DEC_POSIX(pmarcel_sem_t*, sem_open, (const char *name, int flags, ...));
DEC_POSIX(int, sem_unlink, (const char *name));

