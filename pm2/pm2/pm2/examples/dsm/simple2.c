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

#ifndef DISP
#define DISP(arg...)
#endif

#ifndef DISP_VAL
#define DISP_VAL(arg...)
#endif

int DSM_SERVICE;
dsm_mutex_t L;
pm2_completion_t c;
int *ptr, *start;


void f()
{
  int i, n = 10;
  DISP("Entering f");
  for (i = 0; i < n; i++) {
    DISP_VAL("f: i", i);
    dsm_mutex_lock(&L);
    DISP_VAL("f: locked", i);
    (*ptr)++;
    DISP_VAL("f: incremented", i);
    dsm_mutex_unlock(&L);
    DISP_VAL("f: unlocked", i);
    tfprintf(stderr,"ptr = %p, *ptr = %d\n",ptr, *ptr);
  }
  DISP("Sending completion signal");
  pm2_completion_signal(&c); 
  DISP("Completion signal sent");
}

static void DSM_func(void)
{
  DISP("Starting service");
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata(); 
  pm2_thread_create(f, NULL);
}


int pm2_main(int argc, char **argv)
{
  int i, j;

  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);

  //pm2_set_dsm_protocol(&dsmlib_migrate_thread_prot);
  pm2_set_dsm_protocol(&dsmlib_ddm_li_hudak_prot);

  pm2_set_dsm_page_distribution(DSM_BLOCK);

  dsm_mutex_init(&L, NULL);

  dsm_set_pseudo_static_area_size(4096 * 16);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: simple <number of threads per node>\n");
      exit(1);
    }

  dsm_display_page_ownership();

  start = (int*)dsm_get_pseudo_static_dsm_start_addr();
  ptr = start + 1024*1 + 1023;
  fprintf(stderr,"ptr = %p\n",ptr);

  if(pm2_self() == 0) { /* master process */
    pm2_completion_init(&c, NULL, NULL);
    *ptr = 0;
    
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
      {
	DISP("waiting for completion");
	pm2_completion_wait(&c);
	DISP("completion ok");
      }
    

    tfprintf(stderr, "*ptr=%d\n", *ptr );
    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
