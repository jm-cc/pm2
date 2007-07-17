
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

ma_bubble_sched_t current_sched = 
#if defined(BUBBLE_SCHED_SPREAD)
#warning "[1;33m<<< [1;37mBubble scheduler [1;32mspread[1;37m selected[1;33m >>>[0m"
	&marcel_bubble_spread_sched;
#elif defined(BUBBLE_SCHED_AFFINITY)
#warning "[1;33m<<< [1;37mBubble scheduler [1;32maffinity[1;37m selected[1;33m >>>[0m"
	&marcel_bubble_affinity_sched;
#elif defined(BUBBLE_SCHED_NULL)
#warning "[1;33m<<< [1;37mBubble scheduler [1;32mnull[1;37m selected[1;33m >>>[0m"
	&marcel_bubble_null_sched;
#elif defined(BUBBLE_SCHED_EXPLODE)
#warning "[1;33m<<< [1;37mBubble scheduler [1;32mexplode[1;37m selected[1;33m >>>[0m"
	&marcel_bubble_explode_sched;
#elif defined(BUBBLE_SCHED_STEAL)
#warning "[1;33m<<< [1;37mBubble scheduler [1;32msteal[1;37m selected[1;33m >>>[0m"
	&marcel_bubble_steal_sched;
#elif defined(BUBBLE_SCHED_GANG)
#warning "[1;33m<<< [1;37mBubble scheduler [1;32mgang[1;37m selected[1;33m >>>[0m"
	&marcel_bubble_gang_sched;
#else
#error "No chosen bubble scheduler!"
#endif

static marcel_mutex_t current_sched_mutex = MARCEL_MUTEX_INITIALIZER;

int marcel_bubble_init(marcel_bubble_t *bubble) {
	PROF_EVENT1(bubble_sched_new,bubble);
	*bubble = (marcel_bubble_t) MARCEL_BUBBLE_INITIALIZER(*bubble);
	ma_stats_reset(&bubble->sched);
	ma_stats_reset(&bubble->hold);
	PROF_EVENT2(sched_setprio,bubble,bubble->sched.prio);
	return 0;
}

void __marcel_init __ma_bubble_sched_start(void) {
	marcel_mutex_lock(&current_sched_mutex);
	if (current_sched->start)
		current_sched->start();
	if (current_sched->submit)
	  current_sched->submit(&marcel_root_bubble.sched);
	marcel_mutex_unlock(&current_sched_mutex);
}

marcel_bubble_sched_t *marcel_bubble_change_sched(marcel_bubble_sched_t *new_sched) {
	ma_bubble_sched_t old;
	marcel_mutex_lock(&current_sched_mutex);
	old = current_sched;
	if (old->exit)
		old->exit();
	current_sched = new_sched;
	if (new_sched->init)
		new_sched->init();
	if (new_sched->start)
		new_sched->start();
	marcel_mutex_unlock(&current_sched_mutex);
	return old;
}

int marcel_bubble_setid(marcel_bubble_t *bubble, int id) {
	PROF_EVENT2(bubble_setid,bubble, id);
	return 0;
}

int marcel_bubble_setinitrq(marcel_bubble_t *bubble, ma_runqueue_t *rq) {
	bubble->sched.sched_holder = &rq->hold;
	return 0;
}

int marcel_bubble_setinitlevel(marcel_bubble_t *bubble, marcel_topo_level_t *level) {
	marcel_bubble_setinitrq(bubble, &level->sched);
	return 0;
}

int marcel_bubble_setinithere(marcel_bubble_t *bubble) {
	bubble->sched.sched_holder = SELF_GETMEM(sched.internal.entity.sched_holder);
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
	h = ma_bubble_holder_rawlock(bubble); \
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
	ma_bubble_holder_unlock_softirq(h);
	return 0;
}

int marcel_bubble_setprio_locked(marcel_bubble_t *bubble, int prio) {
	VARS;
	if (prio == bubble->sched.prio) return 0;
	HOLDER();
	SETPRIO(prio);
	return 0;
}

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

int
marcel_bubble_submit()
{
  if (current_sched)
    if (current_sched->submit)
      current_sched->submit(&marcel_root_bubble.sched);

  return 0;
}

/******************************************************************************
 *
 * Insertion dans une bulle
 *
 */

