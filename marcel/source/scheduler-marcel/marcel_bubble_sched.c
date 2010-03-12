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

ma_atomic_t ma_idle_scheduler = MA_ATOMIC_INIT(0);
ma_spinlock_t ma_idle_scheduler_lock = MA_SPIN_LOCK_UNLOCKED;
#ifdef MA__BUBBLES
marcel_bubble_t marcel_root_bubble = MARCEL_BUBBLE_INITIALIZER(marcel_root_bubble);

/** \brief The current bubble scheduler */
static marcel_bubble_sched_t *current_sched = &marcel_bubble_null_sched;

/** \brief Mutex protecting concurrent accesses to \e current_sched.  */
static marcel_mutex_t current_sched_mutex = MARCEL_MUTEX_INITIALIZER;

int marcel_bubble_init(marcel_bubble_t *bubble) {
	PROF_EVENT1(bubble_sched_new,bubble);
	*bubble = (marcel_bubble_t) MARCEL_BUBBLE_INITIALIZER(*bubble);
	/* TODO: add logic for naming bubbles usefully */
	snprintf(bubble->as_holder.name,MARCEL_MAXNAMESIZE,"bubble_h %p", bubble);
	snprintf(bubble->as_entity.name,MARCEL_MAXNAMESIZE,"bubble_e %p", bubble);
	sched_debug("bubble %p: as_entity = %p, as_holder = %p\n", bubble, &bubble->as_entity, &bubble->as_holder);
	ma_stats_reset(&bubble->as_entity);
	ma_stats_reset(&bubble->as_holder);
	PROF_EVENT2(sched_setprio,bubble,bubble->as_entity.prio);
	return 0;
}

void __marcel_init __ma_bubble_sched_start(void) {
	marcel_mutex_lock(&current_sched_mutex);
	if (current_sched->start)
		current_sched->start(current_sched);
	//if (current_sched->submit)
	//current_sched->submit(&marcel_root_bubble.as_entity);
	ma_bubble_gather(&marcel_root_bubble);
	ma_holder_t *h = ma_bubble_holder_lock_softirq(&marcel_root_bubble);
	ma_bubble_lock(&marcel_root_bubble);
	int state = ma_get_entity(&marcel_root_bubble.as_entity);
	ma_bubble_unlock(&marcel_root_bubble);
	ma_holder_rawunlock(h);
	h = &(&marcel_topo_vp_level[0])->rq.as_holder;
	ma_holder_rawlock(h);
	ma_put_entity(&marcel_root_bubble.as_entity, h, state);
	ma_holder_unlock_softirq(h);
	
	marcel_mutex_unlock(&current_sched_mutex);
}

marcel_bubble_sched_t *marcel_bubble_current_sched (void) {
	marcel_bubble_sched_t *sched;
	marcel_mutex_lock(&current_sched_mutex);
	sched = current_sched;
	marcel_mutex_unlock(&current_sched_mutex);
	return sched;
}

marcel_bubble_sched_t *marcel_bubble_change_sched(marcel_bubble_sched_t *new_sched) {
	marcel_bubble_sched_t *old;
	marcel_mutex_lock(&current_sched_mutex);
	old = current_sched;
	ma_bubble_exit();
	current_sched = new_sched;
	if (new_sched->init)
		new_sched->init(new_sched);
	if (new_sched->start)
		new_sched->start(new_sched);
	marcel_mutex_unlock(&current_sched_mutex);
	return old;
}

/* This function is meant to be called before Marcel is fully initialized,
 * e.g., while command-line options are being parsed.  */
marcel_bubble_sched_t *marcel_bubble_set_sched(marcel_bubble_sched_t *new_sched) {
	marcel_bubble_sched_t *old;
	old = current_sched;
	current_sched = new_sched;
	return old;
}

/* Turns idle scheduler on. This function returns 1 if an idle
   scheduler was already running. */
int
ma_activate_idle_scheduler (void) {
  int ret = 1;
  if (!ma_idle_scheduler_is_running ()) {
    ma_spin_lock (&ma_idle_scheduler_lock);
    if (!ma_idle_scheduler_is_running ()) {
      ma_atomic_inc (&ma_idle_scheduler);
      ret = 0;
    }
    ma_spin_unlock (&ma_idle_scheduler_lock);
  }
  return ret;
}

/* Turns idle scheduler off. This function returns 1 if no idle
   scheduler was previously running. */
int
ma_deactivate_idle_scheduler (void) {
  int ret = 1;
  if (ma_idle_scheduler_is_running ()) {
    ma_spin_lock (&ma_idle_scheduler_lock);
    if (ma_idle_scheduler_is_running ()) {
      ma_atomic_dec (&ma_idle_scheduler);
      ret = 0;
    }
    ma_spin_unlock (&ma_idle_scheduler_lock);
  }
  return ret;
}

/* Checks whether an idle scheduler is currently running. */
int
ma_idle_scheduler_is_running (void) {
  return ma_atomic_read (&ma_idle_scheduler);
}

void ma_bubble_move_top_and_submit (marcel_bubble_t *b) {
  ma_deactivate_idle_scheduler ();
  
  ma_bubble_gather (b);

  ma_local_bh_disable ();
  ma_preempt_disable ();

  /* TODO: Only lock what needs to be locked! */
  ma_bubble_lock_all (b, marcel_topo_level (0, 0));
  ma_move_entity (&b->as_entity, &marcel_topo_level(0,0)->rq.as_holder);
  /* TODO: Only unlock what needs to be unlocked! */
  ma_bubble_unlock_all (b, marcel_topo_level (0, 0));
  marcel_bubble_submit (b);

  ma_preempt_enable_no_resched ();
  ma_local_bh_enable ();
  
  ma_activate_idle_scheduler ();  
}

/* Application is entering steady state, let's start
   thread/bubble distribution and active work stealing
   algorithm. */ 
void marcel_bubble_sched_begin (void) {
  ma_bubble_move_top_and_submit (&marcel_root_bubble);
}

/* Calls the `submit' function of the current bubble scheduler. */
int marcel_bubble_submit (marcel_bubble_t *b) {
  if (current_sched) {
    if (current_sched->submit) {
      current_sched->submit (current_sched, &b->as_entity);
      return 0;
    }
  }
  return 1;
}

static int ma_entity_is_in_sched_scope (marcel_bubble_sched_t *sched, marcel_entity_t *e) {
  struct marcel_topo_level *sched_root_level = sched->root_level;
  struct marcel_topo_level *entity_level = ma_get_parent_rq (e)->topolevel;

  return ma_topo_is_in_subtree (sched_root_level, entity_level);
}

