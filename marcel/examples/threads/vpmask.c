
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

void bench_change_vpmask(unsigned nb)
{
  tick_t t1, t2;
  int i = nb;
  int vp = 0;

  GET_TICK(t1);
  while(--i) {
    marcel_vpmask_t vpmask = ~(1<<vp);
    marcel_change_vpmask(&vpmask);
    vp = 1-vp;
  }
  GET_TICK(t2);

  printf("change vpmask =  %fus\n", TIMING_DELAY(t1, t2) / (double)nb);
  return;
 
}

int marcel_main(int argc, char *argv[])
{ 
  int essais = 3;

  timing_init();

  marcel_init(&argc, argv);

  if(argc != 2) {
    fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  while(essais--) {
    bench_change_vpmask(atol(argv[1]));
  }

  marcel_end();

  return 0;
}
