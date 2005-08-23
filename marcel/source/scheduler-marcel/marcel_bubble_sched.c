
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

int marcel_entity_setschedlevel(marcel_entity_t *entity, int level) {
#ifdef MA__LWPS
	if (level>marcel_topo_nblevels-1)
		level=marcel_topo_nblevels-1;
	entity->sched_level = level;
#endif
	return 0;
}
int marcel_entity_getschedlevel(__const marcel_entity_t *entity, int *level) {
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

marcel_bubble_t *marcel_bubble_holding_entity(marcel_entity_t *e) {
	MA_BUG_ON(ma_holder_type(e->holder) != MA_BUBBLE_HOLDER);
	return ma_bubble_holder(e->holder);
}

static void __do_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	bubble_sched_debug("inserting %p in bubble %p\n",entity,bubble);
	if (entity->type == MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_insert_bubble,tbx_container_of(entity,marcel_bubble_t,sched),bubble);
	else {
		marcel_t t = tbx_container_of(entity, marcel_task_t, sched.internal);
		if (t->sched.state == MA_TASK_BORNING)
			ma_set_task_state(t, MA_TASK_RUNNING);
		PROF_EVENT2(bubble_sched_insert_thread,tbx_container_of(entity,marcel_task_t,sched.internal),bubble);
	}
	list_add_tail(&entity->entity_list, &bubble->heldentities);
	entity->holder=&bubble->hold;
#ifdef MARCEL_BUBBLE_STEAL
	list_add_tail(&entity->run_list, &bubble->runningentities);
#endif
}

int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
#ifdef MARCEL_BUBBLE_EXPLODE
	ma_runqueue_t *rq = NULL;
#endif
	LOG_IN();

#ifdef MARCEL_BUBBLE_EXPLODE
	if (bubble->status != MA_BUBBLE_OPENED) {
retryclosed:
#endif
		ma_holder_lock_softirq(&bubble->hold);
#ifdef MARCEL_BUBBLE_EXPLODE
		if (bubble->status == MA_BUBBLE_OPENED) {
			ma_holder_unlock_softirq(&bubble->hold);
			goto retryopened;
		}
#endif
		__do_bubble_insertentity(bubble,entity);
		ma_holder_unlock_softirq(&bubble->hold);
#ifdef MARCEL_BUBBLE_EXPLODE
	} else {
retryopened:
		rq = bubble->sched.init_rq;
		ma_holder_lock_softirq(&rq->hold);
		ma_holder_rawlock(&bubble->hold);
		if (bubble->status != MA_BUBBLE_OPENED
				|| rq != bubble->sched.init_rq) {
			ma_holder_rawunlock(&bubble->hold);
			ma_holder_unlock_softirq(&rq->hold);
			goto retryclosed;
		}
		__do_bubble_insertentity(bubble,entity);
		/* containing bubble already has exploded ! wake up this one */
		entity->init_rq=rq;
		bubble->nbrunning++;
		if (entity->type == MA_BUBBLE_ENTITY)
			ma_activate_entity(entity, &rq->hold);
		ma_holder_rawunlock(&bubble->hold);
		ma_holder_unlock_softirq(&rq->hold);
	}
#endif
	LOG_OUT();
	return 0;
}

void marcel_wake_up_bubble(marcel_bubble_t *bubble) {
	ma_runqueue_t *rq;
	LOG_IN();
	if (!(bubble->sched.holder)) {
		rq = &ma_main_runqueue;
		bubble->sched.holder = &rq->hold;
	} else {
		MA_BUG_ON(!ma_holder_type(bubble->sched.holder) == MA_RUNQUEUE_HOLDER);
		rq = ma_rq_holder(bubble->sched.holder);
	}
	ma_holder_lock_softirq(&rq->hold);
	bubble_sched_debug("waking up bubble %p on rq %s\n",bubble,rq->name);
	PROF_EVENT2(bubble_sched_wake,bubble,rq);
	ma_activate_entity(&bubble->sched, &rq->hold);
	ma_holder_unlock_softirq(&rq->hold);
	LOG_OUT();
}

