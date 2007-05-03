
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

/******************************************************************************
 *
 * Vol de bulle local.
 *
 */

#ifdef MA__BUBBLES

static int total_nr_ready(marcel_bubble_t *b) {
	int nr_ready = b->hold.nr_ready;
	marcel_entity_t *e;
	// XXX: pas fiable du tout ! Il faudrait verrouiller la bulle !
	ma_holder_rawlock(&b->hold);
	list_for_each_entry(e, &b->heldentities, bubble_entity_list)
		if (e->type == MA_BUBBLE_ENTITY)
			nr_ready += total_nr_ready(ma_bubble_entity(e));
	ma_holder_rawunlock(&b->hold);
	return nr_ready;
}

static marcel_bubble_t *find_interesting_bubble(ma_runqueue_t *rq, int up_power) {
	int i, nbrun;
	// TODO: partir plut�t de la bulle "root" et descendre dedans ?
	// Le probl�me, c'est qu'en ne regardant que les bubbles dans les
	//runqueues, on loupe les grosses bulles qui ne contiennent pas de
	//thread pr�t...
	marcel_entity_t *e;
	marcel_bubble_t *b;
	if (!rq->hold.nr_ready)
		return NULL;
	for (i = 0; i < MA_MAX_PRIO; i++) {
		list_for_each_entry_reverse(e, ma_array_queue(rq->active,i), run_list) {
#if 0
			else if (!list_empty(ma_array_queue(rq->expired, i)))
				e = list_entry(ma_array_queue(rq->expired,i)->prev, marcel_entity_t, run_list);
	#endif
			if (e->type == MA_TASK_ENTITY)
				continue;
			b = ma_bubble_entity(e);
			if ((nbrun = total_nr_ready(b)) >= up_power) {
			//if (b->hold.nr_ready >= up_power) {
				bubble_sched_debug("bubble %p has %d running, better for rq father %s with power %d\n", b, nbrun, rq->father->name, up_power);
				return b;
			}
		}
	}
	return NULL;
}

/* TODO: add penality when crossing NUMA levels */
static int see(struct marcel_topo_level *level, int up_power) {
	/* have a look at work worth stealing from here */
	ma_runqueue_t *rq = &level->sched, *rq2;
	marcel_bubble_t *b, *b2;
	marcel_task_t *t;
	marcel_entity_t *e;
	int first = 1;
	int nbrun;
	int ret = 0;
	bubble_sched_debugl(7,"see %d %d\n", level->type, level->number);
	if (!rq)
		return 0;
	ma_preempt_disable();
	//if (find_interesting_bubble(rq, up_power, 0)) {
		ma_holder_rawlock(&rq->hold);
		/* recommencer une fois verrouill� */
		b = find_interesting_bubble(rq, up_power);
		if (!b) {
			ma_holder_rawunlock(&rq->hold);
			bubble_sched_debugl(7,"no interesting bubble\n");
		} else {
			// XXX: calcul dupliqu� ici
			nbrun = total_nr_ready(b);
			for (rq2 = rq;
				/* emmener juste assez haut pour moi */
				  !marcel_vpmask_vp_ismember(&rq2->vpset, LWP_NUMBER(LWP_SELF))
				&& rq2->father
				/* et juste assez haut par rapport au nombre de threads */
				&& marcel_vpmask_weight(&rq2->father->vpset) <= nbrun
				; rq2 = rq2->father)
				bubble_sched_debugl(7,"looking up to rq %s\n", rq2->father->name);
			if (!rq2 ||
					/* pas int�ressant pour moi, on laisse les autres se d�brouiller */
					/* todo: le faire quand m�me ? */
					!marcel_vpmask_vp_ismember(&rq2->vpset,LWP_NUMBER(LWP_SELF))) {
				bubble_sched_debug("%s doesn't suit for me and %d threads\n",rq2?rq2->name:"anything",nbrun);
				ma_holder_rawunlock(&rq->hold);
			} else {
				bubble_sched_debug("rq %s seems good, stealing bubble %p\n", rq2->name, b);
				ret = 1;
				PROF_EVENT2(bubble_sched_switchrq, b, rq2);
				/* laisser d'abord ce qui est ordonnanc� sur place */
				ma_holder_rawlock(&b->hold);
				list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
					b2 = NULL;
					if (e->type == MA_BUBBLE_ENTITY)
						b2 = ma_bubble_entity(e);
					else
						t = ma_task_entity(e);
					if ((b2 &&
							 /* ne pas toucher � ce qui tourne actuellement */
							(//b2->hold.nr_scheduled ||
							 /* laisser la premi�re bulle */
							 first)
							 /* ne pas toucher � ce qui tourne actuellement */
							) || (!b2 && MA_TASK_IS_RUNNING(t))) {
						int state;
						if (b2) {
							first = 0;
							bubble_sched_debug("detaching running bubble %p from %p\n", b2, b);
						} else
							bubble_sched_debug("detaching running task %p from %p\n", t, b);

						state = ma_get_entity(e);
						ma_put_entity(e, &rq->hold, state);
						if (b2)
							b2->settled = 1;
					}
				}
				ma_holder_rawunlock(&b->hold);
				/* enlever b de rq */
				int bstate = ma_get_entity(&b->sched);
				ma_holder_rawunlock(&rq->hold);
				ma_holder_rawlock(&rq2->hold);
				ma_holder_rawlock(&b->hold);
				/* b contient encore tout ce qui n'est pas ordonnanc�, les distribuer */
				list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
					b2 = NULL;
					if (e->type == MA_BUBBLE_ENTITY)
						b2 = ma_bubble_entity(e);
					else
						t = ma_task_entity(e);
					if (e->sched_holder == &b->hold) {
						int state;
						/* encore dans la bulle, faire sortir cela */
						/* TODO: prendre tout de suite du boulot pour soi ? */
						if (b2)
							bubble_sched_debug("detaching bubble %p from %p\n", b2, b);
						else
							bubble_sched_debug("detaching task %p from %p\n", t, b);
						state = ma_get_entity(e);
						ma_put_entity(e, &rq2->hold, state);
						if (b2)
							b2->settled = 1;
						/* et forcer � redescendre au m�me niveau qu'avant */
						e->sched_level = rq->level;
					}
				}
				ma_holder_rawunlock(&b->hold);
				/* mettre b sur rq2 */
				ma_put_entity(&b->sched, &rq2->hold, bstate);
				b->sched.sched_level = rq2->level;
				ma_holder_rawunlock(&rq2->hold);
			}
		}
	//}
	ma_preempt_enable();
	return ret;
}
static int see_down(struct marcel_topo_level *level, 
		struct marcel_topo_level *me) {
	ma_runqueue_t *rq = &level->sched;
	int power = 0;
	int i = 0, n = level->arity;
	bubble_sched_debugl(7,"see_down from %d %d\n", level->type, level->number);
	if (rq) {
		power = marcel_vpmask_weight(&rq->vpset);
	}
	if (me) {
		/* si l'appelant fait partie des fils, l'�viter */
		n--;
		i = (me->index + 1) % level->arity;
	}
	while (n--) {
		/* examiner les fils */
		if (see(level->children[i], power) || see_down(level->children[i], NULL))
			return 1;
		i = (i+1) % level->arity;
	}
	bubble_sched_debugl(7,"down done\n");
	return 0;
}

