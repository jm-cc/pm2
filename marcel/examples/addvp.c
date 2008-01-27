
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
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#ifndef MA__LWPS
#warning addvp needs SMP or NUMA option
#else
static marcel_kthread_mutex_t kthread_mutex = MARCEL_KTHREAD_MUTEX_INITIALIZER;

any_t blocker(any_t arg)
{
  struct timespec t = { .tv_sec = 2, .tv_nsec = 0};
  marcel_setname(marcel_self(),"blocker");
  marcel_printf("  sleeping %lds\n", t.tv_sec);
  while (nanosleep(&t,&t) == -1 && errno == EINTR);
  marcel_printf("  slept, trying taking kthread_mutex\n");
  if (marcel_kthread_mutex_trylock(&kthread_mutex)) {
    marcel_printf("  not free, trying harder\n");
    marcel_kthread_mutex_lock(&kthread_mutex);
  }
  marcel_printf("  got kthread_mutex, sleeping %lds\n", t.tv_sec = 3);
  while (nanosleep(&t,&t) == -1 && errno == EINTR);
  marcel_printf("  releasing kthread_mutex\n");
  marcel_kthread_mutex_unlock(&kthread_mutex);
  marcel_printf("  released kthread_mutex\n");
  return NULL;
}
#endif

int marcel_main(int argc, char **argv)
{
#ifdef MA__LWPS
  marcel_attr_t attr;
  unsigned lwp;
  marcel_t t;
  unsigned long start;

  marcel_init(&argc, argv);

  marcel_setname(marcel_self(),"main");
  marcel_printf("wait a bit (%p)\n",marcel_self());
  marcel_delay(1000);
  lwp = marcel_add_lwp();
  marcel_printf("created additionnal LWP, wait a bit\n");
  marcel_delay(1000);
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, tbx_true);
  marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(lwp));
  marcel_create(&t, &attr, blocker, NULL);
  marcel_printf("created blocker thread, busy a bit\n");
  start = marcel_clock();
  while(marcel_clock() < start + 2000);
  marcel_printf("done busy\n");

  marcel_printf("taking kthread_mutex\n");
  marcel_kthread_mutex_lock(&kthread_mutex);
  marcel_printf("got kthread_mutex, busy a bit\n");
  start = marcel_clock();
  while(marcel_clock() < start + 1000);
  marcel_printf("releasing kthread_mutex\n");
  marcel_kthread_mutex_unlock(&kthread_mutex);
  marcel_printf("released kthread_mutex, busy a bit\n");
  start = marcel_clock();
  while(marcel_clock() < start + 2000);
  marcel_printf("taking kthread_mutex again\n");
  marcel_kthread_mutex_lock(&kthread_mutex);
  marcel_printf("got kthread_mutex\n");

  marcel_fflush(stdout);
  marcel_end();
#endif
  return 0;
}



