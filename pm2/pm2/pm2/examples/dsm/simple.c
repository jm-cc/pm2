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
#include "dsm_pm2.h"
#include "rpc_defs.h"
#include "timing.h"

#define COUNTER 

marcel_sem_t main_sem;

dsm_mutex_t L;

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
  int i, n = 10;

  for (i = 0; i < n; i++) {
    //    atomic_inc(&a);
    dsm_mutex_lock(&L);
    toto1++;
    dsm_mutex_unlock(&L);
  }
}

BEGIN_SERVICE(TEST_DSM)
     f();
     pm2_quick_async_rpc(0,DSM_V,NULL,NULL);
END_SERVICE(TEST_DSM)


BEGIN_SERVICE(DSM_V)
     marcel_sem_V(&main_sem);
END_SERVICE(DSM_V)
     


int pm2_main(int argc, char **argv)
{
  int i, j;

  if (argc != 3)
    {
      fprintf(stderr, "Usage: simple <number of nodes> <number of threads per node>\n");
      exit(1);
    }
  pm2_rpc_init();

  DECLARE_LRPC(TEST_DSM);
  DECLARE_LRPC(DSM_V);

  marcel_sem_init(&main_sem, 0);

  //pm2_set_dsm_protocol(&dsmlib_migrate_thread_prot);
  pm2_set_dsm_protocol(&dsmlib_ddm_li_hudak_prot);

  pm2_set_dsm_page_distribution(DSM_CENTRALIZED, 0);

  dsm_mutex_init(&L, NULL);

  pm2_init(&argc, argv);

  dsm_display_page_ownership();
  if(pm2_self() == 0) { /* master process */

    for (j=0; j < pm2_config_size(); j++)
      for (i=0; i< atoi(argv[2]) ; i++) {
	pm2_async_rpc(j, TEST_DSM, NULL, NULL);
      }

    for (i = 0 ; i < atoi(argv[1]) * atoi(argv[2]); i++)
      marcel_sem_P(&main_sem);

    tfprintf(stderr, "toto1=%d\n", toto1);

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}









