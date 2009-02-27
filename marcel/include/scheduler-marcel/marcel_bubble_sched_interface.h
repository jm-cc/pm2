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

#section marcel_structures
#depend "scheduler/marcel_holder.h[types]"

/* Forward declaration.  */
struct ma_bubble_sched_struct;

/* Initialization */
typedef int (*ma_bubble_sched_init)(struct ma_bubble_sched_struct *);

/* Start */
typedef int (*ma_bubble_sched_start)(struct ma_bubble_sched_struct *);

/* Termination */
typedef int (*ma_bubble_sched_exit)(struct ma_bubble_sched_struct *);

/* Submission of a set of entities to be scheduled */
typedef int (*ma_bubble_sched_submit)(struct ma_bubble_sched_struct *, marcel_entity_t *);

/* Called to force a new distribution from a specific bubble. */
typedef int (*ma_bubble_sched_shake)(struct ma_bubble_sched_struct *, marcel_bubble_t *);

/* Called when a vp is idle.  Preemption and bottom halves are already
 * disabled.  Return non-zero if the bubble scheduler's work stealing algorithm
 * succeeded, zero otherwise.  */
typedef int (*ma_bubble_sched_vp_is_idle)(struct ma_bubble_sched_struct *, unsigned);

/* Called on bubble tick */
typedef int (*ma_bubble_sched_tick)(struct ma_bubble_sched_struct *, marcel_bubble_t *);

/* Called when basic scheduler encounters an entity on rq at priority idx.
 * Returns the next entity (must be thread) to be schedule (as well as its
 * locked holder in nexth), or NULL if ma_schedule() must restart (and in such
 * case nexth must have been unlocked. */
typedef marcel_entity_t *
(*ma_bubble_sched_sched)(struct ma_bubble_sched_struct *,
												 marcel_entity_t *nextent, ma_runqueue_t *rq,
												 ma_holder_t **nexth, int idx);

/** \brief Bubble scheduler instances.  */
struct ma_bubble_sched_struct {
  ma_bubble_sched_init init;
  ma_bubble_sched_start start;
  ma_bubble_sched_exit exit;
  ma_bubble_sched_submit submit;
  ma_bubble_sched_shake shake;
  ma_bubble_sched_vp_is_idle vp_is_idle;
  ma_bubble_sched_tick tick;
  ma_bubble_sched_sched sched;
  void *priv;
};


#section types
typedef struct ma_bubble_sched_struct marcel_bubble_sched_t;

#section macros

/** \brief Define a bubble scheduler named \param name, whose
 * \code marcel_bubble_sched_t is initialized with
 * \param field_initializers.  */
#define MARCEL_DEFINE_BUBBLE_SCHEDULER(_name, _field_initializers...)	\
  marcel_bubble_sched_t marcel_bubble_ ## _name ## _sched =		\
    {									\
      _field_initializers						\
    }