/* Calls the `submit' function of the bubble scheduler passed as argument. */
int marcel_bubble_submit_to_sched (marcel_bubble_sched_t *sched, marcel_bubble_t *b) {
  MA_BUG_ON (!sched->root_level);

  ma_bubble_gather (b);

  ma_local_bh_disable ();
  ma_preempt_disable ();

  /* Check whether bubble _b_ lies in the topology subtree handled by
     scheduler _sched_. */
  if (!ma_entity_is_in_sched_scope (sched, &b->as_entity)) {
    ma_bubble_lock_all (b, marcel_topo_level (0, 0));
    ma_move_entity (&b->as_entity, &sched->root_level->rq.as_holder);
    ma_bubble_unlock_all (b, marcel_topo_level (0, 0));  
  }
  
  if (sched->submit) {
    sched->submit (sched, &b->as_entity);
    
    ma_preempt_enable_no_resched ();
    ma_local_bh_enable ();
    
    ma_resched_bubble_contents (b);
    
    return 0;
  }

  ma_preempt_enable_no_resched ();
  ma_local_bh_enable ();

  return 1;  
}

/* Application is entering ending state, let's prevent idle
   schedulers from stealing anything. */ 
void marcel_bubble_sched_end (void) {
	/* FIXME: explain why marcel_bubble_sched_begin does not perform a 
	 * corresponding call to ma_activate_idle_scheduler */
  ma_deactivate_idle_scheduler ();
}

/* Explicit way of asking the underlying bubble scheduler to
   distribute threads and bubbles from scratch. */
void marcel_bubble_shake (void) {
  /* First try to call the bubble scheduler specific `shake'
     function. */
  if (current_sched) {
    if (current_sched->shake) {
      current_sched->shake (current_sched);
    } else {
      /* Default behavior for shake (). */
      ma_bubble_move_top_and_submit (&marcel_root_bubble);
    }
  }
}

int ma_bubble_notify_idle_vp(unsigned int vp) {
	int ret;
	if (current_sched->vp_is_idle)
		ret = current_sched->vp_is_idle(current_sched, vp);
	else
		ret = 0;
	return ret;
}

int ma_bubble_tick(marcel_bubble_t *bubble) {
	int ret;
	if (current_sched->tick)
		ret = current_sched->tick(current_sched, bubble);
	else
		ret = 0;
	return ret;
}

int ma_bubble_exit(void) {
	int ret;
	if (current_sched->exit)
		ret = current_sched->exit(current_sched);
	else
		ret = 0;
	return ret;
}


int marcel_bubble_setid(marcel_bubble_t *bubble, int id) {
	PROF_EVENT2(bubble_setid,bubble, id);
	bubble->id = id;
	return 0;
}

/* Temporarily bring the bubble on the runqueue RQ by setting its sched_holder accordingly.
 * . The bubble's natural holder is left unchanged.
 * . The actual move is deferred until the bubble is woken up. */
int marcel_bubble_scheduleonrq(marcel_bubble_t *bubble, ma_runqueue_t *rq) {
	bubble->as_entity.sched_holder = &rq->as_holder;
	return 0;
}

/* Temporarily bring the bubble on level LEVEL's runqueue by setting its sched_holder accordingly.
 * . The bubble's natural holder is left unchanged.
 * . The actual move is deferred until the bubble is woken up. */
int marcel_bubble_scheduleonlevel(marcel_bubble_t *bubble, marcel_topo_level_t *level) {
	marcel_bubble_scheduleonrq(bubble, &level->rq);
	return 0;
}

/* Temporarily bring the bubble on the current thread's holder by setting its sched_holder accordingly.
 * Inheritence occurs only if the thread sched holder is a runqueue.
 * . The bubble's natural holder is left unchanged.
 * . The actual move is deferred until the bubble is woken up. */
int marcel_bubble_scheduleonthreadholder(marcel_bubble_t *bubble) {
	ma_holder_t *h = SELF_GETMEM(as_entity.sched_holder);
	
	MA_BUG_ON(!h);
	if (h && ma_holder_type(h) == MA_RUNQUEUE_HOLDER) {
		// hériter du holder du thread courant
		PROF_EVENTSTR(sched_status, "inherit current thread sched holder");
		bubble->as_entity.sched_holder = h;
	}
	PROF_EVENTSTR(sched_status, "do not inherit current thread sched holder");
	return 0;
}


/** \brief Sets the "scheduling level" of entity \e entity to \e level, i.e.
 * the depth (within the machine's topology) where it should be scheduled or
 * burst. This information is used by the Burst Scheduler.
 */
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
	h && ma_holder_type(h) == MA_RUNQUEUE_HOLDER && bubble->as_entity.ready_holder && bubble->as_entity.ready_holder_data
#define RAWLOCK_HOLDER() \
	h = ma_bubble_holder_rawlock(bubble); \
	running = RUNNING()
#define HOLDER() \
	h = bubble->as_entity.ready_holder; \
	running = RUNNING()
#define SETPRIO(_prio); \
	if (running) \
		ma_rq_dequeue_entity(&bubble->as_entity, ma_rq_holder(h)); \
	PROF_EVENT2(sched_setprio,bubble,_prio); \
	MA_BUG_ON(bubble->as_entity.prio == _prio); \
	bubble->as_entity.prio = _prio; \
	if (running) \
		ma_rq_enqueue_entity(&bubble->as_entity, ma_rq_holder(h));

int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio) {
	VARS;
	if (prio == bubble->as_entity.prio) return 0;
	ma_preempt_disable();
	ma_local_bh_disable();
	RAWLOCK_HOLDER();
	SETPRIO(prio);
	ma_bubble_holder_unlock_softirq(h);
	return 0;
}

int marcel_bubble_setprio_locked(marcel_bubble_t *bubble, int prio) {
	VARS;
	if (prio == bubble->as_entity.prio) return 0;
	HOLDER();
	SETPRIO(prio);
	return 0;
}

int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio) {
	*prio = bubble->as_entity.prio;
	return 0;
}

marcel_bubble_t *marcel_bubble_holding_entity(marcel_entity_t *e) {
	ma_holder_t *h = e->natural_holder;
	if (!h || h->type != MA_BUBBLE_HOLDER) {
		h = e->sched_holder;
		if (!h || ma_holder_type(h) != MA_BUBBLE_HOLDER)
			h = &marcel_root_bubble.as_holder;
	}
	bubble_sched_debugl(7,"entity %p is held by bubble %p\n", e, ma_bubble_holder(h));
	return ma_bubble_holder(h);
}

/******************************************************************************
 *
 * Insertion dans une bulle
 *
 */

/* recursively sets the sched holder of an entity tree to bubble. if the entity
 * tree is not already locked, give 1 as do_lock.
 * the target bubble (which must be a top bubble) must be locked as well as its holding runqueue (if any) */
