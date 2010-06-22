
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

static volatile tbx_bool_t have_to_work = tbx_true;

static any_t sample(any_t arg)
{
   tprintf("thread will sleep during 2 seconds...\n");

   marcel_delay(2000);

   tprintf("thread will work a little bit...\n");

   while(have_to_work) /* active wait */ ;

   tprintf("thread termination\n");
   return NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t pid;

   marcel_init(&argc, argv);

   tprintf("thread creation\n");

   marcel_create(&pid, NULL, sample, NULL);

   marcel_delay(500);

   marcel_delay(1600);

   have_to_work = tbx_false;

   marcel_join(pid, &status);

   tprintf("thread termination catched\n");

   marcel_delay(10);

   marcel_end();
   return 0;
}
