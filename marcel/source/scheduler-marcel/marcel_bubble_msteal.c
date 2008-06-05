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

/******************************************************************
 *
 * Vol de bulle local.
 *
 ******************************************************************/

#ifdef MA__BUBBLES

#include <math.h>

/* TODO: gérer le cas où un thread qu'on est en train de voler meurt (pas de chance...) */

/* Calcul du score d'une entite */
static int ma_stolen_score(marcel_entity_t *entity, int penalty)
{
	int score = 0;

	/* Load of the entity */
	int load = MA_LOAD_SCORE * log10((double)ma_entity_load(entity));

	if (entity->type == MA_BUBBLE_ENTITY)
		ma_bubble_lock(ma_bubble_entity(entity));
	if (!checkload)
		load = MA_LOAD_SCORE * log10((double) MA_DEFAULT_LOAD * ma_count_threads_in_entity(entity));
	if (entity->type == MA_BUBBLE_ENTITY)
		ma_bubble_unlock(ma_bubble_entity(entity));

	/* Mother bubble shared memory */
	int affinity;
	if (entity->init_holder)
		affinity = MA_FATHER_AFFINITY_SCORE * log10((double)ma_bubble_memory_affinity(ma_bubble_holder(entity->init_holder)));
	else
		affinity = 0;

	/* TODO : optimiser update */
	//TODO : lock entity
	//int volume = MA_VOLUME_SCORE * ma_compute_total_attraction(entity,weight_coef,access_min);
	/* TODO : Do not take care of quantity but pinfo number */
	int volume = MA_VOLUME_SCORE * log10((double)ma_entity_memory_volume(entity,0));

	score += load;
	/* Cache Penalty */
	if (penalty == MA_VP_LEVEL_PENALTY)
		score -= MA_CACHE_SCORE;
	/* Node */
	if (penalty == MA_NODE_LEVEL_PENALTY)
		score -= affinity + volume;

	return (score >= 0 ? score:0);
}

/* We want an active entity */
static int ma_entity_is_active(marcel_entity_t *e)
{
	if (e->type == MA_THREAD_ENTITY)
		return (MA_TASK_IS_RUNNING(ma_task_entity(e)));
	else if (e->type == MA_THREAD_SEED_ENTITY)
		return 0;
	else
	{
		MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
		marcel_entity_t *downentity;
		ma_holder_rawlock(&ma_bubble_entity(e)->as_holder);
		list_for_each_entry(downentity, &ma_bubble_entity(e)->heldentities, bubble_entity_list)
			{
				if (ma_entity_is_active(downentity) == 1) {
					ma_holder_rawunlock(&ma_bubble_entity(e)->as_holder);
					return 1;
				}
			}
		ma_holder_rawunlock(&ma_bubble_entity(e)->as_holder);
		return 0;
	}
	return -1;
}

/* Bulle considere comme bien vivante */
#define MA_LIFE_LOAD 50

