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

#define COUNTER 

int DSM_SERVICE;

dsm_mutex_t L;

pm2_completion_t c;

BEGIN_DSM_DATA
atomic_t a = {0};
DSM_NEWPAGE
int toto1 = 0;
DSM_NEWPAGE
int toto2 = 0;
DSM_NEWPAGE
int toto3 = 0;
DSM_NEWPAGE
int toto4 = 0;
DSM_NEWPAGE
int toto5 = 0;
DSM_NEWPAGE
int toto6 = 0;
DSM_NEWPAGE
int toto7 = 0;
DSM_NEWPAGE
int toto8 = 0;
DSM_NEWPAGE
int toto9 = 0;
END_DSM_DATA


void f()
{
  int i, n = 20;

  for (i = 0; i < n; i++) {
    // atomic_inc(&a);
    dsm_mutex_lock(&L);
    toto1++;
    dsm_mutex_unlock(&L);
  }
  pm2_completion_signal(&c); 
}

static void DSM_func(void)
{
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata(); 
  pm2_thread_create(f, NULL);
}


int pm2_main(int argc, char **argv)
{
  int i, j;

  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);

  //dsm_set_default_protocol(MIGRATE_THREAD);
  dsm_set_default_protocol(LI_HUDAK);

  pm2_set_dsm_page_distribution(DSM_BLOCK, 16);

  dsm_mutex_init(&L, NULL);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: simple <number of threads per node>\n");
      exit(1);
    }

  dsm_display_page_ownership();

  if(pm2_self() == 0) { /* master process */
    pm2_completion_init(&c, NULL, NULL);

    /* create local threads */
    for (i=0; i< atoi(argv[1]) ; i++)
      pm2_thread_create(f, NULL); 

    /* create remote threads */
    for (j=1; j < pm2_config_size(); j++)
      for (i=0; i< atoi(argv[1]) ; i++) {
	pm2_rawrpc_begin(j, DSM_SERVICE, NULL);
	pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
	pm2_rawrpc_end();
      }

    for (i = 0 ; i < atoi(argv[1]) * pm2_config_size(); i++)
      pm2_completion_wait(&c);

      tfprintf(stderr, "toto1=%d\n", toto1);
    //tfprintf(stderr, "a=%d\n", a.counter);
    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
