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


#include <stdio.h>
#include <marcel.h>
#include <timing.h>

any_t foo(any_t arg)
{
  marcel_detach(marcel_self());
  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  int i;

  timing_init();
  marcel_init(&argc, argv);

  lock_task();
  TIMING_EVENT("Avant boucle");
  for(i=0; i<10; i++) {
    marcel_create(NULL, NULL, foo, NULL);
  }
  TIMING_EVENT("Apres boucle");
  unlock_task();

  marcel_delay(500);

  lock_task();
  TIMING_EVENT("Avant boucle");
  for(i=0; i<10; i++) {
    marcel_create(NULL, NULL, foo, NULL);
  }
  TIMING_EVENT("Apres boucle");
  unlock_task();
  marcel_end();

  return 0;
}
