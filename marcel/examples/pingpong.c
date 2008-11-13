
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

marcel_sem_t sem1 = MARCEL_SEM_INITIALIZER(0), sem2 = MARCEL_SEM_INITIALIZER(1);

any_t f(any_t arg)
{
  register long n = (long)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n) {
    marcel_sem_P(&sem2);
    marcel_sem_V(&sem1);
  }
  TBX_GET_TICK(t2);

  marcel_printf("pingpong =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (2*(double)(long)arg));
  return NULL;
}

void bench_pingpong(unsigned long nb)
{
  marcel_attr_t attr;
  marcel_t pid;
  any_t status;
  register long n;

  if(!nb)
    return;

  n = nb >> 1;
  n++;

  marcel_attr_init(&attr);
#ifdef MA__LWPS
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(1 % marcel_nbvps()));
#endif
  marcel_create(&pid, &attr, f, (any_t)n);

  while(--n) {
    marcel_sem_P(&sem1);
    marcel_sem_V(&sem2);
  }

  marcel_join(pid, &status);
}

int marcel_main(int argc, char *argv[])
{ 
  int essais = 3;

  marcel_init(&argc, argv);

  if(argc != 2) {
    marcel_fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  marcel_vpset_t vpset = MARCEL_VPSET_VP(0);
  marcel_apply_vpset(&vpset);

  while(essais--)
    bench_pingpong(atol(argv[1]));

  marcel_end();

  return 0;
}
