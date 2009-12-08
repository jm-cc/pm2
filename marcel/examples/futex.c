
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

/* futex.c */

#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"

typedef struct {
  unsigned long held;
  marcel_futex_t futex;
} lock_t;

lock_t mylock = { .held = 0, .futex = MARCEL_FUTEX_INITIALIZER };

int tot_retries;

void lock(lock_t *l) {
  int retries = 0;
  while (ma_test_and_set_bit(0, &l->held)) {
  /* Already held, sleep */
    marcel_futex_wait(&l->futex, &l->held, 1, 1, MA_MAX_SCHEDULE_TIMEOUT);
    retries++;
  }
  tot_retries += retries;
}

void unlock(lock_t *l) {
  ma_clear_bit(0, &l->held);
  marcel_futex_wake(&l->futex, 1);
}

int val;

void *f(void *arg) {
  int n = (intptr_t) arg;

  while (n--) {
    lock(&mylock);
    val++;
    unlock(&mylock);
  }

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  int nt, n, i;

  marcel_init(&argc, argv);

  if (argc < 3) {
    marcel_fprintf(stderr, "usage: %s <nbthreads> <nbloops>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  nt = atoi(argv[1]);
  n = atoi(argv[2]);

  for (i = 0; i < nt; i++)
    marcel_create(NULL, NULL, f, (void*) (intptr_t) n);

  marcel_end();

  printf("%d total retries\n", tot_retries);

  if (val != n*nt) {
    printf("%d, should be %d\n", val, n * nt);
    exit(1);
  }

  return 0;
}



