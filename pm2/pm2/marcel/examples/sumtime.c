/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include "marcel.h"
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
  job *j = (job *)arg;
  job j1, j2;

  if(j->inf == j->sup) {
    j->res = j->inf;
    marcel_sem_V(&j->sem);
    return NULL;
  }

  job_init(&j1, j->inf, (j->inf+j->sup)/2);
  job_init(&j2, (j->inf+j->sup)/2+1, j->sup);

  marcel_create(NULL, &attr, sum, (any_t)&j1);
  marcel_create(NULL, &attr, sum, (any_t)&j2);

  marcel_sem_P(&j1.sem);
  marcel_sem_P(&j2.sem);

  j->res = j1.res+j2.res;
  marcel_sem_V(&j->sem);
  return NULL;
}

Tick t1, t2;

int marcel_main(int argc, char **argv)
{
  job j;
  unsigned long temps;

  marcel_init(&argc, argv);

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_AFFINITY);

  marcel_sem_init(&j.sem, 0);
  j.inf = 1;
  if(argc > 1) {
    j.sup = atoi(argv[1]);
    marcel_create(NULL, &attr, sum, (any_t)&j);
    marcel_sem_P(&j.sem);
    printf("Sum from 1 to %d = %d\n", j.sup, j.res);
  } else {
    LOOP(bcle)
      printf("Enter a rather small integer (0 to quit) : ");
      scanf("%d", &j.sup);
      if(j.sup == 0)
	EXIT_LOOP(bcle);
      GET_TICK(t1);
      marcel_create(NULL, &attr, sum, (any_t)&j);
      marcel_sem_P(&j.sem);
      GET_TICK(t2);
      printf("Sum from 1 to %d = %d\n", j.sup, j.res);
      temps = timing_tick2usec(TICK_DIFF(t1, t2));
      printf("time = %ld.%03ldms\n", temps/1000, temps%1000);
    END_LOOP(bcle)
  }

  marcel_end();
  return 0;
}