/* Find an entity in a bubble */
void ma_find_interesting_entity_in_bubble(ma_holder_t *h, int *totalload, int penalty, int *score, 
														marcel_entity_t **stolen, int recurse) {
	int current_score;
	marcel_entity_t *e;

	MA_BUG_ON(h->type != MA_BUBBLE_HOLDER);

	int number = 0;
	for_each_entity_scheduled_in_bubble_begin(e,ma_bubble_holder(h))
		
		/* Almost empty bubble */

		if (e->type == MA_BUBBLE_ENTITY) 
		{
			if (ma_bubble_entity(e)->old == 0)
			{
				if (ma_entity_load(e) < MA_LIFE_LOAD)
					ma_bubble_entity(e)->old = 1;
				else 
					number ++;
			}
		}
		else
			if (e->init_holder)
				number ++;
	for_each_entity_scheduled_in_bubble_end();

	if (!number)
		return;

	if (number == 1)
	{
		/* Only one entity in bubble. */
		for_each_entity_scheduled_in_bubble_begin(e,ma_bubble_holder(h))
		
			if (e->sched_holder != h)
				continue;
		if (e->type == MA_BUBBLE_ENTITY) 
			if (ma_bubble_entity(e)->old)
				continue;
		
		if (!e->init_holder)
			return;
			
		if (ma_entity_is_active(e) == 0)
		{
			current_score = ma_stolen_score(e, penalty);
			if (checkload)
				*totalload += ma_entity_load(e);
			else
				*totalload += MA_DEFAULT_LOAD;
			
			if (current_score >= *score)
			{
				*score = current_score;
				*stolen = e;
			}
			return;
		}
		else
		{
			/* Active entity, look into */

			if (e->type != MA_BUBBLE_ENTITY)
				return;
			else
			{
				if (!ma_bubble_entity(e)->old)
				{
					MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
					ma_holder_rawlock(&ma_bubble_entity(e)->as_holder);
					ma_find_interesting_entity_in_bubble(&ma_bubble_entity(e)->as_holder, totalload, penalty, score, stolen, recurse + 1);
					ma_holder_rawunlock(&ma_bubble_entity(e)->as_holder);
				}
			}
			for_each_entity_scheduled_in_bubble_end();
		}
	}
	else
	{
		for_each_entity_scheduled_in_bubble_begin(e,ma_bubble_holder(h))
			
			if (e->type == MA_BUBBLE_ENTITY) 
				if (ma_bubble_entity(e)->old)
					continue;
		
		if (ma_entity_is_active(e))
			continue;
		else
		{
			if (!e->init_holder)
				continue;

			/* Best score entity */			
			current_score = ma_stolen_score(e, penalty);
			if (checkload)
				*totalload += ma_entity_load(e);
			else
				*totalload += MA_DEFAULT_LOAD;
	
			if (current_score >= *score)
			{
				*score = current_score;
				*stolen = e;
			}
		}
		for_each_entity_scheduled_in_bubble_end();
	}
}

/* Find entity on runqueue */
void ma_find_interesting_entity_on_runqueue(ma_holder_t *h, int *totalload, int penalty, int *score, 
														  marcel_entity_t **stolen, struct marcel_topo_level **fromlevel, int recurse) {
	int current_score;
	marcel_entity_t *e;

	MA_BUG_ON(h->type != MA_RUNQUEUE_HOLDER);

	*fromlevel = NULL;
		
	int number = 0;
	list_for_each_entry(e, &h->sched_list, sched_list)
		{
			
			/* Almost empty bubble */
			if (e->type == MA_BUBBLE_ENTITY) 
			{
				if (ma_bubble_entity(e)->old == 0)
				{
					ma_bubble_synthesize_stats(ma_bubble_entity(e));
					if (ma_entity_load(e) < MA_LIFE_LOAD)
						ma_bubble_entity(e)->old = 1;
					else 
						number ++;
				}
			}
			else
				if (e->init_holder)
					number ++;
		}
	
	if (!number)
		return;

	if (number == 1)
	{
		list_for_each_entry(e, &h->sched_list, sched_list)
			{
				if (e->type == MA_BUBBLE_ENTITY) 
					if (ma_bubble_entity(e)->old)
						continue;

				if (!e->init_holder)
					return;
				if (ma_entity_is_active(e) == 0)
				{
					/* One non active entity on runqueue */
					current_score = ma_stolen_score(e, penalty);
				
					if (checkload)
						*totalload += ma_entity_load(e);
					else
						*totalload += MA_DEFAULT_LOAD;
			
					if (current_score >= *score)
					{
						*score = current_score;
						*stolen = e;
						*fromlevel = NULL;
					}
					return;
				}
				else
				{
					/* One active entity on runqueue */
					if (e->type != MA_BUBBLE_ENTITY)
						return;
					else
					{
						MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
						if (!ma_bubble_entity(e)->old)
						{
							ma_holder_rawlock(&ma_bubble_entity(e)->as_holder);
							ma_find_interesting_entity_in_bubble(&ma_bubble_entity(e)->as_holder, totalload, penalty, score, stolen, recurse + 1);
							ma_holder_rawunlock(&ma_bubble_entity(e)->as_holder);
							*fromlevel = tbx_container_of(ma_rq_holder(h), struct marcel_topo_level, rq);
						}
					}
				}
			}
	}
	else
	{
		list_for_each_entry(e, &h->sched_list, sched_list)
			{
				if (e->type == MA_BUBBLE_ENTITY) 
					if (ma_bubble_entity(e)->old)
						continue;

				if (!e->init_holder)
					continue;
				if (ma_entity_is_active(e))
					continue;
				else
				{
					current_score = ma_stolen_score(e, penalty);
				
					if (checkload)
						*totalload += ma_entity_load(e);
					else
						*totalload += MA_DEFAULT_LOAD;
								
					if (current_score >= *score)
					{
						*score = current_score;
						*stolen = e;
					}
				}
			}
	}
}


