
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


enum mode {
	ALLOC_METHOD,
	FREE_METHOD,
	NO_FREE_METHOD
};

static ma_container_t *ma_container_from_allocator(ma_allocator_t * allocator, enum mode mode,
						   struct marcel_topo_level *level);

ma_allocator_t *ma_node_allocator = NULL;
static ma_allocator_t *lwp_container_allocator = NULL;
static ma_allocator_t *level_container_allocator = NULL;
ma_per_sth_cur_t ma_per_lwp_cur = MA_PER_STH_CUR_INITIALIZER(MA_PER_LWP_ROOM);
ma_per_sth_cur_t ma_per_level_cur = MA_PER_STH_CUR_INITIALIZER(MA_PER_LEVEL_ROOM);

void ma_allocator_init(void)
{
	MARCEL_LOG_IN();
	lwp_container_allocator = ma_new_obj_allocator(1, (void *(*)(void *))
						       ma_per_lwp_alloc, (void *)
						       sizeof(ma_container_t), NULL, NULL,
						       POLICY_GLOBAL, 0);
	level_container_allocator =
		ma_new_obj_allocator(1, (void *(*)(void *)) ma_per_level_alloc,
				     (void *) sizeof(ma_container_t), NULL, NULL,
				     POLICY_GLOBAL, 0);

	/* We bypass potential safe_malloc checks for the node allocator
	   as the node allocator is not freed for now */
	ma_node_allocator = ma_new_obj_allocator(0, __ma_obj_allocator_malloc,
						 (void *) sizeof(ma_node_t),
						 __ma_obj_allocator_free, NULL,
						 POLICY_HIERARCHICAL_MEMORY, 0);
	MARCEL_LOG_OUT();
}

void ma_allocator_exit(void)
{
	MARCEL_LOG_IN();
	ma_obj_allocator_fini(level_container_allocator);
	ma_obj_allocator_fini(lwp_container_allocator);
	//ma_obj_allocator_fini(ma_node_allocator);
	ma_node_allocator = NULL;
	lwp_container_allocator = NULL;
	level_container_allocator = NULL;
	MARCEL_LOG_OUT();
}

/**
 *
 * @param max_size if 0, the container will have a infinite size, i.e it will never be full
 */
ma_allocator_t *ma_new_obj_allocator(int conservative,
				     void *(*create) (void *), void *create_arg,
				     void (*destroy) (void *, void *),
				     void *destroy_arg, enum ma_policy_t policy,
				     int max_size)
{
	MARCEL_LOG_IN();
	/* allocator data structure initialization */
	ma_allocator_t *allocator = marcel_malloc(sizeof(ma_allocator_t));
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
	MARCEL_LOG_OUT();
	return allocator;
}

void *ma_obj_alloc(ma_allocator_t * allocator, struct marcel_topo_level *level)
{
	void *obj = NULL;
	ma_container_t *container;

	MARCEL_LOG_IN();
	container = ma_container_from_allocator(allocator, ALLOC_METHOD, level);

	if (container)
		/* try to get an object from the container */
		obj = ma_container_get(container);

	/* if previous op fails, we must allocate a new object */
	if (!obj) {
		marcel_extlib_protect();
		obj = allocator->create(allocator->create_arg);
		marcel_extlib_unprotect();
	}

	MARCEL_LOG_OUT();
	return obj;
}

void ma_obj_free(ma_allocator_t * allocator, void *obj, struct marcel_topo_level *level)
{
	ma_container_t *container;

	MARCEL_LOG_IN();
	container = ma_container_from_allocator(allocator, 
						allocator->destroy ? FREE_METHOD : NO_FREE_METHOD,
						level);

	if (container) {
		MARCEL_ALLOC_LOG("Put the object back in the container (%d)\n",
				 ma_container_nb_element(container));
		ma_container_add(container, obj);
	} else if (allocator->destroy) {
		marcel_extlib_protect();
		allocator->destroy(obj, allocator->destroy_arg);
		marcel_extlib_unprotect();
	}
	else
		/* shouldn't ever happen */
		MA_WARN_ON(1);
	MARCEL_LOG_OUT();
}

