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

#ifndef PIOM_SEM_H
#define PIOM_SEM_H

#include <sys/uio.h>
#include <stdint.h>
#include "pioman.h"
#include "pm2_list.h"

#ifdef MARCEL
//#include "marcel.h"
//#include "marcel_sem.h"

typedef marcel_sem_t piom_sem_t;

typedef struct {
    marcel_cond_t cond;
    ma_spinlock_t lock;
    marcel_sem_t sem;
    uint8_t value;
} piom_cond_t;

#else

typedef int piom_sem_t;
typedef uint8_t piom_cond_t;

#endif	/* MARCEL */

void piom_sem_P(piom_sem_t *sem);
void piom_sem_V(piom_sem_t *sem);
void piom_sem_init(piom_sem_t *sem, int initial);

void piom_cond_wait(piom_cond_t *cond, uint8_t mask);
void piom_cond_signal(piom_cond_t *cond, uint8_t mask);
int  piom_cond_test(piom_cond_t *cond, uint8_t mask);
void piom_cond_init(piom_cond_t *cond, uint8_t initial);
void piom_cond_mask(piom_cond_t *cond, uint8_t mask);

#endif	/* PIOM_SEM_H */
