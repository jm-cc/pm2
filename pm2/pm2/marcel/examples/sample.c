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

/* sample.c */

#include <marcel.h>

#define SCHED_POLICY   MARCEL_SCHED_BALANCE

any_t ALL_IS_OK = (any_t)123456789L;

#define NB    3

char *mess[NB] = { "boys", "girls", "people" };

any_t writer(any_t arg)
{ int i, j;

   for(i=0;i<10;i++) {
      tprintf("Hi %s! (I'm %p on vp %d)\n",
	      (char*)arg, marcel_self(), marcel_current_vp());
      j = 1900000; while(j--);
   }

   return ALL_IS_OK;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid[NB];
  marcel_attr_t attr;
  int i;

   marcel_init(&argc, argv);

   marcel_attr_init(&attr);

   for(i=0; i<NB; i++) {
     marcel_attr_setschedpolicy(&attr, SCHED_POLICY);
     marcel_create(&pid[i], &attr, writer, (any_t)mess[i]);
   }

   for(i=0; i<NB; i++) {
     marcel_join(pid[i], &status);
     if(status == ALL_IS_OK)
       tprintf("Thread %p completed ok.\n", pid[i]);
   }

   marcel_end();
   return 0;
}



