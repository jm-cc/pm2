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


int *les_modules, nb_modules;

int autre_module;

marcel_key_t user_key;


BEGIN_SERVICE(LRPC_PING)
  char *msg = tmalloc(128);
  int k;

  tfprintf(stderr, "Allocating %p\n", msg);

  strcpy(msg, "Hello guys !");

  marcel_setspecific(user_key, &msg);
  k = pm2_register_pointer(marcel_specificdatalocation(marcel_self(), user_key));
  pm2_enable_migration();

  while(req.nb--) {
    pm2_printf("%s (@ = %p)\n", msg, msg);
    pm2_freeze();
    pm2_migrate(marcel_self(), autre_module);
  }

  pm2_unregister_pointer(k);

END_SERVICE(LRPC_PING)

void startup_func(int *modules, int nb)
{
  if(pm2_self() == modules[0])
    autre_module = modules[1];
  else
    autre_module = modules[0];
}

void pre_migration(marcel_t pid)
{ 
  char *loc;
  int size;

   loc = *((char **)(*marcel_specificdatalocation(pid, user_key)));
   size = strlen(loc)+1;
   mad_pack_int(MAD_IN_HEADER, &size, 1);
   mad_pack_str(MAD_IN_HEADER, loc);
   tfree(loc);
}

/*
 * Premiere fonction "post-migratique" : on effectue les manipulations
 * ayant trait au deballage des donnees transmises avec le thread.
 */
any_t post_migration(marcel_t pid)
{
  char *msg;
  int size;

  mad_unpack_int(MAD_IN_HEADER, &size, 1);
  msg = tmalloc(size);
  mad_unpack_str(MAD_IN_HEADER, msg);

  return msg;
}

/*
 * Seconde fonction "post-migratique" : on effectue les manipulations
 * modifiant l'etat du thread lui-meme.
 */
void post_post_migration(any_t key)
{
  char **loc = (char **)(*marcel_specificdatalocation(marcel_self(), user_key));

  *loc = (char *)key;
}

int pm2_main(int argc, char **argv)
{
  LRPC_REQ(LRPC_PING) req;

  pm2_init_rpc();

  DECLARE_LRPC(LRPC_PING);

  marcel_key_create(&user_key, NULL);

  pm2_set_startup_func(startup_func);
  pm2_set_pre_migration_func(pre_migration);
  pm2_set_post_migration_func(post_migration);
  pm2_set_post_post_migration_func(post_post_migration);

  pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

  if(les_modules[0] == pm2_self()) {

    req.nb = 10;

    LRPC(les_modules[0], LRPC_PING, STD_PRIO, DEFAULT_STACK,
	 &req, NULL);

    pm2_kill_modules(les_modules, nb_modules);
  }

  pm2_exit();

  printf("Main is ending\n");
  return 0;
}
