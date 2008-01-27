
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

/* smptest.c */

#include "marcel.h"

#define NB    20

static marcel_sem_t sem[NB];

any_t thread_func(any_t arg)
{
  unsigned long num = (long)arg;

  marcel_detach(marcel_self());

  marcel_sem_P(&sem[num]);

  marcel_fprintf(stderr, "Hi! I'm thread %ld on VP %d\n",
	  num, marcel_current_vp());

  if(num != NB-1)
    marcel_sem_V(&sem[num+1]);

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_attr_t attr;
  unsigned long i;

  marcel_init(&argc, argv);

  marcel_attr_init(&attr);

  for(i=0; i<NB; i++)
    marcel_sem_init(&sem[i], 0);

  for(i=0; i<NB; i++) {
    marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(i % marcel_nbvps()));

    marcel_create(NULL, &attr, thread_func, (any_t)i);
  }

  marcel_sem_V(&sem[0]);

  marcel_end();
  return 0;
}