#ifdef MARCEL_BUBBLE_EXPLODE
static void __marcel_close_bubble(marcel_bubble_t *bubble, ma_runqueue_t *rootrq) {
	marcel_entity_t *e;
	marcel_task_t *t;
	marcel_bubble_t *b;
	ma_runqueue_t *rq;

	PROF_EVENT1(bubble_sched_closing,bubble);
	bubble_sched_debug("bubble %p closing\n", bubble);
	bubble->status = MA_BUBBLE_CLOSING;
	list_for_each_entry(e, &bubble->heldentities, entity_list) {
		if (e->type == MA_BUBBLE_ENTITY) {
			b = tbx_container_of(e, marcel_bubble_t, sched);
			ma_holder_lock(&b->hold);
			__marcel_close_bubble(b, rootrq);
			ma_holder_unlock(&b->hold);
		} else {
			t=tbx_container_of(e, marcel_task_t, sched.internal);
			if (t->sched.internal.cur_holder == &rootrq.hold)
				rq = rootrq;
			else
				rq = task_rq_lock(t);
			if (e->array) { /* not running */
				bubble_sched_debug("deactivating task %s(%p) from %s\n", t->name, t, rq->name);
				PROF_EVENT2(bubble_sched_goingback,e,bubble);
				ma_deactivate_task(t,&rq->hold);
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
	ma_holder_rawlock(&bubble->hold);
	PROF_EVENT2(bubble_sched_close,bubble,rq);
	__marcel_close_bubble(bubble, rq);
	if ((wake_bubble = (!bubble->nbrunning))) {
		bubble_sched_debug("last already off, bubble %p closed\n", bubble);
		bubble->status = MA_BUBBLE_CLOSED;
		ma_activate_entity(&bubble->sched, &rq->hold);
	}
	ma_holder_rawunlock(&bubble->hold);
	ma_holder_unlock_softirq(&rq->hold);
	LOG_OUT();
}

static void __do_bubble_explode(marcel_bubble_t *bubble, ma_runqueue_t *rq) {
	marcel_entity_t *new;

	bubble_sched_debug("bubble %p exploding at %s\n", bubble, rq->name);
	PROF_EVENT1(bubble_sched_explode, bubble);
	bubble->status = MA_BUBBLE_OPENED;
	bubble->sched.init_rq = rq;
	list_for_each_entry(new, &bubble->heldentities, entity_list) {
		bubble_sched_debug("activating entity %p on %s\n", new, rq->name);
		new->init_rq=rq;
		if (new->type == MA_BUBBLE_ENTITY || tbx_container_of(new, marcel_task_t, sched.internal)->sched.state == MA_TASK_RUNNING)
			ma_activate_entity(new, &rq->hold));
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
static marcel_entity_t *ma_next_running_in_bubble(
		marcel_bubble_t *root,
		marcel_bubble_t *curb,
		struct list_head *curhead) {
	marcel_entity_t *next;
	marcel_bubble_t *b;
	struct list_head *nexthead;
	enum marcel_holder h;

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
		return ma_next_running_in_bubble(root,b,&curb->sched.run_list);
	}

	next = list_entry(nexthead, marcel_entity_t, run_list);
	bubble_sched_debug("next %p\n",next);

	if (tbx_likely(next->type == MA_TASK_ENTITY))
		return next;

	/* bubble, go down */
	b = tbx_container_of(next, marcel_bubble_t, sched);
	bubble_sched_debug("it is bubble %p\n",b);
	/* attention, il ne faudrait pas qu'elle bouge entre-temps */
	ma_holder_rawlock(&b->hold);
	h = ma_holder_type(b->sched.cur_holder);
	ma_holder_rawunlock(&b->hold);
	if (h == MA_RUNQUEUE_HOLDER) /* bubble on another rq (probably down elsewhere), don't go down */
		return ma_next_running_in_bubble(root,curb,&b->sched.run_list);
	return ma_next_running_in_bubble(root,b,&b->heldentities);
}
#endif

marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_runqueue_t *prevrq, ma_runqueue_t *rq, int idx) {
	marcel_bubble_t *bubble;
#ifdef MARCEL_BUBBLE_STEAL
	marcel_entity_t *next_nextent;
#endif
	LOG_IN();

#ifdef MA__LWPS
	int max_prio;
	ma_runqueue_t *currq;

	/* sur smp, descendre l'entité si besoin est */
	if (nextent->sched_level > rq->level) {
		/* on peut relâcher la runqueue de la tâche courante */
		if (prevrq != rq)
			ma_holder_rawunlock(&prevrq->hold);

		/* s'assurer d'abord que personne n'a activé une entité d'une
		 * telle priorité (ou encore plus forte) avant nous */
		max_prio = idx;
		for (currq = ma_lwp_rq(LWP_SELF); currq != rq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx <= max_prio) {
				bubble_sched_debug("prio %d on lower rq %s in the meanwhile\n", idx, currq->name);
				ma_holder_unlock(&rq->hold);
				LOG_RETURN(NULL);
			}
		}
		for (; currq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx < max_prio) {
				bubble_sched_debug("better prio %d on rq %s in the meanwhile\n", idx, currq->name);
				ma_holder_unlock(&rq->hold);
				LOG_RETURN(NULL);
			}
		}

		ma_deactivate_entity(nextent,&rq->hold);
		/* trouver une runqueue qui va bien */
		for (currq = ma_lwp_rq(LWP_SELF); currq &&
			nextent->sched_level < currq->level;
			currq = currq->father);

		MA_BUG_ON(!currq);
		PROF_EVENT2(bubble_sched_down,
			nextent->type == MA_TASK_ENTITY?
			(void*)list_entry(nextent, marcel_task_t, sched):
			(void*)list_entry(nextent, marcel_bubble_t, sched),
			currq);
		bubble_sched_debug("entity %p going down from %s(%p) to %s(%p)\n", nextent, rq->name, rq, currq->name, currq);

		/* on descend, l'ordre des adresses est donc correct */
		lock_second_rq(rq, currq);
		ma_activate_entity(nextent,&currq->hold);
		double_rq_unlock(rq, currq);
		LOG_RETURN(NULL);
	}
#endif

	if (nextent->type == MA_TASK_ENTITY)
		LOG_RETURN(nextent);

/*
 * c'est une bulle
 */
	bubble = tbx_container_of(nextent, marcel_bubble_t, sched);

#if defined(MARCEL_BUBBLE_EXPLODE)
	/* relâche la queue de la tâche courante: on ne lui fera rien, à part
	 * squatter sa pile avant de recommencer ma_schedule() */
	if (prevrq!=rq)
		ma_holder_rawunlock(&prevrq->lock);

	/* maintenant on peut s'occuper de la bulle */
	/* l'enlever de la queue */
	ma_deactivate_entity(nextent,&rq->hold);
	{
		ma_holder_rawlock(&bubble->hold);
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

		ma_holder_rawunlock(&bubble->hold);
		ma_holder_unlock(&rq->hold);
	}
	LOG_RETURN(NULL);

#elif defined(MARCEL_BUBBLE_STEAL)
	ma_holder_rawlock(&bubble->hold);
	/* Get next thread to run */
	if (!(nextent = bubble->next))
		/* None, restart from beginning */
		nextent = ma_next_running_in_bubble(bubble, bubble,
				&bubble->runningentities);
	if (!nextent)
		/* nothing in bubble, skip */
		goto out;

	bubble_sched_debug("next entity to run %p\n",nextent);
	/* and compute next after that */
	next_nextent = ma_next_running_in_bubble(bubble,
			marcel_bubble_holding_entity(nextent),
			&nextent->run_list);
	bubble_sched_debug("next next ent to run %p\n",next_nextent);

	if (!(bubble->next = next_nextent)) {
out:
		/* end of bubble, put at end of list */
		ma_dequeue_entity_rq(&bubble->sched,rq);
		ma_enqueue_entity_rq(&bubble->sched,rq);
	}

	if (!nextent) {
		/* TODO il faudrait virer la bulle de la runqueue */
		ma_holder_rawunlock(&bubble->hold);
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
	marcel_root_bubble.sched.holder=&ma_main_runqueue.hold;
#ifdef MARCEL_BUBBLE_EXPLODE
	ma_deactivate_running_task(MARCEL_SELF,&ma_main_runqueue);
	marcel_bubble_inserttask(&marcel_root_bubble, MARCEL_SELF);
	__do_bubble_explode(&marcel_root_bubble,&ma_main_runqueue);
	ma_dequeue_entity_rq(&MARCEL_SELF->sched.internal, &ma_main_runqueue);
#endif
#ifdef MARCEL_BUBBLE_STEAL
	ma_activate_entity(&marcel_root_bubble.sched, &ma_main_runqueue.hold);
#endif
}

__ma_initfunc_prio(bubble_sched_init, MA_INIT_BUBBLE_SCHED,
		MA_INIT_BUBBLE_SCHED_PRIO, "Bubble Scheduler");
