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

/* userspace.c */

#include "marcel.h"

#define STACK_SIZE	10000

#define MESSAGE		"Hi boys !"

any_t writer(any_t arg)
{
  int i, j;
  char *s;

  marcel_getuserspace(marcel_self(), (any_t *)&s);

  for(i=0;i<10;i++) {
    tfprintf(stderr, "%s\n", s);
    j = 10000; while(j--);
  }
  return NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t writer_pid;
  marcel_attr_t writer_attr;
  void *ptr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer_attr);

   marcel_attr_setuserspace(&writer_attr, strlen(MESSAGE)+1);
   marcel_attr_setstacksize(&writer_attr, STACK_SIZE);

   marcel_create(&writer_pid, &writer_attr, writer, NULL);

   marcel_getuserspace(writer_pid, &ptr);
   strcpy(ptr, MESSAGE);

   marcel_run(writer_pid, NULL);

   marcel_join(writer_pid, &status);

   tfprintf(stderr, "marcel ended\n");
   return 0;
}
