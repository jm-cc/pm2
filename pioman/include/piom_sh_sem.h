/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_SH_SEM_H
#define PIOM_SH_SEM_H
#include "pioman.h"

#ifdef PIOM_ENABLE_SHM

struct piom_sh_sem;
typedef  struct piom_sh_sem piom_sh_sem_t;
typedef  piom_sh_sem_t* p_piom_sh_sem_t;

#include "piom_sem.h"
#include "pioman.h"
#include "tbx_fast_list.h"

#ifdef MARCEL
#include "marcel.h"
typedef ma_atomic_t piom_atomic_t;
#else
typedef int piom_atomic_t;
#endif /* MARCEL */

struct piom_sh_sem {
	piom_atomic_t value;
	piom_sem_t local_sem;
	/* list of pending sh_sem */
	struct tbx_fast_list_head pending_shs;
} ;


int piom_shs_init(piom_sh_sem_t *sem);

/* wait until the request succeeds */
int piom_shs_P(piom_sh_sem_t* sem);

/* wakes up a distant thread*/
int piom_shs_V(piom_sh_sem_t* sem);

/* returns true if the polling succeeds */
int piom_shs_test(piom_sh_sem_t* sem);
/* todo: add a tryP (could be usefull in mpich2) */

/* polling function called by pioman */
int piom_shs_poll();

/* returns true if there is request to poll */
int piom_shs_polling_is_required();

#define piom_shs_poll_success(sem) do {		\
		piom_shs_V(sem);		\
		piom_sem_V(&(sem)->local_sem);	\
	} while(0)

#endif	/* PIOM_ENABLE_SHM */
#endif /* PIOM_SH_SEM_H */

