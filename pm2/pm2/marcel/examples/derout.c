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
#include "marcel_sem.h"

marcel_sem_t sem;

void deviation1(void *arg)
{
   marcel_fprintf(stdout, "This is a deviation : %s\n", arg);
}

void deviation2(void *arg)
{
   marcel_fprintf(stdout, "Deviation of a blocked thread : %s\n", arg);
}

void deviation3(void *arg)
{
   marcel_fprintf(stdout, "Deviation in order to wake one's self : ");
   marcel_sem_V(&sem);
}

any_t func(any_t arg)
{ int i;

   for(i=0; i<10; i++) {
      marcel_delay(100);
      marcel_printf("Hi %s!\n", arg);
   }

   marcel_sem_V(&sem);

   marcel_printf("Will block...\n");
   marcel_sem_P(&sem);
   marcel_printf("OK !\n");

   return NULL;
}

int marcel_main(int argc, char *argv[])
{ marcel_t pid;
  any_t status;

   marcel_init(&argc, argv);

   marcel_sem_init(&sem, 0);

   marcel_create(&pid, NULL, func, (any_t)"boys");

   marcel_delay(500);

   marcel_deviate(pid, deviation1, "OK !");

   marcel_sem_P(&sem);

   marcel_delay(500);

   marcel_deviate(pid, deviation2, "OK !");

   marcel_delay(500);

   marcel_deviate(pid, deviation3, NULL);

   marcel_join(pid, &status);

   marcel_end();
   return 0;
}
