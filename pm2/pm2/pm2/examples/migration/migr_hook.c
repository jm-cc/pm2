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

#include <pm2.h>

#include <sys/time.h>

static unsigned autre;
static marcel_key_t user_key;
static marcel_sem_t sem;

void thread_func(void *arg)
{
  char *msg = tmalloc(128);
  unsigned nb = (unsigned)arg;

  tfprintf(stderr, "Allocating %p\n", msg);

  strcpy(msg, "Hello guys !");

  marcel_setspecific(user_key, &msg);

  pm2_enable_migration();

  while(nb--) {
    pm2_printf("%s (@ = %p)\n", msg, msg);
    pm2_migrate_self(autre);
  }

  marcel_sem_V(&sem);
}

void startup_func()
{
  autre = (pm2_self() == 0) ? 1 : 0;
}

void pre_migration(marcel_t pid)
{ 
  char *loc;
  int size;

   loc = *((char **)(*marcel_specificdatalocation(pid, user_key)));
   size = strlen(loc)+1;
   old_mad_pack_int(MAD_IN_HEADER, &size, 1);
   old_mad_pack_str(MAD_IN_HEADER, loc);
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

  old_mad_unpack_int(MAD_IN_HEADER, &size, 1);
  msg = tmalloc(size);
  old_mad_unpack_str(MAD_IN_HEADER, msg);

  return msg;
}

/*
 * Seconde fonction "post-migratique" : on effectue les manipulations
 * modifiant l'etat du thread lui-meme.
 */
void post_post_migration(any_t key)
{
  char **loc = (char **)(*marcel_specificdatalocation(marcel_self(),
						      user_key));

  *loc = (char *)key; /* key == msg */
}

int pm2_main(int argc, char **argv)
{
  marcel_key_create(&user_key, NULL);

  pm2_set_startup_func(startup_func);
  pm2_set_pre_migration_func(pre_migration);
  pm2_set_post_migration_func(post_migration);
  pm2_set_post_post_migration_func(post_post_migration);

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) {

    marcel_sem_init(&sem, 0);

    pm2_thread_create(thread_func, (void *)10);
    marcel_sem_P(&sem);

    pm2_halt();
  }

  pm2_exit();

  printf("Main is ending\n");
  return 0;
}
