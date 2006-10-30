
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

#section variables
extern unsigned long marcel_stats_load_offset;

#section marcel_variables
extern unsigned long ma_stats_nbthreads_offset, ma_stats_last_ran_offset,
		ma_stats_memory_offset;

#section marcel_types
typedef char ma_stats_t[MARCEL_STATS_ROOM];
typedef void ma_stats_synthesis_t(void * __restrict dest, const void * __restrict src);
typedef void ma_stats_reset_t(void *dest);

#section marcel_variables
extern ma_stats_t ma_stats_reset_func, ma_stats_synthesis_func, ma_stats_size;

#section marcel_macros
#define __ma_stats_get(stats, offset) ((void*)&((stats)[offset]))
#define ma_stats_get(object, offset) __ma_stats_get((object)->stats, (offset))
#define ma_stats_reset_func(offset) (*(ma_stats_reset_t **)__ma_stats_get(ma_stats_reset_func, (offset)))
#define ma_stats_synthesis_func(offset) (*(ma_stats_synthesis_t **)__ma_stats_get(ma_stats_synthesis_func, (offset)))
#define ma_stats_size(offset) (*(size_t *)__ma_stats_get(ma_stats_size, (offset)))
#define ma_stats_reset(object) __ma_stats_reset((object)->stats)
#define ma_stats_synthesize(dest, src) __ma_stats_synthesize((dest)->stats, (src)->stats)

#section marcel_functions
unsigned long ma_stats_alloc(ma_stats_reset_t *reset_function, ma_stats_synthesis_t *synthesis_function, size_t size);
void __ma_stats_reset(ma_stats_t stats);
void __ma_stats_synthesize(ma_stats_t dest, ma_stats_t src);

ma_stats_synthesis_t ma_stats_long_sum_synthesis;
ma_stats_reset_t ma_stats_long_sum_reset;
ma_stats_synthesis_t ma_stats_long_max_synthesis;
ma_stats_reset_t ma_stats_long_max_reset;

#section functions
long *marcel_stats_get(marcel_t t, unsigned long offset);

#section macros
#define marcel_stats(kind) (*marcel_stats_get(NULL, marcel_stats_##kind##_offset))
