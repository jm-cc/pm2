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

static void __sched_submit(marcel_entity_t *e[], int ne, struct marcel_topo_level **l)
{
  int i;

  marcel_bubble_deactivate_idle_scheduler();
  
  for (i = 0; i < ne; i++) 
    {	      
      int state = ma_get_entity(e[i]);
      ma_put_entity(e[i], &l[0]->sched.hold, state);
    }

  marcel_bubble_activate_idle_scheduler();
}

static int
explode_sched_init()
{
  return 0;
}

static int
explode_sched_exit()
{
  return 0;
}

static int
explode_sched_submit(marcel_entity_t *e)
{
  struct marcel_topo_level *l = marcel_topo_level(0,0);
  __sched_submit(&e, 1, &l);

  return 0;
}

static int
explode_sched_vp_is_idle(unsigned vp)
{
  /* Forcer une remontée ? */
  return 0;
}

static marcel_entity_t *
explode_sched_sched(marcel_entity_t *nextent, ma_runqueue_t *rq, ma_holder_t **nexth, int idx)
{
	int max_prio;
	ma_runqueue_t *currq;
	marcel_entity_t *e;
	/* sur smp, descendre l'entité si besoin est */
	if (/*ma_idle_scheduler &&*/ nextent->sched_level > rq->level) {
		/* s'assurer d'abord que personne n'a activé une entité d'une
		 * telle priorité (ou encore plus forte) avant nous */
		bubble_sched_debugl(7,"%p should go down\n", nextent);
		max_prio = idx;
		for (currq = ma_lwp_vprq(MA_LWP_SELF); currq != rq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx <= max_prio) {
				bubble_sched_debugl(7,"prio %d on lower rq %s in the meanwhile\n", idx, currq->name);
				sched_debug("unlock(%p)\n", rq);
				ma_holder_unlock(&rq->hold);
				return NULL;
			}
		}
		for (; currq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx < max_prio) {
				bubble_sched_debugl(7,"better prio %d on rq %s in the meanwhile\n", idx, currq->name);
				sched_debug("unlock(%p)\n", rq);
				ma_holder_unlock(&rq->hold);
				return NULL;
			}
		}

		ma_deactivate_entity(nextent,&rq->hold);

		/* nextent est RUNNING, on peut déverrouiller la runqueue,
		 * personne n'y touchera */
		sched_debug("unlock(%p)\n", rq);
		ma_holder_rawunlock(&rq->hold);

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
		bubble_sched_debug("entity %p going down from %s(%p) to %s(%p)\n", nextent, rq->name, rq, currq->name, currq);

		/* on descend, l'ordre des adresses est donc correct */
		ma_holder_rawlock(&currq->hold);
		nextent->sched_holder = &currq->hold;
		ma_activate_entity(nextent,&currq->hold);
		ma_holder_rawunlock(&currq->hold);
		if (rq != currq) {
			ma_preempt_enable();
			return NULL;
		}
	}

	if (nextent->type != MA_BUBBLE_ENTITY)
		LOG_RETURN(nextent);

	marcel_bubble_t *bubble = ma_bubble_entity(nextent);

	/* maintenant on peut s'occuper de la bulle */
	/* l'enlever de la queue */
	ma_dequeue_entity(&bubble->as_entity,&rq->hold);
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
	//ma_atomic_set(&bubble->sched.time_slice,MARCEL_BUBBLE_TIMESLICE*bubble->nbrunning); /* TODO: plutôt arbitraire */
	list_for_each_entry(e, &bubble->heldentities, bubble_entity_list)
		ma_move_entity(e, &rq->hold);

	ma_holder_rawunlock(&bubble->as_holder);
	sched_debug("unlock(%p)\n", rq);
	ma_holder_unlock(&rq->hold);
	return NULL;
}

static int
explode_sched_tick(marcel_bubble_t *b)
{
  /* TODO: gather bubble */
  return 0;
}

struct ma_bubble_sched_struct marcel_bubble_explode_sched = {
  .init = explode_sched_init,
  .exit = explode_sched_exit,
  .submit = explode_sched_submit,
  .vp_is_idle = explode_sched_vp_is_idle,
  .tick = explode_sched_tick,
  .sched = explode_sched_sched,
};

#endif /* MA__BUBBLES */

