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

marcel_mutex_t mutex;
marcel_cond_t cond;

any_t f(any_t arg)
{
  struct timeval now;
  struct timespec ts;

  marcel_mutex_lock(&mutex);
  for(;;) {
    gettimeofday(&now, NULL);

    ts.tv_sec = now.tv_sec + 1; /* timeout d'une seconde */
    ts.tv_nsec = now.tv_usec * 1000;

    if(marcel_cond_timedwait(&cond, &mutex, &ts) != ETIMEDOUT)
      break;
    tprintf("What a boring job, isn't it ?\n");
  }
  marcel_mutex_unlock(&mutex);

  tprintf("What a relief !\n");

  return 0;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid;

  marcel_init(&argc, argv);

  marcel_mutex_init(&mutex, NULL);
  marcel_cond_init(&cond, NULL);

  marcel_create(&pid, NULL, f,  NULL);

  marcel_delay(3100);
  marcel_cond_signal(&cond);

  marcel_join(pid, &status);

  marcel_end();
  return 0;
}
