
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

#include "thread.h"
#include "utils.h"
#include <stdio.h>

#ifdef PTHREAD
volatile int started;
volatile int finished;

any_t f_busy(any_t arg)
{
  marcel_printf("I'm alive\n");
  started = 1;
  while (!finished);
  marcel_printf("I'm dead\n");

  return NULL;
}

any_t f_idle(any_t arg)
{
  marcel_printf("I'm idle (2)\n");
  started = 1;
  while (!finished) sleep(1);
  marcel_printf("I'm dead (2)\n");

  return NULL;
}
#endif

void bench_apply_vpset(unsigned nb)
{
  tick_t t1, t2;
  int i = nb;
  int vp = 0;

  GET_TICK(t1);
  while(--i) {
    marcel_vpset_t vpset = 1<<vp;
    marcel_apply_vpset(&vpset);
    vp = 1-vp;
  }
  GET_TICK(t2);

  printf("apply vpset =  %fus\n", TIMING_DELAY(t1, t2) / (double)nb);
  return;
 
}

#ifdef PTHREAD
void bench_migrate(unsigned long nb, int active)
{
  tick_t t1, t2;
  marcel_t pid;
  any_t status;
  register long n = nb;

  int i = 2;

  if(!nb)
    return;

  finished = 0;
  started = 0;
  marcel_create(&pid, NULL, active ? f_busy : f_idle, (any_t)n);
  while (!started);

  GET_TICK(t1);
  while(--n) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    pthread_setaffinity_np(pid, sizeof(cpuset), &cpuset);
    i = 5-i;
  }
  GET_TICK(t2);
  printf("migration time =  %fus\n", TIMING_DELAY(t1, t2) / (double)nb);
  finished = 1;
  marcel_join(pid, &status);
}
#endif

int marcel_main(int argc, char *argv[])
{ 
  int essais = 3;

  timing_init();

  marcel_init(&argc, argv);

  if(argc != 2) {
    fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  if (marcel_nbvps() < 4) {
    fprintf(stderr, "I need at least 4 processors to run\n");
    exit(1);
  }

  while(essais--) {
    bench_apply_vpset(atol(argv[1]));
#ifdef PTHREAD
    bench_migrate(atol(argv[1]), 1);
    bench_migrate(atol(argv[1]), 0);
#endif
  }

  marcel_end();

  return 0;
}
