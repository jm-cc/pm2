
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

#include "thread.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>


marcel_attr_t attr;

#define TMAX 100

marcel_t tid[TMAX];

double *m1, *m2, *m3;
int nb_threads;
int size;

any_t prod(any_t arg)
{
  int start=(int)arg;
  int i,j;
  int end;
  end=start+size/nb_threads;

  for (i=start; i<end; i++) {
	  for (j=0; j<size; j++) {
		  m3[i]+=m1[j]*m2[i];
	  }
  }

  return NULL;
}

tick_t t1, t2;

int marcel_main(int argc, char **argv)
{
  unsigned long temps;

  int i,j;

  timing_init();
  marcel_init(argc, argv);

  marcel_attr_init(&attr);
  //marcel_attr_setdetachstate(&attr, tbx_true);
  //marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_AFFINITY);

  size = atoi(argv[1]);
  nb_threads = atoi(argv[2]);

  m1=malloc(sizeof(double)*size);
  m2=malloc(sizeof(double)*size);
  m3=malloc(sizeof(double)*size);

  for (i=0; i<size; i++) {
	  m1[i]=2.4;
	  m2[i]=2.4;
	  m3[i]=0;
  }
  for (j=0; j<4; j++) {

	  GET_TICK(t1);
	  for (i=0; i<nb_threads; i++) {
		  marcel_create(&tid[i], &attr, prod, (any_t)(size*i/nb_threads));
	  }
	  for (i=0; i<nb_threads; i++) {
		  marcel_join(tid[i], NULL);
	  }
	  GET_TICK(t2);
	  printf("Prod of %d at %i\t", size, nb_threads);
	  temps = TIMING_DELAY(t1, t2);
	  printf("time = %ld.%03ldms\n", temps/1000, temps%1000);
  }
#ifdef PROFILE
  profile_stop();
#endif

  marcel_end();
  return 0;
}



