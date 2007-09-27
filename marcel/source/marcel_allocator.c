
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
#include <stdlib.h>
#include <sys/types.h>
#ifndef SOLARIS_SYS
#include <stdint.h>
#endif
#include <inttypes.h>

enum mode {
	ALLOC_METHOD,
	FREE_METHOD,
	NO_FREE_METHOD,
};

ma_container_t *ma_get_container(ma_allocator_t * allocator, enum mode mode);

ma_allocator_t *ma_node_allocator = NULL;
static ma_allocator_t *lwp_container_allocator = NULL;
static ma_allocator_t *level_container_allocator = NULL;
ma_per_sth_cur_t ma_per_lwp_cur = MA_PER_STH_CUR_INITIALIZER(MA_PER_LWP_ROOM);
ma_per_sth_cur_t ma_per_level_cur =
MA_PER_STH_CUR_INITIALIZER(MA_PER_LEVEL_ROOM);

void __marcel_init ma_allocator_init(void)
{
	lwp_container_allocator = ma_new_obj_allocator(1, (void *(*)(void *))
	    ma_per_lwp_alloc, (void *)
	    sizeof(ma_container_t), NULL, NULL, POLICY_GLOBAL, 0);
	level_container_allocator =
	    ma_new_obj_allocator(1, (void *(*)(void *)) ma_per_level_alloc,
	    (void *) sizeof(ma_container_t), NULL, NULL, POLICY_GLOBAL, 0);

	/* We bypass potential safe_malloc checks for the node allocator
	   as the node allocator is not freed for now */
	ma_node_allocator =
	    ma_new_obj_allocator(0, __ma_obj_allocator_malloc,
	    (void *) sizeof(ma_node_t),
	    __ma_obj_allocator_free, NULL, POLICY_HIERARCHICAL_MEMORY, 0);
}

void ma_allocator_exit(void)
{
	ma_obj_allocator_fini(level_container_allocator);
	ma_obj_allocator_fini(lwp_container_allocator);
	//ma_obj_allocator_fini(ma_node_allocator);
	ma_node_allocator = NULL;
	lwp_container_allocator = NULL;
	level_container_allocator = NULL;
}

ma_allocator_t *ma_new_obj_allocator(int conservative,
    void *(*create) (void *), void *create_arg,
    void (*destroy) (void *, void *),
    void *destroy_arg, enum ma_policy_t policy, int max_size)
{
	/* Initialisation de la structure de l'allocateur selon les parametres */
	ma_allocator_t *allocator = __marcel_malloc(sizeof(ma_allocator_t));
	allocator->create = create;
	allocator->create_arg = create_arg;
	allocator->policy = policy;
	allocator->destroy = destroy;
	allocator->destroy_arg = destroy_arg;
	allocator->conservative = conservative;
	allocator->max_size = max_size;
	allocator->init = 0;
	allocator->container.obj = NULL;
	ma_obj_allocator_init(allocator);
	return allocator;
}

void *ma_obj_alloc(ma_allocator_t * allocator)
{
	void *obj = NULL;
	ma_container_t *container;
	container = ma_get_container(allocator, ALLOC_METHOD);

	if (container)
		/* Essaye d'obtenir un objet du container */
		obj = ma_container_get(container);

	/* Si on en trouve un c'est bon */
	/* Sinon on doit en allouer un nouveau */
	if (!obj)
		obj = allocator->create(allocator->create_arg);

	return obj;
}

void ma_obj_free(ma_allocator_t * allocator, void *obj)
{
	ma_container_t *container;
	container = ma_get_container(allocator, allocator->destroy?FREE_METHOD:NO_FREE_METHOD);

	if (container)
		ma_container_add(container, obj);
	else if (allocator->destroy)
		allocator->destroy(obj, allocator->destroy_arg);
	else
		/* shouldn't ever happen */
		MA_WARN_ON(1);
}

