
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
#include <stdlib.h>
#include <limits.h>

#define GANGS 6
#define THREADS 16

marcel_bubble_t gang[GANGS];

any_t work(any_t arg) {
  int i;
  int n = (int) arg;
  for (i=0;i<1000000000;i++)
	  if (!(i%10000000))
  		printf("%d %d\n",n,i);
  marcel_printf("%d done\n",n);
  return NULL;
}

extern ma_runqueue_t ma_dontsched_runqueue;
extern ma_runqueue_t ma_main_runqueue;

int marcel_main(int argc, char **argv)
{
  int i,j;
  marcel_attr_t attr;
  char name[MARCEL_MAXNAMESIZE];
  struct marcel_sched_param p = { .sched_priority = MA_DEF_PRIO-1 } ;
  marcel_t gangsched;

  marcel_init(&argc, argv);

  marcel_setname(marcel_self(), "gang");
  marcel_sched_setparam(marcel_self(), &p);

#ifdef PROFILE
  profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);

  marcel_attr_setprio(&attr, MA_DEF_PRIO-1);
  marcel_attr_setname(&attr, "gang scheduler");
  marcel_create(&gangsched,&attr,marcel_gang_scheduler,NULL);

  marcel_attr_setprio(&attr, MA_DEF_PRIO);

  for (i=0; i<GANGS; i++) {
    marcel_bubble_init(&gang[i]);
    marcel_attr_setinitbubble(&attr, &gang[i]);
    for (j=0; j<(i+1)%THREADS; j++) {
      snprintf(name,sizeof(name),"%d-%d",i,j);
      marcel_attr_setname(&attr,name);
      marcel_create(NULL,&attr,work,(any_t)(i*100+j));
    }
    marcel_wake_up_bubble(&gang[i]);
  }

  //marcel_start_playing();

  for (i=0; i<GANGS; i++)
    marcel_bubble_join(&gang[i]);
  marcel_cancel(gangsched);

  fflush(stdout);
  marcel_end();
  return 0;
}



