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

exception USER_ERROR = "USER_ERROR";

void deviation(void *arg)
{
   RAISE(USER_ERROR);
}

any_t func(any_t arg)
{ int i;

   BEGIN
     for(i=0; i<10; i++) {
       marcel_delay(100);
       tprintf("Hi %s!\n", arg);
     }
   EXCEPTION
     WHEN(USER_ERROR)
       tprintf("God! I have been disturbed!\n");
       return NULL;
   END
   tprintf("Ok, I was not disturbed.\n");
   return NULL;
}

int marcel_main(int argc, char *argv[])
{ marcel_t pid;
  any_t status;

   marcel_init(&argc, argv);

   marcel_create(&pid, NULL, func, (any_t)"boys");

   marcel_delay(500);
   marcel_deviate(pid, deviation, NULL);

   marcel_join(pid, &status);

   return 0;
}

