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

#include <stdio.h>
#include "pm2.h"
#include "timing.h"
#include "pm2_sync.h"

#define N 10

BEGIN_DSM_DATA
int toto = 0;
END_DSM_DATA

pm2_thread_barrier_t b;

static void startup_func(int argc, char *argv[], void *arg)
{
  pm2_thread_barrier_attr_t attr;
  attr.local =  atoi(argv[1]);

  pm2_thread_barrier_init((pm2_thread_barrier_t *)arg, &attr);
}


void f()
{
  int i;

  for (i = 0; i < N ; i++)
    {
      fprintf(stderr,"[%p] i = %d before barrier\n", marcel_self(), i);
      pm2_thread_barrier(&b);
      fprintf(stderr,"[%p] i = %d after barrier\n", marcel_self(), i);
    }
}


int pm2_main(int argc, char **argv)
{
  int i;

  pm2_push_startup_func(startup_func, (void *)&b);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: threaded_sync <nombre de threads>\n");
      exit(1);
    }

  if ((pm2_config_size()) < 2)
    {
      fprintf(stderr, "This program requires at least 2 nodes\n");
      exit(1);
    }

  /* create local threads */
    for (i=0; i< atoi(argv[1]) ; i++)
	pm2_thread_create(f, NULL); 

    marcel_delay(3000);

    if (pm2_self() == 0)
      pm2_halt();

    pm2_exit();
  
    tfprintf(stderr, "Main is ending\n");
    return 0;
}

