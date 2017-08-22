/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001-2016 "the PM2 team" (see AUTHORS file)
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

#include "piom_private.h"

static struct
{
#if defined(PIOMAN_TOPOLOGY_NONE)
    /** global queue in case topology is unknown */
    struct piom_ltask_queue global_queue;
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
    /** node topology */
    hwloc_topology_t topology;
    /** thread-local location information  */
    pthread_key_t threadinfo_key;
#endif /* PIOMAN_TOPOLOGY_* */
} __piom_topology;

#if defined(PIOMAN_TOPOLOGY_HWLOC)
/** thread-specific information */
struct piom_ltask_threadinfo_s
{
    pthread_t tid;              /**< thread this info is about */
    hwloc_cpuset_t cpuset;      /**< last cpuset location for thread  */
    hwloc_obj_t obj;            /**< last obj encompassing cpuset */
    tbx_tick_t timestamp;
    int count;
};
#endif /* PIOMAN_TOPOLOGY_HWLOC */

/** Get the queue that matches a topology object
 */
piom_ltask_queue_t*piom_topo_get_queue(piom_topo_obj_t obj)
{
    if(obj == NULL)
	{
	    obj = piom_topo_current_obj();
	}
#if defined(PIOMAN_TOPOLOGY_HWLOC)
    while(obj != NULL && (obj->userdata == NULL || (((struct piom_ltask_locality_s*)obj->userdata)->queue == NULL)))
	{
	    obj = obj->parent;
	}
    if(obj == NULL)
	{
	    obj = piom_topo_current_obj();
	    while(obj != NULL && (obj->userdata == NULL || (((struct piom_ltask_locality_s*)obj->userdata)->queue == NULL)))
		{
		    obj = obj->parent;
		}
	}
    assert(obj != NULL);
    const struct piom_ltask_locality_s*local = obj->userdata;
    return local->queue;
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
    const struct piom_ltask_locality_s*local = obj->ltask_data;
    while((obj != NULL) && (local->queue == NULL))
	{
	    obj = obj->father;
	    local = obj->ltask_data;
	}
    assert(local->queue != NULL);
    return local->queue;
#elif defined(PIOMAN_TOPOLOGY_NONE)
    return &__piom_topology.global_queue;
#else /* PIOMAN_TOPOLOGY_* */
#error
    return NULL;
#endif	/* PIOMAN_TOPOLOGY_* */
}

/** create queues attached to topology objects */
void piom_topo_init_ltasks(void)
{
  #if defined(PIOMAN_TOPOLOGY_NONE)
    piom_ltask_queue_init(&__piom_topology.global_queue, piom_topo_full);
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
    int i, j;
    for(i = 0; i < marcel_topo_nblevels; i++)
	{
	    for(j = 0; j < marcel_topo_level_nbitems[i]; j++)
		{
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    piom_ltask_queue_t*queue = TBX_MALLOC(sizeof (piom_ltask_queue_t));
		    piom_ltask_queue_init(queue, l);
		    /*		    queue->parent = (l->father != NULL) ? l->father->ltask_data : NULL; */
		    l->ltask_data = queue;
		}
	}
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
    hwloc_topology_init(&__piom_topology.topology);
    hwloc_topology_load(__piom_topology.topology);
    const int depth = hwloc_topology_get_depth(__piom_topology.topology);
    const hwloc_obj_type_t binding_level = piom_parameters.binding_level;
    int d;
    for(d = 0; d < depth; d++)
	{
	    hwloc_obj_t o = hwloc_get_obj_by_depth(__piom_topology.topology, d, 0);
	    if(o->type == HWLOC_OBJ_MACHINE || o->type == HWLOC_OBJ_NODE || o->type == HWLOC_OBJ_SOCKET || o->type == HWLOC_OBJ_CORE)
		{
		    const int nb = hwloc_get_nbobjs_by_depth(__piom_topology.topology, d);
		    int i;
		    for (i = 0; i < nb; i++)
			{
			    o = hwloc_get_obj_by_depth(__piom_topology.topology, d, i);
			    struct piom_ltask_locality_s*local = TBX_MALLOC(sizeof(struct piom_ltask_locality_s));
			    local->queue = NULL;
			    hwloc_obj_t parent = o->parent;
			    while(parent && parent->userdata == NULL)
				{
				    parent = parent->parent;
				}
			    local->parent = (parent == NULL) ? NULL : parent->userdata;
			    o->userdata = local;
			    if(o->type == binding_level)
				{
				    /* TODO- allocate memory on given obj */
				    piom_ltask_queue_t*queue = TBX_MALLOC(sizeof(piom_ltask_queue_t));
				    piom_ltask_queue_init(queue, o);
				    local->queue = queue;
				}
#ifdef PIOMAN_TRACE
			    char cont_name[32];
			    char cont_type[64];
			    const char*level_label = NULL;
			    switch(o->type)
				{
				case HWLOC_OBJ_MACHINE:
				    level_label = "Machine";
				    break;
				case HWLOC_OBJ_NODE:
				    level_label = "Node";
				    break;
				case HWLOC_OBJ_SOCKET:
				    level_label = "Socket";
				    break;
				case HWLOC_OBJ_CORE:
				    level_label = "Core";
				    break;
				default:
				    break;
				}
			    sprintf(cont_name, "%s_%d", level_label, o->logical_index);
			    sprintf(cont_type, "Container_%s", level_label);
			    local->trace_info.cont_name = strdup(cont_name);
			    local->trace_info.cont_type = strdup(cont_type);
			    local->trace_info.level = o->type;
			    local->trace_info.rank = o->logical_index;
			    local->trace_info.parent = local->parent ? &local->parent->trace_info : NULL;
			    piom_trace_local_new(&local->trace_info);
			    piom_trace_remote_state(o, PIOM_TRACE_STATE_NONE);
#endif /* PIOMAN_TRACE */
			}
		}
	}
    pthread_key_create(&__piom_topology.threadinfo_key, NULL); /* TODO- detructor */
#endif	/* PIOMAN_TOPOLOGY_* */

}


