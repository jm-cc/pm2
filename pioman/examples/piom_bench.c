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

#include <pioman.h>
#include <tbx.h>

#define SEM_LOOPS     50000
#define SPIN_LOOPS    50000
#define TASKLET_LOOPS 10000

static volatile int tasklet_lock = 0;

int test_ltask(void*_arg)
{
  return 0;
}


int main(int argc, char**argv) 
{
  tbx_init(&argc, &argv);
  pioman_init(&argc, argv);

  int i;
  tbx_tick_t t1, t2;

  /* cout d'un sem_V() sem_P() */
  piom_sem_t sem;
  piom_sem_init(&sem, 0);
  printf("piom_sem_V/P (%d loops):", SEM_LOOPS);
  
  TBX_GET_TICK(t1);
  for(i = 0 ; i < SEM_LOOPS ; i++)
    {
      piom_sem_V(&sem);
      piom_sem_P(&sem);
    }
  TBX_GET_TICK(t2);
  printf("\t\t\t%f usec.\n", TBX_TIMING_DELAY(t1, t2)/SEM_LOOPS);
  
  /* cout d'un piom_spin_lock/spin_unlock */
  piom_spinlock_t lock;
  piom_spin_init(&lock);
  printf("piom_spin_lock/unlock (%d loops):", SPIN_LOOPS);

  TBX_GET_TICK(t1);
  for(i = 0 ; i < SPIN_LOOPS ; i++)
    {
      piom_spin_lock(&lock);
      piom_spin_unlock(&lock);
    }
  TBX_GET_TICK(t2);	
  printf("\t\t%f usec.\n", TBX_TIMING_DELAY(t1, t2)/SPIN_LOOPS);
  
  /* Lancement d'une ltask */
  struct piom_ltask*ltasks = malloc(sizeof(struct piom_ltask) * TASKLET_LOOPS);
  printf("Tasklet scheduling (%d loops):", TASKLET_LOOPS);
  TBX_GET_TICK(t1);
  for(i = 0 ; i < TASKLET_LOOPS ; i++)
    {
      piom_ltask_create(&ltasks[i], &test_ltask, NULL, PIOM_LTASK_OPTION_ONESHOT | PIOM_LTASK_OPTION_NOWAIT);
      piom_ltask_submit(&ltasks[i]);
    }
  TBX_GET_TICK(t2);
  printf("\t\t%f usec.\n", TBX_TIMING_DELAY(t1, t2)/TASKLET_LOOPS);
	
  pioman_exit();
  tbx_exit();
  
  return 0;
}
