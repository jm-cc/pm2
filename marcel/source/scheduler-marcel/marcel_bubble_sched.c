
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

marcel_bubble_t marcel_root_bubble = MARCEL_BUBBLE_INITIALIZER(marcel_root_bubble);

MA_DEFINE_PER_LWP(marcel_bubble_t *, bubble_towake, NULL);

int marcel_bubble_init(marcel_bubble_t *bubble) {
	PROF_EVENT1(bubble_sched_new,bubble);
	*bubble = (marcel_bubble_t) MARCEL_BUBBLE_INITIALIZER(*bubble);
	return 0;
}

int marcel_entity_setschedlevel(marcel_bubble_entity_t *entity, int level) {
#ifdef MA__LWPS
	if (level>marcel_topo_nblevels-1)
		level=marcel_topo_nblevels-1;
	entity->sched_level = level;
#endif
	return 0;
}
int marcel_entity_getschedlevel(__const marcel_bubble_entity_t *entity, int *level) {
#ifdef MA__LWPS
	*level = entity->sched_level;
#else
	*level = 0;
#endif
	return 0;
}

int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio) {
	PROF_EVENT2(bubble_sched_setprio,bubble,prio);
	bubble->sched.prio = prio;
	return 0;
}

int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio) {
	*prio = bubble->sched.prio;
	return 0;
}

static void __do_bubble_insertentity(marcel_bubble_t *bubble, marcel_bubble_entity_t *entity) {
	bubble_sched_debug("inserting %p in bubble %p\n",entity,bubble);
	if (entity->type == MARCEL_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_insert_bubble,tbx_container_of(entity,marcel_bubble_t,sched),bubble);
	else
		PROF_EVENT2(bubble_sched_insert_thread,tbx_container_of(entity,marcel_task_t,sched.internal),bubble);
	list_add_tail(&entity->entity_list, &bubble->heldentities);
	entity->holdingbubble=bubble;
#ifdef MARCEL_BUBBLE_STEAL
	list_add_tail(&entity->runningentities, &bubble->runningentities);
#endif
}

int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_bubble_entity_t *entity) {
#ifdef MARCEL_BUBBLE_EXPLODE
	ma_runqueue_t *rq = NULL;
#endif
	LOG_IN();

#ifdef MARCEL_BUBBLE_EXPLODE
	if (bubble->status != MA_BUBBLE_OPENED) {
retryclosed:
#endif
		ma_spin_lock_softirq(&bubble->lock);
#ifdef MARCEL_BUBBLE_EXPLODE
		if (bubble->status == MA_BUBBLE_OPENED) {
			ma_spin_unlock_softirq(&bubble->lock);
			goto retryopened;
		}
#endif
		__do_bubble_insertentity(bubble,entity);
		ma_spin_unlock_softirq(&bubble->lock);
#ifdef MARCEL_BUBBLE_EXPLODE
	} else {
retryopened:
		rq = bubble->sched.init_rq;
		ma_spin_lock_softirq(&rq->lock);
		_ma_raw_spin_lock(&bubble->lock);
		if (bubble->status != MA_BUBBLE_OPENED
				|| rq != bubble->sched.init_rq) {
			_ma_raw_spin_unlock(&bubble->lock);
			ma_spin_unlock_softirq(&rq->lock);
			goto retryclosed;
		}
		__do_bubble_insertentity(bubble,entity);
		/* containing bubble already has exploded ! wake up this one */
		entity->init_rq=rq;
		bubble->nbrunning++;
		activate_entity(entity, rq);
		_ma_raw_spin_unlock(&bubble->lock);
		ma_spin_unlock_softirq(&rq->lock);
	}
#endif
	LOG_OUT();
	return 0;
}

