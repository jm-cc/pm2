/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#ifdef MAMI_ENABLED

#ifndef MAMI_MARCEL_PRIVATE_H
#define MAMI_MARCEL_PRIVATE_H

#include <marcel.h>
#include <tbx_fast_list.h>
#include "mami_list.h"

/** \brief Type of a link for a list of tree nodes */
typedef struct mami_data_link_s mami_data_link_t;

/** \brief Structure of a link for a list of tree nodes */
struct mami_data_link_s {
        struct tbx_fast_list_head list;
        mami_data_t *data;
};

//typedef struct _mami_entity_s mami_entity_t;
LIST_TYPE(mami_entity,
          char type;
          union {
                  marcel_t task;
#ifdef MARCEL_BUBBLES
                  marcel_bubble_t *bubble;
#endif /* MARCEL_BUBBLES */
          } owner;
          );

#define MAMI_ENTITY_IS_TASK(entity) (entity->type == 't')
#define MAMI_ENTITY_IS_BUBBLE(entity) (entity->type == 'b')
#define MAMI_ENTITY_SET_TASK_OWNER(entity, taskowner) entity->type = 't'; entity->owner.task = taskowner;
#ifdef MARCEL_BUBBLES
#  define MAMI_ENTITY_GET_OWNER(entity) (entity->type == 't'?(void *)entity->owner.task:(void *)entity->owner.bubble)
#  define MAMI_ENTITY_SET_BUBBLE_OWNER(entity, bubbleowner) entity->type = 'b'; entity->owner.bubble = bubbleowner;
#else
#  define MAMI_ENTITY_GET_OWNER(entity) (entity->type == 't'?(void *)entity->owner.task:(void *)NULL)
#  define MAMI_ENTITY_SET_BUBBLE_OWNER(entity, bubbleowner) entity->type = 'b'; entity->owner.bubble = NULL;
#endif

extern
int _mami_entity_attach(mami_manager_t *memory_manager,
                        void *buffer,
                        size_t size,
                        mami_entity_t entity,
                        int *node);
extern
int _mami_update_stats_for_entities(mami_manager_t *memory_manager,
                                    mami_data_t *data,
                                    int coeff);
extern
int _mami_update_stats_for_entity(mami_manager_t *memory_manager,
                                  mami_data_t *data,
                                  mami_entity_t entity,
                                  int coeff);

#endif /* MAMI_MARCEL_PRIVATE_H */
#endif /* MAMI_ENABLED */
