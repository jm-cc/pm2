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

/** \file
 * \brief Bubble scheduler interface
 */

/**
 ******************************************************************************
 * scheduler view
 * \defgroup marcel_bubble_sched Bubbles - scheduler programming interface
 *
 * This is the scheduler interface for manipulating bubbles.
 *
 */
/*@{*/

#section marcel_structures
#depend "scheduler/marcel_holder.h[types]"

/** \brief Forward declaration.  */
struct ma_bubble_sched_struct;

typedef int (*ma_bubble_sched_init)(struct ma_bubble_sched_struct *);
typedef int (*ma_bubble_sched_start)(struct ma_bubble_sched_struct *);
typedef int (*ma_bubble_sched_exit)(struct ma_bubble_sched_struct *);
typedef int (*ma_bubble_sched_submit)(struct ma_bubble_sched_struct *, marcel_entity_t *);
typedef int (*ma_bubble_sched_shake)(struct ma_bubble_sched_struct *);
typedef int (*ma_bubble_sched_vp_is_idle)(struct ma_bubble_sched_struct *, unsigned);
typedef int (*ma_bubble_sched_tick)(struct ma_bubble_sched_struct *, marcel_bubble_t *);
typedef marcel_entity_t *
(*ma_bubble_sched_sched)(struct ma_bubble_sched_struct *,
		marcel_entity_t *nextent, ma_runqueue_t *rq,
		ma_holder_t **nexth, int idx);

/** \brief Bubble scheduler instances.  */
struct ma_bubble_sched_struct {
	const struct ma_bubble_sched_class *klass;

	/** \brief Initialization */
	ma_bubble_sched_init init;

	/** \brief Start */
	ma_bubble_sched_start start;

	/** \brief Termination */
	ma_bubble_sched_exit exit;

	/** \brief Submission of a set of entities to be scheduled */
	ma_bubble_sched_submit submit;

	/** \brief Called to force a new distribution from a specific bubble. */
	ma_bubble_sched_shake shake;

	/** \brief Called when a vp is idle.  Preemption and bottom halves are already
	 * disabled.  Return non-zero if the bubble scheduler's work stealing algorithm
	 * succeeded, zero otherwise.  */
	ma_bubble_sched_vp_is_idle vp_is_idle;

	/** \brief Called on bubble tick */
	ma_bubble_sched_tick tick;

	/* Called when basic scheduler encounters an entity on rq at priority idx.
	 * Returns the next entity (must be thread) to be schedule (as well as its
	 * locked holder in nexth), or NULL if ma_schedule() must restart (and in such
	 * case nexth must have been unlocked. */
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
 * \brief Declare a bubble scheduler class named \a _name.  */
#define MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS(_name)										\
  struct marcel_bubble_ ## _name ## _sched;															\
  typedef struct marcel_bubble_ ## _name ## _sched											\
          marcel_bubble_ ## _name ## _sched_t;													\
  extern marcel_bubble_sched_class_t marcel_bubble_ ## _name ## _sched_class

/**
 * \brief Define a bubble scheduler class named \a _name, with \a _default_ctor 
 * the default initializer for instances of this class.  */
#define MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS(_name, _default_ctor)			\
  marcel_bubble_sched_class_t marcel_bubble_ ## _name ## _sched_class =	\
	  {																																		\
			.name = TBX_STRING(_name),																				\
			.instance_size = sizeof(struct marcel_bubble_ ## _name ## _sched), \
			.instantiate = _default_ctor																			\
		};

/*@}*/

