
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

int started;
int finished;

any_t f_busy(any_t arg)
{
  marcel_printf("I'm alive\n");
  started = 1;
  ma_smp_mb();
  while (!finished) ma_smp_rmb();
  marcel_printf("I'm dead\n");

  return NULL;
}

any_t f_idle(any_t arg)
{
  marcel_printf("I'm idle (2)\n");
  started = 1;
  ma_smp_mb();
  while (!finished) marcel_sleep(1);
  marcel_printf("I'm dead (2)\n");

  return NULL;
}

void bench_apply_vpset(unsigned nb, int target_vp)
{
  tbx_tick_t t1, t2;
  int i = nb;
  int vp = 0;

  TBX_GET_TICK(t1);
  while(--i) {
    marcel_vpset_t vpset = MARCEL_VPSET_VP(vp);
    marcel_apply_vpset(&vpset);
    vp = target_vp-vp;
  }
  TBX_GET_TICK(t2);

  marcel_printf("apply vpset =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (double)nb);
  return;

}

void bench_migrate(unsigned long nb, int active, int target_vp)
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
  marcel_attr_setnaturalbubble(&attr, &b);
  marcel_bubble_scheduleonlevel(&b, &marcel_topo_vp_level[target_vp]);
  marcel_wake_up_bubble(&b);
#endif

  int vp = target_vp - 1;

  if (target_vp == 1)
    vp = 2;

  marcel_vpset_t vpset = MARCEL_VPSET_VP(vp);
  marcel_apply_vpset(&vpset);

  ma_holder_t *h[2];
  int i = 0;

  if(!nb)
    return;

  finished = 0;
  started = 0;
  ma_smp_wmb();
  marcel_create(&pid, &attr, active ? f_busy : f_idle, (any_t)n);
  ma_smp_rmb();
  while (!started) ma_smp_rmb();

#ifdef MA__BUBBLES
  h[0] = &b.as_holder;
#else
  h[0] = &marcel_topo_vp_level[target_vp].rq.as_holder;
#endif
  h[1] = &marcel_topo_vp_level[0].rq.as_holder;

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

void bench_resched(unsigned long nb, int target_vp)
{
  tbx_tick_t t1, t2;
  marcel_t pid;
  any_t status;
  register long n = nb;
  marcel_attr_t attr;

  int vp = target_vp - 1;

  if (target_vp == 1)
    vp = 2;

  marcel_attr_init(&attr);
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(vp));

  finished = 0;
  started = 0;
  ma_smp_wmb();
  marcel_create(&pid, &attr, f_busy, (any_t)n);
  ma_smp_rmb();
  while (!started) ma_smp_rmb();

  TBX_GET_TICK(t1);
  while(--n) {
	  ma_set_tsk_need_togo(pid);
	  ma_resched_task(pid, target_vp, ma_get_lwp_by_vpnum(target_vp));
	  ma_topo_vpdata_l(&marcel_topo_vp_level[target_vp], need_resched) = 0;
  }
  TBX_GET_TICK(t2);
  marcel_printf("resched time =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (double)nb);
  finished = 1;
  marcel_join(pid, &status);
}

int marcel_main(int argc, char *argv[])
{
  int essais = 3;
  int vp = 1;

  marcel_init(&argc, argv);

  if(argc < 2 || argc > 3) {
    marcel_fprintf(stderr, "Usage: %s <nb> <vp>\n", argv[0]);
    exit(1);
  }

  if (argc == 3)
    vp = atoi(argv[2]);

  if (!vp) {
    marcel_fprintf(stderr, "VP can't be 0\n");
    exit(1);
  }

  if (marcel_nbvps() < 3) {
    marcel_fprintf(stderr, "I need at least 3 processors to run\n");
    exit(1);
  }

  while(essais--) {
    bench_apply_vpset(atol(argv[1]), vp);
    //bench_migrate(atol(argv[1])*100, 1, vp);
    //bench_migrate(atol(argv[1])*100, 0, vp);
    bench_resched(atol(argv[1])*10, vp);
  }

  marcel_end();

  return 0;
}