void TBX_EXTERN ma_set_sched_holder(marcel_entity_t *e, marcel_bubble_t *bubble, int do_lock) {
	marcel_entity_t *ee;
	marcel_bubble_t *b;
	ma_holder_t *h;
	MA_BUG_ON(!ma_holder_check_locked(&bubble->as_holder));
	
	bubble_sched_debugl(7,"ma_set_sched_holder %p to bubble %p\n",e,bubble);
	e->sched_holder = &bubble->as_holder;
	if (e->type != MA_BUBBLE_ENTITY) {
		if ((h = e->ready_holder) && h != &bubble->as_holder) {
			/* already enqueued */
			/* Ici, on suppose que h est déjà verrouillé ainsi que
			 * sa runqueue, et la runqueue de la bulle */
			if (!(e->ready_holder_data)) {
				/* and already running ! */
				ma_clear_ready_holder(e,h);
				ma_set_ready_holder(e,&bubble->as_holder);
			} else {
				if (ma_holder_type(h) == MA_BUBBLE_HOLDER)
					__ma_bubble_dequeue_entity(e,ma_bubble_holder(h));
				else
					ma_rq_dequeue_entity(e,ma_rq_holder(h));
				ma_clear_ready_holder(e,h);
				ma_set_ready_holder(e,&bubble->as_holder);
				/* Ici, on suppose que la runqueue de bubble
				 * est déjà verrouillée */
				__ma_bubble_enqueue_entity(e,bubble);
			}
		}
	} else {
		b = ma_bubble_entity(e);

		if (do_lock)
			ma_holder_rawlock(&b->as_holder);

		tbx_fast_list_for_each_entry(ee, &b->natural_entities, natural_entities_item) {
			if (ee->sched_holder && ee->sched_holder->type == MA_BUBBLE_HOLDER)
				ma_set_sched_holder(ee, bubble, do_lock);
		if (do_lock)
			ma_holder_rawunlock(&b->as_holder);
		}
	}
}

static void ma_bubble_moveentity(marcel_bubble_t *dst_bubble, marcel_entity_t *entity) {
	/* Basically does a remove together with an insert but without using an
	 * intermediate runqueue */
	marcel_bubble_t *src_bubble;
	int src_bubble_becomes_empty;
	int dst_bubble_was_empty = 0;

	MA_BUG_ON(entity->natural_holder->type != MA_BUBBLE_HOLDER);
	src_bubble = ma_bubble_holder(entity->natural_holder);
	if (src_bubble == dst_bubble)
		return;

	/* remove entity from bubble src */
	ma_holder_lock_softirq(&src_bubble->as_holder);
	MA_BUG_ON(entity->natural_holder != &src_bubble->as_holder);
	entity->natural_holder = NULL;
	entity->sched_holder = NULL;
	tbx_fast_list_del_init(&entity->natural_entities_item);
	marcel_barrier_addcount(&src_bubble->barrier, -1);
	src_bubble_becomes_empty = (!--src_bubble->nb_natural_entities);
	if (entity->type == MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_remove_bubble,ma_bubble_entity(entity),src_bubble);
	else
		PROF_EVENT2(bubble_sched_remove_thread,ma_task_entity(entity),src_bubble);
	ma_holder_rawunlock(&src_bubble->as_holder);

	ma_holder_rawlock(&dst_bubble->as_holder);

	/* insert entity in bubble dst */
	if (!dst_bubble->nb_natural_entities) {
		MA_BUG_ON(!tbx_fast_list_empty(&dst_bubble->natural_entities));
		dst_bubble_was_empty = 1;
	}

	if (entity->type == MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_insert_bubble,ma_bubble_entity(entity),dst_bubble);
	else
		PROF_EVENT2(bubble_sched_insert_thread,ma_task_entity(entity),dst_bubble);

	/* change entity natural_holder */
	tbx_fast_list_add_tail(&entity->natural_entities_item, &dst_bubble->natural_entities);
	marcel_barrier_addcount(&dst_bubble->barrier, 1);
	dst_bubble->nb_natural_entities++;
	entity->natural_holder = &dst_bubble->as_holder;

	/* change entity sched_holder */
	ma_holder_t *sched_bubble_h = dst_bubble->as_entity.sched_holder;
	PROF_EVENTSTR(sched_status, "heriter du sched_holder de la bulle ou on insere");
	if (!sched_bubble_h || ma_holder_type(sched_bubble_h) == MA_RUNQUEUE_HOLDER) {
		PROF_EVENTSTR(sched_status, "c'est la bulle ou on insere qui sert d'ordonnancement");
		sched_bubble_h = &dst_bubble->as_holder;
	}
	entity->sched_holder = sched_bubble_h;
	ma_holder_rawunlock(&dst_bubble->as_holder);

	/* change entity ready_holder too if needed */
	ma_holder_t *src_entity_h = ma_entity_holder_rawlock(entity);
	if (src_entity_h == &src_bubble->as_holder) {
		/* dequeue the entity from its source bubble */
		int state = ma_get_entity(entity);
		ma_holder_rawunlock(src_entity_h);

		/* find the top of the bubble hierarchy and the containing runqueue */
		ma_holder_t *top_sched_bubble_h;
		ma_holder_t *rq_h;
		for (;;) {
			/* take a transient snapshot of the containing runqueue and the key bubbles in the hierarchy */
			ma_holder_rawlock(entity->natural_holder);
			top_sched_bubble_h = sched_bubble_h;
			while (ma_bubble_holder(top_sched_bubble_h)->as_entity.sched_holder->type != MA_RUNQUEUE_HOLDER) {
				top_sched_bubble_h = ma_bubble_holder(top_sched_bubble_h)->as_entity.sched_holder;
			}
			rq_h = ma_bubble_holder(top_sched_bubble_h)->as_entity.sched_holder;
			ma_holder_rawunlock(entity->natural_holder);

			/* lock the containing runqueue and the key bubbles in the hierarchy */
			ma_holder_rawlock(rq_h);
			ma_holder_rawlock(top_sched_bubble_h);
			if (sched_bubble_h != top_sched_bubble_h)
				ma_holder_rawlock(sched_bubble_h);

			/* situation may have changed in-between 1) the locking
			 * block right above where we looked for the top bubble
			 * and runqueue, and 2) the current locking block
			 *
			 * if the situation is the same, leave the loop and go
			 * on with moving the entity
			 *
			 * if the situation is not the same anymore, unlock
			 * everything, and start-over until whatever comes first between
			 * a stable situation or the end of time :-)
			 */
			if ((&ma_to_rq_holder(sched_bubble_h)->as_holder == rq_h))
				break;

			if (sched_bubble_h != top_sched_bubble_h)
				ma_holder_rawunlock(sched_bubble_h);
			ma_holder_rawunlock(top_sched_bubble_h);
			ma_holder_rawunlock(rq_h);
		}

		/* now put entity on the natural holder */
		ma_put_entity(entity, entity->natural_holder, state);

		if (sched_bubble_h != top_sched_bubble_h)
			ma_holder_rawunlock(sched_bubble_h);
		ma_holder_rawunlock(top_sched_bubble_h);
		ma_holder_unlock_softirq(rq_h);
	} else
		/* already out from the bubble, that's ok.  */
		ma_entity_holder_unlock_softirq(src_entity_h);

	if (dst_bubble_was_empty) {
		marcel_mutex_lock(&dst_bubble->join_mutex);
		ma_holder_lock_softirq(&dst_bubble->as_holder);
		if (dst_bubble->join_empty_state == 1  &&  dst_bubble->nb_natural_entities == 1) {
			dst_bubble->join_empty_state = 0;
		}
		ma_holder_unlock_softirq(&dst_bubble->as_holder);
		marcel_mutex_unlock(&dst_bubble->join_mutex);
	}
	/* signal src bubble emptiness when applicable */
	if (src_bubble_becomes_empty) {
		marcel_mutex_lock(&src_bubble->join_mutex);
		ma_holder_lock_softirq(&src_bubble->as_holder);
		if (src_bubble->join_empty_state == 0  &&  src_bubble->nb_natural_entities == 0) {
			src_bubble->join_empty_state = 1;
			marcel_cond_signal(&src_bubble->join_cond);
		}
		ma_holder_unlock_softirq(&src_bubble->as_holder);
		marcel_mutex_unlock(&src_bubble->join_mutex);
	}
}

