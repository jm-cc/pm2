
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

any_t f(any_t arg)
{
  register int n = (int)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n)
    marcel_yield();
  TBX_GET_TICK(t2);

  printf("contsw'time =  %fus\n", TBX_TIMING_DELAY(t1, t2));
  return NULL;
}

any_t f2(any_t arg)
{
  register int n = (int)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n) {
    if (a) {
	    printf("ok\n");
    }
    marcel_yield();
  }
  TBX_GET_TICK(t2);

  printf("contsw'time =  %fus\n", TBX_TIMING_DELAY(t1, t2));
  return NULL;
}

extern void stop_timer(void);

void bench_setjmp(unsigned nb)
{
  tbx_tick_t t1, t2;
  jmp_buf buf;

  TBX_GET_TICK(t1);
  while(--nb) {
    setjmp(buf);
  }
  TBX_GET_TICK(t2);

  printf("setjmp'time =  %fus\n", TBX_TIMING_DELAY(t1, t2));
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

  printf("longjmp'time =  %fus\n", TBX_TIMING_DELAY(t1, t2));
  return;
 
}

void bench_contsw(unsigned nb)
{
  marcel_t pid;
  any_t status;
  register int n;

  if(!nb)
    return;

  n = nb >> 1;
  n++;

  marcel_create(&pid, NULL, f, (any_t)n);
  marcel_yield();

  while(--n)
    marcel_yield();

  marcel_join(pid, &status);
}

void bench_contsw2(unsigned nb)
{
  marcel_t pid;
  any_t status;
  register int n;

  if(!nb)
    return;

  n = nb >> 1;
  n++;

  marcel_create(&pid, NULL, f2, (any_t)n);
  marcel_yield();

  while(--n) {
    if (a) {
	    printf("ok\n");
    }
    marcel_yield();
  }

  marcel_join(pid, &status);
}

void bench_contsw3(unsigned nb)
{
  marcel_t pid;
  any_t status;
  register int n = nb;

  if(!nb)
    return;

  marcel_create(&pid, NULL, f, (any_t)n);
  marcel_join(pid, &status);
}

int marcel_main(int argc, char *argv[])
{ 
  int essais = 3;

  marcel_init(&argc, argv);

  if(argc != 2) {
    fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
    exit(1);
  }

  while(essais--) {
    bench_setjmp(atoi(argv[1]));
    bench_longjmp(atoi(argv[1]));
    bench_contsw(atoi(argv[1]));
    bench_contsw2(atoi(argv[1]));
    bench_contsw3(atoi(argv[1]));
  }

  return 0;
}
