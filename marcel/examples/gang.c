
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

#if !defined(MARCEL_BUBBLE_STEAL)
#warning I need the bubble_steal option
int marcel_main(int argc, char **argv) { fprintf(stderr,"I need the bubble_steal option\n"); }

#elif !defined(MARCEL_GANG_SCHEDULER)
#warning I need the GANG_SCHEDULER config
int marcel_main(int argc, char **argv) { fprintf(stderr,"I need the GANG_SCHEDULER config\n"); }
#else

#define GANGS 5
#define THREADS 5
#define DIFFERENT
//#define BARRIER

extern ma_runqueue_t ma_gang_rq;

#ifdef BARRIER
marcel_barrier_t barrier[GANGS];
#endif

any_t work(any_t arg) {
  int n = (int) arg;
  while(1);
  marcel_printf("%d done\n",n);
  return NULL;
}

extern ma_runqueue_t ma_dontsched_runqueue;
extern ma_runqueue_t ma_main_runqueue;

#ifdef MA__BUBBLES
marcel_bubble_t gang[GANGS];
#endif

int marcel_main(int argc, char **argv)
{
#ifdef MA__BUBBLES
  int i,j;
  marcel_attr_t attr;
  char name[MARCEL_MAXNAMESIZE];
  struct marcel_sched_param p = { .sched_priority = MA_DEF_PRIO-1 } ;
  //marcel_t gangsched;

  marcel_init(&argc, argv);

  marcel_sched_setparam(marcel_self(), &p);

#ifdef PROFILE
  profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, tbx_true);

  //marcel_attr_setprio(&attr, MA_DEF_PRIO-1);
  //marcel_attr_setname(&attr, "gang scheduler");
  //marcel_create(&gangsched,&attr,marcel_gang_scheduler,NULL);

  //marcel_attr_setprio(&attr, MA_DEF_PRIO);

  for (i=0; i<GANGS; i++) {
#ifdef BARRIER
    marcel_barrier_init(&barrier[i], NULL, (i+1)%THREADS);
#endif
    marcel_bubble_init(&gang[i]);
    marcel_bubble_insertbubble(&marcel_root_bubble, &gang[i]);
    marcel_bubble_setinitrq(&gang[i], &ma_gang_rq);
    marcel_attr_setinitbubble(&attr, &gang[i]);
#ifdef DIFFERENT
    for (j=0; j<(i+1)%THREADS; j++) {
#else
    for (j=0; j<THREADS; j++) {
#endif
      snprintf(name,sizeof(name),"%d-%d",i,j);
      marcel_attr_setname(&attr,name);
      marcel_create(NULL,&attr,work,(any_t)(i*100+j));
    }
    marcel_wake_up_bubble(&gang[i]);
  }

  marcel_start_playing();

  for (i=0; i<GANGS; i++)
    marcel_bubble_join(&gang[i]);
  //marcel_cancel(gangsched);

  fflush(stdout);
  marcel_end();
#else
  fprintf(stderr,"%s needs bubbles\n",argv[0]);
#endif
  return 0;
}



#endif
