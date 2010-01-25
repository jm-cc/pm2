/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_BUBBLE_BROWSE_H__
#define __MARCEL_BUBBLE_BROWSE_H__


#include "sys/marcel_flags.h"
#include "marcel_types.h"


#ifdef __MARCEL_KERNEL__


/** Internal data types **/
/* The "seeing" function type. We browse the content of the first
   argument (ma_holder_t *), and we remember that we've originally
   been called from the second one (struct marcel_topo_level *). Note
   that using a ma_holder_t type instead of directly using a
   ma_runqueue_t allows a function of this type to be applied for
   browsing the content of a bubble too. */
typedef int (ma_see_func_t)(ma_holder_t*, void *);


/** Internal functions **/
/* Main browsing function, applies the _my_see_ to each browsed
   runqueue, and stops if nothing has been found before reaching the
   _scope_ level. */
int ma_topo_level_browse (struct marcel_topo_level *from, 
			  int scope,
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
			  int scope,
			  ma_see_func_t my_see,
			  void *args);

/* Determines whether the _l_ level is inside the scope (i.e., if _l_
   is localized under the "height" limit represented by _scope_.) */
int ma_topo_level_is_in_scope (struct marcel_topo_level *l, int scope);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_BUBBLE_BROWSE_H__ **/