void ma_obj_allocator_fini(ma_allocator_t * allocator)
{
#ifdef MA__LWPS
	int i, j;
#endif

	MARCEL_LOG_IN();

	switch (allocator->policy) {
	case POLICY_GLOBAL:{
			ma_container_clear(ma_container_from_allocator(allocator, 
								       ALLOC_METHOD, NULL),
					   allocator->destroy, allocator->destroy_arg);
			marcel_free(allocator->container.obj);
			break;
		}

#ifdef MA__LWPS
	case POLICY_LOCAL:{
			struct marcel_topo_level *vp;
			for_all_vp(vp) {
				ma_container_clear(ma_per_level_data(vp, allocator->container.offset),
						   allocator->destroy,
						   allocator->destroy_arg);
			}
			ma_obj_free(lwp_container_allocator,
				    (void *) allocator->container.offset, NULL);
			break;
		}
	case POLICY_HIERARCHICAL:
	case POLICY_HIERARCHICAL_MEMORY:{
			for (j = marcel_topo_nblevels - 1; j >= 0; --j) {
				for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[j]
								 [i].vpset); ++i) {
					ma_container_clear(ma_per_level_data(&marcel_topo_levels[j][i], 
									     allocator->container.offset),
							   allocator->destroy,
							   allocator->destroy_arg);
				}
#ifdef MA__NUMA
				if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
				    (marcel_topo_levels[j][0].merged_type &
				     (1 << MARCEL_LEVEL_NODE)))
					/* Last memory level, stop here */
					break;
#endif
			}
			ma_obj_free(level_container_allocator,
				    (void *) allocator->container.offset, NULL);
			break;
		}
#endif
	default:{
		MARCEL_ALLOC_LOG("allocator policy is invalid\n");
		break;
	}
	}
	MARCEL_LOG_OUT();
}

void ma_obj_allocator_init(ma_allocator_t * allocator)
{
#ifdef MA__LWPS
	int i, j;
#endif

	MARCEL_LOG_IN();
	if (!allocator) {
		MARCEL_LOG_OUT();
		return;
	}

	if (!(allocator->init)) {
		switch (allocator->policy) {
		case POLICY_GLOBAL:
			allocator->container.obj = marcel_malloc(sizeof(ma_container_t));
			ma_container_init(allocator->container.obj,
					  allocator->conservative, allocator->max_size);
			break;
#ifdef MA__LWPS
		case POLICY_LOCAL:{
				struct marcel_topo_level *vp;
				allocator->container.offset =
					(unsigned long) ma_obj_alloc(lwp_container_allocator, NULL);
				for_all_vp(vp) {
					ma_container_init(ma_per_level_data(vp, allocator->container.offset),
							  allocator->conservative,
							  allocator->max_size);
				}
				break;
			}
		case POLICY_HIERARCHICAL:
		case POLICY_HIERARCHICAL_MEMORY:{
				allocator->container.offset = 
					(unsigned long)ma_obj_alloc(level_container_allocator, NULL);
				for (j = marcel_topo_nblevels - 1; j >= 0; --j) {
					for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[j][i].vpset); ++i) {
						ma_container_init(ma_per_level_data(&marcel_topo_levels[j][i],
										    allocator->container.
										    offset),
								  allocator->conservative,
								  allocator->max_size);
					}
#ifdef MA__NUMA
					if (allocator->policy == POLICY_HIERARCHICAL_MEMORY
					    && (marcel_topo_levels[j][0].merged_type & (1 << MARCEL_LEVEL_NODE)))
						/* Last memory level, stop here */
						break;
#endif
				}
				break;
			}
#endif
		default:
			MARCEL_ALLOC_LOG("allocator policy is invalid\n");
			break;
		}
		allocator->init = 1;

		ma_obj_allocator_init(ma_node_allocator);
	}
	MARCEL_LOG_OUT();
}

