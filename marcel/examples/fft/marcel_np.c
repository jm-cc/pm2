#include "marcel_np.h"
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


/*  #define DEBUG */


extern marcel_mutex_t marcel_end_lock_np;
extern marcel_cond_t marcel_end_cond_np;
extern int marcel_end_np_np;

void marcel_create_np(void *(*func)(void)) {
  marcel_t t;
  marcel_attr_t attr;

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, tbx_true);
  /* marcel_attr_setvp_np(&attr, MARCEL_BEST_VP); */
  marcel_create(&t, &attr, (marcel_func_t)func, NULL);
}

void marcel_wait_for_end_np(int p)
{
  marcel_mutex_lock(&marcel_end_lock_np);
  while (marcel_end_np_np < p)
     marcel_cond_wait(&marcel_end_cond_np, &marcel_end_lock_np); 
  marcel_mutex_unlock(&marcel_end_lock_np);
}


void marcel_signal_end_np(int self, int p)
{
    marcel_mutex_lock(&marcel_end_lock_np); 
    marcel_end_np_np++;
#ifdef DEBUG2
  printf("signal end, I am %p, #threads = %d\n",marcel_self(), marcel_end_np_np);
#endif
     marcel_mutex_unlock(&marcel_end_lock_np); 
     marcel_cond_signal(&marcel_end_cond_np); 
}
