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

#include "pm2.h"

#define NB	2
static char *mess[NB] = { "Hi guys !", "Hi girls !" };

static marcel_sem_t sem;

void thread_func(void *arg)
{
  int i, proc;

  pm2_enable_migration();

  proc = 0;

  for(i=0; i<3*pm2_config_size(); i++) {
    pm2_printf("%s\n", (char *)arg);

    marcel_delay(100);
    if((i+1) % 3 == 0) {

      proc = (proc+1) % pm2_config_size();

      pm2_printf("I'm now going on node [%d]\n", proc);

      pm2_migrate_self(proc);

      pm2_printf("Here I am!\n");
    }
  }

  marcel_sem_V(&sem);
}

int pm2_main(int argc, char **argv)
{
  int i;

  pm2_init(&argc, argv);

  if(pm2_self() == 0) { /* master process */

    marcel_sem_init(&sem, 0);

    for(i=0; i<NB; i++)
      pm2_thread_create(thread_func, mess[i]);

    for(i=0; i<NB; i++)
      marcel_sem_P(&sem);

    pm2_halt();
  }

   pm2_exit();

   tfprintf(stderr, "Main is ending\n");
   return 0;
}
