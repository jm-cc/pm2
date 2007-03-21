
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

int marcel_bubble_setid(marcel_bubble_t *bubble, int id) {
	PROF_EVENT2(bubble_setid,bubble, id);
	return 0;
}

int marcel_bubble_setinitrq(marcel_bubble_t *bubble, ma_runqueue_t *rq) {
	bubble->sched.init_holder = bubble->sched.sched_holder = &rq->hold;
	return 0;
}

int marcel_bubble_setinitlevel(marcel_bubble_t *bubble, marcel_topo_level_t *level) {
	marcel_bubble_setinitrq(bubble, &level->sched);
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
#define RUNNING() \
	h && ma_holder_type(h) == MA_RUNQUEUE_HOLDER && bubble->sched.run_holder && bubble->sched.holder_data
#define RAWLOCK_HOLDER() \
	h = ma_entity_holder_rawlock(&bubble->sched); \
	running = RUNNING()
#define HOLDER() \
	h = bubble->sched.run_holder; \
	running = RUNNING()
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

int marcel_bubble_setprio_locked(marcel_bubble_t *bubble, int prio) {
	VARS;
	if (prio == bubble->sched.prio) return 0;
	HOLDER();
	SETPRIO(prio);
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
	if (bubble->sched.prio == MA_NOSCHED_PRIO)
		DOWAKE();
	ma_entity_holder_rawunlock(h);
	return 0;
}

int marcel_bubble_wake_rq_locked(marcel_bubble_t *bubble) {
	VARS;
	HOLDER();
	if (bubble->sched.prio == MA_NOSCHED_PRIO)
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
			/* already enqueued */
			/* Ici, on suppose que h est déjà verrouillé
			 * ainsi que sa runqueue pour une bulle */
			if (!(e->holder_data)) {
				/* and already running ! */
				ma_deactivate_running_entity(e,h);
				ma_activate_running_entity(e,&bubble->hold);
			} else {
				if (ma_holder_type(h) == MA_BUBBLE_HOLDER)
					__ma_bubble_dequeue_entity(e,ma_bubble_holder(h));
				else
					ma_rq_dequeue_entity(e,ma_rq_holder(h));
				ma_deactivate_running_entity(e,h);
				ma_activate_running_entity(e,&bubble->hold);
				/* Ici, on suppose que la runqueue de bubble
				 * est déjà verrouillée */
				__ma_bubble_enqueue_entity(e,bubble);
			}
		}
	} else {
		MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
		b = ma_bubble_entity(e);
		/* XXX erases real bubble prio */
		marcel_bubble_setprio_locked(b, MA_BATCH_PRIO);
		list_for_each_entry(ee, &b->heldentities, bubble_entity_list) {
			if (ee->sched_holder && ee->sched_holder->type == MA_BUBBLE_HOLDER)
				ma_set_sched_holder(ee, bubble);
		}
	}
}
#endif

/* suppose bubble verrouillée avec softirq */
static void __do_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
#ifdef MARCEL_BUBBLE_STEAL
	ma_holder_t *sched_bubble;
	ma_holder_t *h;
#endif

	/* XXX: will sleep (hence abort) if the bubble was joined ! */
	if (!bubble->nbentities)
		marcel_sem_P(&bubble->join);

#ifdef MARCEL_BUBBLE_STEAL
	h = ma_entity_holder_lock_softirq(entity);

	if (h != &bubble->hold)
		/* XXX: c'est couillu ca */
		ma_holder_rawlock(&bubble->hold);
#endif

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
		if (h != sched_bubble)
			ma_holder_rawlock(sched_bubble);
		bubble_sched_debugl(7,"sched holder %p locked\n", sched_bubble);
	}
	ma_set_sched_holder(entity, ma_bubble_holder(sched_bubble));
	bubble_sched_debugl(7,"unlock sched holder %p\n", sched_bubble);
	if (h != sched_bubble)
		ma_holder_rawunlock(sched_bubble);
	
	ma_entity_holder_unlock_softirq(h);
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

	bubble_sched_debugl(7,"inserting %p in bubble %p\n",entity,bubble);
	__do_bubble_insertentity(bubble,entity);
	bubble_sched_debugl(7,"insertion %p in bubble %p done\n",entity,bubble);

	if (entity->type == MA_TASK_ENTITY) {
		marcel_bubble_t *thread_bubble = &ma_task_entity(entity)->sched.internal.bubble;
		if (thread_bubble->sched.init_holder)
			marcel_bubble_insertentity(bubble,ma_entity_bubble(thread_bubble));
	}
	LOG_RETURN(0);
}
#endif

