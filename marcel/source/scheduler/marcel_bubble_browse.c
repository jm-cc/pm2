/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

/* Browse the entire topology from level _from_, until the criterias
   exposed in _my_see_ are met, or we reach a level of height
   scope. */
int
ma_topo_level_browse(struct marcel_topo_level *from, int scope, ma_see_func_t my_see,
		     void *args)
{
	return ma_topo_level_see_up(from, scope, my_see, args);
}

/* Browse each children level of level _father_, applying the _my_see_
   function to each of them, and avoiding the caller (_me_). */
int
ma_topo_level_see_down(struct marcel_topo_level *father,
		       struct marcel_topo_level *me, ma_see_func_t my_see, void *args)
{
	unsigned int i;
	MARCEL_SCHED_LOG("ma_topo_level_see_down from %d %d\n", father->type,
			 father->number);

	/* If we do have children... */
	if (father->arity) {
		/* ... let's examine them ! */
		for (i = 0; i < father->arity; i++) {
			if (me && father->children[i] == me)
				/* Avoid me if I'm one of the children */
				continue;

			if (my_see(&(&father->children[i]->rq)->as_holder, args)
			    || ma_topo_level_see_down(father->children[i], NULL, my_see,
						      args))
				return 1;
		}
	}

	MARCEL_SCHED_LOG("ma_topo_level_see_down done\n");
	return 0;
}

/* Look up in the levels hierarchy, applying ma_topo_level_see_down to
   each children of _from_'s father. */
int
ma_topo_level_see_up(struct marcel_topo_level *from,
		     int scope, ma_see_func_t my_see, void *args)
{
	struct marcel_topo_level *father = from->father;
	MARCEL_SCHED_LOG("ma_topo_level_see_up from %d %d\n", from->type, from->number);
	if (!father) {
		MARCEL_SCHED_LOG("ma_topo_level_see_up done\n");
		return 0;
	}

	/* Looking upward begins with looking downward ! */
	if (ma_topo_level_see_down(father, from, my_see, args))
		return 1;

	if (ma_topo_level_is_in_scope(father, scope))
		/* Then look from the father's */
		return ma_topo_level_see_up(father, scope, my_see, args);

	/* Return something different to inform the caller we actually
	   did reached _scope_ and didn't find anything. */
	return -1;
}

int ma_topo_level_is_in_scope(struct marcel_topo_level *l, unsigned int scope)
{
	return (l->level >= scope) ? 1 : 0;
}
