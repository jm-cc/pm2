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

/* keys.c */

#include <marcel.h>

#define STACK_SIZE	10000

marcel_key_t key;

void imprime(void)
{
  int i;
  char *str = (char *)marcel_getspecific(key);

   for(i=0;i<10;i++) {
      printf(str);
      marcel_delay(50);
   }
}

any_t writer1(any_t arg)
{
   marcel_setspecific(key, (any_t)"Hi boys!\n");

   imprime();

   return NULL;
}

any_t writer2(any_t arg)
{
   marcel_setspecific(key, (any_t)"Hi girls!\n");

   imprime();

   return NULL;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t writer1_pid, writer2_pid;
  marcel_attr_t writer1_attr, writer2_attr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer1_attr);
   marcel_attr_init(&writer2_attr);

   marcel_attr_setstacksize(&writer1_attr, STACK_SIZE);
   marcel_attr_setstacksize(&writer2_attr, STACK_SIZE);

   marcel_key_create(&key, NULL);

   marcel_create(&writer2_pid, &writer2_attr, writer1, NULL);
   marcel_create(&writer1_pid, &writer1_attr, writer2, NULL);

   marcel_join(writer1_pid, &status);
   marcel_join(writer2_pid, &status);

   return 0;
}
