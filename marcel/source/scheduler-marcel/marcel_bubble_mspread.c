
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006, 2008 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for details.
 */

#include "marcel.h"

#ifdef MA__BUBBLES

/* Bouger la memoire avec les entites */

marcel_bubble_t *registered_bubble;

/* Spread entities e on topology levels l */
/* e has ne items, l has nl items */
static void __marcel_bubble_mspread(marcel_entity_t *e[], int ne, struct marcel_topo_level **l, int nl, 
												int recurse) {
	/*** First part : Analyse, break, count, and recurse in levels or entities ***/
	//marcel_fprintf(stderr,"\nmspread : %d entities on %d level\n",ne,nl);

	int i;
	float per_item_load;
	unsigned long totload;

	if (!ne) {
		bubble_sched_debug("no entity, do nothing\n");
		return;
	}

	MA_BUG_ON(!nl);

	bubble_sched_debug("spreading entit%s", ne>1?"ies":"y");
	for (i=0; i<ne; i++)
		bubble_sched_debug(" %p", e[i]);
	
	bubble_sched_debug(" at level%s", nl>1?"s":"");
	for (i=0; i<nl; i++)
		bubble_sched_debug(" %s", l[i]->rq.name);
	bubble_sched_debug("\n");

	/* One level, recurse in it */
	if (nl == 1) {
		/* Only only level */
		bubble_sched_debug("Ok, just leave %s on %s\n", ne>1?"them":"it", l[0]->rq.name);
		if (l[0]->arity) {
			bubble_sched_debug("and recurse in levels\n");
			return __marcel_bubble_mspread(e, ne, l[0]->children, l[0]->arity, recurse+1);
		}

		bubble_sched_debug("No more possible level recursion, we're done\n");
		return;
	}

	/* Leave small entities on level */
	if (checkload)
	{
		/* Compute valid entity number */
		int valid_ne = 0;
		for (i=0; i<ne; i++)
		{
			if (ma_entity_load(e[i]) >= MA_LIFE_LOAD)
				valid_ne ++;
		}
		/* Create a new valid entity vector and replace the others */
		marcel_entity_t *valid_e[valid_ne];
		if (valid_ne < ne)
		{ 
			int k = 0;
			for (i=0; i<ne; i++)
			{ 
				if (ma_entity_load(e[i]) >= MA_LIFE_LOAD) 
				{
					//marcel_fprintf(stderr,"keep entity %p\n",e[i]);
					valid_e[k++] = e[i]; 
				}
				else 
				{
					ma_runqueue_t *rq;
					int state = ma_get_entity(e[i]);
					if (l[0]->father)
						rq = &l[0]->father->rq;
					else
						rq = &marcel_machine_level[0].rq;
					//marcel_fprintf(stderr,"leave entity %p\n",e[i]);
					ma_put_entity(e[i], &rq->as_holder, state);
				}
			}  
			__marcel_bubble_mspread(valid_e,valid_ne,l,nl,recurse+1);
			return;
		}	
	}

	/* Total load and per_item load */
	totload = 0;
	for (i=0; i<ne; i++) {
		{
			if (checkload)
				totload += ma_entity_load(e[i]);
			else
				totload += MA_DEFAULT_LOAD * ma_count_threads_in_entity(e[i]);
		}
	}

	per_item_load = (float)totload / nl;
	bubble_sched_debug("total load %lu = %d*%.2f\n", totload, nl, per_item_load);

	/* Different sorts : by load, by attraction or by both */
	int big_load;
	if (checkload)
	{
		qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);
		big_load = ma_entity_load(e[0]);
	}
	else
	{
		qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_threads_compar);
		big_load = MA_DEFAULT_LOAD * ma_count_threads_in_entity(e[0]);
	}

	/* The least thick entities first */
	if (l[0]->type >= nodelevel)
		qsort(e, ne, sizeof(e[0]), &increasing_order_entity_attraction_compar);
			
	/** Break biggest and lightest entities **/
	if (big_load >= per_item_load || ne < nl) {
	
		unsigned new_ne = 0;
		unsigned recursed = 0;
		int i, j;
		bubble_sched_debug("e[0]=%ld > %lf=per_item_load || ne=%d < %d=nl, recurse into entities\n", ma_entity_load(e[0]), per_item_load, ne, nl);

		int broken, affinity;
		long int load;
		
		/* First count sub-entities */
		for (i=0; i<ne; i++) {
			broken = 0;
			affinity = (e[i]->type == MA_BUBBLE_ENTITY ? ma_bubble_memory_affinity(ma_bubble_entity(e[i])) : 0);
			load = (checkload ? ma_entity_load(e[i]) : MA_DEFAULT_LOAD * ma_count_threads_in_entity(e[i]));
		
			if (e[i]->type == MA_BUBBLE_ENTITY)
				//marcel_fprintf(stderr,"bubble ");
			//marcel_fprintf(stderr,"entity %p :  load %ld, affinity %d, attraction %d, per_item_load %ld -> \n", e[i], load, affinity, ma_entity_memory_volume(e[i],0), (long)per_item_load);
							
			/* Load difference against thickness on or upon node levels */
			if (e[i]->type == MA_BUBBLE_ENTITY && (load > per_item_load || ne < nl))
			{	
				if ((affinity < MA_AFFINITY_BREAK && l[0]->level <= nodelevel) 
					 || (affinity >= MA_AFFINITY_BREAK && l[0]->level > nodelevel)
					 || load > 1.5 * per_item_load)
					broken = 1;
				/* Not enough entities for levels, be large with <= */
				if (ne + new_ne <= nl)
					broken = 1;
			}

			if (broken)
			{
				//marcel_fprintf(stderr,"eclatement de la bulle %p avec %d sub-entities\n",e[i],ma_bubble_entity(e[i])->nbentities);
				unsigned nb = ma_bubble_entity(e[i])->nbentities;
				if (nb) {
					recursed = 1;
					new_ne += nb;
				}
			}
			else
				new_ne += 1;
		}

		if (recursed) {
			/*New list of entities after bubble breaks */
			marcel_entity_t *new_e[new_ne], *ee;
			j = 0;
			/* Recount */
			new_ne = 0;
			
			for (i=0; i<ne; i++) {
				broken = 0;
				affinity = (e[i]->type == MA_BUBBLE_ENTITY ? ma_bubble_memory_affinity(ma_bubble_entity(e[i])) : 0);
				load = (checkload ? ma_entity_load(e[i]) : MA_DEFAULT_LOAD * ma_count_threads_in_entity(e[i]));
				
				/* Load difference against thickness on or upon node levels */
				if (e[i]->type == MA_BUBBLE_ENTITY && (load > per_item_load || ne < nl))
				{
					if ((affinity < MA_AFFINITY_BREAK && l[0]->level <= nodelevel) 
						 || (affinity >= MA_AFFINITY_BREAK && l[0]->level > nodelevel)
						 || load > MA_TO_BIG * per_item_load)
						broken = 1;
					if (ne + new_ne <= nl)
						broken = 1;
				}

				if (broken)
				{
					marcel_bubble_t *bb = ma_bubble_entity(e[i]);
					list_for_each_entry(ee, &bb->heldentities, bubble_entity_list)
						{
							new_e[j++] = ee;
							marcel_fprintf(stderr,"entity %p load %ld\n",ee,ma_entity_load(ee));
						}
					/* Recount */
					new_ne += ma_bubble_entity(e[i])->nbentities;
				}
				else 
				{
					new_e[j++] = e[i];
					/* Recount */
					new_ne += 1;
				}
			}

			bubble_sched_debug("recurse into entities\n");
			return __marcel_bubble_mspread(new_e, new_ne, l, nl, recurse+1);
		}

		if (ne < nl) {
			/* Grmpf, really not enough parallelism, only
			 * use part of the machine */
			//marcel_fprintf(stderr,"Not enough parallelism, using only %d item%s\n", ne, ne>1?"s":"");
			return __marcel_bubble_mspread(&e[0], ne, l, ne, recurse+1);
		}		
	}
 
	/*** Second part : entities placement by memory, my cache, by load ***/

	if (l[0]->type >= nodelevel)
		//qsort(e, ne, sizeof(e[0]), &decreasing_order_entity_both_compar);
		qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);

	struct marcel_topo_level *l_l[nl];
	struct list_head l_dist[nl];
	unsigned long l_load[nl];
	int l_n[nl];
	/* For entities which are not put on their wanted level */
	marcel_entity_t *later[ne];
	int numlater = 0;
	int overnumber = ne-nl;
	int j, k, m, n;

	/* Distribute from heaviest to lightest */
	i = 0;
	n = 0;
	for (i=0; i<nl; i++) {
		l_l[i] = l[i];
		INIT_LIST_HEAD(&l_dist[i]);
		l_load[i] = 0;
		l_n[i] = 0;
	}

	/* Each entity on its level */
	int sumload = 0;
	for (i=0; i<ne; i++) {
		bubble_sched_debug("entity %p(%ld)\n",e[i],ma_entity_load(e[i]));

		/* Compute each entity attraction and preferred node */
		int weight_coef = 1;
		int attraction = 1;
		int preferred_node = -1;

		int total = ma_compute_total_attraction(e[i], weight_coef, MEDIUM_WEIGHT, attraction, &preferred_node);
		
		int begin = 0;

		//marcel_fprintf(stderr,"entity %p : sumload %d, totload %ld, totload*MA_RATIO %f\n", e[i], sumload, totload, MA_RATIO * totload);

		/** First case : node **/
		if (l_l[0]->level <= nodelevel /* On or upon nodes */
			 && total > MA_ATTRACTION_MIN
			 && preferred_node != -1) /* Preferred node */
		{
			/* Is the preferred node under a present level */
			int mnode = -1;
			/* Find the index of our node */
			for (j = 0 ; j < nl ; j++)
			{
				MA_BUG_ON(!marcel_vpset_weight(&l_l[j]->vpset));
				int nodeperlevel = marcel_vpset_weight(&l_l[j]->vpset) * marcel_nbnodes / marcel_vpset_weight(&marcel_topo_level(0,0)->vpset);
				if (l_l[j]->number == preferred_node % nodeperlevel)
				{
					mnode = j;
					break;
				}
			}

			bubble_sched_debug("add to level %s(%ld)",l_l[mnode]->rq.name,l_load[mnode]);
			/* Rebalance when finishing */
			if (mnode != -1)
			{
				/* Not too loaded level */
				if (l_load[mnode] < per_item_load)
				{
					/* More entities than levels */
					if (overnumber > 1)
						begin = mnode;
					/* Not used level */
					else if (l_load[mnode] == 0)
						begin = mnode;
					else
						begin = -1;
				}
				else
					begin = -1;
			} 
			else
				begin = -1;	 
		}
		else 
		{
			/** Second case : cache **/ 
			if (l_l[0]->level > nodelevel /* Inside node */ 
				 && e[0]->last_vp != -1)/* Preferred vp */
			{	
				bubble_sched_debug("\n********* mode VP : last_vp = %d *********\n",e[0]->last_vp);
				int mvp = -1;
				/* Find the index of our last_vp */
				for (j = 0 ; j < nl ; j++)
				{
					/* Is our vp here ? */
					if (l_l[j]->number == e[i]->last_vp % marcel_vpset_weight(&l_l[j]->vpset))
					{
						mvp = j;
						break;
					}
				}
				
				/* Is there a too big difference to choose our vp ? */
				/* Rebalance when finishing */
				if (mvp != -1)
				{
					/* Not too loaded level */
					if (l_load[mvp] < per_item_load)
					{
						/* More entities than levels */
						if (overnumber > 1)
							begin = mvp;
						/* Not used level */
						else if (l_load[mvp] == 0)
							begin = mvp;
						else
							begin = -1;
					}
					else
						begin = -1;
				} 
				else
					begin = -1;	

				bubble_sched_debug("vp :l_load[0] %ld, l_load[mvp] %ld, l_load[begin] %ld, begin %d\n",l_load[0],l_load[mvp],l_load[begin],begin);
			}
			else
			{
				/** Third case : None **/
				begin = -1;
			}
		}

		/* Later pour regulation */
		if (begin == -1)
		{
			later[numlater] = e[i];
			//marcel_fprintf(stderr,"later %p %p\n",	later[numlater],e[i]);
			numlater++;
			continue;
		}

		//marcel_fprintf(stderr,"now\n");		

		/** Put entity on a level and big sort **/

		/* Entity on level */
		list_add_tail(&e[i]->next,&l_dist[begin]);
		/* Load on level */
		l_load[begin] += (checkload ? ma_entity_load(e[i]) : MA_DEFAULT_LOAD * ma_count_threads_in_entity(e[i]));
		/* One entity more on level */
		l_n[begin]++;

		/* Get entity and put it in new holder */
		bubble_sched_debug("add to level %s(%ld)",l_l[begin]->rq.name,l_load[begin]);
		/* Add this entity (heaviest) to least loaded level item from begin */
		PROF_EVENTSTR(sched_status, "spread: add to level");
		int state = ma_get_entity(e[i]);
		ma_put_entity(e[i], &l_l[begin]->rq.as_holder, state);

		bubble_sched_debug(" -> %ld\n",l_load[begin]);
		marcel_fprintf(stderr,"put entity %p on %d\n",e[i],l_l[begin]->number);

		/* Level sort by load */
		if (nl > begin && l_load[begin] > l_load[begin + 1]) {
			/* TODO: optimize this */
			j = begin + 1;
			k = nl - 1;
			if (l_load[begin] < l_load[k])
				while(1) {
					MA_BUG_ON(l_load[j] >= l_load[begin] || l_load[begin] >= l_load[k]);
					if (j == k-1) {
						k = j;
						break;
					}
					m = (j+k)/2;
					
					bubble_sched_debug("trying %d(%ld) between %d(%ld) and %d(%ld)\n", m, l_load[m], j, l_load[j], k, l_load[k]);
					
					if (l_load[begin] == l_load[m])
						break;
					
					if (l_load[begin] < l_load[m])
						k = m;
					else
						j = m;
				}
			
			bubble_sched_debug("inserting level %s(%ld) in place of %s(%ld)\n", l_l[begin]->rq.name, l_load[begin], l_l[k]->rq.name, l_load[k]);
			{
				unsigned long _l_load;
				struct list_head _l_dist;
				struct marcel_topo_level *_l;
				int _l_n;
				
				/* Save level begin */
				INIT_LIST_HEAD(&_l_dist);
				_l_load = l_load[begin];
				list_splice_init(&l_dist[begin],&_l_dist);
				_l_n = l_n[begin];
				_l = l_l[begin];

				/* Shift levels */
				for (m=begin; m<k; m++) {
					l_load[m] = l_load[m+1];
					list_splice_init(&l_dist[m+1],&l_dist[m]);
					l_n[m] = l_n[m+1];
					l_l[m] = l_l[m+1];
				}
					
				/* Restore level begin */
				l_load[k] = _l_load;
				list_splice(&_l_dist,&l_dist[k]);
				l_n[k] = _l_n;
				l_l[k] = _l;
			}
		}

		/* An entity done */
		overnumber --;
		/* Load put on level */
		sumload += (checkload ? ma_entity_load(e[i]) : (ma_entity_load(e[i]) > MA_LIFE_LOAD ?MA_DEFAULT_LOAD * ma_count_threads_in_entity(e[i]) : 0));
	}


	for (i=0;i<nl;i++)
	{
		marcel_fprintf(stderr,"memload %d %lu\n",l_l[i]->number,l_load[i]);
	}


	/*** Fourth part : Once again for the others entities, only for regulation ***/
	for (i=0; i < numlater; i++)
	{
		/** Put entity on a level and big sort **/
		//marcel_fprintf(stderr,"later entity %p\n", later[i]);

		/* Entity on level */
		list_add_tail(&later[i]->next,&l_dist[0]);
		/* Load on level */
		l_load[0] += (checkload ? ma_entity_load(later[i]) : MA_DEFAULT_LOAD * ma_count_threads_in_entity(later[i]));
		/* One entity more on level */
		l_n[0]++;

		/* Get entity and put it in new holder */
		bubble_sched_debug("add to level %s(%ld)",l_l[0]->rq.name,l_load[0]);
		/* Add this entity (heaviest) to least loaded level item from 0 */
		PROF_EVENTSTR(sched_status, "spread: add to level");
		int state = ma_get_entity(later[i]);
		ma_put_entity(later[i], &l_l[0]->rq.as_holder, state);

		bubble_sched_debug(" -> %ld\n",l_load[0]);
		marcel_fprintf(stderr,"put entity %p on %d\n",later[i],l_l[0]->number);

		/* Level sort by load */
		if (nl > 0 && l_load[0] > l_load[0 + 1]) {
			/* TODO: optimize this */
			j = 0 + 1;
			k = nl - 1;
			if (l_load[0] < l_load[k])
				while(1) {
					MA_BUG_ON(l_load[j] >= l_load[0] || l_load[0] >= l_load[k]);
					if (j == k-1) {
						k = j;
						break;
					}
					m = (j+k)/2;
					
					bubble_sched_debug("trying %d(%ld) between %d(%ld) and %d(%ld)\n", m, l_load[m], j, l_load[j], k, l_load[k]);
					
					if (l_load[0] == l_load[m])
						break;
					
					if (l_load[0] < l_load[m])
						k = m;
					else
						j = m;
				}
			
			bubble_sched_debug("inserting level %s(%ld) in place of %s(%ld)\n", l_l[0]->rq.name, l_load[0], l_l[k]->rq.name, l_load[k]);
			{
				unsigned long _l_load;
				struct list_head _l_dist;
				struct marcel_topo_level *_l;
				int _l_n;
				
				/* Save level 0 */
				INIT_LIST_HEAD(&_l_dist);
				_l_load = l_load[0];
				list_splice_init(&l_dist[0],&_l_dist);
				_l_n = l_n[0];
				_l = l_l[0];

				/* Shift levels */
				for (m=0; m<k; m++) {
					l_load[m] = l_load[m+1];
					list_splice_init(&l_dist[m+1],&l_dist[m]);
					l_n[m] = l_n[m+1];
					l_l[m] = l_l[m+1];
				}
					
				/* Restore level 0 */
				l_load[k] = _l_load;
				list_splice(&_l_dist,&l_dist[k]);
				l_n[k] = _l_n;
				l_l[k] = _l;
			}
		}
	}

	for (i=0;i<nl;i++)
	{
		marcel_fprintf(stderr,"load %d %lu\n",l_l[i]->number,l_load[i]);
	}

	/*** Fourth part : Migration if node level and recursion ***/

	for (i=0; i<nl; i++) {
		bubble_sched_debug("recurse in %s\n",l_l[i]->rq.name);
		marcel_entity_t *ne[l_n[i]];
		marcel_entity_t *e;
		j = 0;
		list_for_each_entry(e,&l_dist[i],next)
			{
				/* Memory migration */
				
				/* Map number to migrate */
				//int weight_coef = 0;
				//int attraction = 1;

				//int moving_memory = ma_compute_total_attraction(e, weight_coef, MEDIUM_WEIGHT, attraction, NULL);

				/* Migration on nodes */
				if (l_l[0]->merged_type & (1<<MARCEL_LEVEL_NODE))
				{
					/* Migration time */
				        //TBX_GET_TICK(start);
					int node = l_l[i]->number;
					ma_move_entity_alldata(e,node);
					//TBX_GET_TICK(finished);
					//bubble_sched_debug("NODE ! mspread migration time : entity %p, moving %d to node %d, time %ld\n", e, moving_memory, node, finished-start);
				}

				/* To respread */
				ne[j++] = e;
			}
		MA_BUG_ON(j != l_n[i]);
		if (l_l[i]->arity)
			__marcel_bubble_mspread(ne, l_n[i], l_l[i]->children, l_l[i]->arity, recurse+1);
		else
			__marcel_bubble_mspread(ne, l_n[i], &l_l[i], 1, recurse+1);
	}
}

