
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

#define FREE_METHOD 0
#define ALLOC_METHOD 1
#define COMMON_FREE_METHOD 2

ma_allocator_t * ma_node_allocator = NULL;
static ma_allocator_t * lwp_container_allocator = NULL;
static ma_allocator_t * level_container_allocator = NULL;
ma_per_sth_cur_t ma_per_lwp_cur = MA_PER_STH_CUR_INITIALIZER(MA_PER_LWP_ROOM);
ma_per_sth_cur_t ma_per_level_cur = MA_PER_STH_CUR_INITIALIZER(MA_PER_LEVEL_ROOM);

void ma_allocator_init(void)
{

  lwp_container_allocator = ma_new_obj_allocator(1,
                                         (void*(*)(void*)) ma_per_lwp_alloc,
					 (void *)sizeof(ma_container_t),
					 NULL,
                                         NULL,
                                         POLICY_GLOBAL,
					 0
					 );
  level_container_allocator = ma_new_obj_allocator(1,
                                         (void*(*)(void*))ma_per_level_alloc,
                                         (void *)sizeof(ma_container_t),
                                         NULL,
                                         NULL,
                                         POLICY_GLOBAL,
					 0
					 );
  ma_node_allocator = ma_new_obj_allocator(0,
					ma_obj_allocator_malloc,
					(void *)sizeof(ma_node_t),
					ma_obj_allocator_free,
					NULL,
					POLICY_HIERARCHICAL,
					0
					);
}

void ma_allocator_exit(void) 
{
  ma_obj_allocator_fini(level_container_allocator);
  ma_obj_allocator_fini(lwp_container_allocator);
  ma_obj_allocator_fini(ma_node_allocator);
  ma_node_allocator = NULL;
  lwp_container_allocator = NULL;
  level_container_allocator = NULL;
}

ma_allocator_t * ma_new_obj_allocator(int conservative,
				       void * (*create)(void *),
				       void * create_arg,
				       void (*destroy)(void *, void *),
				       void * destroy_arg,
				       enum policy_t policy,
				       int max_size				       
				       )
{
  /* Initialisation de la structure de l'allocateur selon les parametres*/
  ma_allocator_t * allocator = malloc(sizeof(ma_allocator_t));
  allocator->create = create;
  allocator->create_arg = create_arg;
  allocator->policy = policy;
  allocator->destroy = destroy;
  allocator->destroy_arg = destroy_arg;
  allocator->conservative = conservative;
  allocator->max_size = max_size;
  allocator->init = 0;
  allocator->container = NULL;
  ma_obj_allocator_init(allocator);
  return allocator;
}

void * ma_obj_alloc(ma_allocator_t * allocator) 
{
  void * obj;
  ma_container_t * container;
  container = ma_get_container(allocator, ALLOC_METHOD);
  
  /* Essaye d'obtenir un objet du container */
  obj = ma_container_get(container);

  if (obj)
    /* Si on en trouve un c'est bon */
    return obj;
  else
    /* Sinon on doit en allouer un nouveau */
    return allocator->create(allocator->create_arg);

  return NULL;
}

void ma_obj_free(ma_allocator_t * allocator, void * obj) 
{
  ma_container_t * container;
  container = ma_get_container(allocator, FREE_METHOD);
  if (container)
    ma_container_add(container, obj);
  else
    allocator->destroy(obj, allocator->destroy_arg);
}

void ma_obj_allocator_fini(ma_allocator_t * allocator)
{  
   int i,j;  

  switch(allocator->policy) {
  case POLICY_GLOBAL:
    {
      ma_container_fini(ma_get_container(allocator, ALLOC_METHOD), allocator->destroy, allocator->destroy_arg);
      free(allocator->container);
    }
    break;
 
 case POLICY_LOCAL:
    for(i=0; i < marcel_nbvps(); ++i)
      {
	ma_container_fini(ma_per_lwp_self_data((long)allocator->container),allocator->destroy, allocator->destroy_arg);
      }
    ma_obj_free(lwp_container_allocator, allocator->container);
    break;

  case POLICY_HIERARCHICAL:
    for(j=0; j < marcel_topo_nblevels; ++j)
      {
        for(i=0; marcel_topo_levels[marcel_topo_nblevels-j-1][i].vpset; ++i)
	  {
	    ma_container_fini(ma_per_level_data(&marcel_topo_levels[marcel_topo_nblevels-j-1][i], (long)(allocator->container)),allocator->destroy, allocator->destroy_arg);
	  }
      }
    ma_obj_free(level_container_allocator, allocator->container);
    break;
  default:
    printf("la politique est mal choisie\n");
    break;    
  } 
}

