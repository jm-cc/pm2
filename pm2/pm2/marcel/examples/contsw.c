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

static unsigned long temps;

any_t f(any_t arg)
{
  register int n = (int)arg;
  Tick t1, t2;

  GET_TICK(t1);
  while(--n)
    marcel_yield();
  GET_TICK(t2);

  temps = timing_tick2usec(TICK_DIFF(t1, t2));
  return NULL;
}

extern void stop_timer(void);

int marcel_main(int argc, char *argv[])
{ 
  marcel_t pid;
  any_t status;
  int nb;
  register int n;

  marcel_init(&argc, argv);

  stop_timer();

  while(1) {
    printf("How many context switches ? ");
    scanf("%d", &nb);
    n = nb;
    if(n==0)
      break;

    n >>= 1;
    n++;

    marcel_create(&pid, NULL, f, (any_t)n);
    marcel_yield();

    while(--n)
      marcel_yield();

    marcel_join(pid, &status);

    printf("time =  %ld.%03ldms\n", temps/1000, temps%1000);
  }
  return 0;
}
