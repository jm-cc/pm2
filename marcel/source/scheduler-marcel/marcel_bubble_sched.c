
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

#define switchrq(e,rq) \
	PROF_EVENT2(bubble_sched_switchrq, \
	e->type == MA_TASK_ENTITY? \
	(void*)ma_task_entity(e): \
	(void*)ma_bubble_entity(e), \
	rq)

#define switchbubble(e,b) do { \
	if ((e)->type == MA_TASK_ENTITY) \
		PROF_EVENT2(bubble_sched_insert_thread, (void*)ma_task_entity(e), b); \
	else \
		PROF_EVENT2(bubble_sched_insert_bubble, (void*)ma_bubble_entity(e), b); \
} while(0)

int ma_idle_scheduler = 0;
ma_rwlock_t ma_idle_scheduler_lock = MA_RW_LOCK_UNLOCKED;

#ifdef MA__BUBBLES
marcel_bubble_t marcel_root_bubble = MARCEL_BUBBLE_INITIALIZER(marcel_root_bubble);

int marcel_bubble_init(marcel_bubble_t *bubble) {
	PROF_EVENT1(bubble_sched_new,bubble);
	*bubble = (marcel_bubble_t) MARCEL_BUBBLE_INITIALIZER(*bubble);
	ma_stats_reset(&bubble->sched);
	ma_stats_reset(&bubble->hold);
	PROF_EVENT2(sched_setprio,bubble,bubble->sched.prio);
	return 0;
}

int marcel_bubble_setinitrq(marcel_bubble_t *bubble, ma_runqueue_t *rq) {
	bubble->sched.init_holder = bubble->sched.sched_holder = &rq->hold;
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

#define VARS \
	int running; \
	ma_holder_t *h
#define CHECK_HOLDER() \
	MA_BUG_ON(h && ma_holder_type(h) != MA_RUNQUEUE_HOLDER); \
	running = h && bubble->sched.run_holder && bubble->sched.holder_data
#define RAWLOCK_HOLDER() \
	h = ma_entity_holder_rawlock(&bubble->sched); \
	CHECK_HOLDER()
#define HOLDER() \
	h = bubble->sched.run_holder; \
	CHECK_HOLDER()
#define SETPRIO(_prio); \
	if (running) \
		ma_rq_dequeue_entity(&bubble->sched, ma_rq_holder(h)); \
	PROF_EVENT2(sched_setprio,bubble,_prio); \
	MA_BUG_ON(bubble->sched.prio == _prio); \
	bubble->sched.prio = _prio; \
	if (running) \
		ma_rq_enqueue_entity(&bubble->sched, ma_rq_holder(h));

int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio) {
	VARS;
	if (prio == bubble->sched.prio) return 0;
	ma_preempt_disable();
	ma_local_bh_disable();
	RAWLOCK_HOLDER();
	SETPRIO(prio);
	ma_entity_holder_unlock_softirq(h);
	return 0;
}

#ifdef MARCEL_BUBBLE_STEAL
#define DOSLEEP() do { \
	SETPRIO(MA_NOSCHED_PRIO); \
} while(0)

int marcel_bubble_sleep_locked(marcel_bubble_t *bubble) {
	VARS;
	if ( (bubble->sched.run_holder && bubble->sched.run_holder == &bubble->hold)
	   || (!bubble->sched.run_holder && bubble->sched.sched_holder == &bubble->hold))
		return 0;
	ma_holder_rawunlock(&bubble->hold);
	RAWLOCK_HOLDER();
	ma_holder_rawlock(&bubble->hold);
	if (list_empty(&bubble->runningentities)) {
		if (bubble->sched.prio != MA_NOSCHED_PRIO)
			DOSLEEP();
	} else
		bubble_sched_debugl(7,"Mmm, %p actually still awake\n", bubble);
	ma_entity_holder_rawunlock(h);
	return 0;
}

int marcel_bubble_sleep_rq_locked(marcel_bubble_t *bubble) {
	VARS;
	HOLDER();
	DOSLEEP();
	return 0;
}

#define DOWAKE() do { \
	/* XXX erases real bubble prio */ \
	SETPRIO(MA_BATCH_PRIO); \
} while(0)
int marcel_bubble_wake_locked(marcel_bubble_t *bubble) {
	VARS;
	if ( (bubble->sched.run_holder && bubble->sched.run_holder == &bubble->hold)
	   || (!bubble->sched.run_holder && bubble->sched.sched_holder == &bubble->hold))
		return 0;
	ma_holder_rawunlock(&bubble->hold);
	RAWLOCK_HOLDER();
	ma_holder_rawlock(&bubble->hold);
	if (list_empty(&bubble->runningentities)) {
		if (bubble->sched.prio == MA_NOSCHED_PRIO)
			DOWAKE();
	} else
		bubble_sched_debugl(7,"Mmm, %p actually still sleeping\n", bubble);
	ma_entity_holder_rawunlock(h);
	return 0;
}

int marcel_bubble_wake_rq_locked(marcel_bubble_t *bubble) {
	VARS;
	HOLDER();
	DOWAKE();
	return 0;
}
#endif /* MARCEL_BUBBLE_STEAL */

int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio) {
	*prio = bubble->sched.prio;
	return 0;
}

marcel_bubble_t *marcel_bubble_holding_entity(marcel_entity_t *e) {
	ma_holder_t *h = e->init_holder;
	if (!h || h->type != MA_BUBBLE_HOLDER) {
		h = e->sched_holder;
		if (!h || ma_holder_type(h) != MA_BUBBLE_HOLDER)
			h = &marcel_root_bubble.hold;
	}
	bubble_sched_debugl(7,"entity %p is held by bubble %p\n", e, ma_bubble_holder(h));
	return ma_bubble_holder(h);
}

/******************************************************************************
 *
 * Insertion dans une bulle
 *
 */

