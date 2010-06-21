
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#include "marcel.h"

static any_t worker(any_t arg) {
  int n = (intptr_t) arg;
  tbx_tick_t t1,t2;

  marcel_usleep(10000); /* synchronisation à deux balles */

  TBX_GET_TICK(t1);

  /* blabla */

  TBX_GET_TICK(t2);

  marcel_printf("worker %d time = %fus\n", n, TBX_TIMING_DELAY(t1, t2));
  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  int i;
  marcel_attr_t attr;
  marcel_init(argc, argv);

  marcel_attr_init(&attr);

  /* on crée un thread par processeur */
  for(i=0; i<marcel_nbvps(); i++) {
    marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(i));
    marcel_create(NULL, &attr, worker, (any_t)(intptr_t)i);
  }

  /* on les laisse tourner */
  marcel_end();
  return 0;
}
