/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
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

/* \brief Attempt to extract a seed \e e from bubble \b. 
 * \return 1 in case of success, 0 in case of failure (in that case, *p_retry is set to 1 if attempt should be retried later, and to 0 otherwise) */
static
int
ma_seed_try_extract_from_bubble (marcel_bubble_t * const b, marcel_entity_t * const e, int * const p_retry) {
	ma_holder_t *h;
	ma_holder_t *h2;
	/* Found a seed that seems to be uninstantiated. Let's have a closer look...
	 *
	 * Having that closer look is a bit tricky however. We are accessing the seed through its
	 * natural holder, but the marcel core scheduler might be concurrently accessing it through its
	 * ready holder if both holders are different. Thus we need to take stop steps to prevent that.
	 * In particular, we should not release the task bubble lock until we know for sure whether _we_
	 * or _the core scheduler_ will be responsible of instantiating the seed. */

	/* Temporarily reduces concurrency. */
	ma_local_bh_disable ();
	ma_preempt_disable ();

	/* Get the current relevant holder of seed e. */
	h = ma_entity_active_holder (e);

	if (h != &b->as_holder) {
		/* If h is the task bubble itself, things are easy: we do not have any additional locking to do.
		 *
		 * On the contrary, if h is not the task bubble, we need to lock it too avoid the instantiation
		 * race condition with the core scheduler.
		 * However, we must do that additional locking unblockingly to avoid a deadlock since we already
		 * hold the task bubble lock. */
		if (!ma_holder_trylock (h)) {
			/* We were not successful in our attempt to grab the seed's active holder lock, but the situation
			 * is not yet settled and we should come back again later. */
			*p_retry = 1;
			ma_preempt_enable ();
			ma_local_bh_enable ();
			goto next;
		}
		/* We got the lock of the holder but in the mean time, the active holder of e may have changed. Thus
		 * we must verify if we are still h is still the active holder of e. */
		h2 = ma_entity_active_holder (e);
		if (h2 != h) {
			/* The situation is a bit fluctuating right now. We should have a look again later. */
			*p_retry = 1;
			ma_holder_rawunlock (h);
			ma_preempt_enable ();
			ma_local_bh_enable ();
			goto next;
		}

		/* We know have the lock on the active holder, but in the mean time, the core scheduler might have started
		 * germinating the seed. If this is the case, this is to late for us and we have to cancel our own attempt. */
		if (ma_task_entity (e)->cur_thread_seed_runner) {
			ma_holder_rawunlock (h);
			ma_preempt_enable ();
			ma_local_bh_enable ();
			goto next;
		}
	}

	/* Ok, now we are sure that we can safely instantiate the seed. */
	{
		/* Remove the seed from its ready holder. */
		int state __attribute__((unused)) = ma_get_entity (e);
	}

	/* Some sanity check. */
	MA_BUG_ON(e->natural_holder != &b->as_holder);

	/* Remove the seed from the bubble. */
	e->natural_holder = NULL;
	e->sched_holder = NULL;
	tbx_fast_list_del_init (&e->natural_entities_item);
	if (h != &b->as_holder) {
		ma_holder_rawunlock (h);
	}
	ma_preempt_enable ();
	ma_local_bh_enable ();

	return 1;

next:
	return 0;
}

static
marcel_t
ma_seed_recursive_lookup (marcel_bubble_t * const b, marcel_bubble_t **p_steal_bubble) {
	int retry = 0;    /* whether we should plan to lookup the task bubble contents later,
			     after a transient failure in the attempt to grab some work. */
	marcel_entity_t *e;
	ma_holder_lock (&b->as_holder);
	tbx_fast_list_for_each_entry (e, &b->natural_entities, natural_entities_item) {
		if (e->type == MA_BUBBLE_ENTITY) {
			marcel_t seed = ma_seed_recursive_lookup (ma_bubble_entity (e), p_steal_bubble);
			if (seed != NULL) {
				if (seed == (marcel_t)(intptr_t)1) {
					retry = 1;
					goto next;
				}

				/* Now, we can unlock the task bubble. */
				ma_holder_unlock (&b->as_holder);

				return seed;
			}
		} else if (e->type == MA_THREAD_SEED_ENTITY && (ma_task_entity (e)->state == MA_TASK_RUNNING) && (ma_task_entity (e)->cur_thread_seed_runner == NULL)) {
			if (!ma_seed_try_extract_from_bubble (b, e, &retry))
				goto next;

			*p_steal_bubble = b;

			/* Now, we can unlock the task bubble. */
			ma_holder_unlock (&b->as_holder);

			marcel_t seed = ma_task_entity (e);
			/* A last sanity check. */
			MA_BUG_ON (seed->cur_thread_seed_runner != NULL);

			return seed;
		}
next:
		;
	}
	ma_holder_unlock (&b->as_holder);
	return (marcel_t)(intptr_t)retry;
}

