
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

#include "marcel.h"

#include <stdio.h>
#include <stdlib.h>

//#define USE_PIPES
//#define AUGMENT_COMPUTING_TIME

typedef struct {
  int inf, sup, res;
#ifdef USE_PIPES
  int channel[2];
#endif
} job;

#ifdef USE_PIPES
static int do_pipe(int *array)
{
  int r = pipe(array);

  if(r == -1) {
    perror("pipe");
    exit(1);
  }

  return r;
}
#endif

static any_t sum(any_t arg)
{
  job *j = (job *)arg;

  LOG_IN();

#ifdef AUGMENT_COMPUTING_TIME
  { int j = 100000; while(j--); }
#endif

  if(j->inf == j->sup) {
    j->res = j->inf;
  } else {
    job j1, j2;
    marcel_t pid1, pid2;

    j1.inf = j->inf; j1.sup = (j->inf + j->sup) / 2;
    j2.inf = j1.sup + 1; j2.sup = j->sup;

#ifdef USE_PIPES
    do_pipe(j1.channel); do_pipe(j2.channel);
#endif

    marcel_create(&pid1, NULL, sum, (any_t)&j1);
    marcel_create(&pid2, NULL, sum, (any_t)&j2);

#ifdef USE_PIPES
    marcel_read(j1.channel[0], &j1.res, sizeof(j1.res));
    marcel_read(j2.channel[0], &j2.res, sizeof(j2.res));

    close(j1.channel[0]); close(j1.channel[1]);
    close(j2.channel[0]); close(j2.channel[1]);
#endif

    marcel_join(pid1, NULL);
    marcel_join(pid2, NULL);

    j->res = j1.res + j2.res;
  }

#ifdef USE_PIPES
  marcel_write(j->channel[1], &j->res, sizeof(j->res));
#endif

  LOG_OUT();

  return NULL;
}

tbx_tick_t t1, t2;

int marcel_main(int argc, char **argv)
{
  job j;
  unsigned long temps;
  marcel_t pid;

  marcel_init(argc, argv);

  if(argc > 1) {

#ifdef PROFILE
    profile_activate(FUT_ENABLE, MARCEL_PROF_MASK | USER_APP_MASK, 1);
#endif

    j.inf = 1;
    j.sup = atoi(argv[1]);

#ifdef USE_PIPES
    do_pipe(j.channel);
#endif

    TBX_GET_TICK(t1);
    marcel_create(&pid, NULL, sum, (any_t)&j);
#ifdef USE_PIPES
    marcel_read(j.channel[0], &j.res, sizeof(j.res));
    close(j.channel[0]); close(j.channel[1]);
#endif
    marcel_join(pid, NULL);
    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    marcel_printf("Sum from 1 to %d = %d\n", j.sup, j.res);
    marcel_printf("time = %ld.%03ldms\n", temps/1000, temps%1000);

#ifdef PROFILE
    profile_stop();
#endif
  }

  marcel_end();
  return 0;
}



