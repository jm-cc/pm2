
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
ma_stats_t ma_stats_funcs;
unsigned long ma_stats_alloc(ma_stats_reset_t *reset_function, ma_stats_synthesis_t *synthesis_function, size_t size) {
	unsigned long offset;
	if (size < sizeof(synthesis_function) + sizeof(reset_function))
		size = sizeof(synthesis_function);
	offset = ma_per_sth_alloc(&stats_cur, size);
	ma_stats_synthesis_func(offset) = synthesis_function;
	ma_stats_reset_func(offset) = reset_function;
	reset_function(ma_stats_get(__main_thread, offset));
	return offset;
}

void ma_stats_unsigned_sum_reset(void *dest) {
	unsigned *data = dest;
	*data = 0;
}
void ma_stats_unsigned_sum_synthesis(void * __restrict dest, const void * __restrict src) {
	unsigned *dest_data = dest;
	const unsigned *src_data = src;
	*dest_data += *src_data;
}
