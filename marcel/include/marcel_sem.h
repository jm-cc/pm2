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
#include "sys/marcel_flags.h"
#include "marcel_alias.h"
#include "marcel_spin.h"
#include "marcel_types.h"


/** Public macros **/
#define MARCEL_SEM_INITIALIZER(initial) { \
	.value = initial, \
	.first = NULL, \
	.lock = MA_SPIN_LOCK_UNLOCKED \
}

#define PMARCEL_SEM_FAILED ((pmarcel_sem_t *) 0)


/** Public functions **/
void marcel_sem_init(marcel_sem_t * s, int initial);
void marcel_sem_P(marcel_sem_t * s);
int marcel_sem_try_P(marcel_sem_t * s);
void marcel_sem_V(marcel_sem_t * s);
int marcel_sem_try_V(marcel_sem_t * s);
int marcel_sem_timed_P(marcel_sem_t * s, unsigned long timeout);
/** Public inline functions **/
static __tbx_inline__ int marcel_sem_destroy(marcel_sem_t * s);
DEC_MARCEL(int, sem_getvalue, (pmarcel_sem_t * __restrict s, int *__restrict sval) __THROW);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal data structures **/
struct semcell_struct {
	marcel_t task;
	struct semcell_struct *next;
	struct semcell_struct *last;	/* Valide uniquement dans la cellule de t�te */
	volatile tbx_bool_t blocked;
};


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_SEM_H__ **/
