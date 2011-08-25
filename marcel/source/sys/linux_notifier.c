
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

/*
 * Similar to some part of:
 *  linux/kernel/sys.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include "marcel.h"

static ma_rwlock_t notifier_lock = MA_RW_LOCK_UNLOCKED;

/**
 *	ma_notifier_chain_register	- Add notifier to a notifier chain
 *	@list: Pointer to root list pointer
 *	@n: New entry in notifier chain
 *
 *	Adds a notifier to a notifier chain.
 *
 *	Currently always returns zero.
 */

int ma_notifier_chain_register(struct ma_notifier_chain *c, struct ma_notifier_block *n)
{
	struct ma_notifier_block **list = &c->chain;
	MARCEL_LOG_IN();
	/* Niveau 6 parce que init donne déjà les infos */
	MARCEL_LOG("Chain [%s]: registering notifier '%s' at prio %i\n",
		   c->name, n->name, n->priority);
	ma_write_lock(&notifier_lock);
	while (*list) {
		if (n->priority > (*list)->priority)
			break;
		MARCEL_LOG("Skipping notifier '%s' at prio %i\n",
			   (*list)->name, (*list)->priority);
		list = &((*list)->next);
	}
	n->next = *list;
	*list = n;
	ma_write_unlock(&notifier_lock);
	MARCEL_LOG_RETURN(0);
}

/**
 *	ma_notifier_chain_unregister - Remove notifier from a notifier chain
 *	@nl: Pointer to root list pointer
 *	@n: New entry in notifier chain
 *
 *	Removes a notifier from a notifier chain.
 *
 *	Returns zero on success, or %-ENOENT on failure.
 */

int ma_notifier_chain_unregister(struct ma_notifier_chain *c, struct ma_notifier_block *n)
{
	struct ma_notifier_block **nl = &c->chain;
	MARCEL_LOG_IN();
	MARCEL_LOG("Chain [%s]: unregistering notifier '%s' at prio %i\n",
		   c->name, n->name, n->priority);
	ma_write_lock(&notifier_lock);
	while ((*nl) != NULL) {
		if ((*nl) == n) {
			*nl = n->next;
			ma_write_unlock(&notifier_lock);
			MARCEL_LOG_RETURN(0);
		}
		nl = &((*nl)->next);
	}
	ma_write_unlock(&notifier_lock);
	MARCEL_LOG("Chain [%s]: notifier '%s' not found\n", c->name, n->name);
	MARCEL_LOG_RETURN(-1);
}

/**
 *	ma_notifier_call_chain - Call functions in a notifier chain
 *	@n: Pointer to root pointer of notifier chain
 *	@val: Value passed unmodified to notifier function
 *	@v: Pointer passed unmodified to notifier function
 *
 *	Calls each function in a notifier chain in turn.
 *
 *	If the return value of the notifier can be and'd
 *	with %MA_NOTIFY_STOP_MASK, then notifier_call_chain
 *	will return immediately, with the return value of
 *	the notifier function which halted execution.
 *	Otherwise, the return value is the return value
 *	of the last notifier function called.
 */

int ma_notifier_call_chain(struct ma_notifier_chain *n, int val, void *v)
{
	int ret = MA_NOTIFY_DONE;
	struct ma_notifier_block *nb = n->chain;

	MARCEL_LOG_IN();
	MARCEL_LOG("Chain [%s]: calling notifiers with value %d\n", n->name, val);
	while (nb) {
		MARCEL_LOG("Chain [%s]  prio %4i: %s\n",
			   n->name, nb->priority,
			   ((val >= 0) && (val < nb->nb_actions) && (nb->actions_name)) ?
			   (nb->actions_name)[val] : nb->name);
		ret = nb->notifier_call(nb, val, v);
		if (ret & MA_NOTIFY_STOP_MASK) {
			MARCEL_LOG("Chain [%s]: calling notifiers aborted\n", n->name);
			MARCEL_LOG_RETURN(ret);
		}
		nb = nb->next;
	}
	MARCEL_LOG_RETURN(ret);
}
