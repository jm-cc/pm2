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

/* static.c */

#include <marcel.h>

any_t ALL_IS_OK = (any_t)123456789L;

char *mess[2] = { "boys", "girls" };

union {
  double bidon; /* for alignment purpose */
  char tab[32*1024];
} pile1, pile2;

void nettoyer(void *pile)
{
  printf("Nettoyage de %p\n", pile);
}

any_t writer(any_t arg)
{
  int i, j;

  marcel_cleanup_push(nettoyer, marcel_self()

);

   for(i=0;i<10;i++) {
      tprintf("Hi %s! (I'm %lx)\n", arg, marcel_self());
      j = 800000; while(j--);
   }

   return ALL_IS_OK;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_attr_t attr;
  marcel_t writer1_pid, writer2_pid;
  
   marcel_init(&argc, argv);

   marcel_attr_init(&attr);

   printf("pile1 = %p, pile2 = %p\n", &pile1, &pile2);

   marcel_attr_setstackaddr(&attr, &pile1);
   marcel_create(&writer1_pid, &attr, writer, (any_t)mess[1]);

   marcel_attr_setstackaddr(&attr, &pile2);
   marcel_create(&writer2_pid, &attr, writer, (any_t)mess[0]);

   marcel_join(writer1_pid, &status);
   if(status == ALL_IS_OK)
      tprintf("Thread %lx completed ok.\n", writer1_pid);

   marcel_join(writer2_pid, &status);
   if(status == ALL_IS_OK)
      tprintf("Thread %lx completed ok.\n", writer2_pid);

   marcel_end();
   return 0;
}



