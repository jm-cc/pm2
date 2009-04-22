
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

/** \file
 * \brief Bubble Spread scheduler functions
 * \defgroup marcel_bubble_mspread Bubble Memory Spread scheduler
 *
 * This is a very basic greedy scheduler that just spreads a given bubble
 * hierarchy over a given topology level while taking load indications into
 * account.
 *
 * @{
 */

#section common
#ifdef MM_HEAP_ENABLED

#section functions
void marcel_bubble_mspread(marcel_bubble_t *b, struct marcel_topo_level *l);
void marcel_bubble_mspread_entities(marcel_entity_t *e[], int ne, struct marcel_topo_level **l, int nl);

#section common
#endif /* MM_HEAP_ENABLED */

/* @} */
