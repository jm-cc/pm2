
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

	/* Mode conservatif */
	if (container->conservative == 1) {
		/*On alloue un nouveau noeud separement */
		new_node = ma_obj_alloc(ma_node_allocator);
	}
	/* Mode non conservatif */
	else {
		/*On stocke le noeud dans l'objet */
		new_node = obj;
	}

	ma_lock_container(container);

	new_node->next_node = container->first_node;
	new_node->obj = obj;

	/* Ajoute le lien a la liste */
	container->first_node = new_node;

	container->nb_element++;

	ma_unlock_container(container);
}

void *ma_container_get(ma_container_t * container)
{
	ma_node_t *node;
	void *obj;

	if (!container->first_node)
		return NULL;

	ma_lock_container(container);

	node = container->first_node;

	if (!node) {
		ma_unlock_container(container);
		return NULL;
	}

	/* Si la liste n'est pas vide */
	/* Recupere le premier noeud et le retire de la liste */

	container->first_node = node->next_node;
	obj = node->obj;

	if (container->conservative == 1)
		ma_obj_free(ma_node_allocator, node);

	container->nb_element--;

	ma_unlock_container(container);
	/* Retourne l'objet */
	return obj;
}

void ma_container_fini(ma_container_t * container, void (*destroy) (void *,
	void *), void *destroy_arg)
{
	ma_node_t *ptr1, *ptr2;

	ma_lock_container(container);

	ptr1 = container->first_node;

	while (ptr1) {
		ptr2 = ptr1;
		ptr1 = ptr1->next_node;
		/* Supprime l' objet */
		if (destroy)
			destroy(ptr2->obj, destroy_arg);
		if (container->conservative == 1) {
			/* Supprime le noeud */
			ma_obj_free(ma_node_allocator, ptr2);
		}
	}
}

int ma_container_nb_element(ma_container_t * container)
{
	return container->nb_element;
}

int ma_container_plein(ma_container_t * container)
{
	return ma_container_nb_element(container) >= container->max_size;
}

void ma_lock_container(ma_container_t * container)
{
	ma_spin_lock(&(container->lock));
}

void ma_unlock_container(ma_container_t * container)
{
	ma_spin_unlock(&(container->lock));
}

void ma_container_init(ma_container_t * container, int conservative,
    int max_size)
{
	container->first_node = NULL;
	container->nb_element = 0;
	container->conservative = conservative;
	container->max_size = max_size;
	ma_spin_lock_init(&container->lock);
}
