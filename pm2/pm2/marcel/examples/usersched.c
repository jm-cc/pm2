
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

static int MY_SCHED_POLICY;

#define NB    3

char *mess[NB] = { "boys", "girls", "people" };

any_t writer(any_t arg)
{
  int i;

   for(i=0;i<10;i++) {
      tprintf("Hi %s! (I'm %p on vp %d)\n",
	      (char*)arg, marcel_self(), marcel_current_vp());
      marcel_delay(100);
   }

   return NULL;
}

/* Sample (reverse) cyclic distribution */
static __lwp_t *my_sched_func(marcel_t pid, __lwp_t *current_lwp)
{
  static __lwp_t *lwp = &__main_lwp;

  mdebug("User scheduler called for thread %p\n", pid);

  lwp = lwp->prev;
  return lwp;
}


int marcel_main(int argc, char *argv[])
{
  marcel_t pid[NB];
  marcel_attr_t attr;
  int i;

   marcel_init(&argc, argv);

   marcel_schedpolicy_create(&MY_SCHED_POLICY, my_sched_func);

   marcel_attr_init(&attr);
   marcel_attr_setschedpolicy(&attr, MY_SCHED_POLICY);

   for(i=0; i<NB; i++)
     marcel_create(&pid[i], &attr, writer, (any_t)mess[i]);

   for(i=0; i<NB; i++)
     marcel_join(pid[i], NULL);

   marcel_end();
   return 0;
}



