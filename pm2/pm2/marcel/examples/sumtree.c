
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

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int inf, sup, res;
} job;

any_t sum(any_t arg)
{
  job *j = (job *)arg;

  if(j->inf == j->sup) {
    j->res = j->inf;
  } else {
    job j1, j2;
    marcel_t pid1, pid2;

    j1.inf = j->inf; j1.sup = (j->inf + j->sup) / 2;
    j2.inf = j1.sup + 1; j2.sup = j->sup;

    marcel_create(&pid1, NULL, sum, (any_t)&j1);
    marcel_create(&pid2, NULL, sum, (any_t)&j2);

    marcel_join(pid1, NULL);
    marcel_join(pid2, NULL);

    j->res = j1.res + j2.res;
  }

  return NULL;
}

tbx_tick_t t1, t2;

int marcel_main(int argc, char **argv)
{
  job j;
  unsigned long temps;
  marcel_t pid;

  marcel_init(&argc, argv);

  if(argc > 1) {

#ifdef PROFILE
    profile_activate(FUT_ENABLE, MARCEL_PROF_MASK);
#endif

   j.inf = 1;
   j.sup = atoi(argv[1]);

   TBX_GET_TICK(t1);
   marcel_create(&pid, NULL, sum, (any_t)&j);
   marcel_join(pid, NULL);
   TBX_GET_TICK(t2);

   temps = TBX_TIMING_DELAY(t1, t2);
   printf("Sum from 1 to %d = %d\n", j.sup, j.res);
   printf("time = %ld.%03ldms\n", temps/1000, temps%1000);

#ifdef PROFILE
   profile_stop();
#endif
  }

  marcel_end();
  return 0;
}