#ifdef MARCEL_BUBBLE_STEAL
void TBX_EXTERN ma_set_sched_holder(marcel_entity_t *e, marcel_bubble_t *bubble) {
	marcel_entity_t *ee;
	marcel_bubble_t *b;
	ma_holder_t *h;
	bubble_sched_debugl(7,"ma_set_sched_holder %p to bubble %p\n",e,bubble);
	h = e->sched_holder;
	e->sched_holder = &bubble->hold;
	if (e->type == MA_TASK_ENTITY) {
		if ((h = e->run_holder) && h != &bubble->hold) {
			MA_BUG_ON(ma_holder_type(h) != MA_BUBBLE_HOLDER);
			/* already enqueued */
			if (!(e->holder_data)) {
				/* and already running ! */
				ma_deactivate_running_entity(e,h);
				ma_activate_running_entity(e,&bubble->hold);
			} else {
				/* Ici, on suppose que h est déjà verrouillé
				 * ainsi que sa runqueue */
				__ma_bubble_dequeue_entity(e,ma_bubble_holder(h));
				ma_deactivate_running_entity(e,h);
				ma_activate_running_entity(e,&bubble->hold);
				/* Ici, on suppose que la runqueue de bubble
				 * est déjà verrouillée */
				__ma_bubble_enqueue_entity(e,bubble);
			}
		}
	} else {
		MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
		/* stop recursing when reaching a bubble that was on a runqueue */
		if (h && h->type == MA_RUNQUEUE_HOLDER) {
			e->sched_holder = h;
		} else {
			b = ma_bubble_entity(e);
			ma_holder_rawlock(&b->hold);
			list_for_each_entry(ee, &b->heldentities, bubble_entity_list) {
				if (ee->sched_holder && ee->sched_holder->type == MA_BUBBLE_HOLDER)
					ma_set_sched_holder(ee, bubble);
			}
			ma_holder_rawunlock(&b->hold);
		}
	}
}
#endif

/* suppose bubble verrouillée avec softirq */
static void __do_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
#ifdef MARCEL_BUBBLE_STEAL
	ma_holder_t *sched_bubble;
#endif
	/* XXX: will sleep (hence abort) if the bubble was joined ! */
	if (!bubble->nbentities)
		marcel_sem_P(&bubble->join);
	//bubble_sched_debugl(7,"__inserting %p in opened bubble %p\n",entity,bubble);
	if (entity->type == MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_insert_bubble,ma_bubble_entity(entity),bubble);
	else
		PROF_EVENT2(bubble_sched_insert_thread,ma_task_entity(entity),bubble);
	list_add_tail(&entity->bubble_entity_list, &bubble->heldentities);
	marcel_barrier_addcount(&bubble->barrier, 1);
	bubble->nbentities++;
	entity->init_holder = &bubble->hold;
#ifdef MARCEL_BUBBLE_EXPLODE
	entity->sched_holder = entity->init_holder;
#endif
#ifdef MARCEL_BUBBLE_STEAL
	sched_bubble = bubble->sched.sched_holder;
	/* si la bulle conteneuse est dans une autre bulle,
	 * on hérite de la bulle d'ordonnancement */
	if (!sched_bubble || ma_holder_type(sched_bubble) == MA_RUNQUEUE_HOLDER) {
		/* Sinon, c'est la conteneuse qui sert de bulle d'ordonnancement */
		sched_bubble = &bubble->hold;
	} else {
		bubble_sched_debugl(7,"unlock new holder %p\n", &bubble->hold);
		ma_holder_rawunlock(&bubble->hold);
		ma_holder_rawlock(sched_bubble);
		bubble_sched_debugl(7,"sched holder %p locked\n", sched_bubble);
	}
	ma_set_sched_holder(entity, ma_bubble_holder(sched_bubble));
	bubble_sched_debugl(7,"unlock sched holder %p\n", sched_bubble);
	/* dans le cas STEAL, on s'occupe de déverrouiller */
	ma_holder_unlock_softirq(sched_bubble);
#endif
}

#ifdef MARCEL_BUBBLE_EXPLODE
int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	ma_holder_t *h = NULL;
	ma_runqueue_t *rq;
	LOG_IN();

	if (!list_empty(&entity->bubble_entity_list)) {
		ma_holder_t *h = entity->init_holder;
		MA_BUG_ON(h->type == MA_RUNQUEUE_HOLDER);
		marcel_bubble_removeentity(ma_bubble_holder(h),entity);
	}
	if (bubble->status != MA_BUBBLE_OPENED) {
retryclosed:
		ma_holder_lock_softirq(&bubble->hold);
		if (bubble->status == MA_BUBBLE_OPENED) {
			ma_holder_unlock_softirq(&bubble->hold);
			goto retryopened;
		}
		bubble_sched_debugl(7,"inserting %p in closed bubble %p\n",entity,bubble);
		__do_bubble_insertentity(bubble,entity);
		ma_holder_unlock_softirq(&bubble->hold);
	} else {
retryopened:
		h = ma_entity_holder_lock_softirq(&bubble->sched);
		MA_BUG_ON(!h);
		ma_holder_rawlock(&bubble->hold);
		if (bubble->status != MA_BUBBLE_OPENED) {
			ma_holder_rawunlock(&bubble->hold);
			ma_entity_holder_unlock_softirq(h);
			goto retryclosed;
		}
		bubble_sched_debugl(7,"inserting %p in opened bubble %p\n",entity,bubble);
		__do_bubble_insertentity(bubble,entity);
		/* containing bubble already has exploded ! wake up this one */
		entity->sched_holder = h;
		bubble->nbrunning++;
		rq = ma_to_rq_holder(h);
		ma_activate_entity(entity, &rq->hold);
#if 0
		/* C'est plus clair de le laisser dans la bulle*/
		PROF_EVENT2(bubble_sched_switchrq,
			entity->type == MA_TASK_ENTITY?
			(void*)ma_task_entity(entity):
			(void*)ma_bubble_entity(entity),
			rq);
#endif
		ma_holder_rawunlock(&bubble->hold);
		ma_entity_holder_unlock_softirq(h);
	}
	LOG_OUT();
	return 0;
}
#endif
#ifdef MARCEL_BUBBLE_STEAL
int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	LOG_IN();

	if (!list_empty(&entity->bubble_entity_list)) {
		ma_holder_t *h = entity->init_holder;
		MA_BUG_ON(h->type == MA_RUNQUEUE_HOLDER);
		marcel_bubble_removeentity(ma_bubble_holder(h),entity);
	}

	ma_holder_lock_softirq(&bubble->hold);
	bubble_sched_debugl(7,"inserting %p in bubble %p\n",entity,bubble);
	__do_bubble_insertentity(bubble,entity);
	bubble_sched_debugl(7,"insertion %p in bubble %p done\n",entity,bubble);
	LOG_OUT();
	return 0;
}
#endif

/* l'entité doit être vraiment morte (déqueuée notament).
 * Retourne 1 si on doit libérer le sémaphore join. */
