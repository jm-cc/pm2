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

/* active.c */

#include <marcel.h>

volatile boolean have_to_work = TRUE;

any_t sample(any_t arg)
{
   tprintf("thread will sleep during 2 seconds...\n");

   marcel_delay(2000);

   tprintf("thread will work a little bit...\n");

   while(have_to_work) /* active wait */ ;

   tprintf("thread termination\n");
   return NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t pid;

   marcel_init(&argc, argv);

   tprintf("active threads = %d\n", marcel_activethreads());

   tprintf("thread creation\n");

   marcel_create(&pid, NULL, sample, NULL);

   marcel_delay(500);

   tprintf("active threads = %d\n", marcel_activethreads());

   marcel_delay(1600);

   tprintf("active threads = %d\n", marcel_activethreads());

   have_to_work = FALSE;

   marcel_join(pid, &status);

   tprintf("thread termination catched\n");

   marcel_delay(10);

   tprintf("active threads = %d\n", marcel_activethreads());
   return 0;
}