static ma_container_t *ma_container_from_allocator(ma_allocator_t * allocator, enum mode mode,
						   struct marcel_topo_level *level LWPS_VAR_UNUSED)
{
	if (mode > 2)
		MARCEL_ALLOC_LOG("mode is invalid\n");

	//mode 0 pour FREE_METHOD
	//mode 1 pour ALLOC_METHOD

	if (allocator->policy == POLICY_GLOBAL) {
		return allocator->container.obj;
	}
#ifdef MA__LWPS
	if (allocator->policy == POLICY_LOCAL) {
		if (ma_vpnum(MA_LWP_SELF) >= (int) marcel_nbvps())
			/* Compl�tement arbitraire, il faudrait voir ce qu'on fait pour les LWP suppl�mentaires. Sans doute au moins choisir le bon noeud NUMA, ou au moins allouer de mani�re cyclique */
			return ma_per_lwp_data(ma_get_lwp_by_vpnum(0),
					       allocator->container.offset);

		return ma_per_lwp_self_data(allocator->container.offset);
	}
	// On fait une boucle while qui remonte le niveau hierarchique
	// jusqu'a ce que l'on trouve un container qui a un nombre d'element > 0.
	// Si on ne trouve pas de container plein sur toute la topologie, on
	// retourne le container de niveau le plus bas (= le plus proche du processeur).

	if ((allocator->policy == POLICY_HIERARCHICAL
	     || allocator->policy == POLICY_HIERARCHICAL_MEMORY)
	    && mode == ALLOC_METHOD) {
		struct marcel_topo_level *niveau_courant =
		    level ? level : ma_per_lwp(vp_level, MA_LWP_SELF);

		if (ma_vpnum(MA_LWP_SELF) >= (int) marcel_nbvps()) {
			/* Compl�tement arbitraire, il faudrait voir ce qu'on fait pour les LWP suppl�mentaires. Sans doute au moins choisir le bon noeud NUMA, ou au moins allouer de mani�re cyclique */
			if (niveau_courant) {
				return ma_per_level_data(niveau_courant,
							 allocator->container.offset);
			} else if (allocator->policy == POLICY_HIERARCHICAL_MEMORY) {
				return ma_per_level_data(&marcel_topo_vp_level[0],
							 allocator->container.offset);
			} else {
				return ma_per_level_data(marcel_machine_level,
							 allocator->container.offset);
			}
		}

		while (niveau_courant) {
			ma_container_t *container_courant;
			container_courant = ma_per_level_data(niveau_courant,
							      allocator->container.offset);
			if (ma_container_nb_element(container_courant) > 0)
				return container_courant;
#ifdef MA__NUMA
			if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
			    (niveau_courant->merged_type & (1 << MARCEL_LEVEL_NODE)))
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
	// Nota: d�sactiv� pour les mesures:
	// si les containers sont tous
	// pleins on retoune quand meme le container de plus proche du processeur : donc il va deborder...

	if ((allocator->policy == POLICY_HIERARCHICAL
	     || allocator->policy == POLICY_HIERARCHICAL_MEMORY) && (mode == FREE_METHOD
								     || mode == NO_FREE_METHOD)) {
		struct marcel_topo_level *niveau_courant =
		    level ? level : ma_per_lwp(vp_level, MA_LWP_SELF);

		if (ma_vpnum(MA_LWP_SELF) >= (int) marcel_nbvps()) {
			/* Compl�tement arbitraire, il faudrait voir ce qu'on fait pour les LWP suppl�mentaires. Sans doute au moins choisir le bon noeud NUMA, ou au moins allouer de mani�re cyclique */
			if (niveau_courant) {
				return ma_per_level_data(niveau_courant,
							 allocator->container.offset);
			} else if (allocator->policy == POLICY_HIERARCHICAL_MEMORY) {
				return ma_per_level_data(&marcel_topo_vp_level[0],
							 allocator->container.offset);
			} else {
				return ma_per_level_data(marcel_machine_level,
							 allocator->container.offset);
			}
		}

		while (niveau_courant) {
			ma_container_t *container_courant;
			container_courant = ma_per_level_data(niveau_courant,
							      allocator->container.offset);
			if (!ma_container_is_full(container_courant))
				return container_courant;
#ifdef MA__NUMA
			if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
			    (niveau_courant->merged_type & (1 << MARCEL_LEVEL_NODE)))
				/* Last memory level, stop here */
				break;
#endif
			niveau_courant = niveau_courant->father;
		}
		if (mode == FREE_METHOD) {
			/* trop-plein, lib�rer */
			return NULL;
		} else {
			if (niveau_courant) {
				return ma_per_level_data(niveau_courant,
							 allocator->container.offset);
			} else if (allocator->policy == POLICY_HIERARCHICAL_MEMORY) {
				return ma_per_level_data(&marcel_topo_vp_level[0],
							 allocator->container.offset);
			} else {
				return ma_per_level_data(marcel_machine_level,
							 allocator->container.offset);
			}
		}
	}
#endif
	MA_BUG();
	return NULL;
}

