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


static int *les_modules, nb_modules, autre_module;

static int var_glob;

BEGIN_SERVICE(LRPC_SAMPLE)
  int k1, k2, k3;
  int *ptr_null = NULL;
  int *ptr_glob = &var_glob;
  int i, *ptr_loc = &i;

  k1 = pm2_register_pointer(&ptr_null);
  k2 = pm2_register_pointer(&ptr_glob);
  k3 = pm2_register_pointer(&ptr_loc);

  pm2_enable_migration();

  pm2_printf("Avant :\n");
  pm2_printf("   marcel_self : %lx\n", marcel_self());
  pm2_printf("   user_space : %lx\n", marcel_self()->user_space_ptr);
  pm2_printf("   parametres : %lx\n", &req);
  pm2_printf("   ptr_null : %lx\n", ptr_null);
  pm2_printf("   ptr_glob : %lx\n", ptr_glob);
  pm2_printf("   ptr_loc : %lx\n", ptr_loc);
  
  pm2_freeze();
  pm2_migrate(marcel_self(), autre_module);

  pm2_printf("Apres :\n");
  pm2_printf("   marcel_self : %lx\n", marcel_self());
  pm2_printf("   user_space : %lx\n", marcel_self()->user_space_ptr);
  pm2_printf("   parametres : %lx\n", &req);
  pm2_printf("   ptr_null : %lx\n", ptr_null);
  pm2_printf("   ptr_glob : %lx\n", ptr_glob);
  pm2_printf("   ptr_loc : %lx\n", ptr_loc);

  pm2_unregister_pointer(k1);
  pm2_unregister_pointer(k2);
  pm2_unregister_pointer(k3);

END_SERVICE(LRPC_SAMPLE)

void f(void)
{
  LRPC_REQ(LRPC_SAMPLE) req;

  strcpy(req.tab, "bidon");
  LRPC(pm2_self(), LRPC_SAMPLE, STD_PRIO, DEFAULT_STACK, &req, NULL);
}

void startup_func(int *modules, int nb)
{
  if(pm2_self() == modules[0])
    autre_module = modules[1];
  else {
    autre_module = modules[0];
    /* Pour forcer l'allocation du thread a une addresse differente : */
    malloc(10000);
  }
}

int pm2_main(int argc, char **argv)
{
   pm2_init_rpc();

   DECLARE_LRPC(LRPC_SAMPLE);

   pm2_set_startup_func(startup_func);

   pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

   if(les_modules[0] == pm2_self()) {

      f();

      pm2_kill_modules(les_modules, nb_modules);
   }

   pm2_exit();

   printf("Main is ending\n");

   return 0;
}
