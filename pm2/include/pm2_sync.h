
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

#ifndef PM2_SYNC_EST_DEF
#define PM2_SYNC_EST_DEF

#define _MAX_PROT_PER_BARRIER 10

#include "marcel.h"

typedef struct {
  int *tab_sync;
  marcel_mutex_t sync_mutex;
} pm2_barrier_t;


typedef struct {
  marcel_mutex_t mutex;
  marcel_cond_t cond;
  pm2_barrier_t node_barrier;
  int local;
  int nb;
  int prot[_MAX_PROT_PER_BARRIER];
  int nb_prot;
} pm2_thread_barrier_t;

typedef struct {
  int local;
  int prot[_MAX_PROT_PER_BARRIER];
  int nb_prot;
} pm2_thread_barrier_attr_t;



static __inline__ void pm2_thread_barrier_attr_register_protocol(pm2_thread_barrier_attr_t *attr, int prot) __attribute__ ((unused));
static __inline__ void pm2_thread_barrier_attr_register_protocol(pm2_thread_barrier_attr_t *attr, int prot)
{
  fprintf(stderr, "nb prot = %d\n", attr->nb_prot);
  if (attr->nb_prot >= _MAX_PROT_PER_BARRIER)
    MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
  attr->prot[attr->nb_prot++] = prot;
}


void pm2_barrier_init_rpc();

void pm2_sync_init(int myself, int confsize);


/* The following two primitives need to be called by PM2 in a startup function,
to allow for proper barrier initialization. */
void pm2_barrier_init(pm2_barrier_t *bar);

void pm2_thread_barrier_init(pm2_thread_barrier_t *bar, pm2_thread_barrier_attr_t *attr);

/* Node-level barrier */
void pm2_barrier(pm2_barrier_t *bar);

/*Thread-level barrier */
void pm2_thread_barrier(pm2_thread_barrier_t *bar);

#endif
