
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

#define CPU0ONLY
//#define dprintf(fmt,args...) fprintf(stderr,fmt,##args)
#define dprintf(fmt,args...) (void)0

#define MAX_BUBBLE_LEVEL 6
//#define MAX_BUBBLE_LEVEL 0

typedef struct {
  int inf, sup, res;
  marcel_sem_t sem;
  int level;
} job;

marcel_attr_t attr,commattr;

static __inline__ void job_init(job *j, int inf, int sup, int level)
{
  j->inf = inf;
  j->sup = sup;
  marcel_sem_init(&j->sem, 0);
  j->level = level;
}

any_t sum(any_t arg)
{
  job *j = (job *)arg;
  job j1, j2;
  char name[MARCEL_MAXNAMESIZE];

  snprintf(name,sizeof(name),"%d-%d",j->inf,j->sup);
  marcel_setname(marcel_self(),name);

  dprintf("sum(%p,%p,%p) started\n",j,&j1,&j2);

  if(j->inf == j->sup) {
    j->res = j->inf;
    marcel_sem_V(&j->sem);
    return NULL;
  }

  job_init(&j1, j->inf, (j->inf+j->sup)/2, j->level + 1);
  job_init(&j2, (j->inf+j->sup)/2+1, j->sup, j->level + 1);

  if (j->level<MAX_BUBBLE_LEVEL) {
    marcel_bubble_t b1,b2;
    marcel_t p1,p2;
    marcel_bubble_init(&b1);
    marcel_bubble_init(&b2);
    marcel_bubble_setschedlevel(&b1,j->level);
    marcel_bubble_setschedlevel(&b2,j->level);
    marcel_create_dontsched(&p1, &commattr, sum, (any_t)&j1);
    marcel_create_dontsched(&p2, &commattr, sum, (any_t)&j2);
    marcel_bubble_inserttask(&b1, p1);
    marcel_bubble_inserttask(&b2, p2);
    marcel_bubble_insertbubble(marcel_bubble_holding_task(marcel_self()),&b1);
    marcel_bubble_insertbubble(marcel_bubble_holding_task(marcel_self()),&b2);
    marcel_sem_P(&j1.sem);
    marcel_sem_P(&j2.sem);
  } else {
    marcel_create(NULL, &attr, sum, (any_t)&j1);
    marcel_create(NULL, &attr, sum, (any_t)&j2);
    marcel_sem_P(&j1.sem);
    marcel_sem_P(&j2.sem);
  }

  j->res = j1.res+j2.res;
  marcel_sem_V(&j->sem);
  return NULL;
}

tbx_tick_t t1, t2;

void compute(job j) {
  int nbvps;
  unsigned long temps;

  nbvps = marcel_nbvps();
  TBX_GET_TICK(t1);
  marcel_create(NULL, &attr, sum, (any_t)&j);
  marcel_sem_P(&j.sem);

  TBX_GET_TICK(t2);
  printf("Sum from 1 to %d = %d\n", j.sup, j.res);
  temps = TBX_TIMING_DELAY(t1, t2);
  printf("time = %ld.%03ldms\n", temps/1000, temps%1000);
}

int marcel_main(int argc, char **argv)
{
  job j;

  marcel_trace_on();
  marcel_init(&argc, argv);

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_AFFINITY);
  marcel_attr_setstayinbubble(&attr, TRUE);
#ifdef CPU0ONLY
  marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(0));
#endif

  marcel_attr_init(&commattr);
  marcel_attr_setdetachstate(&commattr, TRUE);
  marcel_attr_setschedpolicy(&commattr, MARCEL_SCHED_AFFINITY);
#ifdef CPU0ONLY
  marcel_attr_setvpmask(&commattr, MARCEL_VPMASK_ALL_BUT_VP(0));
#endif

  marcel_sem_init(&j.sem, 0);
  j.inf = 1;
  j.level = 0;
  if(argc > 1) {
    // Execution unique en utilisant l'argument en ligne de commande
#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif
    j.sup = atoi(argv[1]);
    compute(j);
#ifdef PROFILE
   profile_stop();
#endif
  } else {
    // Execution interactive
    LOOP(bcle)
      printf("Enter a rather small integer (0 to quit) : ");
      scanf("%d", &j.sup);
      if(j.sup <= 0)
	EXIT_LOOP(bcle);
      compute(j);
    END_LOOP(bcle)
  }

  fflush(stdout);
  marcel_end();
  return 0;
}