int __marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	MA_BUG_ON(entity->init_holder != &bubble->hold);
	entity->init_holder = NULL;
	/* le sched holder pourrait être autre (vol) */
	entity->sched_holder = NULL;
	MA_BUG_ON(entity->run_holder);
	entity->run_holder = NULL;
	list_del_init(&entity->bubble_entity_list);
	marcel_barrier_addcount(&bubble->barrier, -1);
	bubble->nbentities--;
	return (!bubble->nbentities);
}

int marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	int res;
	LOG_IN();
	ma_holder_lock_softirq(&bubble->hold);
	res = __marcel_bubble_removeentity(bubble,entity);
	ma_holder_unlock_softirq(&bubble->hold);
	if (res)
		marcel_sem_V(&bubble->join);
	LOG_OUT();
	return 0;
}

#ifdef MARCEL_BUBBLE_STEAL
/* Détacher une bulle (et son contenu) de la bulle qui la contient, pour
 * pouvoir la placer ailleurs */
int marcel_bubble_detach(marcel_bubble_t *b) {
	ma_holder_t *h = b->sched.sched_holder;
	marcel_bubble_t *hb;
	ma_holder_t *hh;
	LOG_IN();
	PROF_EVENT1(bubble_detach, b);

	MA_BUG_ON(ma_holder_type(h)==MA_RUNQUEUE_HOLDER);
	hb = ma_bubble_holder(h);

	hh = hb->sched.sched_holder;
	MA_BUG_ON(ma_holder_type(hh)!=MA_RUNQUEUE_HOLDER);

	ma_holder_lock_softirq(hh);
	ma_holder_rawlock(h);
	ma_set_sched_holder(&b->sched, b);
	b->sched.sched_holder = NULL;
	ma_holder_rawunlock(h);
	ma_holder_unlock_softirq(hh);
	LOG_OUT();
	return 0;
}
#endif

void marcel_wake_up_bubble(marcel_bubble_t *bubble) {
	ma_runqueue_t *rq;
	ma_holder_t *h;
	LOG_IN();
	if (!(bubble->sched.init_holder)) {
		rq = &ma_main_runqueue;
		bubble->sched.sched_holder = bubble->sched.init_holder = &rq->hold;
	} else {
		MA_BUG_ON(ma_holder_type(h = bubble->sched.sched_holder)
				!= MA_RUNQUEUE_HOLDER);
		rq = ma_rq_holder(h);
	}
	ma_holder_lock_softirq(&rq->hold);
	bubble_sched_debug("waking up bubble %p on rq %s\n",bubble,rq->name);
	PROF_EVENT2(bubble_sched_wake,bubble,rq);
	ma_activate_entity(&bubble->sched, &rq->hold);
	ma_holder_unlock_softirq(&rq->hold);
	LOG_OUT();
}

void marcel_bubble_join(marcel_bubble_t *bubble) {
	ma_holder_t *h;
	LOG_IN();
	marcel_sem_P(&bubble->join);
	h = ma_entity_holder_lock_softirq(&bubble->sched);
	MA_BUG_ON(!list_empty(&bubble->heldentities));
	if (bubble->sched.run_holder) {
		if (bubble->sched.holder_data)
			ma_dequeue_entity(&bubble->sched, h);
		ma_deactivate_running_entity(&bubble->sched, h);
	}
	ma_entity_holder_unlock_softirq(h);
	if ((h = bubble->sched.init_holder)
		&& h->type == MA_BUBBLE_HOLDER
		&& h != &bubble->hold)
		marcel_bubble_removeentity(ma_bubble_holder(h), &bubble->sched);
	PROF_EVENT1(bubble_sched_join,bubble);
	LOG_OUT();
}

#undef marcel_sched_exit
void marcel_sched_exit(marcel_t t) {
	marcel_bubble_t *b = &t->sched.internal.bubble;
	if (b->sched.init_holder) {
		/* bubble initialized */
		marcel_bubble_join(b);
	}
}

#undef marcel_bubble_barrier
int marcel_bubble_barrier(marcel_bubble_t *bubble) {
	return marcel_barrier_wait(&bubble->barrier);
}

/******************************************************************************
 *
 * Statistiques
 *
 */
static void __ma_bubble_synthesize_stats(marcel_bubble_t *bubble) {
	marcel_bubble_t *b;
	marcel_entity_t *e;
	marcel_task_t *t;
	ma_stats_reset(&bubble->hold);
	ma_stats_synthesize(&bubble->hold, &bubble->sched);
	list_for_each_entry(e, &bubble->heldentities, bubble_entity_list) {
		if (e->type == MA_BUBBLE_ENTITY) {
			b = ma_bubble_entity(e);
			ma_holder_rawlock(&b->hold);
			__ma_bubble_synthesize_stats(b);
			ma_holder_rawunlock(&b->hold);
			ma_stats_synthesize(&bubble->hold, &b->hold);
		} else {
			t = ma_task_entity(e);
			ma_stats_synthesize(&bubble->hold, &t->sched.internal.entity);
		}
	}
}
void ma_bubble_synthesize_stats(marcel_bubble_t *bubble) {
	ma_holder_lock_softirq(&bubble->hold);
	__ma_bubble_synthesize_stats(bubble);
	ma_holder_unlock_softirq(&bubble->hold);
}

/******************************************************************************
 * Refermeture de bulle
 */

#ifdef MARCEL_BUBBLE_EXPLODE
static void __marcel_close_bubble(marcel_bubble_t *bubble, ma_runqueue_t *rootrq) {
	marcel_entity_t *e;
	marcel_task_t *t;
	marcel_bubble_t *b;
	ma_holder_t *h;

	PROF_EVENT1(bubble_sched_closing,bubble);
	bubble_sched_debug("bubble %p closing\n", bubble);
	bubble->status = MA_BUBBLE_CLOSING;
	list_for_each_entry(e, &bubble->heldentities, bubble_entity_list) {
		if (e->type == MA_BUBBLE_ENTITY) {
			b = ma_bubble_entity(e);
			ma_holder_rawlock(&b->hold);
			__marcel_close_bubble(b, rootrq);
			ma_holder_rawunlock(&b->hold);
		} else {
			t = ma_task_entity(e);
			h = ma_task_sched_holder(t);
			if (h!=&rootrq->hold) {
				MA_BUG_ON(!(h = ma_task_holder_rawlock(t)));
				MA_BUG_ON(h->type != MA_RUNQUEUE_HOLDER);
				MA_BUG_ON(h==&rootrq->hold);
			}
			if (!MA_TASK_IS_RUNNING(t)) { /* not running somewhere */
				bubble_sched_debugl(7,"deactivating task %s(%p) from %p\n", t->name, t, h);
				PROF_EVENT2(bubble_sched_goingback,e,bubble);
				if (MA_TASK_IS_READY(t))
					ma_deactivate_task(t,h);
				bubble->nbrunning--;
			}
			if (h!=&rootrq->hold)
				ma_task_holder_rawunlock(h);
		}
	}
}