static int __do_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	int ret = 1;
	int bubble_was_empty = 0;

	ma_holder_lock_softirq(&bubble->as_holder);

	/* XXX: will sleep (hence abort) if the bubble was joined ! */
	if (!bubble->nb_natural_entities) {
		MA_BUG_ON(!tbx_fast_list_empty(&bubble->natural_entities));
		bubble_was_empty = 1;
	}

	//bubble_sched_debugl(7,"__inserting %p in opened bubble %p\n",entity,bubble);
	if (entity->type == MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_insert_bubble,ma_bubble_entity(entity),bubble);
	else
		PROF_EVENT2(bubble_sched_insert_thread,ma_task_entity(entity),bubble);
	tbx_fast_list_add_tail(&entity->natural_entities_item, &bubble->natural_entities);
	marcel_barrier_addcount(&bubble->barrier, 1);
	bubble->nb_natural_entities++;
	entity->natural_holder = &bubble->as_holder;
	/* If an entity is already scheduled on some runqueue, we do not
	 * reconsider that schedule, we only record the relation with the
	 * bubble.
	 *
	 * TODO: maybe we could call the bubble scheduler to notify the
	 * insertion in a bubble.
	 */
	 if (!entity->sched_holder || entity->sched_holder->type == MA_BUBBLE_HOLDER) {
		ma_holder_t *sched_bubble = bubble->as_entity.sched_holder;
		PROF_EVENTSTR(sched_status, "heriter du sched_holder de la bulle ou on insere");
		/* si la bulle conteneuse est dans une autre bulle,
		 * on hérite de la bulle d'ordonnancement */
		if (!sched_bubble || ma_holder_type(sched_bubble) == MA_RUNQUEUE_HOLDER) {
			/* Sinon, c'est la conteneuse qui sert de bulle d'ordonnancement */
			PROF_EVENTSTR(sched_status, "c'est la bulle ou on insere qui sert d'ordonnancement");
			sched_bubble = &bubble->as_holder;
		}
		entity->sched_holder = sched_bubble;
		ret = 0;
	}
	ma_holder_unlock_softirq(&bubble->as_holder);
	if (bubble_was_empty) {
		marcel_mutex_lock(&bubble->join_mutex);
		ma_holder_lock_softirq(&bubble->as_holder);
		if (bubble->join_empty_state == 1  &&  bubble->nb_natural_entities == 1) {
			bubble->join_empty_state = 0;
		}
		ma_holder_unlock_softirq(&bubble->as_holder);
		marcel_mutex_unlock(&bubble->join_mutex);
	}
	return ret;
}

/* Permanently bring the entity inside the bubble by setting its sched_holder and natural_holder accordingly. */
int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	LOG_IN();

	if (!tbx_fast_list_empty(&entity->natural_entities_item)) {
		if (ma_bubble_holder(entity->natural_holder) == bubble)
			LOG_RETURN(0);

		/* If the entity is already in a bubble, move it directly to
		 * the destination bubble. We cannot use
		 * marcel_bubble_removeentity because a side effect of
		 * marcel_bubble_removeentity is to put the entity on a
		 * runqueue which causes trouble within a subsequent
		 * __do_bubble_insertentity. Thus we use ma_bubble_moveentity
		 * instead. */
		ma_bubble_moveentity(bubble, entity);
	} else {
		bubble_sched_debugl(7,"inserting %p in bubble %p\n",entity,bubble);
		int sched_holder_already_set = __do_bubble_insertentity(bubble,entity);
		if (sched_holder_already_set &&
				entity->type == MA_BUBBLE_ENTITY && entity->sched_holder->type == MA_RUNQUEUE_HOLDER) {
			/* sched holder was already set to something else, wake the bubble there */
			PROF_EVENTSTR(sched_status, "sched holder was already set to something else, wake the bubble there");
			ma_holder_t *h = ma_entity_holder_lock_softirq(entity);
			if (!entity->ready_holder)
				ma_set_ready_holder(entity, entity->sched_holder);
			MA_BUG_ON(entity->ready_holder->type != MA_RUNQUEUE_HOLDER);
			if (!entity->ready_holder_data)
				ma_enqueue_entity(entity, entity->ready_holder);
			PROF_EVENT2(bubble_sched_switchrq, ma_bubble_entity(entity), ma_rq_holder(entity->ready_holder));
			ma_entity_holder_unlock_softirq(h);
		}
		bubble_sched_debugl(7,"insertion %p in bubble %p done\n",entity,bubble);
	}
	/* TODO: dans le cas d'un thread, il faudrait aussi le déplacer dans son nouveau sched_holder s'il n'en avait pas déjà un, non ? */

	if (entity->type == MA_THREAD_ENTITY) {
		marcel_bubble_t *children_bubble = &ma_task_entity(entity)->default_children_bubble;
		if (children_bubble->as_entity.natural_holder)
			marcel_bubble_insertentity(bubble,ma_entity_bubble(children_bubble));
	}
	LOG_RETURN(0);
}

/* Removes entity from bubble.
 * Put entity on bubble's holding runqueue as a fallback */
int marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity) {
	int bubble_becomes_empty;
	LOG_IN();

	/* Remove entity from bubble */
	ma_holder_lock_softirq(&bubble->as_holder);
	MA_BUG_ON(entity->natural_holder != &bubble->as_holder);
	entity->natural_holder = NULL;
	entity->sched_holder = NULL;
	tbx_fast_list_del_init(&entity->natural_entities_item);
	marcel_barrier_addcount(&bubble->barrier, -1);
	bubble_becomes_empty = (!--bubble->nb_natural_entities);
	if ((entity)->type != MA_BUBBLE_ENTITY)
		PROF_EVENT2(bubble_sched_remove_thread, (void*)ma_task_entity(entity), bubble);
	else
		PROF_EVENT2(bubble_sched_remove_bubble, (void*)ma_bubble_entity(entity), bubble);
	ma_holder_rawunlock(&bubble->as_holder);

	/* Before announcing it, remove it scheduling-wise too */
	ma_holder_t *h = ma_entity_holder_rawlock(entity);
	if (h == &bubble->as_holder) {
		/* we need to get this entity out of this bubble 
		 * and we put it on a runqueue, either the rq holder of the bubble
		 * when possible, or the vp runqueue otherwise */
		int state = ma_get_entity(entity);
		ma_holder_rawunlock(h);
		ma_runqueue_t *rq = ma_to_rq_holder(bubble->as_entity.sched_holder);
		if (!rq)
			rq = ma_lwp_vprq(MA_LWP_SELF);
		ma_holder_rawlock(&rq->as_holder);
		ma_put_entity(entity, &rq->as_holder, state);
		ma_holder_unlock_softirq(&rq->as_holder);
	} else
		/* already out from the bubble, that's ok.  */
		ma_entity_holder_unlock_softirq(h);
	if (bubble_becomes_empty) {
		marcel_mutex_lock(&bubble->join_mutex);
		ma_holder_lock_softirq(&bubble->as_holder);
		if (bubble->join_empty_state == 0  &&  bubble->nb_natural_entities == 0) {
			bubble->join_empty_state = 1;
			marcel_cond_signal(&bubble->join_cond);
		}
		ma_holder_unlock_softirq(&bubble->as_holder);
		marcel_mutex_unlock(&bubble->join_mutex);
	}
	LOG_OUT();
	return 0;
}