void ma_obj_allocator_init(ma_allocator_t * allocator)
{
  int i, j;

  if(!(allocator->init))
    {

      switch(allocator->policy) {
      case POLICY_GLOBAL: 
	allocator->container = malloc(sizeof(ma_container_t));
	ma_container_init(allocator->container, allocator->conservative, allocator->max_size);
	break;

      case POLICY_LOCAL: {
	ma_lwp_t lwp;
	allocator->container = ma_obj_alloc(lwp_container_allocator);

	for_all_lwp(lwp) {
	     ma_container_init(ma_per_lwp_data(lwp,(long)(allocator->container)), allocator->conservative, allocator->max_size);
	}
	break;
      }

      case POLICY_HIERARCHICAL:

	allocator->container = ma_obj_alloc(level_container_allocator);
       	//allocator->container = (void*)ma_per_level_alloc(sizeof(ma_container_t));	
	for(j=0; j < marcel_topo_nblevels; ++j)
	  {
            for(i=0; marcel_topo_levels[marcel_topo_nblevels-j-1][i].vpset; ++i)
	      {
		ma_container_init(ma_per_level_data(&marcel_topo_levels[marcel_topo_nblevels-j-1][i], (long)(allocator->container)), allocator->conservative, allocator->max_size);
	      }
	  }

	break;

      default:
	printf("La politique n'est pas bien choisie\n");
	}
      
      allocator->init = 1;

      ma_obj_allocator_init(ma_node_allocator);
		       
    }  
}

ma_container_t * ma_get_container(ma_allocator_t * allocator, int mode)
{
  if (mode != 0 && mode != 1  && mode != 2)
    printf("Erreur : le mode n'est pas bien defini -- ma_get_container\n");

  //mode 0 pour FREE_METHOD
  //mode 1 pour ALLOC_METHOD

  //On peut transformer tout ca avec un switch !

  if (allocator->policy == POLICY_GLOBAL) 
    {
      return allocator->container;
    }
  
  if (allocator->policy == POLICY_LOCAL)
    {
      return ma_per_lwp_self_data((long)allocator->container);
    }


  // On fait une boucle while qui remonte le niveau hierarchique 
  // jusqu'a ce que l'on trouve un container qui a un nombre d'element > 0.
  // Si on ne trouve pas de container plein sur toute la topologie, on 
  // retourne le container de niveau le plus bas (= le plus proche du processeur). 

  if (allocator->policy == POLICY_HIERARCHICAL && mode == ALLOC_METHOD)
    {
     struct marcel_topo_level* niveau_courant = ma_per_lwp(lwp_level, LWP_SELF);
      do
	{
	  ma_container_t* container_courant;	
	  container_courant = ma_per_level_data(niveau_courant, (long)allocator->container);
	  if (ma_container_nb_element(container_courant) > 0)
	    return container_courant;
	  else
	    niveau_courant = niveau_courant->father;
	} while(niveau_courant);
      return ma_per_level_data(ma_per_lwp(lwp_level, LWP_SELF),(long)allocator->container);
    }
  

  // Dans ce cas la, le container a retourner en premier est celui de niveau le plus 
  // bas si il est vide, sinon si il est rempli au-dela d'un certain niveau, il faut retourner 
  // le container au-dessus (dans la hierarchie) et ainsi de suite, 
  // 
  // Nota: désactivé pour les mesures:
  // si les containers sont tous 
  // pleins on retoune quand meme le container de plus proche du processeur : donc il va deborder...

  if (allocator->policy == POLICY_HIERARCHICAL && mode == FREE_METHOD)
    {
      struct marcel_topo_level* niveau_courant = ma_per_lwp(lwp_level, LWP_SELF);
      do 
	{
	  ma_container_t* container_courant;	
	  container_courant = ma_per_level_data(niveau_courant, (long)allocator->container);
	  if(!ma_container_plein(container_courant))
	    {
	    return container_courant;
	    }
	  else
	    niveau_courant = niveau_courant->father;
	} while(niveau_courant);
      /* trop-plein, libérer */
      return NULL;
	//return ma_per_level_data(ma_per_lwp(lwp_level, LWP_SELF), (long)allocator->container);
    }
  return NULL;
}

void * ma_obj_allocator_malloc(void * arg) 
{
  void * res;
  res = malloc((size_t)(intptr_t)arg);
  return res;
}

void ma_obj_allocator_free(void * obj, void * foo) 
{
  free(obj);
}

unsigned long ma_per_sth_alloc(ma_per_sth_cur_t *cur, size_t size)
{
  unsigned long ret;
  ma_spin_lock(&cur->lock);
  ret = cur->cur;
  cur->cur += size;
  MA_BUG_ON(cur->cur > cur->max);
  ma_spin_unlock(&cur->lock);
  return ret;
}

#undef ma_per_lwp_alloc
unsigned long ma_per_lwp_alloc(size_t size) {
  return ma_per_sth_alloc(&ma_per_lwp_cur, size);
}

#undef ma_per_level_alloc
unsigned long ma_per_level_alloc(size_t size) {
  return ma_per_sth_alloc(&ma_per_level_cur, size);
}
