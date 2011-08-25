
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

#include "pm2.h"

#define NB	2
static char *mess[NB] = { "Hi guys !", "Hi girls !" };

static marcel_sem_t sem;

void thread_func(void *arg)
{
  int i, proc;
  tbx_tick_t tick1,tick2;

  pm2_enable_migration();

  proc = 0;

  for(i=0; i<3*pm2_config_size(); i++) {
    pm2_printf("%s\n", (char *)arg);

    marcel_delay(100);
    if((i+1) % 3 == 0) {

      proc = (proc+1) % pm2_config_size();

      pm2_printf("I'm now going on node [%d]\n", proc);

      TBX_GET_TICK(tick1);
      pm2_migrate_self(proc);
      TBX_GET_TICK(tick2);

      pm2_printf("Here I am within %lu µsec!\n",TBX_TIMING_DELAY(tick1,tick2));
    }
  }

  marcel_sem_V(&sem);
}

int pm2_main(int argc, char **argv)
{
  int i;

  pm2_init(argc, argv);

  if(pm2_self() == 0) { /* master process */

    marcel_sem_init(&sem, 0);

    for(i=0; i<NB; i++)
      pm2_thread_create(thread_func, mess[i]);

    for(i=0; i<NB; i++)
      marcel_sem_P(&sem);

    pm2_halt();
  }

   pm2_exit();

   tfprintf(stderr, "Main is ending\n");
   return 0;
}
