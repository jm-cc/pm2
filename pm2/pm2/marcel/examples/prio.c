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

/* prio.c */

#include <marcel.h>

#define STACK_SIZE	10000

char *mess[2] = { "boys", "girls" };

any_t writer(any_t arg)
{
  int i, j;

  for(i=0;i<10;i++) {
    tfprintf(stderr, "Hi %s!\n", (char *)arg);
    j = 4000000; while(j--);
  }
  return NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t writer1_pid, writer2_pid;
  marcel_attr_t writer_attr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer_attr);

   marcel_attr_setstacksize(&writer_attr, STACK_SIZE);

   marcel_create(&writer2_pid, &writer_attr, writer, (any_t)mess[0]);

   marcel_attr_setprio(&writer_attr, 3);

   marcel_create(&writer1_pid, &writer_attr, writer,  (any_t)mess[1]);

   marcel_join(writer1_pid, &status);
   marcel_join(writer2_pid, &status);

   return 0;
}
