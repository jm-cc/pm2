
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

#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#include <stdio.h>
#include <sys/time.h>

//#define DEBUG
#undef MA__BUBBLES

tbx_tick_t t1, t2, t3;

any_t f(any_t arg)
{
  TBX_GET_TICK(t3);
#ifdef DEBUG
  marcel_fprintf(stderr, "Hi! I'm %p on LWP %d\n", marcel_self(), marcel_current_vp());
#endif
  return NULL;
}

int seed;

any_t main_thread(void *arg)
{
  marcel_attr_t attr;
  long nb;
  int essais = 5;
#ifdef MA__BUBBLES
  marcel_bubble_t b;
  marcel_bubble_init(&b);
  marcel_bubble_scheduleonrq(&b,&marcel_topo_vp_level[0].rq);
  marcel_wake_up_bubble(&b);
  marcel_bubble_insertentity(&b, &marcel_self()->as_entity);
#endif

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, tbx_true);
  marcel_attr_setseed(&attr, seed);
  marcel_attr_setprio(&attr, MA_BATCH_PRIO);
#ifdef MA__BUBBLES
  marcel_attr_setnaturalbubble(&attr, &b);
#else
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(0));
#endif

  while(essais--) {
    nb = (long)arg;

    TBX_GET_TICK(t1);
    while(nb--) {
#ifdef DEBUG
	marcel_fprintf(stderr, "Creating %ld\n", nb);
#endif
	marcel_create(NULL, &attr, f, NULL);
    }
    TBX_GET_TICK(t2);

    marcel_sleep(1);
    marcel_printf("create =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (long)arg);
    marcel_printf("exec   =  %fus\n", TBX_TIMING_DELAY(t2, t3) / (long)arg);
  }
#ifdef MA__BUBBLES
  marcel_bubble_removeentity(&b, &marcel_self()->as_entity);
  marcel_bubble_join(&b);
#endif

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_attr_t attr;
  marcel_t t;

  marcel_init(&argc, argv);

  if(argc != 2) {
    marcel_fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  marcel_attr_init(&attr);
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(0));
  marcel_create(&t, &attr, main_thread, (void *)atol(argv[1]));
  marcel_join(t, NULL);
  seed = 1;
  marcel_create(&t, &attr, main_thread, (void *)atol(argv[1]));
  marcel_join(t, NULL);

  marcel_end();
  return 0;
}

