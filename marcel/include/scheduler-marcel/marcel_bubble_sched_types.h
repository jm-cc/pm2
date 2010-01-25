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


#ifndef __MARCEL_BUBBLE_SCHED_TYPES_H__
#define __MARCEL_BUBBLE_SCHED_TYPES_H__


#include "tbx_compiler.h"
#include "tbx_fast_list.h"
#include "tbx_types.h"
#include "marcel_config.h"
#include "marcel_types.h"
#include "marcel_spin.h"
#ifdef MM_HEAP_ENABLED
#include "mm_heap_alloc.h"
#include "mm_heap_numa_alloc.h"
#endif


/** Public data types **/
typedef struct ma_bubble_sched_class  marcel_bubble_sched_class_t;
typedef struct ma_bubble_sched_struct marcel_bubble_sched_t;

/* \brief Holder type wrapper. */
typedef struct ma_holder ma_holder_t;
typedef struct ma_entity marcel_entity_t;

/** \brief Type of a bubble */
typedef struct marcel_bubble marcel_bubble_t;


/** Public data structures **/
/** \brief Holder types: runqueue or bubble */
enum marcel_holder {
	/** \brief runqueue [::ma_runqueue] type holder. */
	MA_RUNQUEUE_HOLDER,
#ifdef MA__BUBBLES
	/** \brief bubble [::marcel_bubble] type holder. */
	MA_BUBBLE_HOLDER,
#endif
};

/** \brief Entity types: bubble or thread */
enum marcel_entity {
#ifdef MA__BUBBLES
	/** \brief bubble [::marcel_bubble] type entity. */
	MA_BUBBLE_ENTITY,
#endif
	/** \brief thread type entity (task [::marcel_task] subtype). */
	MA_THREAD_ENTITY,
	/** \brief thread seed (task [::marcel_task] subtype)*/
	MA_THREAD_SEED_ENTITY,
};

/** \brief Virtual class of an entity holder. An \e holder is a container
 * dedicated to contain \e entities of type ::ma_entity.  Examples of holder
 * subclasses include runqueues (::ma_runqueue) and bubbles (::marcel_bubble). */
struct ma_holder {
	/** \brief Dynamic type of the effective holder subclass. */
	enum marcel_holder type;
	/** \brief Lock of holder */
	ma_spinlock_t lock;
	/** \brief List of entities running or ready to run in this holder */
	struct tbx_fast_list_head ready_entities;
	/** \brief Number of entities in the ma_holder::ready_entities list above */
	unsigned long nb_ready_entities;

	/** \brief Name of the holder */
	char name[MARCEL_MAXNAMESIZE];

#ifdef MARCEL_STATS_ENABLED
	/** \brief Synthesis of statistics of contained entities */
	ma_stats_t stats;
#endif /* MARCEL_STATS_ENABLED */
};

/** \brief Virtual class of a holder entity. An \e entity is a containee of a \e holder (::ma_holder).
 * Examples of entity subclasses include tasks (::marcel_task) and bubbles (::marcel_bubble).
 *
 * An entity is (potentially):
 * - held in some natural, application-structure-dependent, holder
 *   (#natural_holder)
 * - put on some application-state-dependent scheduling holder (#sched_holder)
 *   by bubble scheduler algorithms, indicating a target onto which the entity
 *   should migrate as soon as
 *   _possible_ (but not necessarily immediately) if it is not already running
 *   there
 * - queued or running on some ready holder (#ready_holder).
 *
 * That is, the #sched_holder and the #ready_holder work as a dampening team in a
 * way similar to double buffering schemes where the #ready_holder represents the
 * last instantiated scheduling decision while the #sched_holder represents the
 * upcoming scheduling decision that the scheduling algorithm is currently busy
 * figuring out.
 *
 * If #ready_holder_data is \p NULL then the entity is currently running. Otherwise, it is
 * currently preempted out and can be found in #cached_entities_item.
 */
