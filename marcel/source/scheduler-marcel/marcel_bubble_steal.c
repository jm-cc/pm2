
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

struct marcel_bubble_steal_sched {
	marcel_bubble_sched_t scheduler;
};

static int total_nb_ready_entities(marcel_bubble_t *b) {
	int nb_ready_entities = b->as_holder.nb_ready_entities;
	marcel_entity_t *e;
	// XXX: pas fiable du tout ! Il faudrait verrouiller la bulle !
	ma_holder_rawlock(&b->as_holder);
	tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item)
		if (e->type == MA_BUBBLE_ENTITY)
			nb_ready_entities += total_nb_ready_entities(ma_bubble_entity(e));
	ma_holder_rawunlock(&b->as_holder);
	return nb_ready_entities;
}

static marcel_bubble_t *find_interesting_bubble(ma_runqueue_t *rq, int up_power) {
	int i, nbrun;
	// TODO: partir plutôt de la bulle "root" et descendre dedans ?
	// Le problème, c'est qu'en ne regardant que les bubbles dans les
	//runqueues, on loupe les grosses bulles qui ne contiennent pas de
	//thread prêt...
	marcel_entity_t *e;
	marcel_bubble_t *b;
	if (!rq->as_holder.nb_ready_entities)
		return NULL;
	for (i = 0; i < MA_MAX_PRIO; i++) {
		tbx_fast_list_for_each_entry_reverse(e, ma_array_queue(rq->active,i), cached_entities_item) {
			if (e->type != MA_BUBBLE_ENTITY)
				continue;
			b = ma_bubble_entity(e);
			if ((nbrun = total_nb_ready_entities(b)) >= up_power) {
				bubble_sched_debug("bubble %p has %d running, better for rq father %s with power %d\n", b, nbrun, rq->father->as_holder.name, up_power);
				return b;
			}
		}
	}
	return NULL;
}

