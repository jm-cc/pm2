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


#include "clone.h"

static clone_t clone;

#define N       3
#define BOUCLES 2

any_t master(any_t arg)
{
  int b;
  volatile int i;

  marcel_detach(marcel_self());

  for(b=0; b<BOUCLES; b++) {

    marcel_delay(1000);

    i = 5;

    CLONE_BEGIN(&clone);

    fprintf(stderr,
	    "Je suis en section parallele "
	    "(delta = %lx, i = %d)\n",
	    clone_my_delta(),
	    ++(*((int *)((char *)&i - clone_my_delta()))));

    marcel_delay(1000);

    CLONE_END(&clone);

    fprintf(stderr, "ouf, les esclaves ont termine\n");
  }

  clone_terminate(&clone);

  return NULL;
}

any_t slave(any_t arg)
{
  marcel_detach(marcel_self());

  CLONE_WAIT(&clone);

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  int i;

  marcel_init(&argc, argv);

  clone_init(&clone, N);

  marcel_create(NULL, NULL, master, NULL);

  for(i=0; i<N; i++)
    marcel_create(NULL, NULL, slave, NULL);

  marcel_end();

  return 0;
}
