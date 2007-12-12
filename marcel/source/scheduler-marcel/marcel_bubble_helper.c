/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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
#include <string.h>

#ifdef MA__BUBBLES

long ma_entity_load(marcel_entity_t *e) 
{
  if (e->type == MA_BUBBLE_ENTITY)
    return *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e), marcel_stats_load_offset);
  else
    return *(long*)ma_task_stats_get(ma_task_entity(e), marcel_stats_load_offset);
}

void
marcel_bubble_activate_idle_scheduler()
{
  if (!ma_idle_scheduler)
    {
      ma_write_lock(&ma_idle_scheduler_lock);
      ma_idle_scheduler = 1;
      ma_write_unlock(&ma_idle_scheduler_lock);
    }
}

void
marcel_bubble_deactivate_idle_scheduler()
{
  if (ma_idle_scheduler)
    {
      ma_write_lock(&ma_idle_scheduler_lock);
      ma_idle_scheduler = 0;
      ma_write_unlock(&ma_idle_scheduler_lock);
    }
}

int ma_decreasing_order_entity_load_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;
	long l1 = ma_entity_load(e1);
	long l2 = ma_entity_load(e2);
	return l2 - l1; /* decreasing order */
}

int ma_increasing_order_entity_load_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;
	long l1 = ma_entity_load(e1);
	long l2 = ma_entity_load(e2);
	return l1 - l2; /* increasing order */
}

/* Suppose que rq est verrouillée */
int ma_count_entities_on_rq(ma_runqueue_t *rq)
{
  marcel_entity_t *ee;
  int ne = 0;

  list_for_each_entry(ee, &rq->hold.sched_list, sched_list)
    {
      if (ee->type == MA_BUBBLE_ENTITY)
	{
	  marcel_bubble_t *bb = ma_bubble_entity(ee);
	  if (bb->hold.nr_ready)
	    ne++;
	}
      else 
	{
	  /* Crade, juste pour tester */
#if 1 /* On prend ou non le thread main quand on ordonnance */
	  if (ee->type == MA_THREAD_ENTITY)
	    if ((ma_task_entity(ee)) != __main_thread)
	      ne++;
#else
	  ne++;
#endif
	}
    }

  return ne;
}

/* Suppose que rq est verrouillée + on a appelé synthesize au préalable */ 
int ma_count_all_entities_on_rq(ma_runqueue_t *rq)
{
  marcel_entity_t *ee;
  int ne = 0;

  list_for_each_entry(ee, &rq->hold.sched_list, sched_list)
    {
      if (ee->type == MA_BUBBLE_ENTITY)
	{
	  marcel_bubble_t *b = ma_bubble_entity(ee);
	  if (b->hold.nr_ready)
	    ne += ma_entity_load(ee);
	}
      else 
	{
	  /* Crade, juste pour tester */
#if 1 /* On prend ou non le thread main quand on ordonnance */
	  if (ee->type == MA_THREAD_ENTITY)
	    if ((ma_task_entity(ee)) != __main_thread)
	      ne++;
#else
	  ne++;
#endif
	}
    }

  return ne;
}

/* Suppose que rq est verrouillée */
void ma_get_entities_from_rq(ma_runqueue_t *rq, marcel_entity_t *e[])
{
  marcel_entity_t *ee;
  int i = 0;

  list_for_each_entry(ee, &rq->hold.sched_list, sched_list)
    {
      if (ee->type == MA_BUBBLE_ENTITY)
	{
	  marcel_bubble_t *bb = ma_bubble_entity(ee);
	  if (bb->hold.nr_ready)
	    e[i++] = ee;
	}
      else
	{
	  /* Crade, juste pour tester */
#if 1 /* On prend ou non le thread main quand on ordonnance */
	  if (ee->type == MA_THREAD_ENTITY)
	    if ((ma_task_entity(ee)) != __main_thread)
	      e[i++] = ee;
#else
	  e[i++] = ee;
#endif
	}    
    }
}

int
ma_gather_all_bubbles_on_rq(ma_runqueue_t *rq)
{
  marcel_entity_t *e;
  int ret = 0;
  
  list_for_each_entry(e, &rq->hold.sched_list, sched_list)
    if (e->type == MA_BUBBLE_ENTITY)
      {
	marcel_bubble_t *b = ma_bubble_entity(e);
	
	if (b->hold.nr_ready > 2)
	  {
	    __ma_bubble_gather(b, b);
	    ret = 1;
	  }
      }
  
  return ret;
}

#endif /* MA__BUBBLES */
