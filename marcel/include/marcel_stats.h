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


#ifndef __MARCEL_STATS_H__
#define __MARCEL_STATS_H__


#include "sys/marcel_flags.h"
#include "marcel_types.h"
#include "scheduler/marcel_sched_types.h"
#include "marcel_sched_generic.h"


/** Public global variables **/
#ifdef MARCEL_STATS_ENABLED

#define MA_VPSTATS_NO_LAST_VP -1
#define MA_VPSTATS_CONFLICT -2

/** \brief Offset of the "load" statistics (arbitrary user-provided) */
extern unsigned long marcel_stats_load_offset;

/* TODO: Watch out! We make this available outside of Marcel for now
   to be able to set artificial memory guidelines in example
   programs. This should move to the marcel_variable section soon. */
#ifdef MM_MAMI_ENABLED
/** \brief Offset of the "per node allocated memory" (i.e. the total
    amount of memory per node allocated by the considered entity)
    statistics*/
extern unsigned long ma_stats_memnode_offset;
#endif /* MM_MAMI_ENABLED */

#endif /* MARCEL_STATS_ENABLED */


/** Public functions **/
#ifdef MARCEL_STATS_ENABLED

long *marcel_task_stats_get(marcel_t t, unsigned long offset);
/** \brief Application-level function for accessing statistics of a given thread (or the current one if \e t is NULL) */
#define marcel_stats_get(t,kind) marcel_task_stats_get(t, marcel_stats_##kind##_offset)

/** \brief Application-level function for accessing statistics of a given bubble. */
#define marcel_bubble_stats_get(b,kind) ma_bubble_stats_get(b, marcel_stats_##kind##_offset)

#endif /* MARCEL_STATS_ENABLED */


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifdef MARCEL_STATS_ENABLED

#define __ma_stats_get(stats, offset) ((void*)&((stats)[offset]))
/** \brief Gets the statistical value at the given offset in the \e stats member
 * of the given object */
#define ma_stats_get(object, offset) __ma_stats_get((object)->stats, (offset))
#define ma_stats_reset_func(offset) (*(ma_stats_reset_t **)__ma_stats_get(ma_stats_reset_func, (offset)))
#define ma_stats_synthesis_func(offset) (*(ma_stats_synthesis_t **)__ma_stats_get(ma_stats_synthesis_func, (offset)))
#define ma_stats_size(offset) (*(size_t *)__ma_stats_get(ma_stats_size, (offset)))
/** \brief Resets all statistics for the given object */
#define ma_stats_reset(object) __ma_stats_reset((object)->stats)
/** \brief Synthesizes all statistics for the given objects */
#define ma_stats_synthesize(dest, src) __ma_stats_synthesize((dest)->stats, (src)->stats)

#endif /* MARCEL_STATS_ENABLED */

#ifdef MARCEL_STATS_ENABLED
#define ma_stats_set(cast,stats,offset,val) *(cast *) ma_stats_get((stats),(offset))=(val)
#define ma_stats_add(cast,stats,offset,val) *(cast *) ma_stats_get((stats),(offset))+=(val)
#define ma_stats_sub(cast,stats,offset,val) *(cast *) ma_stats_get((stats),(offset))-=(val)
#else /* MARCEL_STATS_ENABLED */
#define ma_stats_set(cast,stats,offset,val)
#define ma_stats_add(cast,stats,offset,val)
#define ma_stats_sub(cast,stats,offset,val)
#endif /* MARCEL_STATS_ENABLED */


/** Internal data types **/
#ifdef MARCEL_STATS_ENABLED

/** \brief Type of a synthesizing function. */
typedef void ma_stats_synthesis_t(void * __restrict dest, const void * __restrict src);
/** \brief Type of a reset function. */
typedef void ma_stats_reset_t(void *dest);

#endif /* MARCEL_STATS_ENABLED */


/** Internal global variables **/
#ifdef MARCEL_STATS_ENABLED