void marcel_close_bubble(marcel_bubble_t *bubble) {
	int wake_bubble;
	ma_holder_t *h;
	ma_runqueue_t *rq;
	LOG_IN();

	h = ma_entity_holder_lock_softirq(&bubble->sched);
	MA_BUG_ON(h->type != MA_RUNQUEUE_HOLDER);
	rq = ma_rq_holder(h);
	ma_holder_rawlock(&bubble->hold);
	PROF_EVENT2(bubble_sched_close,bubble,rq);
	__marcel_close_bubble(bubble, rq);
	if ((wake_bubble = (!bubble->nbrunning))) {
		bubble_sched_debug("last already off, bubble %p closed\n", bubble);
		bubble->status = MA_BUBBLE_CLOSED;
		/* et on la remet pour plus tard */
		ma_activate_entity(&bubble->sched, &rq->hold);
	}
	ma_holder_rawunlock(&bubble->hold);
	ma_entity_holder_unlock_softirq(&rq->hold);
	LOG_OUT();
}

static void __do_bubble_explode(marcel_bubble_t *bubble, ma_runqueue_t *rq) {
	marcel_entity_t *new;

	bubble_sched_debug("bubble %p exploding at %s\n", bubble, rq->name);
	PROF_EVENT1(bubble_sched_explode, bubble);
	bubble->status = MA_BUBBLE_OPENED;
	list_for_each_entry(new, &bubble->heldentities, bubble_entity_list) {
		bubble_sched_debugl(7,"activating entity %p on %s\n", new, rq->name);
		new->sched_holder=&rq->hold;
		bubble->nbrunning++;
		MA_BUG_ON(new->run_holder && new->run_holder->type != MA_BUBBLE_HOLDER);
		if (!new->run_holder && (new->type != MA_TASK_ENTITY || ma_task_entity(new)->sched.state == MA_TASK_RUNNING))
			ma_activate_entity(new, &rq->hold);
		else
			bubble_sched_debugl(7,"task %p not running (%d)\n",ma_task_entity(new), ma_task_entity(new)->sched.state);
	}
}
#endif

void ma_bubble_tick(marcel_bubble_t *bubble) {
#ifdef MARCEL_BUBBLE_EXPLODE
	if (bubble->status != MA_BUBBLE_CLOSING)
		marcel_close_bubble(bubble);
#endif
}

/******************************************************************************
 *
 * Self-scheduling
 *
 */

#if 0
// Abandonné: plutôt que parcourir l'arbre des bulles, on a un cache de threads prêts
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
		bubble_sched_debugl(7,"got at end of curb, go up\n");
		if (curb == root)
			bubble_sched_debugl(7,"finished root\n");
			/* finished root bubble */
			return NULL;
		b = marcel_bubble_holding_bubble(curb);
		bubble_sched_debugl(7,"up to %p\n",b);
		return ma_next_running_in_bubble(root,b,&curb->sched.run_list);
	}

	next = list_entry(nexthead, marcel_entity_t, run_list);
	bubble_sched_debugl(7,"next %p\n",next);

	if (tbx_likely(next->type == MA_TASK_ENTITY))
		return next;

	/* bubble, go down */
	b = ma_bubble_entity(next);
	bubble_sched_debugl(7,"it is bubble %p\n",b);
	/* attention, il ne faudrait pas qu'elle bouge entre-temps */
	ma_holder_rawlock(&b->hold);
	h = ma_holder_type(b->sched.run_holder);
	ma_holder_rawunlock(&b->hold);
	if (h == MA_RUNQUEUE_HOLDER) /* bubble on another rq (probably down
					elsewhere), don't go down */
		return ma_next_running_in_bubble(root,curb,&b->sched.run_list);
	return ma_next_running_in_bubble(root,b,&b->heldentities);
}
#endif
#endif

/* on détient le verrou du holder de nextent */
marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_holder_t *prevh, ma_runqueue_t *rq,
		ma_holder_t **nexth, int idx) {
	marcel_bubble_t *bubble;
#ifdef MA__LWPS
	int max_prio;
	ma_runqueue_t *currq;
#endif

	LOG_IN();

#ifdef MA__LWPS
	/* sur smp, descendre l'entité si besoin est */
	if (ma_idle_scheduler && nextent->sched_level > rq->level) {
		/* s'assurer d'abord que personne n'a activé une entité d'une
		 * telle priorité (ou encore plus forte) avant nous */
		bubble_sched_debugl(7,"%p should go down\n", nextent);
		max_prio = idx;
		for (currq = ma_lwp_vprq(LWP_SELF); currq != rq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx <= max_prio) {
				bubble_sched_debugl(7,"prio %d on lower rq %s in the meanwhile\n", idx, currq->name);
				sched_debug("unlock(%p)\n", rq);
				ma_holder_unlock(&rq->hold);
				LOG_RETURN(NULL);
			}
		}
		for (; currq; currq = currq->father) {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx < max_prio) {
				bubble_sched_debugl(7,"better prio %d on rq %s in the meanwhile\n", idx, currq->name);
				sched_debug("unlock(%p)\n", rq);
				ma_holder_unlock(&rq->hold);
				LOG_RETURN(NULL);
			}
		}

		ma_deactivate_entity(nextent,&rq->hold);

		/* nextent est RUNNING, on peut déverrouiller la runqueue,
		 * personne n'y touchera */
		sched_debug("unlock(%p)\n", rq);
		ma_holder_rawunlock(&rq->hold);

		/* trouver une runqueue qui va bien */
		for (currq = ma_lwp_vprq(LWP_SELF); currq &&
			nextent->sched_level < currq->level;
			currq = currq->father);

		MA_BUG_ON(!currq);
		PROF_EVENT2(bubble_sched_switchrq,
			nextent->type == MA_TASK_ENTITY?
			(void*)ma_task_entity(nextent):
			(void*)ma_bubble_entity(nextent),
			currq);
		bubble_sched_debug("entity %p going down from %s(%p) to %s(%p)\n", nextent, rq->name, rq, currq->name, currq);

		/* on descend, l'ordre des adresses est donc correct */
		ma_holder_rawlock(&currq->hold);
		nextent->sched_holder = &currq->hold;
		ma_activate_entity(nextent,&currq->hold);
		ma_holder_rawunlock(&currq->hold);
		if (rq != currq) {
			ma_preempt_enable();
			LOG_RETURN(NULL);
		}
	}
