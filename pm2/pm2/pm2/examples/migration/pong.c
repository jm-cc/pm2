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

#include "rpc_defs.h"
#include <sys/time.h>
#include <stdlib.h>

#define ESSAIS 5

static int *les_modules, nb_modules, autre_module;

static Tick t1, t2;

BEGIN_SERVICE(LRPC_PING)
  unsigned long temps;
  unsigned n, nping;

  marcel_enablemigration(marcel_self());

  for(n=0; n<ESSAIS; n++) {
    nping = req.nb;
    GET_TICK(t1);
    while(1) {
      if(nping-- == 0)
	break;
      pm2_migrate_self(autre_module);
    }
    GET_TICK(t2);
    temps = timing_tick2usec(TICK_DIFF(t1, t2));
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }

END_SERVICE(LRPC_PING)

void f(int argc, char *argv[])
{ 
  LRPC_REQ(LRPC_PING) req;

  tprintf("*** Migration sous %s ***\n", mad_arch_name());

  if(argc > 1)
    req.nb = atoi(argv[1]);
  else {
    tprintf("Combien d'aller-et-retour ? ");
    scanf("%d", &req.nb);
  }

  LRPC(les_modules[0], LRPC_PING, STD_PRIO, DEFAULT_STACK,
       &req, NULL);
}

void startup_func(int *modules, int nb)
{
  if(pm2_self() == modules[0])
    autre_module = modules[1];
  else
    autre_module = modules[0];
}

int pm2_main(int argc, char **argv)
{
   pm2_init_rpc();

   DECLARE_LRPC(LRPC_PING);

   pm2_set_startup_func(startup_func);

   pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

   if(les_modules[0] == pm2_self()) {

      f(argc, argv);

      pm2_kill_modules(les_modules, nb_modules);
   }

   pm2_exit();

   return 0;
}