/* Find an entity on level */
void ma_see(struct marcel_topo_level *level, int *totalload, int penalty, int *score, marcel_entity_t **stolen, struct marcel_topo_level **fromlevel) 
{
	ma_runqueue_t *rq = &level->rq;
	
	if (!rq)
		return;

	ma_holder_rawlock(&rq->as_holder);
	ma_find_interesting_entity_on_runqueue(&rq->as_holder, totalload, penalty, score, stolen, fromlevel, 0);
	ma_holder_rawunlock(&rq->as_holder);
}


/* Find an entity under level */
void ma_see_down(struct marcel_topo_level *level, struct marcel_topo_level *me, 
					  int *totalload, int penalty, int *score, marcel_entity_t **stolen, 
					  struct marcel_topo_level **otherlevel, struct marcel_topo_level **fromlevel)
{
	int i = 0, n = level->arity;

	if (me) {
		/* Don't search in myself */
		n--;
		i = (me->index + 1) % level->arity;
	}

	if (!n)
	{
		*score = 0;
		*stolen = NULL;
		*fromlevel = NULL;
		return;
	}

	*score = 0;
	int see_totalload = 0;
	int see_score = 0;
	int down_totalload = 0;
	int down_score = 0;
	marcel_entity_t *see_stolen, *down_stolen;
	struct marcel_topo_level *see_fromlevel, *down_fromlevel;

	while (n--) {
		/* In brothers */
		ma_see(level->children[i], &see_totalload, penalty, &see_score, &see_stolen, &see_fromlevel);
		if (see_totalload > *totalload)
		{
			*totalload = see_totalload;
			*stolen = see_stolen;
			*fromlevel = see_fromlevel;
			*score = see_score;
			
			*otherlevel = level->children[i];
		}
		i = (i+1) % level->arity;
	}

	/* Again in children */
	i = 0;
	n = level->arity;
	if (me) {
		n--;
		i = (me->index + 1) % level->arity;
	}

	while (n--) {
		ma_see_down(level->children[i], NULL, &down_totalload, penalty, &down_score, &down_stolen, otherlevel, &down_fromlevel);
		if (down_totalload > *totalload)
		{
			*totalload = down_totalload;
			*stolen = down_stolen;
			*fromlevel = down_fromlevel;
			*score = down_score;

			*otherlevel = level->children[i];
		}
		i = (i+1) % level->arity;
	}
}


/* Put bubble holder stolen entity on common level (start and finish) */
 void ma_work_on_entity(marcel_entity_t *entity, int is_stolen, struct marcel_topo_level* father, struct marcel_topo_level* otherlevel, struct marcel_topo_level *fromlevel)
{
	marcel_entity_t *upentity, *downentity;
	
	/* Counts ancestries */
	int number = 1;
	upentity = entity;
	for (;;)
	{
		if (upentity->init_holder == 0)
		{
			break;
		}
		
		if (upentity->sched_holder->type == MA_RUNQUEUE_HOLDER 
			 && ma_rq_holder(upentity->sched_holder) == &father->rq)
			break;
			 
		upentity = &ma_bubble_holder(upentity->init_holder)->as_entity;
		number ++;
	}

	marcel_entity_t *family[number];
	number = 0;
	family[number] = entity;
	for (;;)
	{
		if (family[number]->init_holder == 0)
		{
			break;
		}
		
		if (family[number]->sched_holder->type == MA_RUNQUEUE_HOLDER 
			 && ma_rq_holder(family[number]->sched_holder) == &father->rq)
			break;
			 
		family[number+1] = &ma_bubble_holder(family[number]->init_holder)->as_entity;
		number ++;
	}

	number --;

	while (number)
	{
		int state = -1;
		ma_holder_t *holder = family[number]->sched_holder;
		if (family[number]->type == MA_BUBBLE_ENTITY) {

			/* Put family[number] in its bubble */
			ma_holder_t *rqholder = &ma_to_rq_holder(holder)->as_holder;
			if (rqholder)
				ma_holder_rawlock(rqholder);
			if (holder != rqholder)
				ma_holder_rawlock(holder);
			ma_bubble_lock(ma_bubble_entity(family[number]));
			if (family[number]->init_holder)
				state = ma_get_entity(family[number]);
			if (holder != rqholder)
				ma_holder_rawunlock(holder);

			/* We leave here scheduled entities we found on the bubble  we want to lift */
			for_each_entity_scheduled_in_bubble_begin(downentity,ma_bubble_entity(family[number]))
				/* To avoid bubble on mother bubble runqueue */
				if (rqholder != &marcel_topo_level(0,0)->rq.as_holder)
				{
					state = ma_get_entity(downentity);
					if (&ma_to_rq_holder(downentity->init_holder)->as_holder == rqholder)
						ma_put_entity(downentity, downentity->init_holder, state);
					else
						ma_put_entity(downentity, rqholder, state);
				}
			if (downentity->type == MA_BUBBLE_ENTITY)
				__ma_bubble_unlock(ma_bubble_entity(downentity));
			for_each_entity_scheduled_in_bubble_end();
			
			if (rqholder)
				ma_holder_rawunlock(rqholder);

			ma_bubble_unlock(ma_bubble_entity(family[number]));
		} else {

			if (family[number]->init_holder != holder) {
				ma_holder_rawlock(holder);
				state = ma_get_entity(family[number]);
				ma_holder_rawunlock(holder);
			}
		}

		if (family[number]->init_holder)
		{
			ma_holder_rawlock(family[number]->init_holder);
			ma_put_entity(family[number],family[number]->init_holder,state);
			ma_holder_rawunlock(family[number]->init_holder);
		}
		number --;
	}
}

