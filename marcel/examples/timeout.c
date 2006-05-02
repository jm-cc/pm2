
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

/* timeout.c */

#include "marcel.h"

marcel_sem_t sem;

any_t f(any_t arg)
{
  MARCEL_LOOP(b)
    MARCEL_EXCEPTION_BEGIN
      marcel_sem_timed_P(&sem, 500);
      MARCEL_EXIT_LOOP(b);
    MARCEL_EXCEPTION
      MARCEL_EXCEPTION_WHEN(MARCEL_TIME_OUT)
        tprintf("What a boring job, isn't it ?\n");
    MARCEL_EXCEPTION_END
  MARCEL_END_LOOP(b)
  tprintf("What a relief !\n");

  return 0;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid;

  marcel_init(&argc, argv);

  marcel_sem_init(&sem, 0);

  marcel_create(&pid, NULL, f,  NULL);

  marcel_delay(2100);
  marcel_sem_V(&sem);

  marcel_join(pid, &status);

  marcel_end();
  return 0;
}