#endif

	if (nextent->type == MA_TASK_ENTITY)
		LOG_RETURN(nextent);

/*
 * c'est une bulle
 */
	bubble = ma_bubble_entity(nextent);

#if defined(MARCEL_BUBBLE_EXPLODE)
	/* maintenant on peut s'occuper de la bulle */
	/* l'enlever de la queue */
	ma_deactivate_entity(nextent,&rq->hold);
	ma_holder_rawlock(&bubble->hold);
	/* XXX: time_slice proportionnel au parallélisme de la runqueue */
/* ma_atomic_set est une macro et certaines versions de gcc n'aiment pas
les #ifdef dans les arguments de macro...
*/
#ifdef MA__LWPS
#  define _TIME_SLICE marcel_vpmask_weight(&rq->vpset)
#else
#  define _TIME_SLICE 1
#endif
	ma_atomic_set(&bubble->sched.time_slice, MARCEL_BUBBLE_TIMESLICE*_TIME_SLICE);
#undef _TIME_SLICE
	bubble_sched_debugl(7,"timeslice %u\n",ma_atomic_read(&bubble->sched.time_slice));
	__do_bubble_explode(bubble,rq);
	ma_atomic_set(&bubble->sched.time_slice,MARCEL_BUBBLE_TIMESLICE*bubble->nbrunning); /* TODO: plutôt arbitraire */

	ma_holder_rawunlock(&bubble->hold);
	sched_debug("unlock(%p)\n", rq);
	ma_holder_unlock(&rq->hold);
	LOG_RETURN(NULL);

#elif defined(MARCEL_BUBBLE_STEAL)
	sched_debug("locking bubble %p\n",bubble);
	ma_holder_rawlock(&bubble->hold);

	/* bulle fraîche, on l'amène près de nous */
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
			LOG_RETURN(NULL);
		}
	}

	/* en général, on arrive à l'éviter */
	if (list_empty(&bubble->runningentities)) {
		bubble_sched_debug("warning: bubble %p empty\n", bubble);
		/* on ne devrait jamais ordonnancer une bulle de priorité NOSCHED ! */
 		MA_BUG_ON(bubble->sched.prio == MA_NOSCHED_PRIO);
		marcel_bubble_sleep_rq_locked(bubble);
		sched_debug("unlock(%p)\n", rq);
		ma_holder_rawunlock(&rq->hold);
		ma_holder_unlock(&bubble->hold);
		LOG_RETURN(NULL);
	}

	nextent = list_entry(bubble->runningentities.next, marcel_entity_t, run_list);
	bubble_sched_debugl(7,"next entity to run %p\n",nextent);

	sched_debug("unlock(%p)\n", rq);
	ma_holder_rawunlock(&rq->hold);
	*nexth = &bubble->hold;
	LOG_RETURN(nextent);
#else
#error Please choose a bubble algorithm
#endif
}

/******************************************************************************
 *
 * Vol de bulle local.
 *
 */

#ifdef MARCEL_BUBBLE_STEAL
#ifdef MA__LWPS

static int total_nr_running(marcel_bubble_t *b) {
	int nr_running = b->hold.nr_running;
	marcel_entity_t *e;
	// XXX: pas fiable du tout ! Il faudrait verrouiller la bulle !
	ma_holder_rawlock(&b->hold);
	list_for_each_entry(e, &b->heldentities, bubble_entity_list)
		if (e->type == MA_BUBBLE_ENTITY)
			nr_running += total_nr_running(ma_bubble_entity(e));
	ma_holder_rawunlock(&b->hold);
	return nr_running;
}