void ma_obj_allocator_fini(ma_allocator_t * allocator)
{
#ifdef MA__LWPS
	int i, j;
#endif

	switch (allocator->policy) {
	case POLICY_GLOBAL:
		{
			ma_container_fini(ma_get_container
			    (allocator, ALLOC_METHOD),
			    allocator->destroy, allocator->destroy_arg);
			__marcel_free(allocator->container.obj);
		}
		break;

#ifdef MA__LWPS
	case POLICY_LOCAL:{
			struct marcel_topo_level *vp;
			for_all_vp(vp) {
				ma_container_fini(ma_per_level_data
				    (vp, allocator->container.offset),
				    allocator->destroy, allocator->destroy_arg);
			}
			ma_obj_free(lwp_container_allocator,
			    (void *) allocator->container.offset);
			break;
		}

	case POLICY_HIERARCHICAL:
	case POLICY_HIERARCHICAL_MEMORY:
		for (j = marcel_topo_nblevels-1; j > 0; ++j) {
			for (i = 0; marcel_topo_levels[j][i].vpset; ++i) {
				ma_container_fini(ma_per_level_data
				    (&marcel_topo_levels[j][i],
					(allocator->container.
					    offset)),
				    allocator->destroy, allocator->destroy_arg);
			}
#ifdef MA__NUMA
			if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
					marcel_topo_levels[j][0].type == MARCEL_LEVEL_NODE)
				/* Last memory level, stop here */
				break;
#endif
		}
		ma_obj_free(level_container_allocator,
		    (void *) allocator->container.offset);
		break;
#endif
	default:
		printf("la politique est mal choisie\n");
		break;
	}
}

void ma_obj_allocator_init(ma_allocator_t * allocator)
{
#ifdef MA__LWPS
	int i, j;
#endif

	if (!allocator)
		return;

	if (!(allocator->init)) {

		switch (allocator->policy) {
		case POLICY_GLOBAL:
			allocator->container.obj =
			    __marcel_malloc(sizeof(ma_container_t));
			ma_container_init(allocator->container.obj,
			    allocator->conservative, allocator->max_size);
			break;

#ifdef MA__LWPS
		case POLICY_LOCAL:{
				struct marcel_topo_level *vp;
				allocator->container.offset = (unsigned long)
				    ma_obj_alloc(lwp_container_allocator);

				for_all_vp(vp) {
					ma_container_init(ma_per_level_data
					    (vp, (allocator->
						    container.offset)),
					    allocator->
					    conservative, allocator->max_size);
				}
				break;
			}

		case POLICY_HIERARCHICAL:
		case POLICY_HIERARCHICAL_MEMORY:

			allocator->container.offset = (unsigned long)
			    ma_obj_alloc(level_container_allocator);
			for (j = marcel_topo_nblevels-1; j > 0; ++j) {
				for (i = 0; marcel_topo_levels[j][i].vpset; ++i) {
					ma_container_init(ma_per_level_data
					    (&marcel_topo_levels
						[j][i],
						(allocator->
						    container.offset)),
					    allocator->
					    conservative, allocator->max_size);
				}
#ifdef MA__NUMA
				if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
						marcel_topo_levels[j][0].type == MARCEL_LEVEL_NODE)
					/* Last memory level, stop here */
					break;
#endif
			}

			break;
#endif

		default:
			printf("La politique n'est pas bien choisie\n");
		}

		allocator->init = 1;

		ma_obj_allocator_init(ma_node_allocator);

	}
}

ma_container_t *ma_get_container(ma_allocator_t * allocator, enum mode mode)
{
	if (mode < 0 || mode > 2)
		printf
		    ("Erreur : le mode n'est pas bien defini -- ma_get_container\n");

	//mode 0 pour FREE_METHOD
	//mode 1 pour ALLOC_METHOD

	//On peut transformer tout ca avec un switch !

	if (allocator->policy == POLICY_GLOBAL) {
		return allocator->container.obj;
	}
#ifdef MA__LWPS
	if (allocator->policy == POLICY_LOCAL) {
		return ma_per_lwp_self_data(allocator->container.offset);
	}
	// On fait une boucle while qui remonte le niveau hierarchique 
	// jusqu'a ce que l'on trouve un container qui a un nombre d'element > 0.
	// Si on ne trouve pas de container plein sur toute la topologie, on 
	// retourne le container de niveau le plus bas (= le plus proche du processeur). 

	if ((allocator->policy == POLICY_HIERARCHICAL || allocator->policy == POLICY_HIERARCHICAL_MEMORY) && mode == ALLOC_METHOD) {
		struct marcel_topo_level *niveau_courant =
		    ma_per_lwp(vp_level, LWP_SELF);
		while (niveau_courant) {
			ma_container_t *container_courant;
			container_courant =
			    ma_per_level_data(niveau_courant,
			    allocator->container.offset);
			if (ma_container_nb_element(container_courant) > 0)
				return container_courant;
#ifdef MA__NUMA
			if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
					niveau_courant->type == MARCEL_LEVEL_NODE)
				/* Last memory level, stop here */
				break;
#endif
			niveau_courant = niveau_courant->father;
		}
		return NULL;
	}
	// Dans ce cas la, le container a retourner en premier est celui de niveau le plus 
	// bas si il est vide, sinon si il est rempli au-dela d'un certain niveau, il faut retourner 
	// le container au-dessus (dans la hierarchie) et ainsi de suite, 
	// 
	// Nota: désactivé pour les mesures:
	// si les containers sont tous 
	// pleins on retoune quand meme le container de plus proche du processeur : donc il va deborder...

	if ((allocator->policy == POLICY_HIERARCHICAL || allocator->policy == POLICY_HIERARCHICAL_MEMORY) && (mode == FREE_METHOD || mode == NO_FREE_METHOD)) {
		struct marcel_topo_level *niveau_courant =
		    ma_per_lwp(vp_level, LWP_SELF);
		while (niveau_courant) {
			ma_container_t *container_courant;
			container_courant =
			    ma_per_level_data(niveau_courant,
			    allocator->container.offset);
			if (!ma_container_plein(container_courant))
				return container_courant;
#ifdef MA__NUMA
			if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
					niveau_courant->type == MARCEL_LEVEL_NODE)
				/* Last memory level, stop here */
				break;
#endif
			niveau_courant = niveau_courant->father;
		}
		if (mode == FREE_METHOD) {
			/* trop-plein, libérer */
			return NULL;
		} else {
			return ma_per_level_data(marcel_machine_level,
			    allocator->container.offset);
		}
	}
