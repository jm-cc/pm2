
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

static ma_per_sth_cur_t stats_cur = MA_PER_STH_CUR_INITIALIZER(MARCEL_STATS_ROOM);
ma_stats_t ma_stats_reset_func, ma_stats_synthesis_func, ma_stats_size;
unsigned long ma_stats_alloc(ma_stats_reset_t *reset_function, ma_stats_synthesis_t *synthesis_function, size_t size) {
	unsigned long offset;
	size = (size + sizeof(void(*)())-1) & ~(sizeof(void(*)())-1);
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

void __ma_stats_reset(ma_stats_t stats) {
	unsigned long offset;
	for (offset = 0; offset < stats_cur.cur; offset += ma_stats_size(offset))
		ma_stats_reset_func(offset)(__ma_stats_get(stats,offset));
}

void __ma_stats_synthesize(ma_stats_t dest, ma_stats_t src) {
	unsigned long offset;
	for (offset = 0; offset < stats_cur.cur; offset += ma_stats_size(offset))
		ma_stats_synthesis_func(offset)(__ma_stats_get(dest,offset),
			__ma_stats_get(src,offset));
}

long *ma_stats_get(marcel_t t, unsigned long offset) {
	if (!t)
		t = MARCEL_SELF;
	return ma_task_stats_get(t, offset);
}

long *ma_bubble_stats_get(marcel_bubble_t *b, unsigned long offset) {
	return ma_bubble_stats_get(b, offset);
}