static int see_up(struct marcel_topo_level *level) {
	struct marcel_topo_level *father = level->father;
	bubble_sched_debugl(7,"see_up from %d %d\n", level->type, level->number);
	if (!father)
		return 0;
	/* regarder vers le haut, c'est d'abord regarder en bas */
	if (see_down(father, level))
	  return 1;

	/* puis chez le p�re */
	if (see_up(father))
	  return 1;
	  
	bubble_sched_debugl(7,"up done\n");
	return 0;
}


int marcel_bubble_steal_work(unsigned vp) {
#ifdef MA__LWPS
	ma_read_lock(&ma_idle_scheduler_lock);
	if (ma_idle_scheduler) {
	  struct marcel_topo_level *me =
	    &marcel_topo_vp_level[marcel_current_vp()];
	  bubble_sched_debugl(7,"bubble steal on %d\n", LWP_NUMBER(LWP_SELF));
	  /* couln't find work on local runqueue, go see elsewhere */
	  ma_read_unlock(&ma_idle_scheduler_lock);
	  return see_up(me);
	}
	ma_read_unlock(&ma_idle_scheduler_lock);
#endif
	return 0;
}

static marcel_entity_t *
steal_sched_sched(marcel_entity_t *nextent, ma_runqueue_t *rq, ma_holder_t **nexth, int idx) {
	if (nextent->type == MA_TASK_ENTITY)
		/* Don't touch tasks */
		return nextent;

	marcel_bubble_t *bubble = ma_bubble_entity(nextent);

	/* Fresh bubble, put it near us. */
	if (ma_idle_scheduler && !bubble->settled) {
		ma_runqueue_t *rq2 = ma_lwp_vprq(LWP_SELF);
		bubble->settled = 1;
		if (rq != rq2) {
			bubble_sched_debug("settling bubble %p on rq %s\n", bubble, rq2->name);
			ma_deactivate_entity(&bubble->sched, &rq->hold);
			ma_holder_rawunlock(&rq->hold);
			ma_holder_rawunlock(&bubble->hold);

			ma_holder_rawlock(&rq2->hold);
			PROF_EVENT2(bubble_sched_switchrq, bubble, rq2);
			ma_holder_rawlock(&bubble->hold);
			ma_activate_entity(&bubble->sched, &rq2->hold);
			ma_holder_rawunlock(&rq2->hold);
			ma_holder_unlock(&bubble->hold);
			return NULL;
		}
	}
	return nextent;
}

static int
steal_sched_start()
{
	marcel_bubble_activate_idle_scheduler();
	return 0;
}

struct ma_bubble_sched_struct marcel_bubble_steal_sched = {
	.start = steal_sched_start,
	.sched = steal_sched_sched,
	.vp_is_idle = marcel_bubble_steal_work,
};

#endif
