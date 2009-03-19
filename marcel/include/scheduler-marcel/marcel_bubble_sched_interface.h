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

/* Called to force a new distribution from the marcel_root_bubble bubble. */
typedef int (*ma_bubble_sched_shake)(struct ma_bubble_sched_struct *);

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
	const struct ma_bubble_sched_class *klass;

  ma_bubble_sched_init init;
  ma_bubble_sched_start start;
  ma_bubble_sched_exit exit;
  ma_bubble_sched_submit submit;
  ma_bubble_sched_shake shake;
  ma_bubble_sched_vp_is_idle vp_is_idle;
  ma_bubble_sched_tick tick;
  ma_bubble_sched_sched sched;
};


#section types
#depend "marcel_bubble_sched_interface.h[marcel_structures]"

/** \brief A bubble scheduler class.  */
struct ma_bubble_sched_class {
	/** \brief The class name.  */
	const char *name;

	/** \brief The size (in bytes) of an instance of this class.  */
	size_t instance_size;

	/** \brief A method to initialize an instance of this class with default
			parameter values.  */
	int (*instantiate)(struct ma_bubble_sched_struct *);
};

typedef struct ma_bubble_sched_class  marcel_bubble_sched_class_t;
typedef struct ma_bubble_sched_struct marcel_bubble_sched_t;


#section macros

/**
 * \brief Declare a bubble scheduler class named \param _name.  */
#define MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS(_name)										\
  struct marcel_bubble_ ## _name ## _sched;															\
  typedef struct marcel_bubble_ ## _name ## _sched											\
          marcel_bubble_ ## _name ## _sched_t;													\
  extern marcel_bubble_sched_class_t marcel_bubble_ ## _name ## _sched_class

/**
 * \brief Define a bubble scheduler class named \param _name, with \param
 * _default_ctor the default initializer for instances of this class.  */
#define MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS(_name, _default_ctor)			\
  marcel_bubble_sched_class_t marcel_bubble_ ## _name ## _sched_class =	\
	  {																																		\
			.name = TBX_STRING(_name),																				\
			.instance_size = sizeof(struct marcel_bubble_ ## _name ## _sched), \
			.instantiate = _default_ctor																			\
		};