static marcel_bubble_t *find_interesting_bubble(ma_runqueue_t *rq, int up_power) {
	int i, nbrun;
	// TODO: partir plutôt de la bulle "root" et descendre dedans ?
	// Le problème, c'est qu'en ne regardant que les bubbles dans les
	//runqueues, on loupe les grosses bulles qui ne contiennent pas de
	//thread prêt...
	marcel_entity_t *e;
	marcel_bubble_t *b;
	if (!rq->hold.nr_running)
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
			if ((nbrun = total_nr_running(b)) >= up_power) {
			//if (b->hold.nr_running >= up_power) {
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
	bubble_sched_debugl(7,"see %d %d\n", level->type, level->number);
	if (!rq)
		return 0;
	ma_preempt_disable();
	//if (find_interesting_bubble(rq, up_power, 0)) {
		ma_holder_rawlock(&rq->hold);
		/* recommencer une fois verrouillé */
		b = find_interesting_bubble(rq, up_power);
		if (!b) {
			ma_holder_rawunlock(&rq->hold);
			bubble_sched_debugl(7,"no interesting bubble\n");
		} else {
			// XXX: calcul dupliqué ici
			nbrun = total_nr_running(b);
			for (rq2 = rq;
				/* emmener juste assez haut pour moi */
				  !marcel_vpmask_vp_ismember(&rq2->vpset, LWP_NUMBER(LWP_SELF))
				&& rq2->father
				/* et juste assez haut par rapport au nombre de threads */
				&& marcel_vpmask_weight(&rq2->father->vpset) <= nbrun
				; rq2 = rq2->father)
				bubble_sched_debugl(7,"looking up to rq %s\n", rq2->father->name);
			if (!rq2 ||
					/* pas intéressant pour moi, on laisse les autres se débrouiller */
					/* todo: le faire quand même ? */
					!marcel_vpmask_vp_ismember(&rq2->vpset,LWP_NUMBER(LWP_SELF))) {
				bubble_sched_debug("%s doesn't suit for me and %d threads\n",rq2?rq2->name:"anything",nbrun);
				ma_holder_rawunlock(&rq->hold);
			} else {
				bubble_sched_debug("rq %s seems good, stealing bubble %p\n", rq2->name, b);
				PROF_EVENT2(bubble_sched_switchrq, b, rq2);
				/* laisser d'abord ce qui est ordonnancé sur place */
				ma_holder_rawlock(&b->hold);
				list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
					b2 = NULL;
					if (e->type == MA_BUBBLE_ENTITY)
						b2 = ma_bubble_entity(e);
					else
						t = ma_task_entity(e);
					if ((b2 &&
							 /* ne pas toucher à ce qui tourne actuellement */
							(b2->hold.nr_scheduled ||
							 /* laisser la première bulle */
							 first)
							 /* ne pas toucher à ce qui tourne actuellement */
							) || (!b2 && MA_TASK_IS_RUNNING(t))) {
						int state;
						if (b2) {
							first = 0;
							bubble_sched_debug("detaching running bubble %p from %p\n", b2, b);
						} else
							bubble_sched_debug("detaching running task %p from %p\n", t, b);

						state = ma_get_entity(e);
						ma_put_entity(e, &rq->hold, state);
						if (b2) {
							PROF_EVENT2(bubble_sched_switchrq, b2, rq);
							b2->settled = 1;
						} else
							PROF_EVENT2(bubble_sched_switchrq, t, rq);
					}
				}
				ma_holder_rawunlock(&b->hold);
				/* enlever b de rq */
				ma_get_entity(&b->sched);
				ma_holder_rawunlock(&rq->hold);
				ma_holder_rawlock(&rq2->hold);
				ma_holder_rawlock(&b->hold);
				/* b contient encore tout ce qui n'est pas ordonnancé, les distribuer */
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
						if (b2) {
							PROF_EVENT2(bubble_sched_switchrq, b2, rq2);
							b2->settled = 1;
						} else
							PROF_EVENT2(bubble_sched_switchrq, t, rq2);
						/* et forcer à redescendre au même niveau qu'avant */
						e->sched_level = rq->level;
					}
				}
				ma_holder_rawunlock(&b->hold);
				/* mettre b sur rq2 */
				ma_put_entity(&b->sched, &rq2->hold, MA_ENTITY_RUNNING);
				PROF_EVENT2(bubble_sched_switchrq, b, rq2);
				b->sched.sched_level = rq2->level;
				ma_holder_rawunlock(&rq2->hold);
			}
		}
	//}
	ma_preempt_enable();
	return 0;
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
		/* si l'appelant fait partie des fils, l'éviter */
		n--;
		i = (me->index + 1) % level->arity;
	}
	while (n--) {
		/* examiner les fils */
		if (see(level->sons[i], power) || see_down(level->sons[i], NULL))
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

#endif
#endif

int marcel_bubble_steal_work(void) {
#ifdef MA__LWPS
#ifdef MARCEL_BUBBLE_STEAL
	ma_read_lock(&ma_idle_scheduler_lock);
	if (ma_idle_scheduler) {
		struct marcel_topo_level *me =
			&marcel_topo_vp_level[LWP_NUMBER(LWP_SELF)];
		bubble_sched_debugl(7,"bubble steal on %d\n", LWP_NUMBER(LWP_SELF));
		/* couln't find work on local runqueue, go see elsewhere */
		ma_read_unlock(&ma_idle_scheduler_lock);
		return see_up(me);
	}
	ma_read_unlock(&ma_idle_scheduler_lock);
#endif
#endif
#ifdef MARCEL_GANG_SCHEDULER
	/* This lwp is idle... TODO: check other that other LWPs of the same controller are idle too, and get another gang (that gang terminated) */
#endif
	return 0;
}

/******************************************************************************
 *
 * Répartiteur de bulles.
 *
 * Suppose que la hiérarchie est rassemblée.
 *
 */

#include <math.h>
static inline long entity_load(marcel_entity_t *e) {
	if (e->type == MA_BUBBLE_ENTITY)
		return *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e), marcel_stats_load_offset);
	else
		return *(long*)ma_task_stats_get(ma_task_entity(e), marcel_stats_load_offset);
}

