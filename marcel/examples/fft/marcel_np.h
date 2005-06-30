
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

#include <marcel.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef struct {
  marcel_sem_t mutex, wait;
  int nb;
} marcel_bar_t_np;

extern void marcel_barinit_np(marcel_bar_t_np *bar);

extern void marcel_create_np(void *(*func)(void));

extern void marcel_wait_for_end_np(int p);

extern void marcel_signal_end_np(int self, int p);

extern void marcel_barrier_np(marcel_bar_t_np *bar, int p);