/* Détacher une bulle (et son contenu) de la bulle qui la contient, pour
 * pouvoir la placer ailleurs */
int ma_bubble_detach(marcel_bubble_t *b) {
	/* h, hb and hh are used only in MA_BUG_ON tests. This is intentional. */
	ma_holder_t *h = b->as_entity.sched_holder;
	marcel_bubble_t *hb;
	ma_holder_t *hh;
	LOG_IN();
	PROF_EVENT1(bubble_detach, b);

	MA_BUG_ON(ma_holder_type(h)==MA_RUNQUEUE_HOLDER);
	hb = ma_bubble_holder(h);

	hh = hb->as_entity.sched_holder;
	MA_BUG_ON(ma_holder_type(hh)!=MA_RUNQUEUE_HOLDER);

	ma_bubble_lock(b);
	/* XXX: the top bubble and its runqueue must be locked! */
	ma_set_sched_holder(&b->as_entity, b, 1);
	ma_bubble_unlock(b);
	LOG_OUT();
	return 0;
}

void marcel_wake_up_bubble(marcel_bubble_t *bubble) {
	ma_holder_t *h;
	LOG_IN();
	/* If no scheduling runqueue was initially specified, use the current
	 * one */
	if (!(h = (bubble->as_entity.sched_holder))) {
		h = ma_task_sched_holder(MARCEL_SELF);
		while (h->type == MA_BUBBLE_HOLDER) {
			marcel_bubble_t *b = ma_bubble_holder(h);
			h = b->as_entity.sched_holder;
		}
		bubble->as_entity.sched_holder = h;
	}
	ma_holder_lock_softirq(h);
	bubble_sched_debug("waking up bubble %p in holder %p\n",bubble,h);
	if (h->type == MA_BUBBLE_HOLDER) {
		PROF_EVENT2(bubble_sched_bubble_goingback,bubble,ma_bubble_holder(h));
		/* XXX: the runqueue must be locked. */
		ma_set_sched_holder(&bubble->as_entity,ma_bubble_holder(h),1);
	} else {
		PROF_EVENT2(bubble_sched_wake,bubble,ma_rq_holder(h));
		ma_set_ready_holder(&bubble->as_entity,h);
		ma_enqueue_entity(&bubble->as_entity,h);
	}
	ma_holder_unlock_softirq(h);
	ma_top_add_bubble(bubble);
	if (current_sched->submit)
	  current_sched->submit(current_sched, &bubble->as_entity);
	LOG_OUT();
}

void marcel_bubble_join(marcel_bubble_t *bubble) {
	ma_holder_t *h;
	LOG_IN();
	marcel_mutex_lock(&bubble->join_mutex);
	while (bubble->join_empty_state != 1) {
		marcel_cond_wait(&bubble->join_cond, &bubble->join_mutex);
	}
	marcel_mutex_unlock(&bubble->join_mutex);

	h = ma_bubble_holder_lock_softirq(bubble);
	MA_BUG_ON(bubble->nb_natural_entities);
	MA_BUG_ON(!tbx_fast_list_empty(&bubble->natural_entities));
	if (bubble->as_entity.ready_holder) {
		if (bubble->as_entity.ready_holder_data)
			ma_dequeue_entity(&bubble->as_entity, h);
		ma_clear_ready_holder(&bubble->as_entity, h);
	}
	ma_bubble_holder_unlock_softirq(h);
	if ((h = bubble->as_entity.natural_holder)
		&& h->type == MA_BUBBLE_HOLDER
		&& h != &bubble->as_holder)
		marcel_bubble_removeentity(ma_bubble_holder(h), &bubble->as_entity);
	ma_top_del_bubble(bubble);
	PROF_EVENT1(bubble_sched_join,bubble);
	LOG_OUT();
}

#undef marcel_sched_exit
void marcel_sched_exit(marcel_t t) {
	marcel_bubble_t *b = &t->default_children_bubble;
	if (b->as_entity.natural_holder) {
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
	ma_stats_reset(&bubble->as_holder);
	ma_stats_synthesize(&bubble->as_holder, &bubble->as_entity);
	tbx_fast_list_for_each_entry(e, &bubble->natural_entities, natural_entities_item) {
		if (e->type == MA_BUBBLE_ENTITY) {
			b = ma_bubble_entity(e);
			ma_holder_rawlock(&b->as_holder);
			__ma_bubble_synthesize_stats(b);
			ma_holder_rawunlock(&b->as_holder);
			ma_stats_synthesize(&bubble->as_holder, &b->as_holder);
		} else {
			t = ma_task_entity(e);
			ma_stats_synthesize(&bubble->as_holder, &t->as_entity);
		}
	}
}
void ma_bubble_synthesize_stats(marcel_bubble_t *bubble) {
        ma_holder_lock_softirq(&bubble->as_holder);
	__ma_bubble_synthesize_stats(bubble);
	ma_holder_unlock_softirq(&bubble->as_holder);
}

static void
__do_snapshot (struct marcel_topo_level *from) 
{
  marcel_entity_t *e;
  for_each_entity_scheduled_on_runqueue (e, &from->rq) 
    {
      /* We set the last_topo_level statistics to the topo_level the
	 entity is currently scheduled on. */
      *(struct marcel_topo_level **) ma_stats_get (e, ma_stats_last_topo_level_offset) = from;
      if (e->type == MA_BUBBLE_ENTITY) 
	{
	  /* In case of a bubble, we set the last_topo_level
	     statistics of included entities to the topo_level that
	     holds the holding bubble. */
	  marcel_entity_t *e1;
	  for_each_entity_scheduled_in_bubble_begin (e1, ma_bubble_entity (e))
	    *(struct marcel_topo_level **) ma_stats_get (e1, ma_stats_last_topo_level_offset) = from;
	  for_each_entity_scheduled_in_bubble_end ()
	}
    }
}

/* Keep track of the current threads and bubbles distribution, by
   updating the last_topo_level statistics of every scheduled
   entity. */
void
ma_bubble_snapshot (struct marcel_topo_level *from) 
{
#ifdef MARCEL_STATS_ENABLED
  __do_snapshot (from);

  if (!from->arity)
    return;

  {  
    unsigned int i;

    for (i = 0; i < from->arity; i++)
      ma_bubble_snapshot (from->children[i]);
  }
#endif /* MARCEL_STATS_ENABLED */
}

/******************************************************************************
 *
 * Rassembleur d'une hiérarchie de la bulle
 *
 */

void __ma_bubble_gather(marcel_bubble_t *b, marcel_bubble_t *rootbubble) {
	marcel_entity_t *e;
	
	tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
		int state;

		if (e->type == MA_BUBBLE_ENTITY)
			__ma_bubble_gather(ma_bubble_entity(e), b->as_entity.sched_holder && b->as_entity.sched_holder->type == MA_RUNQUEUE_HOLDER ? b : rootbubble);

		if (e->sched_holder == &b->as_holder || e->sched_holder == &rootbubble->as_holder)
			/* déjà rassemblé */
			continue;

		MA_BUG_ON(e->natural_holder != &b->as_holder);
		state = ma_get_entity(e);
		mdebug("putting back %p in bubble %p(%p)\n", e, b, &b->as_holder);
		PROF_EVENTSTR(sched_status,"gather: putting back entity");
		ma_put_entity(e, &b->as_holder, state);
	}
}

