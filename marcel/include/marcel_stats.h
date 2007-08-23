
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

/** \file
 * \brief Statistics interface
 */

/** \defgroup marcel_stats Statistics Interface
 *
 * This is the interface for gathering statistics about threads and bubbles.
 * Threads have one ma_stats_t buffer for themselves. Bubbles have one
 * ma_stats_t buffer for themselves and another one for the synthesis of
 * statistics of their held entities.
 * For a given "statistic" (number of thread, thread load, ...) values are held
 * at some offset (ma_stats_nbthreads_offset for instance) in the ma_stats_t
 * buffers. It's up to the caller to know how to cast the pointer. For instance,
 * 
 * load = *(long*)ma_stats_get(object, marcel_stats_load_offset);
 *
 * For declaring a statistics, you just need to call ma_stats_alloc to which
 * you give
 * - the size of the data
 * - the reset function which must set a neutral value for the statistics
 * - the synthesis function which "adds" one value to an accumulation buffer.
 *
 * With just this, ma_stats_synthesize is able to iterate over all statistics,
 * call reset on the synthesis buffer of bubbles, and accumulate values in it.
 *
 * @{
 */

#section variables
/** \brief Offset of the "load" statistics (arbitrary user-provided) */
extern unsigned long marcel_stats_load_offset;

#section marcel_variables
/** \brief Offset of the "number of thread" statistics */
extern unsigned long ma_stats_nbthreads_offset;
/** \brief Offset of the "number of ghost thread" statistics */
extern unsigned long ma_stats_nbghostthreads_offset;
/** \brief Offset of the "number of running thread" statistics */
extern unsigned long ma_stats_nbrunning_offset;
/** \brief Offset of the "last ran time" statistics */
extern unsigned long ma_stats_last_ran_offset;
/** \brief Offset of the "memory usage" statistics */
extern unsigned long ma_stats_memory_offset;

#section marcel_types
/** \brief type of a statistics buffer */
typedef char ma_stats_t[MARCEL_STATS_ROOM];
/** \brief type of a synthesizing function. */
typedef void ma_stats_synthesis_t(void * __restrict dest, const void * __restrict src);
/** \brief type of a reset function. */
typedef void ma_stats_reset_t(void *dest);

#section marcel_variables
/* Storage for declared reset functions, synthesis functions and size of
 * statistics */
extern ma_stats_t ma_stats_reset_func, ma_stats_synthesis_func, ma_stats_size;

#section marcel_macros
#define __ma_stats_get(stats, offset) ((void*)&((stats)[offset]))
/** \brief Get the statistical value at the given offset in the \e stats member
 * of the given object */
#define ma_stats_get(object, offset) __ma_stats_get((object)->stats, (offset))
/** \brief Return the reset function for the statistics at the given offset */
#define ma_stats_reset_func(offset) (*(ma_stats_reset_t **)__ma_stats_get(ma_stats_reset_func, (offset)))
/** \brief Return the synthesis function for the statistics at the given offset */
#define ma_stats_synthesis_func(offset) (*(ma_stats_synthesis_t **)__ma_stats_get(ma_stats_synthesis_func, (offset)))
/** \brief Return the size of the statistics at the given offset */
#define ma_stats_size(offset) (*(size_t *)__ma_stats_get(ma_stats_size, (offset)))
/** \brief Reset all statistics for the given object */
#define ma_stats_reset(object) __ma_stats_reset((object)->stats)
/** \brief Synthesize all statistics for the given objects */
#define ma_stats_synthesize(dest, src) __ma_stats_synthesize((dest)->stats, (src)->stats)

#section marcel_functions
/** \brief Declare a new statistics */
unsigned long ma_stats_alloc(ma_stats_reset_t *reset_function, ma_stats_synthesis_t *synthesis_function, size_t size);
void __ma_stats_reset(ma_stats_t stats);
void __ma_stats_synthesize(ma_stats_t dest, ma_stats_t src);

/** "Sum" synthesis function for longs */
ma_stats_synthesis_t ma_stats_long_sum_synthesis;
/** "Sum" reset function for longs (set to 0) */
ma_stats_reset_t ma_stats_long_sum_reset;
/** "Max" synthesis function for longs */
ma_stats_synthesis_t ma_stats_long_max_synthesis;
/** "Max" reset function for longs (set to 0) */
ma_stats_reset_t ma_stats_long_max_reset;

#section functions
#depend "marcel_sched_generic.h[types]"

long *ma_stats_get(marcel_t t, unsigned long offset);
/** \brief Application-level function for accessing statistics of a given thread (or the current one if \e t is NULL) */
#define marcel_stats_get(t,kind) (t, marcel_stats_##kind##_offset);

long *ma_bubble_stats_get(marcel_bubble_t *b, unsigned long offset);
/** \brief Application-level function for accessing statistics of a given bubble. */
#define marcel_bubble_stats_get(b,kind) (b, marcel_stats_##kind##_offset);

/* @} */
