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

#include <pm2_common.h> /* Here! */
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int inf, sup, res;
  marcel_sem_t sem;/* Here! */
} job;

static void job_init(job *j, int inf, int sup) {
  j->inf = inf;
  j->sup = sup;
  marcel_sem_init(&j->sem, 0);/* Here! */
}

static any_t sum(any_t arg) {
  job *j = (job *)arg;
  job j1, j2;

  if(j->inf == j->sup) {
    j->res = j->inf;
    marcel_sem_V(&j->sem);/* Here! */
    return NULL;
  }

  job_init(&j1, j->inf, (j->inf+j->sup)/2 );
  job_init(&j2, (j->inf+j->sup)/2+1, j->sup);

  marcel_create(NULL, NULL, sum, (any_t)&j1);/* Here! */
  marcel_create(NULL, NULL, sum, (any_t)&j2);/* Here! */
  marcel_sem_P(&j1.sem);/* Here! */
  marcel_sem_P(&j2.sem);/* Here! */

  j->res = j1.res+j2.res;
  marcel_sem_V(&j->sem);/* Here! */
  return NULL;
}

int marcel_main(int argc, char **argv) {/* Here! */
  job j;

  marcel_init(&argc, argv) ;

  marcel_sem_init(&j.sem, 0);/* Here! */
  j.inf = 1;
  j.sup = 1000;

  marcel_create(NULL, NULL, sum, (any_t)&j);/* Here! */
  marcel_sem_P(&j.sem);/* Here! */
  printf("Sum from 1 to %d = %d\n", j.sup, j.res);

  marcel_end() ;
  return 0;
}
