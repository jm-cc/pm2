/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2007-2017 "the PM2 team" (see AUTHORS file)
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

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include <pioman.h>
#include <tbx.h>

#define ITERATIONS   1000000

static double*lats = NULL;
static int iterations = ITERATIONS;
static int comp_double(const void*_a, const void*_b)
{
  const double*a = _a;
  const double*b = _b;
  if(*a < *b)
    return -1;
  else if(*a > *b)
    return 1;
  else
    return 0;
}
static void lats_init(const char*label)
{
  int i;
  if(lats == NULL)
    {
      lats = malloc(sizeof(double) * iterations);
      printf("# %d loops\n", iterations);
      printf("#  min   \t  med    \t  max    \t  q1    \t  q3    \t  d1    \t   d9   \t   p99  \t  avg\n");
    }
  for(i = 0; i < iterations; i++)
    lats[i] = -1.0;
  printf("\n%s\n", label);
}
static void lats_display(void)
{
  qsort(lats, iterations, sizeof(double), &comp_double);
  const double min_lat = lats[0];
  const double max_lat = lats[iterations - 1];
  const double med_lat = lats[(iterations - 1) / 2];
  const double q1_lat  = lats[(iterations - 1) / 4];
  const double q3_lat  = lats[ 3 * (iterations - 1) / 4];
  const double d1_lat  = lats[(iterations - 1) / 10];
  const double d9_lat  = lats[ 9 * (iterations - 1) / 10];
  const double p99_lat = lats[ 99 * (iterations - 1) / 100];
  double avg_lat = 0.0;
  int k;
  for(k = 0; k < iterations; k++)
    {
      avg_lat += lats[k];
    }
  avg_lat /= iterations;
  printf("%8.3lf\t%8.3f\t%8.3f\t%8.3lf\t%8.3lf\t%8.3lf\t%8.3lf\t%8.3lf\t%8.3f\n",
	 min_lat, med_lat, max_lat, q1_lat, q3_lat, d1_lat, d9_lat, p99_lat, avg_lat);

}

static piom_cond_t ltask_cond;

int test_ltask_empty(void*_arg)
{
  return 0;
}

int test_ltask_cond(void*_arg)
{
  piom_cond_signal(&ltask_cond, 1);
  return 0;
}


int main(int argc, char**argv) 
{
  tbx_init(&argc, &argv);
  pioman_init(&argc, argv);

  int i;
  tbx_tick_t t1, t2;

  /* cout d'un sem_V() sem_P() */
  lats_init("piom_sem_V + P");
  piom_sem_t sem;
  piom_sem_init(&sem, 0);
  for(i = 0 ; i < iterations ; i++)
    {
      TBX_GET_TICK(t1);
      piom_sem_V(&sem);
      piom_sem_P(&sem);
      TBX_GET_TICK(t2);
      lats[i] = TBX_TIMING_DELAY(t1, t2);
    }
  lats_display();

  lats_init("piom_spin_lock + unlock");
  piom_spinlock_t lock;
  piom_spin_init(&lock);
  for(i = 0 ; i < iterations ; i++)
    {
      TBX_GET_TICK(t1);
      piom_spin_lock(&lock);
      piom_spin_unlock(&lock);
      TBX_GET_TICK(t2);	
      lats[i] = TBX_TIMING_DELAY(t1, t2);
    }
  lats_display();

  lats_init("ltask submit");
  struct piom_ltask ltask;
  for(i = 0 ; i < iterations ; i++)
    {
      TBX_GET_TICK(t1);
      piom_ltask_create(&ltask, &test_ltask_empty, NULL, PIOM_LTASK_OPTION_ONESHOT);
      piom_ltask_submit(&ltask);
      TBX_GET_TICK(t2);
      piom_ltask_wait(&ltask);
      piom_ltask_destroy(&ltask);
      lats[i] = TBX_TIMING_DELAY(t1, t2);
    }
  lats_display();

  lats_init("ltask submit + ltask wait");
  for(i = 0 ; i < iterations ; i++)
    {
      TBX_GET_TICK(t1);
      piom_ltask_create(&ltask, &test_ltask_empty, NULL, PIOM_LTASK_OPTION_ONESHOT);
      piom_ltask_submit(&ltask);
      piom_ltask_wait(&ltask);
      TBX_GET_TICK(t2);
      piom_ltask_destroy(&ltask);
      lats[i] = TBX_TIMING_DELAY(t1, t2);
    }
  lats_display();

  lats_init("ltask submit + cond wait");
  for(i = 0 ; i < iterations ; i++)
    {
      piom_cond_init(&ltask_cond, 0);
      TBX_GET_TICK(t1);
      piom_ltask_create(&ltask, &test_ltask_cond, NULL, PIOM_LTASK_OPTION_ONESHOT);
      piom_ltask_submit(&ltask);
      piom_cond_wait(&ltask_cond, 1);
      TBX_GET_TICK(t2);
      piom_ltask_wait(&ltask);
      piom_ltask_destroy(&ltask);
      lats[i] = TBX_TIMING_DELAY(t1, t2);
    }
  lats_display();

  lats_init("piom cond signal + cond wait");
  piom_cond_t cond;
  for(i = 0 ; i < iterations ; i++)
    {
      piom_cond_init(&cond, 0);
      TBX_GET_TICK(t1);
      piom_cond_signal(&cond, 1);
      piom_cond_wait(&cond, 1);
      TBX_GET_TICK(t2);
      piom_cond_destroy(&cond);
      lats[i] = TBX_TIMING_DELAY(t1, t2);
    }
  lats_display();
  
  pioman_exit();
  tbx_exit();
  
  return 0;
}