void ma_obj_allocator_print(ma_allocator_t * allocator)
{
	if (allocator->policy == POLICY_GLOBAL) {
		fprintf(stderr, "global: %d elements\n",
			allocator->container.obj->nb_element);
		return;
	}
	if (allocator->policy == POLICY_LOCAL) {
		ma_lwp_t lwp;
		fprintf(stderr, "local\n");
		ma_lwp_list_lock_read();
		ma_for_all_lwp(lwp)
			fprintf(stderr, "%4d", ma_vpnum(lwp));
		fprintf(stderr, "\n");
		ma_for_all_lwp(lwp)
			fprintf(stderr, "%4d", 
				((ma_container_t *)ma_per_lwp_self_data(allocator->container.offset))->nb_element);
		ma_lwp_list_unlock_read();
		fprintf(stderr, "\n");
		return;
	}
	if (allocator->policy == POLICY_HIERARCHICAL
	    || allocator->policy == POLICY_HIERARCHICAL_MEMORY) {
		int i, j;
		fprintf(stderr, "hierarchical\n");
		for (j = marcel_topo_nblevels - 1; j >= 0; --j) {
			for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[j][i].vpset); ++i)
				fprintf(stderr, "%4u", marcel_topo_levels[j][i].number);
			fprintf(stderr, "\n");
			for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[j][i].vpset); ++i)
				fprintf(stderr, "%4d", 
					((ma_container_t *)ma_per_level_data(&marcel_topo_levels[j][i],
									     (allocator->container.offset)))->nb_element);
			fprintf(stderr, "\n");
#ifdef MA__NUMA
			if (allocator->policy == POLICY_HIERARCHICAL_MEMORY &&
			    (marcel_topo_levels[j][0].merged_type & (1 << MARCEL_LEVEL_NODE)))
				/* Last memory level, stop here */
				break;
#endif
		}
	}
}

void *ma_obj_allocator_malloc(void *arg)
{
	void *res;
	marcel_extlib_protect();
	res = TBX_MALLOC((size_t) (intptr_t) arg);
	marcel_extlib_unprotect();
	return res;
}

void ma_obj_allocator_free(void *obj, void *foo TBX_UNUSED)
{
	marcel_extlib_protect();
	TBX_FREE(obj);
	marcel_extlib_unprotect();
}

/* safe_malloc bypass version
 */
void *__ma_obj_allocator_malloc(void *arg)
{
	void *res;
	res = marcel_malloc((size_t) (intptr_t) arg);
	return res;
}

void __ma_obj_allocator_free(void *obj, void *foo TBX_UNUSED)
{
	marcel_free(obj);
}

unsigned long ma_per_sth_alloc(ma_per_sth_cur_t * cur, size_t size)
{
	unsigned long ret;
	ma_spin_lock(&cur->lock);
	size = (size + sizeof(void (*)(void)) - 1) & ~(sizeof(void (*)(void)) - 1);
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
