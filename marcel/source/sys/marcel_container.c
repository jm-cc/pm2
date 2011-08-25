
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

#include "marcel.h"

void ma_container_add(ma_container_t * container, void *obj)
{
	ma_node_t *new_node;

	if (container->conservative == 1) {
		/** conservative mode: create a copy of the object **/
		new_node = ma_obj_alloc(ma_node_allocator, NULL);
	}
	else
		new_node = obj;
	new_node->obj = obj;

	ma_lock_container(container);

	/** add the object into the container **/
	new_node->next_node = container->first_node;
	container->first_node = new_node;
	container->nb_element++;

	ma_unlock_container(container);
}

void *ma_container_get(ma_container_t * container)
{
	ma_node_t *node;
	void *obj;

	ma_lock_container(container);

	/** check if the container is empty **/
	node = container->first_node;
	if (tbx_unlikely(NULL == node)) {
		ma_unlock_container(container);
		return NULL;
	}

	/** retrieve the first element and remove it from the container **/
	container->first_node = node->next_node;
	container->nb_element--;

	ma_unlock_container(container);

	obj = node->obj;
	if (container->conservative == 1) {
		/** free previously allocated object **/
		ma_obj_free(ma_node_allocator, node, NULL);
	}

	return obj;
}

void ma_container_clear(ma_container_t * container, void (*destroy) (void *, void *), 
			void *destroy_arg)
{
	ma_node_t *head, *cur, *next;
	int conservative;

	ma_lock_container(container);
	head = container->first_node;
	if (tbx_unlikely(NULL == head)) { // container is empty
		ma_unlock_container(container);
		return;
	}
	
	/** mark the container as empty **/
	conservative = container->conservative;
	container->first_node = NULL;
	container->nb_element = 0;
	ma_unlock_container(container);

	/** call destroy function on each object **/
	cur = head;
	if (destroy) {
		do {
			next = cur->next_node;
			destroy(cur->obj, destroy_arg);
		} while ((cur = next));
	}

	/** free previously allocated objects **/
	cur = head;
	if (conservative == 1) {
		do {
			next = cur->next_node;
			ma_obj_free(ma_node_allocator, cur, NULL);
		} while ((cur = next));
	}
}

void ma_container_init(ma_container_t * container, int conservative, int max_size)
{
	container->first_node = NULL;
	container->nb_element = 0;
	container->conservative = conservative;
	container->max_size = max_size;
	ma_spin_lock_init(&container->lock);
}
