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

/* stack.c */

#include <marcel.h>

any_t writer(any_t arg)
{ int i, j;
  unsigned long st1, st2;
  boolean option_enabled;

   BEGIN
      st1 = marcel_unusedstack();
   EXCEPTION
      WHEN(USE_ERROR) /* rien */
   END

   for(i=0;i<10;i++) {
      tfprintf(stderr, "Hi %s!\n", arg);
      j = 400000; while(j--);
   }

   BEGIN
      st2 = marcel_unusedstack();
      tfprintf(stderr, "Free stack evolved from %ld to %ld\n", st1, st2);
   EXCEPTION
      WHEN(USE_ERROR) /* rien */
   END

   marcel_exit(NULL);
}

any_t recur(any_t arg)
{ int n = (int)arg;

   if(n % 10 == 0)
      tprintf("%d\n", n);

   if(n != 0)
      return recur((any_t)(n-1));

   return NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t pid;

   marcel_init(&argc, argv);

   marcel_create(&pid, NULL, writer, "Boys");

   marcel_join(pid, &status);
   if(status == NULL)
      tprintf("Thread %lx completed ok.\n", pid);

   printf("Now a recursive one :\n");

   marcel_create(&pid, NULL, recur, (any_t)500);

   marcel_join(pid, &status);
   if(status == NULL)
      tprintf("Thread %lx completed ok.\n", pid);

   marcel_end();
}
