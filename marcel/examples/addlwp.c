
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

any_t blocker(any_t arg)
{
  struct timespec t = { .tv_sec = 1, .tv_nsec = 0};
  marcel_setname(marcel_self(),"blocker");
  marcel_printf("blocker sleeping 1s\n");
  while (nanosleep(&t,&t) == -1 && errno == EINTR);
  marcel_printf("blocker slept, done\n");
  return NULL;
}

int marcel_main(int argc, char **argv)
{
  marcel_attr_t attr;
  unsigned lwp;
  marcel_t t;
  unsigned long start;

  marcel_init(&argc, argv);

  marcel_setname(marcel_self(),"main");
  marcel_printf("wait a bit\n");
  marcel_delay(1000);
  lwp = marcel_add_lwp();
  marcel_printf("created additionnal LWP, wait a bit\n");
  marcel_delay(1000);
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(lwp));
  marcel_create(&t, &attr, blocker, NULL);
  marcel_printf("created blocker thread, busy a bit\n");
  start = marcel_clock();
  while(marcel_clock() < start + 2000);
  marcel_printf("done busy\n");

  fflush(stdout);
  marcel_end();
  return 0;
}



