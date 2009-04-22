
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

#ifdef MARCEL_STATS_ENABLED
#include "marcel.h"

static ma_per_sth_cur_t stats_cur = MA_PER_STH_CUR_INITIALIZER(MARCEL_STATS_ROOM);
ma_stats_t ma_stats_reset_func, ma_stats_synthesis_func, ma_stats_size;
unsigned long ma_stats_alloc(ma_stats_reset_t *reset_function, ma_stats_synthesis_t *synthesis_function, size_t size) {
	unsigned long offset;
	size = (size + sizeof(void(*)(void))-1) & ~(sizeof(void(*)(void))-1);
	offset = ma_per_sth_alloc(&stats_cur, size);
	ma_stats_reset_func(offset) = reset_function;
	ma_stats_synthesis_func(offset) = synthesis_function;
	ma_stats_size(offset) = size;
	reset_function(ma_task_stats_get(__main_thread, offset));
	return offset;
}

void ma_stats_long_sum_reset(void *dest) {
	long *data = dest;
	*data = 0;
}
void ma_stats_long_sum_synthesis(void * __restrict dest, const void * __restrict src) {
	long *dest_data = dest;
	const long *src_data = src;
	*dest_data += *src_data;
}

TBX_FUN_ALIAS(void, ma_stats_long_max_reset, ma_stats_long_sum_reset, (void *dest), (dest));
void ma_stats_long_max_synthesis(void * __restrict dest, const void * __restrict src) {
	long *dest_data = dest;
	const long *src_data = src;
	if (*src_data > *dest_data)
		*dest_data = *src_data;
}

#ifdef MM_MAMI_ENABLED
void ma_stats_memnode_sum_reset(void *dest) {
  memset (dest, 0, marcel_nbnodes * sizeof (long));
}
void ma_stats_memnode_sum_synthesis(void * __restrict dest, const void * __restrict src) {
  int i;
  for (i = 0; i < marcel_nbnodes; i++) {
    ((long *)dest)[i] += ((long *)src)[i];
  }
}
#endif /* MM_MAMI_ENABLED */

void ma_stats_last_vp_sum_reset (void *dest) {
  long *data = dest;
  *data = -1;
}
void ma_stats_last_vp_sum_synthesis (void * __restrict dest, const void * __restrict src) {
  long *dest_data = dest;
  const long *src_data = src;

  switch (*dest_data) {
  case MA_VPSTATS_CONFLICT:
    /* if someone already told that this subtree contains different
       last_vps, the job is done. */
    break;

  case MA_VPSTATS_NO_LAST_VP:
    /* dest is not set yet, whatever src contains will be fine. */
    *dest_data = *src_data;
    break;

  default:
    /* dest has already been set to something, we have to check if src
       holds something different. */
    switch (*src_data) {
    case MA_VPSTATS_CONFLICT:
      *dest_data = MA_VPSTATS_CONFLICT;
      break;

    case MA_VPSTATS_NO_LAST_VP:
      break;

    default:
      if (*dest_data != *src_data)
	*dest_data = MA_VPSTATS_CONFLICT;
    }
    break;
  }
}

void ma_stats_last_topo_level_sum_reset (void *dest) {
  struct marcel_topo_level **data = dest;
  *data = NULL;
}

void __ma_stats_reset(ma_stats_t stats) {
	unsigned long offset;
	for (offset = 0; offset < stats_cur.cur; offset += ma_stats_size(offset))
		ma_stats_reset_func(offset)(__ma_stats_get(stats,offset));
}

void __ma_stats_synthesize(ma_stats_t dest, ma_stats_t src) {
	unsigned long offset;
	/* Passing NULL as the ma_stats_synthesis_t argument of
	   ma_stats_alloc() indicates that we don't need to sum things
	   up into bubbles for the considered statistics. */
	for (offset = 0; offset < stats_cur.cur; offset += ma_stats_size(offset)) {
	  if (ma_stats_synthesis_func(offset)) {
	    ma_stats_synthesis_func(offset)(__ma_stats_get(dest,offset),
					    __ma_stats_get(src,offset));
	  }
	}
}

long *ma_cumulative_stats_get (marcel_entity_t *e, unsigned long offset) {
  return (e->type == MA_BUBBLE_ENTITY) ?
    (long *) ma_bubble_hold_stats_get (ma_bubble_entity (e), offset)
    : (long *) ma_stats_get (e, offset);
}

long *marcel_task_stats_get(marcel_t t, unsigned long offset) {
	if (!t)
		t = MARCEL_SELF;
	return ma_task_stats_get(t, offset);
}
#endif /* MARCEL_STATS_ENABLED */