/**********************************************************/

ma_spinlock_t spread_lock = MA_SPIN_LOCK_UNLOCKED;
/* Spread a bubble on a level with memory information */
void marcel_bubble_mspread(marcel_bubble_t *b, struct marcel_topo_level *l) {
	ma_spin_lock_softirq(&spread_lock);
	unsigned vp;
	marcel_entity_t *e = &b->as_entity;
	ma_bubble_synthesize_stats(b);
/* XXX: suppose that the bubble is not held out of topo hierarchy under
 * level l */
	ma_bubble_lock_all(b, l);
	__ma_bubble_gather(b, b);
	__marcel_bubble_mspread(&e, 1, &l, 1, 0);
	PROF_EVENTSTR(sched_status, "spread: done");

/* resched existing threads */
	marcel_vpset_foreach_begin(vp,&l->vpset)
		ma_lwp_t lwp = ma_vp_lwp[vp];
	ma_resched_task(ma_per_lwp(current_thread,lwp),vp,lwp);
	marcel_vpset_foreach_end()

		ma_bubble_unlock_all(b, l);

	ma_spin_unlock_softirq(&spread_lock);
}

/* Spread entities on levels with memory information */
void marcel_bubble_spread_entities(marcel_entity_t *e[], int ne, struct marcel_topo_level **l, int nl)
{
	ma_spin_lock_softirq(&spread_lock);
	__marcel_bubble_mspread(e, ne, l, nl, 0); 

	int i,vp;
	for (i=0;i<nl;i++)
	{
		marcel_vpset_foreach_begin(vp,&l[i]->vpset)
			ma_lwp_t lwp = ma_vp_lwp[vp];
		ma_resched_task(ma_per_lwp(current_thread,lwp),vp,lwp);
		marcel_vpset_foreach_end()
	}

	ma_spin_unlock_softirq(&spread_lock);
}

#endif /* BUBBLES */