void marcel_wake_up_bubble(marcel_bubble_t *bubble) {
	ma_runqueue_t *rq;
	LOG_IN();
	if (!(rq = bubble->sched.init_rq))
		rq = bubble->sched.init_rq = &ma_main_runqueue;
	ma_spin_lock_softirq(&rq->lock);
	bubble_sched_debug("waking up bubble %p on rq %s\n",bubble,rq->name);
	PROF_EVENT2(bubble_sched_wake,bubble,rq);
	activate_entity(&bubble->sched, rq);
	ma_spin_unlock_softirq(&rq->lock);
	LOG_OUT();
}

#ifdef MARCEL_BUBBLE_EXPLODE
static void __marcel_close_bubble(marcel_bubble_t *bubble, ma_runqueue_t *rootrq) {
	marcel_bubble_entity_t *e;
	marcel_task_t *t;
	marcel_bubble_t *b;
	ma_runqueue_t *rq;

	PROF_EVENT1(bubble_sched_closing,bubble);
	bubble_sched_debug("bubble %p closing\n", bubble);
	bubble->status = MA_BUBBLE_CLOSING;
	list_for_each_entry(e, &bubble->heldentities, entity_list) {
		if (e->type == MARCEL_BUBBLE_ENTITY) {
			b = tbx_container_of(e, marcel_bubble_t, sched);
			ma_spin_lock(&b->lock);
			__marcel_close_bubble(b, rootrq);
			ma_spin_unlock(&b->lock);
		} else {
			t=tbx_container_of(e, marcel_task_t, sched.internal);
			if (t->sched.internal.cur_rq == rootrq)
				rq = rootrq;
			else
				rq = task_rq_lock(t);
			if (e->array) { /* not running */
				bubble_sched_debug("deactivating task %s(%p) from %s\n", t->name, t, rq->name);
				PROF_EVENT2(bubble_sched_goingback,e,bubble);
				deactivate_task(t,rq);
				bubble->nbrunning--;
			}
			if (rq != rootrq)
				task_rq_unlock(rq);
		}
	}
}

void marcel_close_bubble(marcel_bubble_t *bubble) {
	int wake_bubble;
	ma_runqueue_t *rq;
	LOG_IN();

	rq = entity_rq_lock(&bubble->sched);
	_ma_raw_spin_lock(&bubble->lock);
	PROF_EVENT2(bubble_sched_close,bubble,rq);
	__marcel_close_bubble(bubble, rq);
	if ((wake_bubble = (!bubble->nbrunning))) {
		bubble_sched_debug("last already off, bubble %p closed\n", bubble);
		bubble->status = MA_BUBBLE_CLOSED;
		activate_entity(&bubble->sched, rq);
	}
	_ma_raw_spin_unlock(&bubble->lock);
	ma_spin_unlock_softirq(&rq->lock);
	LOG_OUT();
}

static void __do_bubble_explode(marcel_bubble_t *bubble, ma_runqueue_t *rq) {
	marcel_bubble_entity_t *new;

	bubble_sched_debug("bubble %p exploding at %s\n", bubble, rq->name);
	PROF_EVENT1(bubble_sched_explode, bubble);
	bubble->status = MA_BUBBLE_OPENED;
	bubble->sched.init_rq = rq;
	list_for_each_entry(new, &bubble->heldentities, entity_list) {
		bubble_sched_debug("activating entity %p on %s\n", new, rq->name);
		new->init_rq=rq;
		activate_entity(new, rq);
		bubble->nbrunning++;
	}
}
#endif

void ma_bubble_tick(marcel_bubble_t *bubble) {
#ifdef MARCEL_BUBBLE_EXPLODE
	if (bubble->status != MA_BUBBLE_CLOSING)
		marcel_close_bubble(bubble);
#endif
}

