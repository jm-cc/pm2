
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

/* yield-to-team.c */

/* This program exercises the marcel_yield_to_team function */

#include "marcel.h"

#define NB_YIELDS 512

struct yield_info {
  unsigned nb_buddies;
  marcel_t *buddies;
  double *mask;
};

static void * f (void *arg)
{
  int i;
  struct yield_info *info = (struct yield_info *)arg;
  for (i = 0; i < NB_YIELDS; i++)
    if (marcel_yield_to_team(info->buddies, info->mask, info->nb_buddies) == 0) {
      marcel_printf("Yeepee thread %p did its job!\n", marcel_self());
      break;
    }
     
  return 0;
}

int
main (int argc, char **argv)
{
  marcel_init(&argc, argv);
  int i;
  /* Oversubscribe the machine */
  unsigned nb_threads = marcel_nbprocessors * 2; 
  
  marcel_t threads[nb_threads];
  marcel_attr_t thread_attr;
  marcel_attr_init(&thread_attr);
  marcel_attr_setpreemptible (&thread_attr, tbx_false);
  marcel_thread_preemption_disable();

  struct yield_info outer_info;
  outer_info.nb_buddies = nb_threads;
  outer_info.buddies = threads;
  outer_info.mask = calloc(nb_threads, sizeof(double));
  
  for (i = 0; i < nb_threads; i++) {
    outer_info.mask[i] = i%2;
  }
 
  for (i = 0; i < nb_threads; i++) {
    marcel_create (threads + i, &thread_attr, f, &outer_info);
    marcel_printf("Thread %d::%p is %savailable.\n", i, threads[i], i%2 ? "not ":"");
  }
  
  f(&outer_info);

  for (i = 0; i < nb_threads; i++) {
    marcel_join(threads[i], NULL);
  }      
  
  free(outer_info.mask);
  marcel_end();
  return 0;
}
