
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

#ifdef MARCEL_SUSPEND_ENABLED
any_t func(any_t arg)
{
  int i;

  for(i=0; i<10; i++) {
    marcel_delay(100);
    marcel_fprintf(stderr, "Hi %s!\n", (char *)arg);
  }

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_t pid;
  any_t status;
  marcel_attr_t attr;

  marcel_init(&argc, argv);

  marcel_attr_init(&attr);
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(marcel_nbvps()-1));

  marcel_create(&pid, &attr, func, (any_t)"boys");

  marcel_delay(500);

  marcel_suspend(pid);

  marcel_fprintf(stderr, "I think I managed to suspend this crazy thread...\n");

  marcel_delay(3000);

  marcel_fprintf(stderr, "Well, ok. Let's resume it again!\n");

  marcel_resume(pid);

  marcel_join(pid, &status);

  marcel_end();

  return 0;
}
#else
#  warning Marcel suspend must be enabled for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr, "'marcel suspend' feature disabled in the flavor\n");
  return 0;
}
#endif