#ifdef MARCEL_BUBBLE_STEAL
/* given an entity, find next running entity within the bubble */
static marcel_bubble_entity_t *ma_next_running_in_bubble(
		marcel_bubble_t *root,
		marcel_bubble_t *curb,
		struct list_head *curhead) {
	marcel_bubble_entity_t *next;
	marcel_bubble_t *b;
	struct list_head *nexthead;
	ma_runqueue_t *rq;

	nexthead = curhead->next;
	if (tbx_unlikely(nexthead == &curb->runningentities)) {
		/* Got at the end of the list of curb bubble, go up */
		bubble_sched_debug("got at end of curb, go up\n");
		if (curb == root)
			bubble_sched_debug("finished root\n");
			/* finished root bubble */
			return NULL;
		b = marcel_bubble_holding_bubble(curb);
		bubble_sched_debug("up to %p\n",b);
		return ma_next_running_in_bubble(root,b,&curb->sched.runningentities);
	}

	next = list_entry(nexthead, marcel_bubble_entity_t, runningentities);
	bubble_sched_debug("next %p\n",next);

	if (tbx_likely(next->type == MARCEL_TASK_ENTITY))
		return next;

	/* bubble, go down */
	b = tbx_container_of(next, marcel_bubble_t, sched);
	bubble_sched_debug("it is bubble %p\n",b);
	/* attention, il ne faudrait pas qu'elle bouge entre-temps */
	_ma_raw_spin_lock(&b->lock);
	rq = b->sched.cur_rq;
	_ma_raw_spin_unlock(&b->lock);
	if (rq) /* bubble on another rq (probably down elsewhere), don't go down */
		return ma_next_running_in_bubble(root,curb,&b->sched.runningentities);
	return ma_next_running_in_bubble(root,b,&b->heldentities);
}
#endif

marcel_bubble_entity_t *ma_bubble_sched(marcel_bubble_entity_t *nextent,
		ma_runqueue_t *prevrq, ma_runqueue_t *rq, int idx) {
	marcel_bubble_t *bubble;
#ifdef MARCEL_BUBBLE_STEAL
	marcel_bubble_entity_t *next_nextent;
#endif
	LOG_IN();

#ifdef MA__LWPS
	int max_prio;
	ma_runqueue_t *currq;

	/* sur smp, descendre l'entité si besoin est */
	if (nextent->sched_level > rq->level) {
		/* on peut relâcher la runqueue de la tâche courante */
		if (prevrq != rq)
			_ma_raw_spin_unlock(&prevrq->lock);

		/* s'assurer d'abord que personne n'a activé une entité d'une
		 * telle priorité (ou encore plus forte) avant nous */
		max_prio = idx;
		for (currq = ma_lwp_rq(LWP_SELF); currq != rq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx <= max_prio) {
				bubble_sched_debug("prio %d on lower rq %s in the meanwhile\n", idx, currq->name);
				ma_spin_unlock(&rq->lock);
				LOG_RETURN(NULL);
			}
		}
		for (; currq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx < max_prio) {
				bubble_sched_debug("better prio %d on rq %s in the meanwhile\n", idx, currq->name);
				ma_spin_unlock(&rq->lock);
				LOG_RETURN(NULL);
			}
		}

		deactivate_entity(nextent,rq);
		/* trouver une runqueue qui va bien */
		for (currq = ma_lwp_rq(LWP_SELF); currq &&
			nextent->sched_level < currq->level;
			currq = currq->father);

		MA_BUG_ON(!currq);
		PROF_EVENT2(bubble_sched_down,
			nextent->type == MARCEL_TASK_ENTITY?
			(void*)list_entry(nextent, marcel_task_t, sched):
			(void*)list_entry(nextent, marcel_bubble_t, sched),
			currq);
		bubble_sched_debug("entity %p going down from %s(%p) to %s(%p)\n", nextent, rq->name, rq, currq->name, currq);

		/* on descend, l'ordre des adresses est donc correct */
		lock_second_rq(rq, currq);
		activate_entity(nextent,currq);
		double_rq_unlock(rq, currq);
		LOG_RETURN(NULL);
	}
#endif

	if (nextent->type == MARCEL_TASK_ENTITY)
		LOG_RETURN(nextent);