/* TODO: add penality when crossing NUMA levels */
static int see(struct marcel_topo_level *level, int up_power) {
	/* have a look at work worth stealing from here */
	ma_runqueue_t *rq = &level->rq, *rq2;
	marcel_bubble_t *b, *b2 TBX_UNUSED;
	marcel_task_t *t;
	marcel_entity_t *e;
	int first = 1;
	int nbrun;
	int ret = 0;
	bubble_sched_debugl(7,"see %d %d\n", level->type, level->number);
	if (!rq)
		return 0;
	ma_holder_rawlock(&rq->as_holder);
	/* recommencer une fois verrouillé */
	b = find_interesting_bubble(rq, up_power);
	if (!b) {
		ma_holder_rawunlock(&rq->as_holder);
		bubble_sched_debugl(7,"no interesting bubble\n");
		return 0;
	}
	// XXX: calcul dupliqué ici
	nbrun = total_nb_ready_entities(b);
	for (rq2 = rq;
			/* emmener juste assez haut pour moi */
			!marcel_vpset_isset(&rq2->vpset, ma_vpnum(MA_LWP_SELF))
			&& rq2->father
			/* et juste assez haut par rapport au nombre de threads */
			&& marcel_vpset_weight(&rq2->father->vpset) <= nbrun
			; rq2 = rq2->father)
		bubble_sched_debugl(7,"looking up to rq %s\n", rq2->father->as_holder.name);
	if (!rq2 ||
			/* pas intéressant pour moi, on laisse les autres se débrouiller */
			/* todo: le faire quand même ? */
			!marcel_vpset_isset(&rq2->vpset,ma_vpnum(MA_LWP_SELF))) {
		bubble_sched_debug("%s doesn't suit for me and %d threads\n",rq2?rq2->as_holder.name:"anything",nbrun);
		ma_holder_rawunlock(&rq->as_holder);
		return 0;
	}

	bubble_sched_debug("rq %s seems good, stealing bubble %p\n", rq2->as_holder.name, b);
	ret = 1;
	PROF_EVENT2(bubble_sched_switchrq, b, rq2);
	/* laisser d'abord ce qui est ordonnancé sur place */
	ma_holder_rawlock(&b->as_holder);
	tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
		if (e->type == MA_BUBBLE_ENTITY) {
			/* laisser la première bulle */
			if (!first)
				continue;
			first = 0;
			b2 = ma_bubble_entity(e);
			e->settled = 1;
			bubble_sched_debug("detaching running bubble %p from %p\n", b2, b);
		} else {
			t = ma_task_entity(e);
			/* ne pas toucher à ce qui tourne actuellement */
			if (!MA_TASK_IS_RUNNING(t))
				continue;
			bubble_sched_debug("detaching running task %p from %p\n", t, b);
		}
		ma_move_entity(e, &rq->as_holder);
	}
	ma_holder_rawunlock(&b->as_holder);
	/* enlever b de rq */
	int bstate = ma_get_entity(&b->as_entity);
	ma_holder_rawunlock(&rq->as_holder);
	ma_holder_rawlock(&rq2->as_holder);
	ma_holder_rawlock(&b->as_holder);
	/* b contient encore tout ce qui n'est pas ordonnancé, les distribuer */
	tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
		if (e->sched_holder == &b->as_holder)
			continue;
		/* encore dans la bulle, faire sortir cela */
		/* TODO: prendre tout de suite du boulot pour soi ? */
		if (e->type == MA_BUBBLE_ENTITY) {
			b2 = ma_bubble_entity(e);
			e->settled = 1;
			bubble_sched_debug("detaching bubble %p from %p\n", b2, b);
		} else {
			t = ma_task_entity(e);
			bubble_sched_debug("detaching task %p from %p\n", t, b);
		}
		ma_move_entity(e, &rq2->as_holder);
		/* et forcer à redescendre au même niveau qu'avant */
		e->sched_level = rq->level;
	}
	ma_holder_rawunlock(&b->as_holder);
	/* mettre b sur rq2 */
	ma_put_entity(&b->as_entity, &rq2->as_holder, bstate);
	b->as_entity.sched_level = rq2->level;
	ma_holder_rawunlock(&rq2->as_holder);

	return ret;
}
static int see_down(struct marcel_topo_level *level, 
		struct marcel_topo_level *me) {
	ma_runqueue_t *rq = &level->rq;
	int power = 0;
	int i = 0, n = level->arity;
	bubble_sched_debugl(7,"see_down from %d %d\n", level->type, level->number);
	if (rq) {
		power = marcel_vpset_weight(&rq->vpset);
	}
	if (me) {
		/* si l'appelant fait partie des fils, l'éviter */
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

	/* puis chez le père */
	if (see_up(father))
	  return 1;
	  
	bubble_sched_debugl(7,"up done\n");
	return 0;
}


/** \brief Steal work
 *
 * This function may typically be called when a processor becomes idle. It will
 * look for bubbles to steal first locally (neighbour processors), then more
 * and more globally.
 *
 * \return 1 if it managed to steal work, 0 else.
 */
static int steal_work(marcel_bubble_sched_t *self, unsigned vp TBX_UNUSED) {
#ifdef MA__LWPS
	ma_spin_lock(&ma_idle_scheduler_lock);
	if (ma_atomic_read (&ma_idle_scheduler)) {
	  struct marcel_topo_level *me =
	    &marcel_topo_vp_level[marcel_current_vp()];
	  bubble_sched_debugl(7,"bubble steal on %d\n", ma_vpnum(MA_LWP_SELF));
	  /* couln't find work on local runqueue, go see elsewhere */
	  ma_spin_unlock(&ma_idle_scheduler_lock);
	  return see_up(me);
	}
	ma_spin_unlock(&ma_idle_scheduler_lock);
#endif
	return 0;
}

static marcel_entity_t *
steal_sched_sched(marcel_bubble_sched_t *self, marcel_entity_t *nextent, ma_runqueue_t *rq, ma_holder_t **nexth, int idx) {
	if (nextent->type != MA_BUBBLE_ENTITY)
		/* Don't touch tasks */
		return nextent;

	marcel_bubble_t *bubble = ma_bubble_entity(nextent);

	/* Fresh bubble, put it near us. */
	if (ma_idle_scheduler_is_running () && !nextent->settled) {
		ma_runqueue_t *rq2 = ma_lwp_vprq(MA_LWP_SELF);
		nextent->settled = 1;
		if (rq != rq2) {
			bubble_sched_debug("settling bubble %p on rq %s\n", bubble, rq2->as_holder.name);
			ma_dequeue_entity(&bubble->as_entity, &rq->as_holder);
			ma_clear_ready_holder(&bubble->as_entity, &rq->as_holder);
			ma_holder_rawunlock(&rq->as_holder);
			ma_holder_rawunlock(&bubble->as_holder);

			ma_holder_rawlock(&rq2->as_holder);
			PROF_EVENT2(bubble_sched_switchrq, bubble, rq2);
			ma_holder_rawlock(&bubble->as_holder);
			ma_set_ready_holder(&bubble->as_entity,&rq2->as_holder);
			ma_enqueue_entity(&bubble->as_entity,&rq2->as_holder);
			ma_holder_rawunlock(&rq2->as_holder);
			ma_holder_unlock(&bubble->as_holder);
			return NULL;
		}
	}
	return nextent;
}

static int
steal_sched_start(marcel_bubble_sched_t *self)
{
	ma_activate_idle_scheduler();
	return 0;
}


static int
make_default_scheduler (marcel_bubble_sched_t *scheduler) {
  scheduler->klass = &marcel_bubble_steal_sched_class;
	scheduler->start = steal_sched_start;
	scheduler->sched = steal_sched_sched;
	scheduler->vp_is_idle = steal_work;
	return 0;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS (steal, make_default_scheduler);

#endif
