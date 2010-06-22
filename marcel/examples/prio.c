
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

/* active.c */

#include "marcel.h"

volatile int finished;

static
any_t sample(any_t arg)
{
   int i;
   long me = (long) arg;
   while (!finished) {
      for (i=0;i<10000000;i++);
      marcel_printf("I'm %ld\n",me);
   }
   tprintf("%ld finished\n", me);
   return NULL;
}

int marcel_main(int argc, char *argv[])
{
   any_t status;
   marcel_t pid1,pid2;
   struct marcel_sched_param sched_param = { .sched_priority = MA_BATCH_PRIO };

   marcel_init(&argc, argv);

   marcel_create(&pid1, NULL, sample, (any_t) 1);
   marcel_create(&pid2, NULL, sample, (any_t) 2);

   marcel_delay(1000);

   marcel_sched_setparam(pid1, &sched_param);

   marcel_delay(1000);
   finished = 1;

   marcel_join(pid1, &status);
   marcel_join(pid2, &status);

   tprintf("thread termination catched\n");

   marcel_end();

   return 0;
}
