
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
#include "marcel_sem.h"

static marcel_sem_t sem1;
static marcel_sem_t sem2;

static volatile tbx_bool_t stay_busy = tbx_true;

void deviation0(void *arg)
{
  marcel_fprintf(stderr, "Deviation of a busy thread : %s\n", (char *)arg);
  stay_busy = tbx_false;
}

void deviation1(void *arg)
{
  marcel_fprintf(stderr, "Deviation of a sleeping thread: %s\n", (char *)arg);
  marcel_disable_deviation();
}

void deviation2(void *arg)
{
  marcel_fprintf(stderr, "Delayed deviation: %s\n", (char *)arg);
}

void deviation3(void *arg)
{
  marcel_fprintf(stderr, "Deviation of a blocked thread: %s\n", (char *)arg);
}

void deviation4(void *arg)
{
  marcel_fprintf(stderr, "Deviation in order to wake one's self: ");
  marcel_sem_V(&sem2);
}

any_t func(any_t arg)
{
  int i;

  marcel_fprintf(stderr, "I'm on VP(%d)\n", marcel_current_vp());

  while(stay_busy) ;

  for(i=0; i<10; i++) {
    marcel_delay(100);
    marcel_fprintf(stderr, "Hi %s!\n", (char *)arg);
  }

  marcel_enable_deviation();

  marcel_sem_V(&sem1);

  marcel_fprintf(stderr, "Will block...\n");
  marcel_sem_P(&sem2);
  marcel_fprintf(stderr, "OK !\n");

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_t pid;
  any_t status;
  marcel_attr_t attr;

  marcel_init(&argc, argv);

  marcel_sem_init(&sem1, 0);
  marcel_sem_init(&sem2, 0);

  marcel_attr_init(&attr);
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(marcel_nbvps()-1));

  marcel_create(&pid, &attr, func, (any_t)"boys");

  marcel_delay(100);

  marcel_deviate(pid, deviation0, "Ok!");

  marcel_delay(500);

  marcel_deviate(pid, deviation1, "Ok!");

  marcel_delay(50);

  marcel_deviate(pid, deviation2, "Only now!");

  marcel_sem_P(&sem1);

  marcel_delay(500);

  marcel_deviate(pid, deviation3, "Ok!");

  marcel_delay(100);

  marcel_deviate(pid, deviation4, NULL);

  marcel_join(pid, &status);

  marcel_end();

  return 0;
}
