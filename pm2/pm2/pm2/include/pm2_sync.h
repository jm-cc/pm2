
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#include "marcel.h"

typedef struct {
  int *tab_sync;
  marcel_mutex_t sync_mutex;
} pm2_barrier_t;


typedef struct {
  marcel_sem_t mutex, wait;
  pm2_barrier_t node_barrier;
  int local;
  int nb;
} pm2_thread_barrier_t;

typedef struct {
  int local;
} pm2_thread_barrier_attr_t;


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
