
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
#include <sys/time.h>

static tbx_bool_t finished = tbx_false;

static
any_t looper(any_t arg)
{
  marcel_fprintf(stderr, "Looper lauched on LWP %d\n",
	  marcel_current_vp());

  while(!finished)
    marcel_yield();

  return NULL;
}

static
any_t f(any_t arg)
{
  marcel_sem_V((marcel_sem_t *)arg);
  /* marcel_fprintf(stderr, "Hi! I'm on LWP %d\n", marcel_current_vp()); */
  return NULL;
}

extern void stop_timer(void);

tbx_tick_t t1, t2;

static
any_t main_thread(void *arg)
{
  marcel_attr_t attr;
  marcel_sem_t sem;
  long nb;
  int essais = 3;

  marcel_detach(marcel_self());

  //  marcel_fprintf(stderr, "Main thread launched on LWP %d\n",
  //  marcel_current_vp());

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, tbx_true);
#ifdef MA__LWPS
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(0));

  //marcel_create(NULL, &attr, looper, NULL);
#endif

  marcel_sem_init(&sem, 0);

  while(essais--) {
    nb = (long)arg;

    if(nb == 0)
      break;

    while(nb--) {

      if(nb != 0) {
	marcel_create(NULL, &attr, f, (any_t)&sem);
	marcel_sem_P(&sem);
      } else {
	TBX_GET_TICK(t1);
#ifdef PROFILE
	profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif
	marcel_create(NULL, &attr, f, (any_t)&sem);
	marcel_sem_P(&sem);
#ifdef PROFILE
	profile_stop();
#endif
	TBX_GET_TICK(t2);
	marcel_printf("time =  %fus\n", TBX_TIMING_DELAY(t1, t2));
      }
    }
  }

  finished = tbx_true;
  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_attr_t attr;

  marcel_init(&argc, argv);

  if(argc != 2) {
    marcel_fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  marcel_attr_init(&attr);
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(0));
  marcel_create(NULL, &attr, main_thread, (void *)atol(argv[1]));

  marcel_end();
  return 0;
}