void TBX_EXTERN ma_set_sched_holder(marcel_entity_t *e, marcel_bubble_t *bubble, int do_lock) {
	marcel_entity_t *ee;
	marcel_bubble_t *b;
	ma_holder_t *h;
	bubble_sched_debugl(7,"ma_set_sched_holder %p to bubble %p\n",e,bubble);
	h = e->sched_holder;
	e->sched_holder = &bubble->hold;
	if (e->type != MA_BUBBLE_ENTITY) {
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
		b = ma_bubble_entity(e);
		if (do_lock)
			ma_holder_rawlock(&b->hold);
		list_for_each_entry(ee, &b->heldentities, bubble_entity_list) {
			if (ee->sched_holder && ee->sched_holder->type == MA_BUBBLE_HOLDER)
				ma_set_sched_holder(ee, bubble, do_lock);
		if (do_lock)
			ma_holder_rawunlock(&b->hold);
		}
	}
}

/* suppose bubble verrouillée avec softirq */
static int __do_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	int ret = 1;
	/* XXX: will sleep (hence abort) if the bubble was joined ! */
	if (!bubble->nbentities) {
		MA_BUG_ON(!list_empty(&bubble->heldentities));
		marcel_sem_P(&bubble->join);
	}

	ma_holder_lock_softirq(&bubble->hold);

	//bubble_sched_debugl(7,"__inserting %p in opened bubble %p\n",entity,bubble);
	if (entity->type == MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_insert_bubble,ma_bubble_entity(entity),bubble);
	else
		PROF_EVENT2(bubble_sched_insert_thread,ma_task_entity(entity),bubble);
	list_add_tail(&entity->bubble_entity_list, &bubble->heldentities);
	marcel_barrier_addcount(&bubble->barrier, 1);
	bubble->nbentities++;
	entity->init_holder = &bubble->hold;
	if (!entity->sched_holder || entity->sched_holder->type == MA_BUBBLE_ENTITY) {
		ma_holder_t *sched_bubble = bubble->sched.sched_holder;
		/* si la bulle conteneuse est dans une autre bulle,
		 * on hérite de la bulle d'ordonnancement */
		if (!sched_bubble || ma_holder_type(sched_bubble) == MA_RUNQUEUE_HOLDER) {
			/* Sinon, c'est la conteneuse qui sert de bulle d'ordonnancement */
			sched_bubble = &bubble->hold;
		}
		entity->sched_holder = sched_bubble;
		ret = 0;
	}
	ma_holder_unlock_softirq(&bubble->hold);
	return ret;
}

int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	LOG_IN();

	if (!list_empty(&entity->bubble_entity_list)) {
		ma_holder_t *h = entity->init_holder;
		if (ma_bubble_holder(h) == bubble)
			LOG_RETURN(0);
		MA_BUG_ON(h->type == MA_RUNQUEUE_HOLDER);
		marcel_bubble_removeentity(ma_bubble_holder(h),entity);
	}

	bubble_sched_debugl(7,"inserting %p in bubble %p\n",entity,bubble);
	if (__do_bubble_insertentity(bubble,entity) && entity->type == MA_BUBBLE_ENTITY && entity->sched_holder->type == MA_RUNQUEUE_HOLDER) {
		/* sched holder was already set to something else, wake the bubble there */
		ma_holder_lock_softirq(entity->sched_holder);
		ma_activate_entity(entity, entity->sched_holder);
		PROF_EVENT2(bubble_sched_switchrq, ma_bubble_entity(entity), ma_rq_holder(entity->sched_holder));
		ma_holder_unlock_softirq(entity->sched_holder);
	}
	bubble_sched_debugl(7,"insertion %p in bubble %p done\n",entity,bubble);

	if (entity->type == MA_THREAD_ENTITY) {
		marcel_bubble_t *thread_bubble = &ma_task_entity(entity)->sched.internal.bubble;
		if (thread_bubble->sched.init_holder)
			marcel_bubble_insertentity(bubble,ma_entity_bubble(thread_bubble));
	}
	LOG_RETURN(0);
}

int marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	int res;
	LOG_IN();

	/* Remove entity from bubble */
	ma_holder_lock_softirq(&bubble->hold);
	MA_BUG_ON(entity->init_holder != &bubble->hold);
	entity->init_holder = NULL;
	entity->sched_holder = NULL;
	list_del_init(&entity->bubble_entity_list);
	marcel_barrier_addcount(&bubble->barrier, -1);
	res = (!--bubble->nbentities);
	if ((entity)->type != MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_remove_thread, (void*)ma_task_entity(entity), bubble);
	else
		PROF_EVENT2(bubble_sched_remove_bubble, (void*)ma_bubble_entity(entity), bubble);
	ma_holder_rawunlock(&bubble->hold);

	ma_holder_t *h, *new_holder;
	/* Before announcing it, remove it scheduling-wise too */
	new_holder = bubble->sched.sched_holder;
	if (new_holder == &bubble->hold)
		/* TODO: que faire d'autre ? (cas d'une bulle et ses entités
		 * non schedulées sur une runqueue...) */
		new_holder = &ma_lwp_vprq(LWP_SELF)->hold;
	h = ma_entity_holder_rawlock(entity);
	if (h == &bubble->hold) {
		/* we need to get this entity out of this bubble */
		int state = ma_get_entity(entity);
		ma_holder_rawunlock(h);
		ma_holder_rawlock(new_holder);
		ma_put_entity(entity, new_holder, state);
		ma_holder_unlock_softirq(new_holder);
	} else
		/* already out from the bubble, that's ok.  */
		ma_entity_holder_unlock_softirq(h);
	if (res)
		marcel_sem_V(&bubble->join);
	LOG_OUT();
	return 0;
}

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
	ma_set_sched_holder(&b->sched, b, 1);
	ma_bubble_unlock(b);
	LOG_OUT();
	return 0;
}

void marcel_wake_up_bubble(marcel_bubble_t *bubble) {
	ma_holder_t *h;
	LOG_IN();
	/* If no initial runqueue was specified, use the current one */
	if (!(h = (bubble->sched.sched_holder))) {
		h = ma_task_sched_holder(MARCEL_SELF);
		while (h->type == MA_BUBBLE_HOLDER) {
			marcel_bubble_t *b = ma_bubble_holder(h);
			h = b->sched.sched_holder;
		}
		bubble->sched.sched_holder = h;
	}
	ma_holder_lock_softirq(h);
	bubble_sched_debug("waking up bubble %p in holder %p\n",bubble,h);
	if (h->type == MA_BUBBLE_HOLDER) {
		PROF_EVENT2(bubble_sched_bubble_goingback,bubble,ma_bubble_holder(h));
		ma_set_sched_holder(&bubble->sched,ma_bubble_holder(h),1);
	} else {
		PROF_EVENT2(bubble_sched_wake,bubble,ma_rq_holder(h));
		ma_activate_entity(&bubble->sched,h);
	}
	ma_holder_unlock_softirq(h);
	ma_top_add_bubble(bubble);
	if (current_sched->submit)
	  current_sched->submit(&bubble->sched);
	LOG_OUT();
}