/*
 * c'est une bulle
 */
	bubble = tbx_container_of(nextent, marcel_bubble_t, sched);

#if defined(MARCEL_BUBBLE_EXPLODE)
	/* relâche la queue de la tâche courante: on ne lui fera rien, à part
	 * squatter sa pile avant de recommencer ma_schedule() */
	if (prevrq!=rq)
		_ma_raw_spin_unlock(&prevrq->lock);

	/* maintenant on peut s'occuper de la bulle */
	/* l'enlever de la queue */
	deactivate_entity(nextent,rq);
	{
		_ma_raw_spin_lock(&bubble->lock);
		/* XXX: time_slice proportionnel au parallélisme de la runqueue */
/* ma_atomic_set est une macro et certaines versions de gcc n'aiment pas
   les #ifdef dans les arguments de macro...
 */
#ifdef MA__LWPS
#  define _TIME_SLICE MA_CPU_WEIGHT(&rq->cpuset)
#else
#  define _TIME_SLICE 1
#endif
		ma_atomic_set(&bubble->sched.time_slice, 10*_TIME_SLICE);
#undef _TIME_SLICE
		bubble_sched_debug("timeslice %u\n",ma_atomic_read(&bubble->sched.time_slice));
		__do_bubble_explode(bubble,rq);
		ma_atomic_set(&bubble->sched.time_slice,10*bubble->nbrunning); /* TODO: plutôt arbitraire */

		_ma_raw_spin_unlock(&bubble->lock);
		ma_spin_unlock(&rq->lock);
	}
	LOG_RETURN(NULL);

#elif defined(MARCEL_BUBBLE_STEAL)
	_ma_raw_spin_lock(&bubble->lock);
	/* Get next thread to run */
	if (!(nextent = bubble->next))
		/* None, restart from beginning */
		nextent = ma_next_running_in_bubble(bubble, bubble,
				&bubble->runningentities);
	bubble_sched_debug("next entity to run %p\n",nextent);
	/* and compute next after that */
	next_nextent = ma_next_running_in_bubble(bubble,
			marcel_bubble_holding_entity(nextent),
			&nextent->runningentities);
	bubble_sched_debug("next next ent to run %p\n",next_nextent);

	if (!(bubble->next = next_nextent)) {
		/* end of bubble, put at end of list */
		dequeue_entity(&bubble->sched,rq->active);
		enqueue_entity(&bubble->sched,rq->active);
	}

	if (!nextent) {
		/* TODO il faudrait virer la bulle de la runqueue */
		_ma_raw_spin_unlock(&bubble->lock);
		double_rq_unlock(prevrq,rq);
	}
	LOG_RETURN(nextent);
#else
#error Please choose a bubble algorithm
#endif
}

void __marcel_init bubble_sched_init() {
	PROF_EVENT1_ALWAYS(bubble_sched_new,&marcel_root_bubble);
	PROF_EVENT2_ALWAYS(bubble_sched_down,&marcel_root_bubble,&ma_main_runqueue);
	marcel_root_bubble.sched.init_rq=&ma_main_runqueue;
#ifdef MARCEL_BUBBLE_EXPLODE
	deactivate_running_task(MARCEL_SELF,&ma_main_runqueue);
	marcel_bubble_inserttask(&marcel_root_bubble, MARCEL_SELF);
	__do_bubble_explode(&marcel_root_bubble,&ma_main_runqueue);
	dequeue_task(MARCEL_SELF, MARCEL_SELF->sched.internal.array);
#endif
#ifdef MARCEL_BUBBLE_STEAL
	activate_entity(&marcel_root_bubble.sched, &ma_main_runqueue);
#endif
}

__ma_initfunc_prio(bubble_sched_init, MA_INIT_BUBBLE_SCHED,
		MA_INIT_BUBBLE_SCHED_PRIO, "Bubble Scheduler");
