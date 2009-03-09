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

#ifdef MA__BUBBLES

struct marcel_bubble_explode_sched {
	marcel_bubble_sched_t scheduler;
};

static void __sched_submit(marcel_entity_t *e[], int ne, struct marcel_topo_level **l)
{
  int i;

  ma_deactivate_idle_scheduler();
  
  for (i = 0; i < ne; i++) 
    {	      
      int state = ma_get_entity(e[i]);
      ma_put_entity(e[i], &l[0]->rq.as_holder, state);
    }

  ma_activate_idle_scheduler();
}

static int
explode_sched_init(marcel_bubble_sched_t *self)
{
  return 0;
}

static int
explode_sched_exit(marcel_bubble_sched_t *self)
{
  return 0;
}

static int
explode_sched_submit(marcel_bubble_sched_t *self, marcel_entity_t *e)
{
  struct marcel_topo_level *l = marcel_topo_level(0,0);
  __sched_submit(&e, 1, &l);

  return 0;
}

static int
explode_sched_vp_is_idle(marcel_bubble_sched_t *self, unsigned vp TBX_UNUSED)
{
  /* Forcer une remontée ? */
  return 0;
}

static marcel_entity_t *
explode_sched_sched(marcel_bubble_sched_t *self, marcel_entity_t *nextent, ma_runqueue_t *rq, ma_holder_t **nexth, int idx)
{
	int max_prio;
	ma_runqueue_t *currq;
	marcel_entity_t *e;
	marcel_bubble_t *bubble;
	/* sur smp, descendre l'entité si besoin est */
	if (/*ma_idle_scheduler &&*/ nextent->sched_level > rq->level) {
		/* s'assurer d'abord que personne n'a activé une entité d'une
		 * telle priorité (ou encore plus forte) avant nous */
		bubble_sched_debugl(7,"%p should go down\n", nextent);
		max_prio = idx;
		for (currq = ma_lwp_vprq(MA_LWP_SELF); currq != rq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx <= max_prio) {
				bubble_sched_debugl(7,"prio %d on lower rq %s in the meanwhile\n", idx, currq->as_holder.name);
				sched_debug("unlock(%p)\n", rq);
				ma_holder_unlock(&rq->as_holder);
				return NULL;
			}
		}
		for (; currq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx < max_prio) {
				bubble_sched_debugl(7,"better prio %d on rq %s in the meanwhile\n", idx, currq->as_holder.name);
				sched_debug("unlock(%p)\n", rq);
				ma_holder_unlock(&rq->as_holder);
				return NULL;
			}
		}

		ma_dequeue_entity(nextent,&rq->as_holder);
		ma_clear_ready_holder(nextent,&rq->as_holder);

		/* nextent est RUNNING, on peut déverrouiller la runqueue,
		 * personne n'y touchera */
		sched_debug("unlock(%p)\n", rq);
		ma_holder_rawunlock(&rq->as_holder);

		/* trouver une runqueue qui va bien */
		for (currq = ma_lwp_vprq(MA_LWP_SELF); currq &&
			nextent->sched_level < currq->level;
			currq = currq->father);

		MA_BUG_ON(!currq);
		PROF_EVENT2(bubble_sched_switchrq,
			nextent->type == MA_BUBBLE_ENTITY?
			(void*)ma_bubble_entity(nextent):
			(void*)ma_task_entity(nextent),
			currq);
		bubble_sched_debug("entity %p going down from %s(%p) to %s(%p)\n", nextent, rq->as_holder.name, rq, currq->as_holder.name, currq);

		/* on descend, l'ordre des adresses est donc correct */
		ma_holder_rawlock(&currq->as_holder);
		nextent->sched_holder = &currq->as_holder;
		ma_set_ready_holder(nextent,&currq->as_holder);
		ma_enqueue_entity(nextent,&currq->as_holder);
		ma_holder_rawunlock(&currq->as_holder);
		if (rq != currq) {
			ma_preempt_enable();
			return NULL;
		}
	}

	if (nextent->type != MA_BUBBLE_ENTITY)
		LOG_RETURN(nextent);

	bubble = ma_bubble_entity(nextent);

	/* maintenant on peut s'occuper de la bulle */
	/* l'enlever de la queue */
	ma_dequeue_entity(&bubble->as_entity,&rq->as_holder);
	ma_holder_rawlock(&bubble->as_holder);
	/* XXX: time_slice proportionnel au parallélisme de la runqueue */
/* ma_atomic_set est une macro et certaines versions de gcc n'aiment pas
les #ifdef dans les arguments de macro...
*/
#ifdef MA__LWPS
#  define _TIME_SLICE marcel_vpset_weight(&rq->vpset)
#else
#  define _TIME_SLICE 1
#endif
	ma_atomic_set(&bubble->as_entity.time_slice, MARCEL_BUBBLE_TIMESLICE*_TIME_SLICE);
#undef _TIME_SLICE
	bubble_sched_debugl(7,"timeslice %u\n",ma_atomic_read(&bubble->as_entity.time_slice));
	//__do_bubble_explode(bubble,rq);
	//ma_atomic_set(&bubble->as_entity.time_slice,MARCEL_BUBBLE_TIMESLICE*bubble->nbrunning); /* TODO: plutôt arbitraire */
	list_for_each_entry(e, &bubble->natural_entities, natural_entities_item)
		ma_move_entity(e, &rq->as_holder);

	ma_holder_rawunlock(&bubble->as_holder);
	sched_debug("unlock(%p)\n", rq);
	ma_holder_unlock(&rq->as_holder);
	return NULL;
}

static int
explode_sched_tick(marcel_bubble_sched_t *self, marcel_bubble_t *b TBX_UNUSED)
{
  /* TODO: gather bubble */
  return 0;
}


static int
make_default_scheduler(marcel_bubble_sched_t *scheduler) {
  scheduler->klass = &marcel_bubble_explode_sched_class;

  scheduler->init = explode_sched_init;
  scheduler->exit = explode_sched_exit;
  scheduler->submit = explode_sched_submit;
  scheduler->vp_is_idle = explode_sched_vp_is_idle;
  scheduler->tick = explode_sched_tick;
  scheduler->sched = explode_sched_sched;

	return 0;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS (explode, make_default_scheduler);

#endif /* MA__BUBBLES */