void ma_bubble_gather(marcel_bubble_t *b) {
	ma_bubble_lock_all(b, marcel_machine_level);
	__ma_bubble_gather(b, b);
	PROF_EVENTSTR(sched_status,"gather: done");
	ma_bubble_unlock_all(b, marcel_machine_level);
}

void ma_bubble_gather_here(marcel_bubble_t *b, struct marcel_topo_level *level) {
	ma_bubble_lock_all_but_level(b, level);
	__ma_bubble_gather(b, b);
	PROF_EVENTSTR(sched_status,"gather: done");
	ma_bubble_unlock_all_but_level(b, level);
}

/******************************************************************************
 *
 * Push hierarchies of threads & bubbles up to avoid a now-disabled vpset
 *
 */

/* Where should I push up starting from h? */
static ma_runqueue_t *push_up_to(ma_holder_t *h) {
	ma_runqueue_t *target = NULL;
	if (h != NULL  &&  h->type == MA_RUNQUEUE_HOLDER) {
		ma_runqueue_t *rq = ma_rq_holder(h);
		if (marcel_vpset_isincluded(&marcel_disabled_vpset, &rq->vpset)) {
			/* No VP to run, have to go up */
			for (target = rq;
				marcel_vpset_isincluded(&marcel_disabled_vpset, &target->vpset);
				target = target->father)
				;
		}
	}
	return target;
}

/* Where should I push up entity e? (if it has to at all) */
static ma_runqueue_t *where_to_push(marcel_entity_t *e) {
	ma_runqueue_t *target = NULL;

	target = push_up_to(e->sched_holder);
	if (!target)
		target = push_up_to(e->ready_holder);

	return target;
}

/* Push thread up */
static void ma_push_thread_up(marcel_t t) {
	marcel_entity_t *e = ma_entity_task(t);
	ma_runqueue_t *target;
	marcel_bubble_t *gatherb = NULL;
	ma_holder_t *h;

	marcel_entity_t *e2 = e;
	/* While there are bubbles containing us */
	while ((h = e2->natural_holder) && h->type == MA_BUBBLE_HOLDER) {
		marcel_bubble_t *b = ma_bubble_holder(h);
		e2 = ma_entity_bubble(b);
		h = e2->sched_holder;

		/* Check whether it has to be moved itself */
		if (h && h->type == MA_RUNQUEUE_HOLDER) {
			ma_runqueue_t *rq = ma_rq_holder(h);
			if (marcel_vpset_isincluded(&marcel_disabled_vpset, &rq->vpset))
				/* bubble scheduled on disabled runqueue, we'll have to move it  */
				gatherb = b;
		}

		if (gatherb != b) {
			h = e2->ready_holder;
			if (h && h->type == MA_RUNQUEUE_HOLDER) {
				ma_runqueue_t *rq = ma_rq_holder(h);
				if (marcel_vpset_isincluded(&marcel_disabled_vpset, &rq->vpset))
					/* bubble running on disabled runqueue, we'll have to move it  */
					gatherb = b;
			}
		}
	}

	/* Found a whole bubble that needs to be moved, gather all its content, and move */
	if (gatherb) {
		target = where_to_push(ma_entity_bubble(gatherb));
		MA_BUG_ON(!target);
		__ma_bubble_gather(gatherb, gatherb);
		PROF_EVENTSTR(sched_status,"gather: done");
		mdebug("moving bubble %p to %s\n", gatherb, ma_holder_rq(target)->name);
		ma_move_entity(ma_entity_bubble(gatherb), ma_holder_rq(target));
		return;
	}
	/* Thread is not in a bubble that needs to be pushed up, push it alone if needed */

	target = where_to_push(e);
	if (target) {
		mdebug("moving thread %p to %s\n", t, ma_holder_rq(target)->name);
		ma_move_entity(e, ma_holder_rq(target));
	}
}

void ma_push_entities(const marcel_vpset_t *vpset)
{
	/*struct marcel_topo_level *level, *found;*/
	struct marcel_topo_level *l;
	marcel_t t;

#ifdef IDEALLY
	/* Find the lowest level that contains all newly disabled VPs but still
	 * has a few other VPs to schedule the poor orphaned threads */
	level = marcel_machine_level;
	while(1) {
		found = NULL;
		for (i = 0; i < level->arity; i++) {
			if (marcel_vpset_intersect(&level->children[i]->vpset, vpset)
					&& !marcel_vpset_isequal(&level->children[i]->vpset, vpset)
					) {
				if (found) {
					found = (void*) -1;
					break;
				} else
					found = level->children[i];
			}
		}
		/* Only matters in some subpart of the machine, see there */
		if (found && found != (void*) -1)
			level = found;
		else
			break;
	};
	marcel_printf("gathering back to %s\n", level->rq.as_holder.name);
#endif /* And only lock that part of the machine */

	/* FIXME: but too bad, we can't easily know which threads & bubbles are
	 * scheduled on these runqueues but sleeping, we have to look for the
	 * whole list of threads */

	/* FIXME: we don't have the list of bubbles, we hope that all bubbles
	 * have at least on thread in it :/ */

	/* Push all threads up */
	ma_topo_lock_all(marcel_machine_level);
	ma_local_bh_disable();
	ma_preempt_disable();
	for_all_vp(l) {
		_ma_raw_spin_lock(&ma_topo_vpdata_l(l,threadlist_lock));
		tbx_fast_list_for_each_entry(t, &ma_topo_vpdata_l(l, all_threads), all_threads)
			ma_push_thread_up(t);
		_ma_raw_spin_unlock(&ma_topo_vpdata_l(l,threadlist_lock));
	}
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
	ma_topo_unlock_all(marcel_machine_level);
}

/******************************************************************************
 *
 * Locking a hierarchy of bubbles. We need to lock runqueues with immediately held bubbles first, and then the sub-bubbles.
 *
 */

static void __ma_bubble_lock_all(marcel_bubble_t *b, marcel_bubble_t *root_bubble) {
	marcel_entity_t *e;
	if (b->as_entity.sched_holder == &root_bubble->as_holder) {
		/* Bubble held in the root bubble of this part of the hierarchy, just need to lock it and its content */
		ma_holder_rawlock(&b->as_holder);

		tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_lock_all(ma_bubble_entity(e), root_bubble);
		}
	} else {
		/* Bubble by itself on a runqueue. If that's not the case, you probably forgot to call ma_put_entity at some point. */
		MA_BUG_ON(b->as_entity.sched_holder->type != MA_RUNQUEUE_HOLDER);
		tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_lock_all(ma_bubble_entity(e), b); /* b is the new root bubble */
		}
	}
}