#define MA_MOVE_MEMORY_MIN 50000
/* Find an entity to stole up */
int ma_see_up(struct marcel_topo_level *level) 
{
	/* NUMA penalties */
	int penalty = MA_DEFAULT_PENALTY;
	if (level->level >= nodelevel)
	{
		/* Out the node */
		penalty = MA_NODE_LEVEL_PENALTY;
	}
	else
	{
		/* In the node, out the cache */
		penalty = MA_VP_LEVEL_PENALTY;
	}

	struct marcel_topo_level *father = level->father;
	
	if (!father)
		return 0;
	
	marcel_entity_t *stolen = NULL;
	int score = 0;
	int totalload = 0;
	struct marcel_topo_level *otherlevel = NULL;
	struct marcel_topo_level *fromlevel = NULL;

	ma_see_down(father, level, &totalload, penalty, &score, &stolen, &otherlevel, &fromlevel);

	if (score)
	{	
		MA_BUG_ON(otherlevel == NULL);

		/* Lift the bubble holder to the common level */
		ma_work_on_entity(stolen, 1, father, otherlevel, fromlevel);
		
		int vp;
		int allthreads, nvp;
		nvp = marcel_vpset_weight(&marcel_topo_level(0,0)->vpset);
		marcel_threadslist(0, NULL, &allthreads, NOT_BLOCKED_ONLY);
		
		/* Work on our vp */
		vp = marcel_current_vp();

		ma_holder_t *holder = stolen->sched_holder;
	
		if (holder)
			ma_holder_rawlock(holder);
		int state = ma_get_entity(stolen);
		if (holder)
			ma_holder_rawunlock(holder);
		holder = &marcel_topo_vp_level[vp].rq.as_holder;
	
		ma_holder_rawlock(holder);
		marcel_fprintf(stderr,"MOVE : !!put stolen entity %p onto vp %d\n", stolen, vp);
		ma_put_entity(stolen, holder, state);
		ma_holder_rawunlock(holder);

		/* Memory migration time */
		float ratio = (float)((float)nvp/(float)marcel_nbnodes); 
		int localnode = (int)(((float)vp)/ratio);
			
		/* Calcul du nombre de pages a bouger */
		//int weight_coef = 0;
		//int attraction = 1;
		//lock bubble entity
		//int moving_memory = ma_compute_total_attraction(e[i], weight_coef, MEDIUM_WEIGHT, attraction, NULL);
		
		/* Move time */
		//unsigned long start, finished;
		//TBX_GET_TICK(start);
		
		/* We move the memory */
		ma_move_entity_alldata(stolen, localnode);
		
		/* Fin du calcul du temps du move */
		//TBX_GET_TICK(finished);
		
		return 1;
	}
	else
	{
		int ret = ma_see_up(father);
		return ret; 
	}
}

#endif /* MA_BUBBLES */
