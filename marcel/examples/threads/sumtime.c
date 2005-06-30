
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
#include <stdlib.h>

typedef struct {
  int inf, sup, res;
  marcel_sem_t sem;
} job;

marcel_attr_t attr;

static __inline__ void job_init(job *j, int inf, int sup)
{
  j->inf = inf;
  j->sup = sup;
  marcel_sem_init(&j->sem, 0);
}

any_t sum(any_t arg)
{
  marcel_t self=marcel_self();
  marcel_t t1, t2;

  job *j = (job *)arg;
  job j1, j2;

  if(j->inf == j->sup) {
    j->res = j->inf;
    marcel_sem_V(&j->sem);
    return (any_t)self;
  }

  job_init(&j1, j->inf, (j->inf+j->sup)/2);
  job_init(&j2, (j->inf+j->sup)/2+1, j->sup);

  marcel_create(&t1, &attr, sum, (any_t)&j1);
  marcel_create(&t2, &attr, sum, (any_t)&j2);

  marcel_sem_P(&j1.sem);
  marcel_sem_P(&j2.sem);

  j->res = j1.res+j2.res;
  marcel_sem_V(&j->sem);
  return NULL;
}

tick_t t1, t2;

int marcel_main(int argc, char **argv)
{
  job j;
  marcel_t tid;
  unsigned long temps;

  timing_init();
  marcel_init(&argc, argv);

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_AFFINITY);

  marcel_sem_init(&j.sem, 0);
  j.inf = 1;
  if(argc > 1) {
    // Execution unique en utilisant l'argument en ligne de commande
    j.sup = atoi(argv[1]);
    GET_TICK(t1);
    marcel_create(&tid, &attr, sum, (any_t)&j);
    marcel_sem_P(&j.sem);
    GET_TICK(t2);
    printf("Sum from 1 to %d = %d\n", j.sup, j.res);
    temps = TIMING_DELAY(t1, t2);
    printf("time = %ld.%03ldms\n", temps/1000, temps%1000);
#ifdef PROFILE
   profile_stop();
#endif
  } else {
	  fprintf(stderr, "Usage: sumtime nb\n");
  }

  marcel_end();
  return 0;
}



