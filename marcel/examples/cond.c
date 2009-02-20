
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

#include "marcel.h"
#include <stdio.h>

static volatile int a=0;
static marcel_cond_t cond;
static marcel_mutex_t mutex;
static marcel_sem_t sem;

static any_t f(any_t arg)
{
  register int n = (int)(intptr_t)arg;
  tbx_tick_t t1, t2;

  marcel_mutex_lock(&mutex);
  marcel_sem_V(&sem);
  TBX_GET_TICK(t1);
  while(--n) {
    marcel_cond_wait(&cond,&mutex);
    marcel_cond_signal(&cond);
  }
  TBX_GET_TICK(t2);
  marcel_mutex_unlock(&mutex);

  marcel_printf("cond'time =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (2*(double)(intptr_t)arg));
  return NULL;
}

static void bench_cond(unsigned nb)
{
  marcel_t pid;
  marcel_attr_t attr;
  any_t status;
  register int n;

  if(!nb)
    return;

  n = nb >> 1;
  n++;

  marcel_attr_init(&attr);
#ifdef MA__LWPS
  if (marcel_nbvps() >= 2) {
    marcel_vpset_t vpset = MARCEL_VPSET_VP(0);
    marcel_apply_vpset(&vpset);
    marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(1));
  }
#endif
#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif
  marcel_cond_init(&cond, NULL);
  marcel_mutex_init(&mutex, NULL);
  marcel_sem_init(&sem, 0);
  marcel_create(&pid, &attr, f, (any_t)(intptr_t)n);

  marcel_sem_P(&sem);
  marcel_mutex_lock(&mutex);
  while(--n) {
    marcel_cond_signal(&cond);
    marcel_cond_wait(&cond,&mutex);
  }
  marcel_mutex_unlock(&mutex);

  marcel_join(pid, &status);
#ifdef PROFILE
   profile_stop();
#endif
}

int marcel_main(int argc, char *argv[])
{
  int essais = 3;

  marcel_init(&argc, argv);

  if(argc != 2) {
    marcel_fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  while(essais--) {
    bench_cond(atoi(argv[1]));
  }

  marcel_end();

  return 0;
}
