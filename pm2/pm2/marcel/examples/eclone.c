
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


#include "clone.h"

static clone_t clone_var;

#define N       3
#define BOUCLES 2

any_t master(any_t arg)
{
  int b;
  volatile int i;

  marcel_detach(marcel_self());

  for(b=0; b<BOUCLES; b++) {

    marcel_delay(1000);

    i = 5;

    CLONE_BEGIN(&clone_var);

    fprintf(stderr,
	    "Je suis en section parallele "
	    "(delta = %lx, i = %d)\n",
	    clone_my_delta(),
	    ++(*((int *)((char *)&i - clone_my_delta()))));

    marcel_delay(1000);

    CLONE_END(&clone_var);

    fprintf(stderr, "ouf, les esclaves ont termine\n");
  }

  clone_terminate(&clone_var);

  return NULL;
}

any_t slave(any_t arg)
{
  marcel_detach(marcel_self());

  CLONE_WAIT(&clone_var);

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  int i;

  marcel_init(&argc, argv);

  clone_init(&clone_var, N);

  marcel_create(NULL, NULL, master, NULL);

  for(i=0; i<N; i++)
    marcel_create(NULL, NULL, slave, NULL);

  marcel_end();

  return 0;
}