struct ma_entity {
	/** \brief Dynamic type of the effective entity subclass. */
	enum marcel_entity type;
	/** \brief Natural holder, i.e. the one that the programmer hinted out of the application structure
	 * (the holding bubble, typically). */
	ma_holder_t *natural_holder;
	/** \brief Scheduling holder, i.e. the holder currently selected by the scheduling algorithms out
	 * of application activity state. It may be modified independently from the ready_holder, in which
	 * case it constitutes a target toward which the entity should migrate in due time. */
	ma_holder_t *sched_holder;
	/** \brief Ready holder, i.e. the transient holder on which the entity is currently running or
	 * queued as eligible for running once its turn comes. */
	ma_holder_t *ready_holder;
	/** \brief Data for the running holder. If the ma_entity#ready_holder is a runqueue \e and if the entity
	 * is \e not running, then the field is pointer to the runqueue's priority queue in which the entity
	 * is currently enqueued */
	void *ready_holder_data;
	/** \brief Item linker to the list of cached/enqueued entities sorted for schedule in this holder.
	 * The list is either marcel_bubble#cached_entities if the
	 * ma_entity#ready_holder is a bubble or ma_runqueue#active if the holder is
	 * a runqueue. */
	struct tbx_fast_list_head cached_entities_item;
	/** \brief Scheduling policy code (see #__MARCEL_SCHED_AVAILABLE). */
	int sched_policy;
	/** \brief Current priority given to the entity. */
	int prio;
	/** \brief Remaining time slice until the entity gets preempted out. */
	ma_atomic_t time_slice;
#ifdef MA__BUBBLES
        /* Marks the entity as settled on the current runqueue. Used by some
        bubble schedulers. (can be 0 or 1) */
        unsigned int settled;
	/** \brief Item linker to the list of natural entities
	 * (marcel_bubble#natural_entities)  in the entity's natural holding bubble. */
	struct tbx_fast_list_head natural_entities_item;
#endif
	/** \brief Item linker to the cumulative list of ready and running entities
	 * (ma_holder#ready_entities) in the entity's ma_entity#ready_holder. */
	struct tbx_fast_list_head ready_entities_item;

#ifdef MA__LWPS
	/** \brief Depth of the (direct or indirect) runqueue onto which the entity is currently scheduled. */
	int sched_level;
#endif

	/** \brief Name of the entity, for debugging purpose. */
	char name[MARCEL_MAXNAMESIZE];

#ifdef MARCEL_STATS_ENABLED
	/** \brief Statistics for the entity */
	ma_stats_t stats;
#endif /* MARCEL_STATS_ENABLED */

#ifdef MM_MAMI_ENABLED
	/** \brief List of memory areas attached to the entity.*/
	struct tbx_fast_list_head memory_areas;
	/** \brief Lock for serializing access to ma_entity#memory_areas */
	marcel_spinlock_t memory_areas_lock;

#endif /* MM_MAMI_ENABLED */

#ifdef MM_HEAP_ENABLED
	/** \brief Back pointer to the NUMA heap allocator used to allocated this object */
	heap_heap_t *heap;
#endif /* MM_HEAP_ENABLED */

#ifdef MA__BUBBLES
	/** \brief General-purpose list item linker for bubble schedulers */
	struct tbx_fast_list_head next;
#endif
};

/** \brief Structure of a bubble. */
struct marcel_bubble {
#ifdef MA__BUBBLES
	/* garder en premier, pour que les conversions bubble / entity soient
	 * triviales */
	/** \brief Entity information */
	struct ma_entity as_entity;
	/** \brief Holder information */
	struct ma_holder as_holder;
	/** \brief List of entities for which the bubble is a natural holder */
	struct tbx_fast_list_head natural_entities;
	/** \brief Number of held entities */
	unsigned nb_natural_entities;
	/** \brief Semaphore for the join operation */
	marcel_sem_t join;

	/** \brief List of entities queued in the so-called "thread cache" */
	struct tbx_fast_list_head cached_entities;
	/** \brief Number of threads that we ran in a row, shouldn't be greater than hold->nb_ready_entities. */
	int num_schedules;

        /** \brief Barrier for the barrier operation */
	marcel_barrier_t barrier;

	/** \brief Whether the bubble is preemptible, i.e., whether one of its
			threads can be preempted by a thread from another bubble.  */
	tbx_bool_t not_preemptible;

        /** \brief Dead bubbles (temporary field)*/
	int old;

        /** bubble identifier */
        int id;
#endif
};


#endif /** __MARCEL_BUBBLE_SCHED_TYPES_H__ **/