#endif
	MA_BUG();
	return NULL;
}

void ma_obj_allocator_print(ma_allocator_t * allocator) {
	if (allocator->policy == POLICY_GLOBAL) {
		fprintf(stderr,"global: %d elements\n",allocator->container.obj->nb_element);
		return;
	}
	if (allocator->policy == POLICY_LOCAL) {
		ma_lwp_t lwp;
		fprintf(stderr,"local\n");
		for_all_lwp(lwp)
			fprintf(stderr,"%4d",LWP_NUMBER(lwp));
		fprintf(stderr,"\n");
		for_all_lwp(lwp)
			fprintf(stderr,"%4d",((ma_container_t*)ma_per_lwp_self_data(allocator->container.offset))->nb_element);
		fprintf(stderr,"\n");
		return;
	}
	if (allocator->policy == POLICY_HIERARCHICAL || allocator->policy == POLICY_HIERARCHICAL_MEMORY) {
		int i,j;
		fprintf(stderr,"hierarchical\n");
		for (j = marcel_topo_nblevels-1; j > 0; ++j) {
			for (i = 0; marcel_topo_levels[j][i].vpset; ++i)
				fprintf(stderr,"%4d",marcel_topo_levels[j][i].number);
			fprintf(stderr,"\n");
			for (i = 0; marcel_topo_levels[j][i].vpset; ++i)
				fprintf(stderr,"%4d",((ma_container_t*)ma_per_level_data(&marcel_topo_levels[j][i],(allocator->container.offset)))->nb_element);
			fprintf(stderr,"\n");
#ifdef MA__NUMA
			if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
					marcel_topo_levels[j][0].type == MARCEL_LEVEL_NODE)
				/* Last memory level, stop here */
				break;
#endif
		}
	}
}

void *ma_obj_allocator_malloc(void *arg)
{
	void *res;
	res = TBX_MALLOC((size_t) (intptr_t) arg);
	return res;
}

void ma_obj_allocator_free(void *obj, void *foo)
{
	TBX_FREE(obj);
}

/* safe_malloc bypass version
 */
void *__ma_obj_allocator_malloc(void *arg)
{
	void *res;
	res = __marcel_malloc((size_t) (intptr_t) arg);
	return res;
}

void __ma_obj_allocator_free(void *obj, void *foo)
{
	__marcel_free(obj);
}

unsigned long ma_per_sth_alloc(ma_per_sth_cur_t * cur, size_t size)
{
	unsigned long ret;
	ma_spin_lock(&cur->lock);
	size = (size + sizeof(void(*)())-1) & ~(sizeof(void(*)())-1);
	ret = cur->cur;
	cur->cur += size;
	MA_BUG_ON(cur->cur > cur->max);
	ma_spin_unlock(&cur->lock);
	return ret;
}

#undef ma_per_lwp_alloc
unsigned long ma_per_lwp_alloc(size_t size)
{
	return ma_per_sth_alloc(&ma_per_lwp_cur, size);
}

#undef ma_per_level_alloc
unsigned long ma_per_level_alloc(size_t size)
{
	return ma_per_sth_alloc(&ma_per_level_cur, size);
}
