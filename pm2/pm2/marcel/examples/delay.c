
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

/* delay.c */

#include "marcel.h"

#define SCHED_POLICY   MARCEL_SCHED_OTHER

#define DELAY 500

any_t ALL_IS_OK = (any_t)0xdeadbeef;

char *mess[2] = { "boys", "girls" };

any_t busy(any_t arg)
{
  long i;

  marcel_detach(marcel_self());

  tprintf("Beginning of busy waiting\n");

  i=1000000000L;
  while(--i) ;

  tprintf("End of busy waiting\n");

  return ALL_IS_OK;
}

any_t writer(any_t arg)
{
  int i;

  for(i=0;i<10;i++) {
    tprintf("Hi %s! (I'm %p on vp %d)\n", (char*)arg,
	    marcel_self(), marcel_current_vp());
    marcel_delay(DELAY);
  }

  return ALL_IS_OK;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_attr_t attr;
  marcel_t writer1_pid, writer2_pid;
  
   marcel_trace_on();
   marcel_init(&argc, argv);

/*   marcel_create(NULL, NULL, busy, NULL); */

   marcel_attr_init(&attr);
   marcel_attr_setschedpolicy(&attr, SCHED_POLICY);

   marcel_create(&writer1_pid, &attr, writer, (any_t)mess[1]);
   marcel_create(&writer2_pid, &attr, writer, (any_t)mess[0]);

   marcel_join(writer1_pid, &status);
   if(status == ALL_IS_OK)
      tprintf("Thread %p completed ok.\n", writer1_pid);

   marcel_join(writer2_pid, &status);
   if(status == ALL_IS_OK)
      tprintf("Thread %p completed ok.\n", writer2_pid);

   marcel_end();
   return 0;
}



