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

/* timeout.c */

#include "marcel.h"

marcel_sem_t sem;

any_t f(any_t arg)
{
  LOOP(b)
    BEGIN
      marcel_sem_timed_P(&sem, 500);
      EXIT_LOOP(b);
    EXCEPTION
      WHEN(TIME_OUT)
        tprintf("What a boring job, isn't it ?\n");
    END
  END_LOOP(b)
  tprintf("What a relief !\n");

  return 0;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid;

  marcel_init(&argc, argv);

  marcel_sem_init(&sem, 0);

  marcel_create(&pid, NULL, f,  NULL);

  marcel_delay(2100);
  marcel_sem_V(&sem);

  marcel_join(pid, &status);

  marcel_end();
  return 0;
}
