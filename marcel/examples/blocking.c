
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
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#ifndef MA__LWPS
#warning add needs SMP or NUMA option
#else

any_t loop(any_t arg)
{
  unsigned long start;
  while(1) {
    marcel_printf("busy looping 1s\n");
    start = marcel_clock();
    while(marcel_clock() < start + 1000);
  }
  return NULL;
}

any_t block(any_t arg)
{
  struct timespec ts;
  int n = (int)(intptr_t) arg;
  char name[16];
  marcel_getname(marcel_self(), name, 16);
  while (n--) {
    marcel_fprintf(stderr,"%s sleeping (VP %d)\n", name, marcel_current_vp());
    marcel_enter_blocking_section();
    fprintf(stderr,"%s really sleeping (VP %d)\n", name, marcel_current_vp());
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    while (nanosleep(&ts,&ts))
      ;
    fprintf(stderr,"%s really slept (VP %d)\n", name, marcel_current_vp());
    marcel_leave_blocking_section();
    marcel_fprintf(stderr,"%s slept (VP %d)\n", name, marcel_current_vp());
  }
  marcel_fprintf(stderr,"%s finished sleeping\n", name);
  return NULL;
}
#endif

int marcel_main(int argc, char **argv)
{
#ifdef MA__LWPS
#define N 10
  marcel_attr_t attr;
  marcel_t loopt;
  marcel_t blockt[N];
  int n = 3, i;
  char blockname[]="block/XXX";

  marcel_init(&argc, argv);

  if (argc > 1)
	  n = atoi(argv[1]);

  marcel_setname(marcel_self(),"main");
  marcel_attr_init(&attr);
  marcel_attr_setname(&attr, "loop");
  marcel_attr_setdetachstate(&attr, tbx_true);
  marcel_create(&loopt, &attr, loop, NULL);
  marcel_attr_setdetachstate(&attr, tbx_false);
  for (i=0;i<N;i++) {
    snprintf(blockname,sizeof(blockname),"block/%d",i);
    marcel_attr_setname(&attr, blockname);
    marcel_create(&blockt[i], &attr, block, (any_t)(intptr_t) n);
  }
  for (i=0;i<N;i++) {
    marcel_join(blockt[i],NULL);
    marcel_fprintf(stderr,"%d finished sleeping\n", i);
  }

  marcel_cancel(loopt);
  marcel_end();
#endif
  return 0;
}



