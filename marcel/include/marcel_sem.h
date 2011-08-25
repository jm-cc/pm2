/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_SEM_H__
#define __MARCEL_SEM_H__


#include <semaphore.h>
#include "tbx_compiler.h"
#include "tbx_types.h"
#include "marcel_config.h"
#include "sys/marcel_alias.h"
#include "marcel_spin.h"
#include "sys/marcel_types.h"


/** Public macros **/
#define MARCEL_SEM_INITIALIZER(initial) { \
	.value = initial, \
	.list = NULL, \
	.lock = MA_SPIN_LOCK_UNLOCKED \
}

#define MARCEL_SEM_FAILED  ((marcel_sem_t *) 0)
#define PMARCEL_SEM_FAILED ((pmarcel_sem_t *) 0)


/** Public data structures **/
typedef struct semcell_struct {
	marcel_t task;
	struct semcell_struct *next;
	struct semcell_struct *prev;
	tbx_bool_t blocked;
} semcell;

struct semaphor_struct {
        int value;
        struct semcell_struct *list;
        ma_spinlock_t lock;     /* For preventing concurrent access from multiple LWPs */
};


/** Public functions **/
DEC_MARCEL(void, sem_init, (marcel_sem_t * s, int initial));
DEC_MARCEL(int, sem_wait, (marcel_sem_t *s));
DEC_MARCEL(int, sem_timedwait, (marcel_sem_t *__restrict s, const struct timespec *__restrict abs_timeout));
DEC_MARCEL(int, sem_trywait, (marcel_sem_t *s));
DEC_MARCEL(int, sem_post, (marcel_sem_t *s));
DEC_MARCEL(int, sem_getvalue, (marcel_sem_t * __restrict s, int *__restrict sval) __THROW);
DEC_MARCEL(int, sem_destroy, (marcel_sem_t *s));
DEC_MARCEL(int, sem_close,(marcel_sem_t *sem));
DEC_MARCEL(marcel_sem_t*, sem_open, (const char *name, int flags,...));
DEC_MARCEL(int, sem_unlink, (const char *name));

DEC_MARCEL(void, sem_P, (marcel_sem_t * s));
DEC_MARCEL(int, sem_try_P, (marcel_sem_t * s));
DEC_MARCEL(void, sem_V, (marcel_sem_t * s));
DEC_MARCEL(int, sem_try_V, (marcel_sem_t * s));
DEC_MARCEL(int, sem_timed_P, (marcel_sem_t * s, unsigned long timeout));


#endif /** __MARCEL_SEM_H__ **/
