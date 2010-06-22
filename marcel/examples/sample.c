
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

/* sample.c */

#include "marcel.h"
#include "pm2_common.h"

any_t ALL_IS_OK = (any_t)123456789L;

#define NB    3

char *mess[NB] = { "boys", "girls", "people" };

any_t writer(any_t arg)
{
  int i, j;

  for(i=0;i<10;i++) {
    tfprintf(stderr, "Hi %s! (I'm %p on vp %d)\n",
	     (char*)arg, marcel_self(), marcel_current_vp());
    j = 20000000; while(j--);
  }

  return ALL_IS_OK;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid[NB];
  int i;

  marcel_init(&argc, argv) ;

  for(i=0; i<NB; i++)
    marcel_create(&pid[i], NULL, writer, (any_t)mess[i]);

  marcel_yield();
  marcel_yield();
  marcel_yield();
  marcel_yield();

  for(i=0; i<NB; i++) {
    marcel_join(pid[i], &status);
    if(status == ALL_IS_OK)
      tprintf("Thread %p completed ok.\n", pid[i]);
  }

  marcel_end() ;

  return 0;
}