/* Retourne 1 si on doit libérer le sémaphore join. */
int __marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	MA_BUG_ON(entity->init_holder != &bubble->hold);
	entity->init_holder = NULL;
	/* le sched holder pourrait être autre (vol) */
	entity->sched_holder = NULL;
	list_del_init(&entity->bubble_entity_list);
	marcel_barrier_addcount(&bubble->barrier, -1);
	bubble->nbentities--;
	if ((entity)->type == MA_TASK_ENTITY)
		PROF_EVENT2(bubble_sched_remove_thread, (void*)ma_task_entity(entity), bubble);
	else
		PROF_EVENT2(bubble_sched_remove_bubble, (void*)ma_bubble_entity(entity), bubble);

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

	ma_bubble_lock(b);
	ma_set_sched_holder(&b->sched, b);
	ma_bubble_unlock(b);
	LOG_OUT();
	return 0;
}
#endif

void marcel_wake_up_bubble(marcel_bubble_t *bubble) {
	ma_runqueue_t *rq;
	ma_holder_t *h;
	LOG_IN();
	if (!(h = (bubble->sched.sched_holder))) {
		h = ma_task_sched_holder(MARCEL_SELF);
		bubble->sched.sched_holder = bubble->sched.init_holder = h;
	}
	rq = ma_to_rq_holder(h);
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
 *
 * Refermeture de bulle
 *
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
 * Rassembleur d'une hiérarchie de la bulle
 *
 */

void __ma_bubble_gather(marcel_bubble_t *b, marcel_bubble_t *rootbubble) {
	marcel_entity_t *e;
	list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
		int state;

		if (e->type == MA_BUBBLE_ENTITY)
			__ma_bubble_gather(ma_bubble_entity(e), b->sched.sched_holder && b->sched.sched_holder->type == MA_RUNQUEUE_HOLDER ? b : rootbubble);

		if (e->sched_holder == &b->hold || e->sched_holder == &rootbubble->hold)
			/* déjà rassemblé */
			continue;

		state = ma_get_entity(e);
		mdebug("putting back %p in bubble %p(%p)\n", e, b, &b->hold);
		PROF_EVENTSTR(sched_status,"gather: putting back entity");
		ma_put_entity(e, &b->hold, state);

		if (e->type == MA_BUBBLE_ENTITY)
			PROF_EVENT2(bubble_sched_bubble_goingback, ma_bubble_entity(e), b);
		else
			PROF_EVENT2(bubble_sched_goingback, ma_task_entity(e), b);
	}
}

void ma_bubble_gather(marcel_bubble_t *b) {
	ma_bubble_lock_all(b, marcel_machine_level);
	__ma_bubble_gather(b, b);
	PROF_EVENTSTR(sched_status,"gather: done");
	ma_bubble_unlock_all(b, marcel_machine_level);
}

/******************************************************************************
 *
 * Verrouilleur d'une hiérarchie de la bulle
 *
 */

static void __ma_bubble_lock_all(marcel_bubble_t *b, marcel_bubble_t *root_bubble) {
	marcel_entity_t *e;
	if (b->sched.sched_holder == &root_bubble->hold) {
		/* Bubble held in root bubble, just need to lock it and its content */
		ma_holder_rawlock(&b->hold);
		list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_lock_all(ma_bubble_entity(e), root_bubble);
		}
	} else {
		/* Bubble by itself on a runqueue */
		MA_BUG_ON(b->sched.sched_holder->type != MA_RUNQUEUE_HOLDER);
		if (!b->sched.holder_data) {
			/* not queued, hence didn't get locked when running ma_topo_lock() */
			ma_holder_rawlock(&b->hold);
		}
		list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_lock_all(ma_bubble_entity(e), b); /* b is the new root bubble */
		}
	}
}