/* \brief Exec an extracted seed inline and handle related cleanup. */
static
void
ma_seed_exec (marcel_t seed) {
	/* Go, go, go... */
	seed->ret_val = seed->f_to_call (seed->arg);

	/* Already finished. */
#ifdef MA__DEBUG
	/* NOTE: this is quite costly, only do it to get gdb's
	 * marcel-threads & marcel_top working */
	marcel_one_task_less(seed);
#endif
	marcel_funerals (seed);
}

/* \brief Update bubble state to account for an inlined and completed seed. */
static
void
ma_seed_bubble_minus_one (marcel_bubble_t *b) {
	int empty;

	ma_holder_lock(&b->as_holder);
	marcel_barrier_addcount (&b->barrier, -1);
	empty = (!--b->nb_natural_entities);
	ma_holder_unlock(&b->as_holder);

	/* Perform the necessary signaling if the bubble becomes empty. */
	if (empty) {
		marcel_mutex_lock(&b->join_mutex);
		ma_holder_lock(&b->as_holder);
		if (b->join_empty_state == 0  &&  b->nb_natural_entities == 0) {
			b->join_empty_state = 1;
			marcel_cond_signal(&b->join_cond);
		}
		ma_holder_unlock(&b->as_holder);
		marcel_mutex_unlock(&b->join_mutex);
	}
}

int
marcel_seed_try_inlining (marcel_bubble_t * const b, int one_shot_mode, int recurse_mode) {
	marcel_entity_t *e;
	int retry;    /* whether we should plan to lookup the task bubble contents later,
			 after a transient failure in the attempt to grab some work. */
again:
	retry = 0;
	ma_holder_lock (&b->as_holder);
	tbx_fast_list_for_each_entry (e, &b->natural_entities, natural_entities_item) {
		if (recurse_mode && e->type == MA_BUBBLE_ENTITY) {
			marcel_bubble_t *steal_bubble = NULL;
			marcel_t seed = ma_seed_recursive_lookup (ma_bubble_entity (e), &steal_bubble);
			if (seed != NULL) {
				if (seed == (marcel_t)(intptr_t)1) {
					retry = 1;
					goto next;
				}
				ma_holder_unlock (&b->as_holder);
				ma_seed_exec(seed);
				ma_seed_bubble_minus_one (steal_bubble);
				if (one_shot_mode)
					return 1;

				/* We cannot just proceed to the next loop iteration because we released the task bubble lock before
				 * instantiating the seed. Thus we need to start the loop over again. */
				goto again;
			}
		} else if (e->type == MA_THREAD_SEED_ENTITY && (ma_task_entity (e)->cur_thread_seed_runner == NULL)) {
			if (!ma_seed_try_extract_from_bubble (b, e, &retry))
				goto next;

			ma_holder_unlock (&b->as_holder);
			ma_seed_exec(ma_task_entity (e));
			ma_seed_bubble_minus_one (b);
			if (one_shot_mode)
				return 1;

			/* We cannot just proceed to the next loop iteration because we released the task bubble lock before
			 * instantiating the seed. Thus we need to start the loop over again. */
			goto again;
		}
next:
		;
	}
	ma_holder_unlock (&b->as_holder);
	if (!one_shot_mode && retry)
		goto again;

	return 0;
}