static int load_compar(const void *_e1, const void *_e2) {
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;
	long l1 = entity_load(e1);
	long l2 = entity_load(e2);
	return l2 - l1; /* decreasing order */
}
static void __marcel_bubble_spread(marcel_entity_t *e[], int ne, struct marcel_topo_level **l, int nl) {
	/* TODO: give imbalance in recursion */
	/* TODO: XXX: lock holders! */
	int i;
	fprintf(stderr,"spreading entit%s", ne>1?"ies":"y");
	for (i=0; i<ne; i++)
		fprintf(stderr," %p", e[i]);
	fprintf(stderr," at level%s", nl>1?"s":"");
	for (i=0; i<nl; i++)
		fprintf(stderr," %s", l[i]->sched.name);
	fprintf(stderr,"\n");

	/* TODO: tune */
	if (ne < nl) {
		/* less entities than items, recurse into entities before items */
		unsigned new_ne = 0;
		unsigned recursed = 0;
		int i, j;
		fprintf(stderr,"%d < %d, recurse into entities\n", ne, nl);

		/* first count */
		for (i=0; i<ne; i++) {
			if (e[i]->type == MA_BUBBLE_ENTITY) {
				unsigned nb = ma_bubble_entity(e[i])->nbentities;
				if (nb) {
					recursed = 1;
					new_ne += nb;
				}
			} else
				new_ne += 1;
		}
		if (recursed) {
			fprintf(stderr,"%d sub-entities\n", new_ne);
			/* now establish the list */
			marcel_entity_t *new_e[new_ne], *ee;
			j = 0;
			for (i=0; i<ne; i++) {
				if (e[i]->type == MA_BUBBLE_ENTITY) {
					marcel_bubble_t *bb = ma_bubble_entity(e[i]);
					list_for_each_entry(ee, &bb->heldentities, bubble_entity_list)
						new_e[j++] = ee;
				} else
					new_e[j++] = e[i];
			}
			/* and recurse */
			__marcel_bubble_spread(new_e, new_ne, l, nl);
		} else {
			/* Grmpf, really not enough parallelism, only
			 * use part of the machine */
			MA_BUG_ON(ne > nl);
			fprintf(stderr,"Not enough parallelism, use only %d item%s\n", ne, ne>1?"s":"");
			__marcel_bubble_spread(&e[0], ne, l, ne);
		}
	}

	else 

	{
		/* More entities than items, distribute */
		int i, j, n, nitems, nentities;
		unsigned long totload = 0, load;
		float per_item_load, bonus = 0, partload;

		/* compute total load */
		for (i=0; i<ne; i++) {
			if (e[i]->type == MA_BUBBLE_ENTITY)
				totload += *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e[i]), marcel_stats_load_offset);
			else
				totload += *(long*)ma_task_stats_get(ma_task_entity(e[i]), marcel_stats_load_offset);
		}
		per_item_load = (float)totload / nl;
		fprintf(stderr,"load %lu = %d*%.2f\n", totload, nl, per_item_load);

		/* Sort entities by load */
		qsort(e, ne, sizeof(e[0]), &load_compar);

		/* Start with heaviest, put on the first item */
		i = 0;
		n = 0;
		while(i<ne) {
			/* when entities' load is 0, just leave them here */
			if (entity_load(e[i]) == 0) {
				fprintf(stderr,"remaining entities are 0-loaded, just leave them here\n");
				int state;
				for (j=i; j<ne; j++) {
					ma_runqueue_t *rq;
					state = ma_get_entity(e[j]);
					if (l[0]->father)
						rq = &l[0]->father->sched;
					else
						rq = &marcel_machine_level[0].sched;
					ma_put_entity(e[j], &rq->hold, state);
					switchrq(e[j], rq);
				}
				break;
			}

			/* start with some bonus or malus */
			partload = -bonus;
			fprintf(stderr,"starting at %s with load %.2f\n", l[n]->sched.name, partload);
			for (j=i; j<ne; j++) {
				load = entity_load(e[j]);
				partload += load;
				fprintf(stderr,"adding entity %p(%lu) -> %.2f\n", e[j], load, partload);
				/* accept 10% imbalance: TODO: tune */
				if (partload > per_item_load * 9 / 10) {
					nentities = j-i+1;
					break;
				}
			}
			if (j == ne)
				nentities = ne-i;

			nitems = (partload + per_item_load / 10) / per_item_load;
			if (!nitems)
				nitems = 1;
			bonus = nitems * per_item_load - partload;
			fprintf(stderr,"Ok, %d item%s and remaining %.2f load\n", nitems, nitems>1?"s":"", bonus);

			if (nitems == 1) {
				/* one item selected, put that there */
				fprintf(stderr,"Ok, put %s on %s\n", nentities>1?"them":"it", l[n]->sched.name);
				int state;
				for (j=i; j<i+nentities; j++) {
					state = ma_get_entity(e[j]);
					ma_put_entity(e[j], &l[n]->sched.hold, state);
					switchrq(e[j], &l[n]->sched);
				}
				if (l[n]->arity) {
					fprintf(stderr,"and recurse\n");
					__marcel_bubble_spread(&e[i], nentities, l[n]->sons, l[n]->arity);
				}
			} else {
				/* several items, just recurse */
				__marcel_bubble_spread(&e[i], nentities, &l[n], nitems);
			}
			n += nitems;
			i += nentities;
			/* TODO: if n>nl, put the rest ! (should only happen if floating point operations are not exact) */
			MA_BUG_ON(n>nl);
		}
	}
	fprintf(stderr,"spreading entit%s", ne>1?"ies":"y");
	for (i=0; i<ne; i++)
		fprintf(stderr," %p", e[i]);
	fprintf(stderr," at level%s", nl>1?"s":"");
	for (i=0; i<nl; i++)
		fprintf(stderr," %s", l[i]->sched.name);
	fprintf(stderr," done.\n");

}

/* Rassembleur de la hiérarchie de la bulle */
static void __ma_bubble_gather(marcel_bubble_t *b, marcel_bubble_t *rootbubble) {
	marcel_entity_t *e;
	ma_holder_rawlock(&b->hold);
	list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
		int state;
		ma_holder_t *h = NULL;

		if (e->type == MA_BUBBLE_ENTITY)
			__ma_bubble_gather(ma_bubble_entity(e), rootbubble);

		if (e->sched_holder == &b->hold || e->sched_holder == &rootbubble->hold)
			/* déjà rassemblé */
			continue;

		if (e->run_holder != &b->hold && e->run_holder != &rootbubble->hold)
			/* verrouiller le conteneur actuel de e */
			/* TODO: en principe, devrait être "plus bas" */
			h = ma_entity_holder_rawlock(e);

		state = ma_get_entity(e);
		fprintf(stderr,"putting back %p in bubble %p(%p)\n", e, b, &b->hold);
		ma_put_entity(e, &b->hold, state);
		PROF_EVENT2(bubble_sched_goingback, e, b);

		if (h)
			ma_entity_holder_rawunlock(h);
	}
	ma_holder_rawunlock(&b->hold);
}
void ma_bubble_gather(marcel_bubble_t *b) {
	ma_preempt_disable();
	ma_local_bh_disable();
	__ma_bubble_gather(b, b);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}