void piom_topo_exit_ltasks(void)
{
#if defined(PIOMAN_TOPOLOGY_NONE)
  piom_ltask_queue_exit((piom_ltask_queue_t*)&__piom_topology.global_queue);
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
  int i, j;
  for(i = marcel_topo_nblevels -1 ; i >= 0; i--)
    {
      for(j = 0; j < marcel_topo_level_nbitems[i]; j++)
	{
	  struct marcel_topo_level *l = &marcel_topo_levels[i][j];
	  piom_ltask_queue_exit((piom_ltask_queue_t*)l->ltask_data);
	  TBX_FREE(l->ltask_data);
	}
    }
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
  const int depth = hwloc_topology_get_depth(__piom_topology.topology);
  int d, i;
  for(d = 0; d < depth; d++)
    {
      for (i = 0; i < hwloc_get_nbobjs_by_depth(__piom_topology.topology, d); i++)
	{
	  hwloc_obj_t o = hwloc_get_obj_by_depth(__piom_topology.topology, d, i);
	  struct piom_ltask_locality_s*local = o->userdata;
	  if(local)
	      {
		  piom_ltask_queue_t*queue = local->queue;
		  if(queue)
		      {
			  piom_trace_remote_state(queue->binding, PIOM_TRACE_STATE_NONE);
			  o->userdata = NULL;
			  free(queue);
		      }
		  free(local);
	      }
	}
    }
#endif /* PIOMAN_TOPOLOGY_* */
  
}


#if defined(PIOMAN_TOPOLOGY_MARCEL)
piom_topo_obj_t piom_get_parent_obj(piom_topo_obj_t obj, enum piom_topo_level_e _level)
{
    const enum marcel_topo_level_e level = _level;
    marcel_topo_level_t *l = obj; /* &marcel_topo_levels[marcel_topo_nblevels - 1][marcel_current_vp()]; */
    while(l != NULL && l->type > level)
	{
	    l = l->father;
	}
    assert(l != NULL);
    return l;
}
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
hwloc_topology_t piom_topo_get(void)
{
    assert(__piom_topology.topology != NULL);
    return __piom_topology.topology;
}
piom_topo_obj_t piom_topo_current_obj(void)
{
    int update = 0;
    struct piom_ltask_threadinfo_s*threadinfo = pthread_getspecific(__piom_topology.threadinfo_key);
    if(threadinfo == NULL)
	{
	    threadinfo = TBX_MALLOC(sizeof(struct piom_ltask_threadinfo_s));
	    pthread_setspecific(__piom_topology.threadinfo_key, threadinfo);
	    threadinfo->tid = pthread_self();
	    threadinfo->cpuset = hwloc_bitmap_alloc();
	    threadinfo->count = 0;
	    update = 1;
	}
    else
	{
	    threadinfo->count++;
	    if(threadinfo->count % 100 == 0)
		{
		    tbx_tick_t now;
		    TBX_GET_TICK(now);
		    double delay = TBX_TIMING_DELAY(threadinfo->timestamp, now);
		    if(delay > 10000)
			update = 1;
		}
	}
    if(update)
	{
	    int rc = hwloc_get_last_cpu_location(__piom_topology.topology, threadinfo->cpuset, HWLOC_CPUBIND_THREAD);
	    if(rc != 0)
		abort();
	    threadinfo->obj = hwloc_get_obj_covering_cpuset(__piom_topology.topology, threadinfo->cpuset);
	    while(threadinfo->obj != NULL && (threadinfo->obj->userdata == NULL))
		{
		    threadinfo->obj = threadinfo->obj->parent;
		}	    
	    if(threadinfo->obj == NULL)
		abort();
	    TBX_GET_TICK(threadinfo->timestamp);
	}
    return threadinfo->obj;
}
#endif

