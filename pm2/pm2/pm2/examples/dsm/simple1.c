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


int *les_modules, nb_modules;

BEGIN_DSM_DATA
DSM_NEWPAGE
int toto_on_page_0 = 1;
int tata_on_page_0 = 2;
DSM_NEWPAGE
int titi_on_page_1 = 3;
END_DSM_DATA

void f()
{
  marcel_delay(1000);
  fprintf(stderr, "toto_on_page_0 = %d\n", toto_on_page_0);
  fprintf(stderr, "tata_on_page_0 = %d\n", tata_on_page_0);
  tata_on_page_0 = 9;
  /*  fprintf(stderr, "tata_on_page_0 = %d\n", tata_on_page_0);
  titi_on_page_1 = 10;
  fprintf(stderr, "titi_on_page_1 = %d\n", titi_on_page_1);*/
}


int pm2_main(int argc, char **argv)
{
  pm2_init_rpc();

  pm2_init(&argc, argv, 3, &les_modules, &nb_modules);

  if(pm2_self() == les_modules[0]) { /* master process */

    f();

    marcel_delay(1000);
    pm2_kill_modules(les_modules, nb_modules);
  }
  else
    if(pm2_self() == les_modules[2])
      fprintf(stderr, "toto_on_page_0 = %d\n", toto_on_page_0);

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}









