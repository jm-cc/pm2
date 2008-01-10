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
/* Initialization */
typedef int (*ma_bubble_sched_init)(void);

/* Start */
typedef int (*ma_bubble_sched_start)(void);

/* Termination */
typedef int (*ma_bubble_sched_exit)(void);

/* Submission of a set of entities to be scheduled */
typedef int (*ma_bubble_sched_submit)(marcel_entity_t *);

/* Called when a vp is idle.  Preemption and bottom halves are already disabled.  */
typedef int (*ma_bubble_sched_vp_is_idle)(unsigned);

/* Called on bubble tick */
typedef int (*ma_bubble_sched_tick)(marcel_bubble_t *);

/* Called when basic scheduler encounters an entity on rq at priority idx.
 * Returns the next entity (must be thread) to be schedule (as well as its
 * locked holder in nexth), or NULL if ma_schedule() must restart (and in such
 * case nexth must have been unlocked. */
typedef marcel_entity_t* (*ma_bubble_sched_sched)(marcel_entity_t *nextent, ma_runqueue_t *rq, ma_holder_t **nexth, int idx);

struct ma_bubble_sched_struct {
  ma_bubble_sched_init init;
  ma_bubble_sched_start start;
  ma_bubble_sched_exit exit;
  ma_bubble_sched_submit submit;
  ma_bubble_sched_vp_is_idle vp_is_idle;
  ma_bubble_sched_tick tick;
  ma_bubble_sched_sched sched;
  void *priv;
};

#section types
typedef struct ma_bubble_sched_struct marcel_bubble_sched_t;

#section marcel_types
#depend "[marcel_structures]"
typedef struct ma_bubble_sched_struct *ma_bubble_sched_t;
