
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

#ifdef MA__BUBBLES

#define GANGS 5
#define THREADS 5
#define DIFFERENT
//#define BARRIER

extern ma_runqueue_t ma_gang_rq;

#ifdef BARRIER
marcel_barrier_t barrier[GANGS];
#endif

any_t work(any_t arg) {
  int n = (int)(intptr_t) arg;
  *marcel_stats_get(marcel_self(), load) = rand()%10;
  marcel_task_memory_attach(NULL,NULL,(rand()%10)<<20,rand()%2);
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
#ifdef MARCEL_GANG_SCHEDULER
    marcel_bubble_setinitrq(&gang[i], &ma_gang_rq);
#endif
    marcel_attr_setinitbubble(&attr, &gang[i]);
#ifdef DIFFERENT
    for (j=0; j<(i+1)%THREADS; j++) {
#else
    for (j=0; j<THREADS; j++) {
#endif
      snprintf(name,sizeof(name),"%d-%d",i,j);
      marcel_attr_setname(&attr,name);
      marcel_create(NULL,&attr,work,(any_t)(uintptr_t)(i*100+j));
    }
#ifdef MARCEL_GANG_SCHEDULER
    marcel_wake_up_bubble(&gang[i]);
#endif
  }

  marcel_delay(5000);
  marcel_start_playing();
  marcel_bubble_spread(&marcel_root_bubble, marcel_topo_level(0,0));

  for (i=0; i<GANGS; i++)
    marcel_bubble_join(&gang[i]);
  //marcel_cancel(gangsched);

  marcel_fflush(stdout);
  marcel_end();
#else
  marcel_fprintf(stderr,"%s needs bubbles\n",argv[0]);
#endif
  return 0;
}

#else /* MA__BUBBLES */
int main(int argc, char *argv[]) {
   fprintf(stderr,"%s needs bubbles\n",argv[0]);
   return 0;
}
#endif /* MA__BUBBLES */
