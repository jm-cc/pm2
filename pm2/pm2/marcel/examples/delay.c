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

/* delay.c */

#include "marcel.h"

any_t ALL_IS_OK = (any_t)0xdeadbeef;

char *mess[2] = { "boys", "girls" };

any_t busy(any_t arg)
{
  long i;

  marcel_detach(marcel_self());

  tprintf("Beginning of busy waiting\n");

  i=1000000000L;
  while(--i) ;

  tprintf("End of busy waiting\n");

  return ALL_IS_OK;
}

any_t writer(any_t arg)
{
  int i;

  for(i=0;i<10;i++) {
    tprintf("Hi %s! (I'm %p on vp %d)\n", (char*)arg,
	    marcel_self(), marcel_current_vp());
    marcel_delay(500);
  }

  return ALL_IS_OK;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t writer1_pid, writer2_pid;
  
   marcel_init(&argc, argv);

/*   marcel_create(NULL, NULL, busy, NULL); */

   marcel_create(&writer1_pid, NULL, writer, (any_t)mess[1]);
   marcel_create(&writer2_pid, NULL, writer, (any_t)mess[0]);

   marcel_join(writer1_pid, &status);
   if(status == ALL_IS_OK)
      tprintf("Thread %p completed ok.\n", writer1_pid);

   marcel_join(writer2_pid, &status);
   if(status == ALL_IS_OK)
      tprintf("Thread %p completed ok.\n", writer2_pid);

   marcel_end();
   return 0;
}