static void __ma_bubble_unlock_all(marcel_bubble_t *b, marcel_bubble_t *root_bubble) {
	marcel_entity_t *e;
	if (b->as_entity.sched_holder == &root_bubble->as_holder) {
		 // TODO: ne devrait pas être nécessaire.
		// || b->as_entity.sched_holder == &marcel_root_bubble.as_holder){
		/* Bubble held in root bubble, just need to unlock its content and it */
		tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_unlock_all(ma_bubble_entity(e), root_bubble);
		}
		ma_holder_rawunlock(&b->as_holder);
	} else {
		/* Bubble by itself on a runqueue. If that's not the case, you probably forgot to call ma_put_entity at some point. */
		MA_BUG_ON(b->as_entity.sched_holder->type != MA_RUNQUEUE_HOLDER);
		tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
			if (e->type == MA_BUBBLE_ENTITY)
				__ma_bubble_unlock_all(ma_bubble_entity(e), b); /* b is the new root bubble */
		}
	}
}

static void ma_topo_lock(struct marcel_topo_level *level);
static void ma_topo_unlock(struct marcel_topo_level *level);

static void __ma_topo_lock(struct marcel_topo_level *level) {
	struct marcel_topo_level *l;
	marcel_entity_t *e;
	int i;

	/* Lock all bubbles queued on that level */
	for_each_entity_scheduled_on_runqueue(e, &level->rq) {
		if (e->type == MA_BUBBLE_ENTITY)
			ma_holder_rawlock(&ma_bubble_entity(e)->as_holder);
	}

	for (i=0; i<level->arity; i++) {
		l = level->children[i];
		ma_topo_lock(l);
	}
}

static void ma_topo_lock(struct marcel_topo_level *level) {
	/* Lock the runqueue */
	ma_holder_rawlock(&level->rq.as_holder);
	__ma_topo_lock(level);
}

static void __ma_topo_unlock(struct marcel_topo_level *level) {
	struct marcel_topo_level *l;
	marcel_entity_t *e;
	int i;

	for (i=0; i<level->arity; i++) {
		l = level->children[i];
		ma_topo_unlock(l);
	}

	/* Unlock all bubbles queued on that level */
	for_each_entity_scheduled_on_runqueue(e, &level->rq) {
		if (e->type == MA_BUBBLE_ENTITY)
			ma_holder_rawunlock(&ma_bubble_entity(e)->as_holder);
	}
}

static void ma_topo_unlock(struct marcel_topo_level *level) {
	__ma_topo_unlock(level);
	/* Now unlock the unqueue */
	ma_holder_rawunlock(&level->rq.as_holder);
}

void ma_topo_lock_levels(struct marcel_topo_level *level) {
	ma_local_bh_disable();
	ma_preempt_disable();
	ma_topo_lock(level);
}

