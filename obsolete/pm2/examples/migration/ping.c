
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

#include <sys/time.h>
#include <stdlib.h>

#define ESSAIS 5

static unsigned autre;
static tbx_tick_t t1, t2;
static marcel_sem_t sem;

void thread_func(void *arg)
{
  unsigned long temps;
  unsigned long n, nping;

  pm2_enable_migration();

  for(n=0; n<ESSAIS; n++) {
    nping = (unsigned long)arg;
    TBX_GET_TICK(t1);
    while(1) {
      if(nping-- == 0)
	break;
      pm2_migrate_self(autre);
    }
    TBX_GET_TICK(t2);
    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }

  marcel_sem_V(&sem);
}

void startup_func(int argc, char *argv[], void *arg)
{
  autre = (pm2_self() == 0) ? 1 : 0;
}

int pm2_main(int argc, char **argv)
{
  unsigned long nb;

  pm2_push_startup_func(startup_func, NULL);

  pm2_init(argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) {

    tprintf("*** Migration ***\n");
    if(argc > 1)
      nb = atoi(argv[1]);
    else {
      tprintf("Combien d'aller-et-retour ? ");
      scanf("%ld", &nb);
    }

    marcel_sem_init(&sem, 0);
    pm2_thread_create(thread_func, (void *)nb);
    marcel_sem_P(&sem);

    pm2_halt();
  }

  pm2_exit();

  return 0;
}
