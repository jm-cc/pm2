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

#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>

#define COMPUTE

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

static unsigned nb_vp;

static void compute(void)
{
  int i;

  i=5000000L;
  while(i--);
}

any_t sum(any_t arg)
{
  job *j = (job *)arg;
  job j1, j2;

  mdebug("LWP %d computes [%d..%d]\n",
	 marcel_current_vp(),
	 j->inf, j->sup);

#ifdef COMPUTE
  compute();
#endif

  if(j->inf == j->sup) {
    j->res = j->inf;
    marcel_sem_V(&j->sem);
    return NULL;
  }

  job_init(&j1, j->inf, (j->inf+j->sup)/2);
  job_init(&j2, (j->inf+j->sup)/2+1, j->sup);

  marcel_create(NULL, &attr, sum, (any_t)&j2);
  marcel_create(NULL, &attr, sum, (any_t)&j1);

  marcel_sem_P(&j1.sem);
  marcel_sem_P(&j2.sem);

  j->res = j1.res+j2.res;
  marcel_sem_V(&j->sem);
  return NULL;
}

Tick t1, t2;

int marcel_main(int argc, char **argv)
{
  job j[32];
  unsigned long temps;
  int N, acc = 0;
  unsigned vp;

  marcel_init(&argc, argv);

  nb_vp = marcel_nbvps();
  mdebug("nb virtual processors = %d\n", nb_vp);

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_OTHER);

  for(;;) {
      printf("Enter a rather small integer (0 to quit) : ");
      scanf("%d", &N);
      if(N == 0)
	break;
      GET_TICK(t1);
      for(vp=0; vp < nb_vp; vp++) {
	marcel_attr_t at = attr;

	j[vp].inf = (N/nb_vp)*vp + 1;
	j[vp].sup = (vp == nb_vp-1) ? N : (N/nb_vp)*(vp+1);
	marcel_sem_init(&j[vp].sem, 0);
	marcel_attr_setschedpolicy(&at, MARCEL_SCHED_FIXED(vp));

	marcel_create(NULL, &at, sum, (any_t)&j[vp]);
      }

      for(vp=0; vp < nb_vp; vp++) {
	marcel_sem_P(&j[vp].sem);
	acc += j[vp].res;
      }
      GET_TICK(t2);
      printf("Sum from 1 to %d = %d\n", N, acc);
      temps = timing_tick2usec(TICK_DIFF(t1, t2));
      printf("time = %ld.%03ldms\n", temps/1000, temps%1000);
  }

  marcel_end();
  return 0;
}