void ma_topo_unlock_levels(struct marcel_topo_level *level) {
	ma_topo_unlock(level);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

void ma_bubble_lock_all_but_levels(marcel_bubble_t *b) {
	/* We can only lock a bubble when it is alone or scheduled on a runqueue.
	 * The best way to make sure of this is to only call lock_all on a root bubble. */
	MA_BUG_ON(b->as_entity.sched_holder != &b->as_holder && b->as_entity.sched_holder->type != MA_RUNQUEUE_HOLDER);	
	__ma_bubble_lock_all(b,b);
}

void ma_bubble_lock_all_but_level(marcel_bubble_t *b, struct marcel_topo_level *level) {
	ma_local_bh_disable();
	ma_preempt_disable();
	__ma_topo_lock(level);
	ma_bubble_lock_all_but_levels(b);
}

void ma_bubble_lock_all(marcel_bubble_t *b, struct marcel_topo_level *level) {
	ma_topo_lock_levels(level);
	ma_bubble_lock_all_but_levels(b);
}

void ma_bubble_unlock_all_but_levels(marcel_bubble_t *b) {
	__ma_bubble_unlock_all(b,b);
}

void ma_bubble_unlock_all_but_level(marcel_bubble_t *b, struct marcel_topo_level *level) {
	__ma_bubble_unlock_all(b,b);
	__ma_topo_unlock(level);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

void ma_bubble_unlock_all(marcel_bubble_t *b, struct marcel_topo_level *level) {
	ma_bubble_unlock_all_but_levels(b);
	ma_topo_unlock_levels(level);
}

static void __ma_bubble_lock_subbubbles(marcel_bubble_t *b) {
	marcel_entity_t *e;
	tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
		if (e->type == MA_BUBBLE_ENTITY) {
			marcel_bubble_t *b = ma_bubble_entity(e);
			if(b->as_entity.sched_holder->type != MA_RUNQUEUE_HOLDER)
				__ma_bubble_lock(b);
		}
	}
}

void __ma_bubble_lock(marcel_bubble_t *b) {
	ma_holder_rawlock(&b->as_holder);
	__ma_bubble_lock_subbubbles(b);
}

static void __ma_bubble_unlock_subbubbles(marcel_bubble_t *b) {
	marcel_entity_t *e;
	tbx_fast_list_for_each_entry(e, &b->natural_entities, natural_entities_item) {
		if (e->type == MA_BUBBLE_ENTITY) {
			marcel_bubble_t *b = ma_bubble_entity(e);
			if(b->as_entity.sched_holder->type != MA_RUNQUEUE_HOLDER)
				__ma_bubble_unlock(b);
		}
	}
}

void __ma_bubble_unlock(marcel_bubble_t *b) {
	__ma_bubble_unlock_subbubbles(b);
	ma_holder_rawunlock(&b->as_holder);
}

static void ma_topo_lock_bubbles(struct marcel_topo_level *level) {
	struct marcel_topo_level *l;
	marcel_entity_t *e;
	int i;
	/* Lock all subbubbles of the bubble queued on that level */
	for_each_entity_scheduled_on_runqueue(e, &level->rq)
	{
		if (e->type == MA_BUBBLE_ENTITY)
		{
			__ma_bubble_lock_subbubbles(ma_bubble_entity(e));
		}
	}
	for (i=0; i<level->arity; i++) {
		l = level->children[i];
		ma_topo_lock_bubbles(l);
	}
}

static void ma_topo_unlock_bubbles(struct marcel_topo_level *level) {
	struct marcel_topo_level *l;
	marcel_entity_t *e;
	int i;

	for (i=0; i<level->arity; i++) {
		l = level->children[i];
		ma_topo_unlock_bubbles(l);
	}

	/* Lock all subbubbles of the bubble queued on that level */
	for_each_entity_scheduled_on_runqueue(e, &level->rq)
	{
		if (e->type == MA_BUBBLE_ENTITY)
		{
			__ma_bubble_unlock_subbubbles(ma_bubble_entity(e));
		}
	}
}

void ma_topo_lock_all(struct marcel_topo_level *level) {
	ma_topo_lock_levels(level);
	ma_topo_lock_bubbles(level);
}

void ma_topo_unlock_all(struct marcel_topo_level *level) {
	ma_topo_unlock_bubbles(level);
	ma_topo_unlock_levels(level);
}

void ma_bubble_lock(marcel_bubble_t *b) {
	ma_local_bh_disable();
	ma_preempt_disable();
	__ma_bubble_lock(b);
}

void ma_bubble_unlock(marcel_bubble_t *b) {
	__ma_bubble_unlock(b);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

/******************************************************************************
 *
 * Self-scheduling de base
 *
 */

/* on détient le verrou du holder de nextent */
marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_runqueue_t *rq, ma_holder_t **nexth, int idx) {
	LOG_IN();

	if (current_sched->sched) {
		if (!current_sched->sched(current_sched, nextent, rq, nexth, idx))
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
	ma_holder_rawlock(&bubble->as_holder);

	/* We generally manage to avoid this */
	if (tbx_fast_list_empty(&bubble->cached_entities)) {
		bubble_sched_debug("warning: bubble %d (%p) empty\n", bubble->id, bubble);
		/* We shouldn't ever schedule a NOSCHED bubble */
		MA_BUG_ON(bubble->as_entity.prio == MA_NOSCHED_PRIO);
		if (bubble->as_entity.ready_holder_data)
			ma_rq_dequeue_entity(&bubble->as_entity, rq);
		sched_debug("unlock(%p)\n", rq);
		ma_holder_rawunlock(&rq->as_holder);
		ma_holder_unlock(&bubble->as_holder);
		LOG_RETURN(NULL);
	}

	if (!bubble->not_preemptible && bubble->num_schedules >= bubble->as_holder.nb_ready_entities) {
		/* we expired our threads, let others execute */
		bubble->num_schedules = 0;
		if (bubble->as_entity.ready_holder_data) {
			/* by putting ourselves at the end of the list */
			ma_rq_dequeue_entity(&bubble->as_entity, rq);
			ma_rq_enqueue_entity(&bubble->as_entity, rq);
		}
		ma_holder_rawunlock(&rq->as_holder);
		ma_holder_unlock(&bubble->as_holder);
		LOG_RETURN(NULL);
	}

	bubble->num_schedules++;

	nextent = tbx_fast_list_entry(bubble->cached_entities.next, marcel_entity_t, cached_entities_item);
	bubble_sched_debugl(7,"next entity to run %p\n",nextent);
	MA_BUG_ON(nextent->type == MA_BUBBLE_ENTITY);

	sched_debug("unlock(%p)\n", rq);
	ma_holder_rawunlock(&rq->as_holder);
	*nexth = &bubble->as_holder;
	LOG_RETURN(nextent);
}


/* Bubble scheduler lookup.  */


/* Include the automatically generated name-to-scheduler mapping.  */
#include <marcel_bubble_sched_lookup.h>

const marcel_bubble_sched_class_t *marcel_lookup_bubble_scheduler_class(const char *name) {
	const struct ma_bubble_sched_desc *it;

	for(it = &ma_bubble_schedulers[0]; it->name != NULL; it++) {
		if (!strcmp(name, it->name))
			return it->klass;
	}

	return NULL;
}

int marcel_bubble_sched_instantiate(const marcel_bubble_sched_class_t *klass, marcel_bubble_sched_t *scheduler) {
	int err;

	memset(scheduler, 0, klass->instance_size);
	err = klass->instantiate(scheduler);
	if (!err)
		scheduler->klass = klass;

	return err;
}

const marcel_bubble_sched_t *marcel_lookup_bubble_scheduler(const char *name) {
	marcel_bubble_sched_t *scheduler = NULL;
	const marcel_bubble_sched_class_t *klass;

	klass = marcel_lookup_bubble_scheduler_class(name);
	if (klass) {
		int err;
		scheduler = marcel_malloc(klass->instance_size, __FILE__, __LINE__);
		if (scheduler) {
			err = marcel_bubble_sched_instantiate(klass, scheduler);
			if (err) {
				marcel_free(scheduler);
				scheduler = NULL;
			}
		}
	}

	return scheduler;
}

void __ma_resched_bubble_contents(marcel_bubble_t *b) {
	marcel_entity_t *e;
	for_each_entity_scheduled_in_bubble_begin (e, b) {
		switch (e->type) {
			case MA_BUBBLE_ENTITY:
				__ma_resched_bubble_contents(b);
				break;
			case MA_THREAD_ENTITY:
				/* only resched running threads */
				if (!(e->ready_holder_data)) {
					marcel_task_t * const t = ma_task_entity(e);
					const int vp	= ma_vpnum(t->lwp);
					if (vp != -1) {
						ma_set_tsk_need_togo(t);
						ma_resched_task(t, vp, t->lwp);
					}
				}
				break;
			default:
				MA_BUG ();

		}
	}
	for_each_entity_scheduled_in_bubble_end ();
}

void ma_resched_bubble_contents(marcel_bubble_t *b) {
	ma_bubble_lock_all(b, marcel_machine_level);
	__ma_resched_bubble_contents(b);
	ma_bubble_unlock_all(b, marcel_machine_level);
}


/******************************************************************************
 *
 * Initialisation
 *
 */

static void __marcel_init bubble_sched_init(void) {
        marcel_root_bubble.as_entity.sched_holder = &ma_main_runqueue.as_holder;
	ma_holder_lock_softirq(&ma_main_runqueue.as_holder);
	ma_set_ready_holder(&marcel_root_bubble.as_entity, &ma_main_runqueue.as_holder);
	ma_enqueue_entity(&marcel_root_bubble.as_entity, &ma_main_runqueue.as_holder);
	ma_holder_unlock_softirq(&ma_main_runqueue.as_holder);
	PROF_EVENT2(bubble_sched_switchrq, &marcel_root_bubble, &ma_main_runqueue);
}

void ma_bubble_sched_init2(void) {
	/* Having main on the main runqueue is both faster and respects priorities */
	ma_holder_lock_softirq(&marcel_root_bubble.as_holder);
	ma_clear_ready_holder(&MARCEL_SELF->as_entity, &marcel_root_bubble.as_holder);
	ma_holder_rawunlock(&marcel_root_bubble.as_holder);
	SELF_GETMEM(as_entity.sched_holder) = &ma_main_runqueue.as_holder;
	PROF_EVENT2(bubble_sched_switchrq, MARCEL_SELF, &ma_main_runqueue);
	ma_holder_rawlock(&ma_main_runqueue.as_holder);
	ma_set_ready_holder(&MARCEL_SELF->as_entity, &ma_main_runqueue.as_holder);
	ma_holder_unlock_softirq(&ma_main_runqueue.as_holder);

	marcel_mutex_lock(&current_sched_mutex);
	if (current_sched->init)
		current_sched->init(current_sched);
	marcel_mutex_unlock(&current_sched_mutex);
}

__ma_initfunc_prio(bubble_sched_init, MA_INIT_BUBBLE_SCHED, MA_INIT_BUBBLE_SCHED_PRIO, "Bubble Scheduler");
#endif /* MA__BUBBLES */
