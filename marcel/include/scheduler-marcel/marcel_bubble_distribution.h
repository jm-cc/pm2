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


#ifndef __MARCEL_BUBBLE_DISTRIBUTION_H__
#define __MARCEL_BUBBLE_DISTRIBUTION_H__


#include "sys/marcel_flags.h"


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/* Initialize an array of ma_distribution_t describing the relations
   between the _nb_entities_ entities scheduled on level _from_ and
   the underlying runqueues (from->children). */
#define ma_distribution_init(levels, from, arity, _nb_entities)	\
do									\
  {									\
    unsigned int __i;							\
									\
    for (__i = 0; __i < ((arity) ? (arity) : 1); __i++) {		\
      (levels)[__i].entities =						\
	(ma_entity_ptr_t *) alloca ((_nb_entities) *			\
				    (sizeof (*(levels)[0].entities)));	\
      (levels)[__i].nb_entities = 0;					\
      (levels)[__i].level = (from != NULL) ? (from)->children[__i] : NULL; \
      (levels)[__i].max_entities = (_nb_entities);			\
      (levels)[__i].total_load = (from != NULL)				\
	? ma_load_from_children ((from)->children[__i])			\
	: 0;								\
									\
      MA_BUG_ON (!(levels)[__i].entities);				\
    }									\
  }									\
 while (0)

/* Destroy an array of _arity_ elements of ma_distribution_t. */
#define ma_distribution_destroy(distribution, arity)	\
  do { } while (0)


/** Internal data types **/
typedef marcel_entity_t * ma_entity_ptr_t;
typedef struct ma_distribution ma_distribution_t;


/** Internal data structures **/
/* Structure that describes entities attracted to a specific level. */
struct ma_distribution {

  /* Array of entities drawn to level _level_ */
  ma_entity_ptr_t *entities;

  /* Number of elements of the _entities_ array */
  unsigned int nb_entities;

  /* Maximum size of entities array */
  unsigned int max_entities;

  /* Total load of entities on the current array */
  unsigned int total_load;

  /* Attraction level for the _entities_ entities */
  struct marcel_topo_level *level;
};


/** Internal functions **/
/* Add the _e_ entity at position _j_ in the
   _distribution_->entities array. */
void ma_distribution_add (marcel_entity_t *e, 
			  ma_distribution_t *distribution, 
			  unsigned int position);

/* Add the _e_ entity at the tail of the _distribution_->entities
   array. */
void ma_distribution_add_tail (marcel_entity_t *e, ma_distribution_t *distribution);

/* Remove and return the last entity from the
   _distribution_->entities array. */
marcel_entity_t *ma_distribution_remove_tail (ma_distribution_t *distribution);

/* Return the index of the least loaded children topo_level, according
   to the information gathered in _distribution_. */
unsigned int ma_distribution_least_loaded_index (const ma_distribution_t *distribution, 
						 unsigned int arity);

/* Return the index of the most loaded children topo_level, according
   to the information gathered in _distribution_. */
unsigned int ma_distribution_most_loaded_index (const ma_distribution_t *distribution, unsigned int arity);

/* Physically moves entities according to the logical distribution
   proposed in _distribution_. */
void ma_apply_distribution (ma_distribution_t *distribution, unsigned int arity);

/* Debugging function that prints the address of every entity on each
   level included in _distribution_. */
void ma_print_distribution (const ma_distribution_t *distribution, unsigned int arity);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_BUBBLE_DISTRIBUTION_H__ **/
