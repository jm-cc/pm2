
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#define MARCEL_INTERNAL_INCLUDE
#include "pm2.h"

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

void startup_func(int argc, char *argv[], void *arg)
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

  pm2_push_startup_func(startup_func, NULL);

  pm2_set_pre_migration_func(pre_migration);
  pm2_set_post_migration_func(post_migration);
  pm2_set_post_post_migration_func(post_post_migration);

  pm2_init(argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
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
