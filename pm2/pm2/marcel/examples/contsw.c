
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

any_t f(any_t arg)
{
  register int n = (int)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n)
    marcel_yield();
  TBX_GET_TICK(t2);

  printf("time =  %fus\n", TBX_TIMING_DELAY(t1, t2));
  return NULL;
}

extern void stop_timer(void);

int marcel_main(int argc, char *argv[])
{ 
  marcel_t pid;
  any_t status;
  int nb, essais = 3;
  register int n;

  if(argc != 2) {
    fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  marcel_init(&argc, argv);

  while(essais--) {
    nb = atoi(argv[1]);
    n = nb;
    if(n==0)
      break;

    n >>= 1;
    n++;

    marcel_create(&pid, NULL, f, (any_t)n);
    marcel_yield();

    while(--n)
      marcel_yield();

    marcel_join(pid, &status);
  }
  return 0;
}