void marcel_bubble_join(marcel_bubble_t *bubble) {
	ma_holder_t *h;
	LOG_IN();
	marcel_sem_P(&bubble->join);
	h = ma_bubble_holder_lock_softirq(bubble);
	MA_BUG_ON(bubble->nbentities);
	MA_BUG_ON(!list_empty(&bubble->heldentities));
	if (bubble->sched.run_holder) {
		if (bubble->sched.holder_data)
			ma_dequeue_entity(&bubble->sched, h);
		ma_deactivate_running_entity(&bubble->sched, h);
	}
	ma_bubble_holder_unlock_softirq(h);
	if ((h = bubble->sched.init_holder)
		&& h->type == MA_BUBBLE_HOLDER
		&& h != &bubble->hold)
		marcel_bubble_removeentity(ma_bubble_holder(h), &bubble->sched);
	ma_top_del_bubble(bubble);
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
		/* Bubble by itself on a runqueue. If that's not the case, you probably forgot to call ma_put_entity at some point. */
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
		/* Bubble by itself on a runqueue. If that's not the case, you probably forgot to call ma_put_entity at some point. */
		MA_BUG_ON(b->sched.sched_holder->type != MA_RUNQUEUE_HOLDER);
		if (!b->sched.holder_data) {
			/* not queued, hence won't get unlocked when running ma_topo_unlock() */
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
	/* We can only lock a bubble when it is alone or scheduled on a runqueue.
	 * The best way to make sure of this is to only call lock_all on a root bubble. */
	MA_BUG_ON(b->sched.sched_holder != &b->hold && b->sched.sched_holder->type != MA_RUNQUEUE_HOLDER);
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
	if (tbx_unlikely(nexthead == &curb->queuedentities)) {
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

	if (tbx_likely(next->type != MA_BUBBLE_ENTITY))
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

/* on détient le verrou du holder de nextent */
marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_runqueue_t *rq, ma_holder_t **nexth, int idx) {
	LOG_IN();

	if (current_sched->sched) {
		marcel_entity_t *next = current_sched->sched(nextent, rq, nexth, idx);
		if (!next)
			/* Bubble scheduler messed it up, restart */
			LOG_RETURN(NULL);
	}

	if (nextent->type != MA_BUBBLE_ENTITY)
		LOG_RETURN(nextent);

/*
 * This is a bubble
 */
	marcel_bubble_t *bubble = ma_bubble_entity(nextent);
	sched_debug("locking bubble %p\n",bubble);
	ma_holder_rawlock(&bubble->hold);

	/* We generally manage to avoid this */
	if (list_empty(&bubble->queuedentities)) {
		bubble_sched_debug("warning: bubble %p empty\n", bubble);
		/* We shouldn't ever schedule a NOSCHED bubble */
 		MA_BUG_ON(bubble->sched.prio == MA_NOSCHED_PRIO);
		if (bubble->sched.holder_data)
			ma_rq_dequeue_entity(&bubble->sched, rq);
		sched_debug("unlock(%p)\n", rq);
		ma_holder_rawunlock(&rq->hold);
		ma_holder_unlock(&bubble->hold);
		LOG_RETURN(NULL);
	}

	/* TODO: comment choisir de l'activer ou non ? */
	if (0 && bubble->num_schedules >= bubble->hold.nr_ready) {
		/* we expired our threads, let others execute */
		bubble->num_schedules = 0;
		if (bubble->sched.holder_data) {
			/* by putting ourselves at the end of the list */
			ma_rq_dequeue_entity(&bubble->sched, rq);
			ma_rq_enqueue_entity(&bubble->sched, rq);
		}
		ma_holder_rawunlock(&rq->hold);
		ma_holder_unlock(&bubble->hold);
		LOG_RETURN(NULL);
	}

	bubble->num_schedules++;

	nextent = list_entry(bubble->queuedentities.next, marcel_entity_t, run_list);
	bubble_sched_debugl(7,"next entity to run %p\n",nextent);
	MA_BUG_ON(nextent->type == MA_BUBBLE_ENTITY);

	sched_debug("unlock(%p)\n", rq);
	ma_holder_rawunlock(&rq->hold);
	*nexth = &bubble->hold;
	LOG_RETURN(nextent);
}

/******************************************************************************
 *
 * Initialisation
 *
 */

static void __marcel_init bubble_sched_init() {
	marcel_root_bubble.sched.sched_holder = &ma_main_runqueue.hold;
	ma_activate_entity(&marcel_root_bubble.sched, &ma_main_runqueue.hold);
	PROF_EVENT2(bubble_sched_switchrq, &marcel_root_bubble, &ma_main_runqueue);
}

void ma_bubble_sched_init2(void) {
	/* Having main on the main runqueue is both faster and respects priorities */
	ma_deactivate_running_entity(&MARCEL_SELF->sched.internal.entity, &marcel_root_bubble.hold);
	SELF_GETMEM(sched.internal.entity.sched_holder) = &ma_main_runqueue.hold;
	PROF_EVENT2(bubble_sched_switchrq, MARCEL_SELF, &ma_main_runqueue);
	ma_activate_running_entity(&MARCEL_SELF->sched.internal.entity, &ma_main_runqueue.hold);

	marcel_mutex_lock(&current_sched_mutex);
	if (current_sched->init)
		current_sched->init();
	marcel_mutex_unlock(&current_sched_mutex);
}

__ma_initfunc_prio(bubble_sched_init, MA_INIT_BUBBLE_SCHED, MA_INIT_BUBBLE_SCHED_PRIO, "Bubble Scheduler");
#endif /* MA__BUBBLES */