void marcel_bubble_spread(marcel_bubble_t *b, struct marcel_topo_level *l) {
	marcel_entity_t *e = &b->sched;
	ma_bubble_synthesize_stats(b);
	ma_preempt_disable();
	ma_local_bh_disable();
	__ma_bubble_gather(b, b);
	ma_holder_rawlock(&b->hold);
	__marcel_bubble_spread(&e, 1, &l, 1);
	ma_holder_rawunlock(&b->hold);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

/******************************************************************************
 *
 * Bubble gang scheduler.
 *
 */

ma_runqueue_t ma_gang_rq;

any_t marcel_gang_scheduler(any_t foo) {
	marcel_entity_t *e, *ee;
	marcel_bubble_t *b;
	ma_runqueue_t *rq, *work_rq = (void*) foo;
	struct list_head *queue;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_gang_rq);
	/* Attendre un tout petit peu que la création de threads se fasse */
	marcel_usleep(1);
	while(1) {
		/* D'abord enlever les jobs de la work_rq */
		rq = work_rq;
		PROF_EVENT1(rq_lock,work_rq);
		ma_holder_lock_softirq(&rq->hold);
		PROF_EVENT1(rq_lock,&ma_gang_rq);
		ma_holder_rawlock(&ma_gang_rq.hold);
		/* Les bulles pleines */
		queue = ma_rq_queue(rq, MA_BATCH_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		/* Les bulles vides */
		queue = ma_rq_queue(rq, MA_NOSCHED_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		/* Mettre ensuite un job sur la work_rq */
		rq = &ma_gang_rq;
		queue = ma_rq_queue(rq, MA_BATCH_PRIO);
		if (!ma_queue_empty(queue)) {
			e = ma_queue_entry(queue);
			MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
			b = ma_bubble_entity(e);
			ma_deactivate_entity(&b->sched, &rq->hold);
			PROF_EVENT2(bubble_sched_switchrq, b, work_rq);
			ma_activate_entity(&b->sched, &work_rq->hold);
		}
		ma_holder_rawunlock(&rq->hold);
		PROF_EVENT1(rq_unlock,&ma_gang_rq);
		ma_holder_unlock_softirq(&work_rq->hold);
		PROF_EVENT1(rq_unlock,work_rq);
		ma_lwp_t lwp;
		/* Et préempter les threads de cette rq */
		for_each_lwp_begin(lwp)
			if (lwp != LWP_SELF && ma_rq_covers(work_rq,LWP_NUMBER(lwp))) {
				ma_holder_rawlock(&ma_lwp_vprq(lwp)->hold);
				ma_set_tsk_need_togo(ma_per_lwp(current_thread,lwp));
				ma_resched_task(ma_per_lwp(current_thread,lwp),lwp);
				ma_holder_rawunlock(&ma_lwp_vprq(lwp)->hold);
			}
		for_each_lwp_end();
		marcel_delay(MARCEL_BUBBLE_TIMESLICE*marcel_gettimeslice()/1000);
	}
	return NULL;
}

/* Nettoyeur: enlève les jobs de la work_rq */
any_t marcel_gang_cleaner(any_t foo) {
	marcel_entity_t *e, *ee;
	marcel_bubble_t *b;
	ma_runqueue_t *rq, *work_rq = (void*) foo;
	struct list_head *queue;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_gang_rq);
	marcel_usleep(1);
	while(1) {
		rq = work_rq;
		ma_holder_lock_softirq(&rq->hold);
		ma_holder_rawlock(&ma_gang_rq.hold);
		/* Les bulles pleines */
		queue = ma_rq_queue(rq, MA_BATCH_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		/* Les bulles vides */
		queue = ma_rq_queue(rq, MA_NOSCHED_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		ma_holder_rawunlock(&rq->hold);
		ma_holder_unlock_softirq(&ma_gang_rq.hold);
		ma_lwp_t lwp;
		/* Et préempter les threads de cette rq */
		for_each_lwp_begin(lwp)
			if (lwp != LWP_SELF && ma_rq_covers(work_rq,LWP_NUMBER(lwp))) {
				ma_holder_rawlock(&ma_lwp_vprq(lwp)->hold);
				ma_set_tsk_need_togo(ma_per_lwp(current_thread,lwp));
				ma_resched_task(ma_per_lwp(current_thread,lwp),lwp);
				ma_holder_rawunlock(&ma_lwp_vprq(lwp)->hold);
			}
		for_each_lwp_end();
		marcel_delay(MARCEL_BUBBLE_TIMESLICE*marcel_gettimeslice()/1000);
	}
	return NULL;
}

#ifdef MARCEL_GANG_SCHEDULER
marcel_t __marcel_start_gang_scheduler(marcel_func_t f, ma_runqueue_t *rq, int sys) {
	marcel_attr_t attr;
	marcel_t t;
	marcel_attr_init(&attr);
	marcel_attr_setname(&attr, "gang scheduler");
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setinitrq(&attr, rq);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
	if (sys) {
		marcel_attr_setflags(&attr, MA_SF_NORUN);
		marcel_create_special(&t, &attr, f, rq);
		marcel_wake_up_created_thread(t);
	} else {
		marcel_create(&t, &attr, f, rq);
	}
	return t;
}
marcel_t marcel_start_gang_scheduler(ma_runqueue_t *rq, int sys) {
	return __marcel_start_gang_scheduler(marcel_gang_scheduler, rq, sys);
}
#endif

void __marcel_init ma_bubble_sched_start(void) {
#ifdef MARCEL_GANG_SCHEDULER
	ma_idle_scheduler = 0;
#if 1
	/* un seul gang scheduler */
	marcel_start_gang_scheduler(&ma_main_runqueue, 0);
#else
	/* un nettoyeur */
	__marcel_start_gang_scheduler(marcel_gang_cleaner, &ma_main_runqueue, 1);
#if 1
	/* deux gang schedulers */
	marcel_start_gang_scheduler(&marcel_topo_levels[1][0].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[1][1].sched, 0);
#else
	/* quatre gang schedulers */
	marcel_start_gang_scheduler(&marcel_topo_levels[2][0].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[2][1].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[2][2].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[2][3].sched, 0);
#endif
#endif
#endif
}

/******************************************************************************
 *
 * Initialisation
 *
 */

static void __marcel_init bubble_sched_init() {
	marcel_root_bubble.sched.sched_holder =
		marcel_root_bubble.sched.init_holder = &ma_main_runqueue.hold;
#ifdef MARCEL_BUBBLE_EXPLODE
	/* make it explode */
	PROF_EVENT2_ALWAYS(bubble_sched_switchrq,&marcel_root_bubble,&ma_main_runqueue);
	__do_bubble_explode(&marcel_root_bubble,&ma_main_runqueue);
#endif
#ifdef MARCEL_BUBBLE_STEAL
	ma_activate_entity(&marcel_root_bubble.sched, &ma_main_runqueue.hold);
	PROF_EVENT2(bubble_sched_switchrq, &marcel_root_bubble, &ma_main_runqueue);
	ma_init_rq(&ma_gang_rq, "gang", MA_DONTSCHED_RQ);
#endif
}

void ma_bubble_sched_init2(void) {
#ifdef MARCEL_BUBBLE_STEAL
	/* Having main on the main runqueue is both faster and respects priorities */
	ma_deactivate_running_entity(&MARCEL_SELF->sched.internal.entity, &marcel_root_bubble.hold);
	SELF_GETMEM(sched.internal.entity.sched_holder) = &ma_main_runqueue.hold;
	PROF_EVENT2(bubble_sched_switchrq, MARCEL_SELF, &ma_main_runqueue);
	ma_activate_running_entity(&MARCEL_SELF->sched.internal.entity, &ma_main_runqueue.hold);
#endif
}

__ma_initfunc_prio(bubble_sched_init, MA_INIT_BUBBLE_SCHED, MA_INIT_BUBBLE_SCHED_PRIO, "Bubble Scheduler");
#endif /* MA__BUBBLES */
