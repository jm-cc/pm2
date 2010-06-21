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


#ifndef __MARCEL_CONTAINER_H__
#define __MARCEL_CONTAINER_H__


#include "sys/marcel_flags.h"
#include "linux_spinlock.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define OBJ_MINSIZE (sizeof(ma_node_t))
/** Internal data types **/
    typedef struct ma_node_t {
	struct ma_node_t *next_node;
	void *obj;
} ma_node_t;

typedef struct ma_container_t {
	ma_node_t *first_node;
	int nb_element;
	int conservative;
	int max_size;
	ma_spinlock_t lock;
} ma_container_t;


/** Internal functions **/
void ma_container_add(ma_container_t * container, void *obj);

void *ma_container_get(ma_container_t * container);

int ma_container_nb_element(ma_container_t * container);

int ma_container_is_full(ma_container_t * container);

void ma_lock_container(ma_container_t * container);

void ma_unlock_container(ma_container_t * container);

void ma_container_init(ma_container_t * container, int conservative, int max_size);

void ma_container_clear(ma_container_t * container, void (*destroy) (void *, void *),
			void *destroy_arg);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_CONTAINER_H__ **/
