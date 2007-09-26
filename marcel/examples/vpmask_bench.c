
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

volatile int started;
volatile int finished;

any_t f_busy(any_t arg)
{
  marcel_printf("I'm alive\n");
  started = 1;
  ma_mb();
  while (!finished);
  marcel_printf("I'm dead\n");

  return NULL;
}

any_t f_idle(any_t arg)
{
  marcel_printf("I'm idle (2)\n");
  started = 1;
  ma_mb();
  while (!finished) marcel_sleep(1);
  marcel_printf("I'm dead (2)\n");

  return NULL;
}

void bench_change_vpmask(unsigned nb)
{
  tbx_tick_t t1, t2;
  int i = nb;
  int vp = 0;

  TBX_GET_TICK(t1);
  while(--i) {
    marcel_vpmask_t vpmask = MARCEL_VPMASK_ALL_BUT_VP(vp);
    marcel_change_vpmask(&vpmask);
    vp = 1-vp;
  }
  TBX_GET_TICK(t2);

  marcel_printf("change vpmask =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (double)nb);
  return;
 
}

void bench_migrate(unsigned long nb, int active)
{
  tbx_tick_t t1, t2;
  marcel_t pid;
  any_t status;
  register long n = nb;
  marcel_attr_t attr;
  marcel_attr_init(&attr);
#ifdef MA__BUBBLES
  marcel_bubble_t b;
  marcel_bubble_init(&b);
  marcel_attr_setinitbubble(&attr, &b);
  marcel_bubble_setinitrq(&b, &marcel_topo_vp_level[2].sched);
  marcel_wake_up_bubble(&b);
#endif

  ma_holder_t *h[2];
  int i = 0;

  if(!nb)
    return;

  finished = 0;
  started = 0;
  ma_wmb();
  marcel_create(&pid, &attr, active ? f_busy : f_idle, (any_t)n);
  ma_rmb();
  while (!started);

#ifdef MA__BUBBLES
  h[0] = &b.hold;
#else
  h[0] = &marcel_topo_vp_level[2].sched.hold;
#endif
  h[1] = &marcel_topo_vp_level[3].sched.hold;

  TBX_GET_TICK(t1);
  while(--n) {
    int state;
    ma_holder_lock_softirq(h[i]);
    state = ma_get_entity(ma_entity_task(pid));
    ma_holder_unlock_softirq(h[i]);
    i = 1-i;
    ma_holder_lock_softirq(h[i]);
    ma_put_entity(ma_entity_task(pid), h[i], state);
    ma_holder_unlock_softirq(h[i]);
  }
  TBX_GET_TICK(t2);
  marcel_printf("migration time =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (double)nb);
  finished = 1;
  marcel_join(pid, &status);
#ifdef MA__BUBBLES
  marcel_bubble_join(&b);
#endif
}

int marcel_main(int argc, char *argv[])
{ 
  int essais = 3;

  marcel_init(&argc, argv);

  if(argc != 2) {
    marcel_fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  if (marcel_nbvps() < 4) {
    marcel_fprintf(stderr, "I need at least 4 processors to run\n");
    exit(1);
  }

  while(essais--) {
    bench_change_vpmask(atol(argv[1]));
    bench_migrate(atol(argv[1])*100, 1);
    bench_migrate(atol(argv[1])*100, 0);
  }

  marcel_end();

  return 0;
}
