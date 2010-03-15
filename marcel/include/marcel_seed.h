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

#ifndef __MARCEL_SEEDS_H__
#define __MARCEL_SEEDS_H__

#ifdef MA__BUBBLES
/** \brief Look for ungerminated seeds in bubble \e b and instantiate those seeds inline.
 *  \param b The bubble to lookup.
 *  \param one_shot_mode Whether to process a single seed (1) or all the seeds (0).
 *  \param recurse_mode Whether to lookup the bubble and its child bubbles (1) or the bubble only (0).
 * */
int
marcel_seed_try_inlining (marcel_bubble_t * const b, int one_shot_mode, int recurse_mode);

#endif
#endif