/** \brief Offset of the "number of thread" statistics */
extern unsigned long ma_stats_nbthreads_offset;
/** \brief Offset of the "number of thread seeds" statistics */
extern unsigned long ma_stats_nbthreadseeds_offset;
/** \brief Offset of the "number of running thread" statistics */
extern unsigned long ma_stats_nbrunning_offset;
/** \brief Offset of the "number of ready thread" statistics */
extern unsigned long ma_stats_nbready_offset;
/** \brief Offset of the "last ran time" statistics */
extern unsigned long ma_stats_last_ran_offset;
/** \brief Offset of the "memory usage" (i.e. the total amount of
    memory attached to the considered entity) statistics */
extern unsigned long ma_stats_memory_offset;
/** \brief Offset of the "last vp" statistics. The value stored at
    this offset represents the number of the last vp the considered
    entity was running on. MA_NO_LAST_VP means that the entity has
    never been executed (in case of a bubble, the whole set of
    included entities has never been
    executed.). MA_CONFLICTING_LAST_VP (only used for bubble entities)
    means that at least two of the contained entities have different
    last_vps, so we can't elect any vp to be the "last_vp" of the
    whole considered bubble. */
extern unsigned long ma_stats_last_vp_offset;
/** \brief Offset of the "last topo_level" statistics, used by bubble
    schedulers to keep track of where the entities were scheduled
    before. */
extern unsigned long ma_stats_last_topo_level_offset;

/* Storage for declared reset functions, synthesis functions and size of
 * statistics */
extern ma_stats_t ma_stats_reset_func, ma_stats_synthesis_func, ma_stats_size;

#endif /* MARCEL_STATS_ENABLED */


/** Internal functions **/
#ifdef MARCEL_STATS_ENABLED
/** \brief Declares a new statistics */
unsigned long ma_stats_alloc(ma_stats_reset_t *reset_function, ma_stats_synthesis_t *synthesis_function, size_t size);
void __ma_stats_reset(ma_stats_t stats);
void __ma_stats_synthesize(ma_stats_t dest, ma_stats_t src);

/** \brief "Sum" synthesis function for longs */
ma_stats_synthesis_t ma_stats_long_sum_synthesis;
/** \brief "Sum" reset function for longs (set to 0) */
ma_stats_reset_t ma_stats_long_sum_reset;
/** \brief "Max" synthesis function for longs */
ma_stats_synthesis_t ma_stats_long_max_synthesis;
/** \brief "Max" reset function for longs (set to 0) */
ma_stats_reset_t ma_stats_long_max_reset;
#ifdef MM_MAMI_ENABLED
/** \brief "Sum" synthesis function for the array of long describing
    the amount of memory allocated on each node. */
ma_stats_synthesis_t ma_stats_memnode_sum_synthesis;
/** \brief "Sum" reset function for the array of long describing the
    amount of memory allocated on each node. */
ma_stats_reset_t ma_stats_memnode_sum_reset;
#endif /* MM_MAMI_ENABLED */
/** \brief "Sum" synthesis function for the last_vp statistics. */
ma_stats_synthesis_t ma_stats_last_vp_sum_synthesis;
/** \brief "Sum" reset function for the last_vp statistics (set to -1) */
ma_stats_reset_t ma_stats_last_vp_sum_reset;
/** \brief "Sum" reset function for the last_topo_level statistics (set to nil) */
ma_stats_reset_t ma_stats_last_topo_level_sum_reset;
/* TODO: unused, just defined to avoid ma_stats_alloc issues
   (ma_stats_alloc doesn't handle NULL functions) */
ma_stats_synthesis_t ma_stats_last_topo_level_sum_synthesis;

#ifdef DOXYGEN
/** \brief Returns the reset function for the statistics at the given offset */
ma_stats_reset_t *ma_stats_reset_func(unsigned long offset);
/** \brief Returns the synthesis function for the statistics at the given offset */
ma_stats_synthesis_t *ma_stats_synthesis_func(unsigned long offset);
/** \brief Returns the size of the statistics at the given offset */
size_t *ma_stats_size(unsigned long offset);
#endif

/** \brief Recursive way to get a statistics of a Marcel entity (In
    case of a bubble, this function returns a synthesized statistics
    that reflects the statistics of all the contained entities.) */
long *ma_cumulative_stats_get (marcel_entity_t *e, unsigned long offset);
#endif /* MARCEL_STATS_ENABLED */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_STATS_H__ **/
