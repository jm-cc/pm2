
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

volatile int a=0;

marcel_t main_thread;

any_t f(any_t arg)
{
  register long n = (long)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n)
    marcel_yield();
  TBX_GET_TICK(t2);

  marcel_printf("contsw'time (schedule+switch) =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (2*(double)(long)arg));
  return NULL;
}

any_t f2(any_t arg)
{
  register long n = (long)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n) {
    if (a) {
	    marcel_printf("ok\n");
    }
    marcel_yield();
  }
  TBX_GET_TICK(t2);

  marcel_printf("contsw'time (schedule+switch) =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (2*(double)(long)arg));
  return NULL;
}

any_t f3(any_t arg)
{
  register long n = (long)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n)
    marcel_yield();
  TBX_GET_TICK(t2);

  marcel_printf("contsw'time (schedule) =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (2*(double)(long)arg));
  return NULL;
}

any_t f4(any_t arg)
{
  register long n = (long)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n)
    marcel_yield_to(main_thread);
  TBX_GET_TICK(t2);

  marcel_printf("contsw'time (yield_to) =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (2*(double)(long)arg));
  return NULL;
}

extern void stop_timer(void);

void bench_setjmp(unsigned nb)
{
  tbx_tick_t t1, t2;
  int i = nb;
  jmp_buf buf;

  TBX_GET_TICK(t1);
  while(--i) {
    setjmp(buf);
  }
  TBX_GET_TICK(t2);

  marcel_printf("setjmp'time =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (double)nb);
  return;
 
}

void bench_longjmp(unsigned nb)
{
  tbx_tick_t t1, t2;
  jmp_buf buf;
  volatile int i = nb;

  TBX_GET_TICK(t1);
  setjmp(buf);
  if(i != 0) {
    i--;
    longjmp(buf, 1);
  }
  TBX_GET_TICK(t2);

  marcel_printf("longjmp'time =  %fus\n", TBX_TIMING_DELAY(t1, t2) / (double)nb);
  return;
 
}

void bench_contsw(unsigned long nb)
{
  marcel_t pid;
  any_t status;
  register long n;
  marcel_attr_t attr;
  marcel_attr_init(&attr);
#ifdef MA_BUBBLES
  marcel_bubble_t b;
  marcel_bubble_init(&b);
  marcel_attr_setinitbubble(&attr, &b);
  marcel_wake_up_bubble(&b);
#endif

  if(!nb)
    return;

  n = nb >> 1;
  n++;

  marcel_create(&pid, &attr, f, (any_t)n);
  marcel_yield();

  while(--n)
    marcel_yield();

  marcel_join(pid, &status);
}

void bench_contsw2(unsigned long nb)
{
  marcel_t pid;
  any_t status;
  register long n;

  if(!nb)
    return;

  n = nb >> 1;
  n++;

  marcel_create(&pid, NULL, f2, (any_t)n);
  marcel_yield();

  while(--n) {
    if (a) {
	    marcel_printf("ok\n");
    }
    marcel_yield();
  }

  marcel_join(pid, &status);
}

void bench_contsw3(unsigned long nb)
{
  marcel_t pid;
  any_t status;
  register long n = nb;
  marcel_attr_t attr;
  marcel_attr_init(&attr);
#ifdef MA_BUBBLES
  marcel_bubble_t b;
  marcel_bubble_init(&b);
  marcel_attr_setinitbubble(&attr, &b);
  marcel_wake_up_bubble(&b);
#endif

  if(!nb)
    return;

  marcel_create(&pid, &attr, f3, (any_t)n);
  marcel_join(pid, &status);
}

void bench_contsw4(unsigned long nb)
{
  marcel_t pid;
  any_t status;
  register long n;
  marcel_attr_t attr;
  marcel_attr_init(&attr);
#ifdef MA_BUBBLES
  marcel_bubble_t b;
  marcel_bubble_init(&b);
  marcel_attr_setinitbubble(&attr, &b);
  marcel_wake_up_bubble(&b);
#endif

  if(!nb)
    return;

  n = nb >> 1;
  n++;

  marcel_create(&pid, &attr, f4, (any_t)n);
  marcel_yield_to(pid);

  while(--n)
    marcel_yield_to(pid);

  marcel_join(pid, &status);
}

int marcel_main(int argc, char *argv[])
{ 
  int essais = 3;

  marcel_init(&argc, argv);

  main_thread = marcel_self();
  if(argc != 2) {
    marcel_fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  while(essais--) {
    bench_setjmp(atol(argv[1]));
    bench_longjmp(atol(argv[1]));
    bench_contsw(atol(argv[1]));
    bench_contsw2(atol(argv[1]));
    bench_contsw3(atol(argv[1]));
    bench_contsw4(atol(argv[1]));
  }

  marcel_end();

  return 0;
}
