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


#ifndef __MARCEL_BUBBLE_MSTEAL_H__
#define __MARCEL_BUBBLE_MSTEAL_H__


#ifdef MM_HEAP_ENABLED
#include "marcel_topology.h"
#endif


/** Public functions **/
#ifdef MM_HEAP_ENABLED
/* possible conflit avec broq ? */

/** \file
 * \brief Bubble Spread scheduler functions
 * \defgroup marcel_bubble_spread Bubble Spread scheduler
 *
 * This is a very basic greedy scheduler that just spreads a given bubble
 * hierarchy over a given topology level while taking load indications into
 * account.
 *
 * @{
 */
extern int marcel_bubble_msteal_see_up(struct marcel_topo_level *level);
#endif /* MM_HEAP_ENABLED */


#endif /** __MARCEL_BUBBLE_MSTEAL_H__ **/
