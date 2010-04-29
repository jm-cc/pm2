/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

#include <marcel.h>

#ifdef MA__BUBBLES

/* Add the _e_ entity at position _j_ in the
   _distribution_->entities array. */
void
ma_distribution_add (marcel_entity_t *e,
		     ma_distribution_t *distribution,
		     unsigned int position) {
  MA_BUG_ON (position != 0 && position >= distribution->nb_entities);
  MA_BUG_ON (distribution->nb_entities >= distribution->max_entities);
  if (distribution->entities[position]) {
    unsigned int size = sizeof (distribution->entities[0]);
    memmove (&distribution->entities[position+1],
	     &distribution->entities[position],
	     (distribution->nb_entities - position) * size);
  }
  distribution->entities[position] = e;
  distribution->nb_entities++;
  distribution->total_load += ma_entity_load (e);
}

/* Add the _e_ entity at the tail of the _distribution_->entities
   array. */
void
ma_distribution_add_tail (marcel_entity_t *e, ma_distribution_t *distribution) {
  MA_BUG_ON (distribution->nb_entities >= distribution->max_entities);
  distribution->entities[distribution->nb_entities] = e;
  distribution->nb_entities++;
  distribution->total_load += ma_entity_load (e);
}

/* Clean the fields of the ma_distribution_t structure passed in
   argument. */
void
ma_distribution_clean (ma_distribution_t *distribution) {
  bzero (distribution->entities, distribution->max_entities * sizeof (distribution->entities[0]));
  distribution->nb_entities = 0;
  distribution->total_load = 0;
}

/* Remove and return the last entity from the
   _distribution->entities array. */
marcel_entity_t *
ma_distribution_remove_tail (ma_distribution_t *distribution) {
  MA_BUG_ON (!distribution->nb_entities);
  marcel_entity_t *removed_entity = distribution->entities[distribution->nb_entities - 1];
  distribution->nb_entities--;
  distribution->total_load -= ma_entity_load (removed_entity);

  return removed_entity;
}

/* Return the index of the least loaded children topo_level, according
   to the information gathered in _distribution_. */
unsigned int
ma_distribution_least_loaded_index (const ma_distribution_t *distribution, unsigned int arity) {
  unsigned int i, res = 0;
  for (i = 1; i < arity; i++) {
    if (distribution[i].total_load < distribution[res].total_load)
      res = i;
  }

  return res;
}

/* Return the index of the most loaded children topo_level, according
   to the information gathered in _distribution_. */
unsigned int
ma_distribution_most_loaded_index (const ma_distribution_t *distribution, unsigned int arity) {
  unsigned int i, res = 0;
  for (i = 1; i < arity; i++) {
    if (distribution[i].total_load > distribution[res].total_load)
      res = i;
  }

  return res;
}

/* Physically moves entities according to the logical distribution
   proposed in _distribution_. */
void
ma_apply_distribution (ma_distribution_t *distribution, unsigned int arity) {
  unsigned int i, j;
  for (i = 0; i < arity; i++) {
    for (j = 0; j < distribution[i].nb_entities; j++) {
      ma_move_entity (distribution[i].entities[j], &distribution[i].level->rq.as_holder);
    }
  }
}

/* Debugging function that prints the address of every entity on each
   level included in _distribution_. */
void
ma_print_distribution (const ma_distribution_t *distribution, unsigned int arity) {
  unsigned int i, j;
  for (i = 0; i < arity; i++) {
    marcel_fprintf (stderr, "children[%d] = {", i);
    if (!distribution[i].nb_entities)
      marcel_fprintf (stderr, " }\n");
    for (j = 0; j < distribution[i].nb_entities; j++) {
      marcel_fprintf (stderr, (j != distribution[i].nb_entities - 1) ? " %p, " : " %p }\n", distribution[i].entities[j]);
    }
  }
}

#endif /* MA__BUBBLES */