static void __ma_bubble_unlock_all(marcel_bubble_t *b, marcel_bubble_t *root_bubble) {
	marcel_entity_t *e;
	if (b->sched.sched_holder == &root_bubble->hold) {
		/* Bubble held in root bubble, just need to unlock its content and it */
		list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_unlock_all(ma_bubble_entity(e), root_bubble);
		}
		ma_holder_rawunlock(&b->hold);
	} else {
		list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_unlock_all(ma_bubble_entity(e), b); /* b is the new root bubble */
		}
		/* Bubble by itself on a runqueue */
		MA_BUG_ON(b->sched.sched_holder->type != MA_RUNQUEUE_HOLDER);
		if (!b->sched.holder_data) {
			/* not queued, hence didn't get locked when running ma_topo_lock() */
			ma_holder_rawunlock(&b->hold);
		}
	}
}

static void ma_topo_lock(struct marcel_topo_level *level) {
	struct marcel_topo_level *l;
	marcel_entity_t *e;
	int prio;
	int i;
	/* Lock the runqueue */
	ma_holder_rawlock(&level->sched.hold);
	/* Lock all bubbles queued on that level */
	for (prio = 0; prio < MA_MAX_PRIO; prio++) {
		list_for_each_entry(e, ma_array_queue(level->sched.active, prio), run_list) {
			if (e->type == MA_BUBBLE_ENTITY) {
				ma_holder_rawlock(&ma_bubble_entity(e)->hold);
			}
		}
	}
	for (i=0; i<level->arity; i++) {
		l = level->children[i];
		ma_topo_lock(l);
	}
}

static void ma_topo_unlock(struct marcel_topo_level *level) {
	struct marcel_topo_level *l;
	marcel_entity_t *e;
	int prio;
	int i;

	for (i=0; i<level->arity; i++) {
		l = level->children[i];
		ma_topo_unlock(l);
	}

	/* Unlock all bubbles queued on that level */
	for (prio = 0; prio < MA_MAX_PRIO; prio++) {
		list_for_each_entry(e, ma_array_queue(level->sched.active, prio), run_list) {
			if (e->type == MA_BUBBLE_ENTITY) {
				ma_holder_rawunlock(&ma_bubble_entity(e)->hold);
			}
		}
	}
	/* Now unlock the unqueue */
	ma_holder_rawunlock(&level->sched.hold);
}

void ma_bubble_lock_all(marcel_bubble_t *b, struct marcel_topo_level *level) {
	ma_local_bh_disable();
	ma_preempt_disable();
	ma_topo_lock(level);
	__ma_bubble_lock_all(b,b);
}

void ma_bubble_unlock_all(marcel_bubble_t *b, struct marcel_topo_level *level) {
	__ma_bubble_unlock_all(b,b);
	ma_topo_unlock(level);
	ma_preempt_enable();
	ma_local_bh_enable();
}

static void __ma_bubble_lock(marcel_bubble_t *b) {
	marcel_entity_t *e;
	ma_holder_rawlock(&b->hold);
	list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
		if (e->type == MA_BUBBLE_ENTITY) {
			marcel_bubble_t *b = ma_bubble_entity(e);
			if(b->sched.sched_holder->type != MA_RUNQUEUE_HOLDER)
				__ma_bubble_lock(b);
		}
	}
}

static void __ma_bubble_unlock(marcel_bubble_t *b) {
	marcel_entity_t *e;
	list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
		if (e->type == MA_BUBBLE_ENTITY) {
			marcel_bubble_t *b = ma_bubble_entity(e);
			if(b->sched.sched_holder->type != MA_RUNQUEUE_HOLDER)
				__ma_bubble_unlock(b);
		}
	}
	ma_holder_rawlock(&b->hold);
}

void ma_bubble_lock(marcel_bubble_t *b) {
	ma_local_bh_disable();
	ma_preempt_disable();
	__ma_bubble_lock(b);
}

void ma_bubble_unlock(marcel_bubble_t *b) {
	__ma_bubble_unlock(b);
	ma_preempt_enable();
	ma_local_bh_enable();
}

/******************************************************************************
 *
 * Self-scheduling de base
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
