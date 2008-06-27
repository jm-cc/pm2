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

/* This file contains functions to browse the topology on a
   hierarchical way, and do whatever you want from the browsed
   runqueues. You can specify what to do by implementing a
   ma_see_func_t function.

   For example, the Affinity scheduler uses a ma_see_func_t function
   called browse_bubble_and_steal, that selects a bubble to steal from
   the considered holder, and then move it to the source runqueue.

   More generally, a call to ma_topo_level_browse() hierarchically
   browses the entire topology until the criterias exposed in the
   corresponding ma_see_func_t function are met.
 */
#section marcel_types
#depend "scheduler/marcel_holder.h[types]"
/* The "seeing" function type. We browse the content of the first
   argument (ma_holder_t *), and we remember that we've originally
   been called from the second one (struct marcel_topo_level *). Note
   that using a ma_holder_t type instead of directly using a
   ma_runqueue_t allows a function of this type to be applied for
   browsing the content of a bubble too. */
typedef int (ma_see_func_t)(void *);

#section marcel_functions
#depend "marcel_topology.h[types]"
#depend "scheduler/marcel_holder.h[types]"
/* Main browsing function, applies the _my_see_ to each browsed
   runqueue */
int ma_topo_level_browse (struct marcel_topo_level *from, 
			  ma_see_func_t my_see, 
			  void *args);

/* Auxiliary functions, called by ma_topo_level_browse(). */
/* TODO: Is it a good idea to make these functions available?
   Shouldn't they be internal? */
int ma_topo_level_see_down (struct marcel_topo_level *from, 
			    struct marcel_topo_level *me, 
			    ma_see_func_t my_see,
			    void *args);

int ma_topo_level_see_up (struct marcel_topo_level *from, 
			  ma_see_func_t my_see,
			  void *args);
