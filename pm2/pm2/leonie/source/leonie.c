/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*
 * leonie.c
 * ========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

/*
 * Leonie general modes
 * ====================
 */
// #define LEO_NO_SPAWN
#define LEO_VCHANNELS

/*
 * Leonie general modes
 * ====================
 */
#ifdef LEO_NO_SPAWN
#warning [1;33m<<< [1;37mLeonie spawn:     [1;31mnot activated [1;33m>>>[0m
#else
#warning [1;33m<<< [1;37mLeonie spawn:     [1;32mactivated [1;33m    >>>[0m
#endif // LEO_NO_SPAWN

#ifdef LEO_VCHANNELS
#warning [1;33m<<< [1;37mLeonie vchannels: [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie vchannels: [1;31mnot activated [1;33m>>>[0m
#endif // LEO_VCHANNELS


/*
 * Static variables
 * ================
 */

/*
 * Topology
 * --------
 */
static p_leo_networks_t     networks     = NULL;
static p_leo_directory_t    directory    = NULL;
static p_leo_spawn_groups_t spawn_groups = NULL;

/*
 * Net server
 * ----------
 */
static p_ntbx_server_t net_server = NULL;

/*
 * Loaders
 * ----------
 */
static p_tbx_htable_t loaders = NULL;

/*
 * Application information
 * -----------------------
 */
static char           *application_name     = NULL;
static char           *application_flavor   = NULL;
static p_tbx_htable_t  application_networks = NULL;


/*
 * Functions
 * =========
 */

static
p_tbx_htable_t
build_network_host_htable(p_tbx_slist_t network_host_slist)
{
  p_tbx_htable_t host_htable = NULL;

  LOG_IN();
  host_htable = leo_htable_init();

  tbx_slist_ref_to_head(network_host_slist);
  do
    {
      p_leoparse_object_t  object    = NULL;
      char                *host_name = NULL;
      
      object = tbx_slist_ref_get(network_host_slist);

      {
	char *temp_name = NULL;
	
	temp_name = leoparse_get_id(object);
	TRACE_STR("expected name", temp_name);
	
	host_name = ntbx_true_name(temp_name);
	TRACE_STR("true name", host_name);
      }

      tbx_htable_add(host_htable, host_name, host_name);
    }
  while (tbx_slist_ref_forward(network_host_slist));
  
  LOG_OUT();

  return host_htable;
}

static
p_leo_dir_fchannel_t
make_fchannel(p_leo_dir_channel_t  dir_channel)
{
  p_leo_dir_fchannel_t       dir_fchannel    = NULL;

  LOG_IN();
  dir_fchannel = leo_dir_fchannel_init();
  
  {
    size_t len = 0;
    
    len = strlen(dir_channel->name);
    dir_fchannel->name = TBX_MALLOC(len + 2);
    CTRL_ALLOC(dir_fchannel->name);
    
    sprintf(dir_fchannel->name, "f%s", dir_channel->name);
  }

  dir_fchannel->channel_name = strdup(dir_channel->name);

  tbx_htable_add(directory->fchannel_htable,
		 dir_fchannel->name, dir_fchannel);
  tbx_slist_append(directory->fchannel_slist, dir_fchannel);
  LOG_OUT();
  
  return dir_fchannel;
}


static
p_tbx_slist_t
expand_sbracket_modifier(p_leoparse_modifier_t modifier,
			 int                   default_value)
{
  p_tbx_slist_t slist  = NULL;
  p_tbx_slist_t ranges = NULL;

  LOG_IN();
  slist = tbx_slist_nil();
  
  if (modifier && modifier->type == leoparse_m_sbracket)
    {      
      ranges = modifier->sbracket;

      if (ranges && !tbx_slist_is_nil(ranges))
	{
	  tbx_slist_ref_to_head(ranges);

	  do
	    {
	      p_leoparse_object_t o = NULL;

	      o = tbx_slist_ref_get(ranges);

	      if (o->type == leoparse_o_integer)
		{
		  int *p_val   = NULL;

		  p_val = TBX_MALLOC(sizeof(int));
		  CTRL_ALLOC(p_val);
	      
		  *p_val = leoparse_get_val(o);
		  tbx_slist_append(slist, p_val);
		}
	      else if (o->type == leoparse_o_range)
		{
		  p_leoparse_range_t range = NULL;
		  int                val   =    0;

		  range = leoparse_get_range(o);

		  for (val = range->begin; val < range->end; val++)
		    {
		      int *p_val   = NULL;
	      
		      p_val = TBX_MALLOC(sizeof(int));
		      CTRL_ALLOC(p_val);
	      
		      *p_val = val;
		      tbx_slist_append(slist, p_val);
		    }
		}
	      else
		FAILURE("parse error : invalid object type");
	    }
	  while (tbx_slist_ref_forward(ranges));
	}
      else
	FAILURE("parse error : no range list");
    }
  else
    {
      int *p_val   = NULL;

      p_val  = TBX_MALLOC(sizeof(int));
      *p_val = default_value;
      tbx_slist_append(slist, p_val);
    }  
  LOG_OUT();
  
  return slist;
}

static
void
process_channel(p_tbx_htable_t channel)
{
  char                  *channel_name            = NULL;
  char                  *channel_network         = NULL;
  p_tbx_slist_t          channel_host_slist      = NULL;
  p_tbx_htable_t         network_htable          = NULL;
  char                  *network_name            = NULL;
  p_tbx_htable_t         network_host_htable     = NULL;
  char                  *network_device_name     = NULL;
  char                  *network_loader          = NULL;
  leo_loader_priority_t  network_loader_priority =
    leo_loader_priority_undefined;
  p_leo_dir_channel_t    dir_channel             = NULL;
  ntbx_process_lrank_t   channel_local_rank      =    0;
  char                  *channel_reference_name  = NULL;
  char                  *driver_reference_name   = NULL;
  p_leo_dir_driver_t     dir_driver              = NULL;
  p_leo_spawn_group_t    spawn_group             = NULL;

  LOG_IN();
  channel_name = leoparse_read_id(channel, "name");
  TRACE_STR("====== Processing channel", channel_name);

  channel_network  = leoparse_read_id(channel, "net");
  TRACE_STR("====== Channel network is", channel_network);

  channel_host_slist = leoparse_read_slist(channel, "hosts");
  
  network_htable = tbx_htable_get(networks->htable, channel_network);
  if (!network_htable)
    FAILURE("unknown network");

  network_name        = leoparse_read_id(network_htable, "name");
  network_device_name = leoparse_read_id(network_htable, "dev");

  if ((network_loader = leoparse_read_id(network_htable, "mandatory_loader")))
    {
      network_loader_priority = leo_loader_priority_high;
    }
  else if ((network_loader =
	    leoparse_read_id(network_htable, "optional_loader")))
    {
      network_loader_priority = leo_loader_priority_medium;
    }
  else
    {
      network_loader_priority = leo_loader_priority_low;
      network_loader = strdup("default");
    }
  
  network_host_htable = tbx_htable_get(network_htable, "host_htable");

  if (!network_host_htable)
    {
      p_tbx_slist_t network_host_slist = NULL;
      
      network_host_slist  = leoparse_read_slist(network_htable, "hosts");
      network_host_htable = build_network_host_htable(network_host_slist);
      tbx_htable_add(network_htable, "host_htable", network_host_htable);
    }

  TRACE_STR("====== effective network name", network_name);
  TRACE_STR("====== network device", network_device_name);
  TRACE_STR("====== network loader", network_loader);
  TRACE_VAL("====== network loader priority", (int)network_loader_priority);

  dir_channel = leo_dir_channel_init();
  dir_channel->name = strdup(channel_name);
  channel_reference_name =
    TBX_MALLOC(strlen("channel") + strlen(channel_name) + 2);
  sprintf(channel_reference_name, "%s:%s", "channel", channel_name);

  dir_driver = tbx_htable_get(directory->driver_htable, network_device_name);

  if (!dir_driver)
    {
      dir_driver = leo_dir_driver_init();
      dir_driver->name = strdup(network_device_name);
      tbx_htable_add(directory->driver_htable, dir_driver->name, dir_driver);
      tbx_slist_append(directory->driver_slist, dir_driver);
    }
  
  dir_channel->driver = dir_driver;

  driver_reference_name =
    TBX_MALLOC(strlen("driver") + strlen(dir_driver->name) + 2);
  sprintf(driver_reference_name, "%s:%s", "driver", dir_driver->name);

  spawn_group = tbx_htable_get(spawn_groups->htable, network_name);
  
  if (!spawn_group)
    {
      spawn_group = leo_spawn_group_init();
      spawn_group->loader_name = strdup(network_loader);
      tbx_htable_add(spawn_groups->htable, network_name, spawn_group);
      tbx_slist_append(spawn_groups->slist, spawn_group);
    }

  tbx_slist_ref_to_head(channel_host_slist);
  do
    {
      p_leoparse_object_t  object            = NULL;
      char                *host_name         = NULL;
      p_leo_dir_node_t     dir_node          = NULL;
      p_tbx_slist_t        process_slist     = NULL;
      const char          *adapter_name      = "default";
      const char          *adapter_selector  = "-";

      object = tbx_slist_ref_get(channel_host_slist);

      {
	char *temp_name = NULL;
	
	temp_name = leoparse_get_id(object);
	TRACE_STR("expected name", temp_name);
	
	host_name = ntbx_true_name(temp_name);
	TRACE_STR("true name", host_name);
      }
      
      if (!tbx_htable_get(network_host_htable, host_name))
	FAILURE("unknown hostname");

      TRACE_STR("====== node hostname", host_name);

      dir_node = tbx_htable_get(directory->node_htable, host_name);
      if (!dir_node)
	{
	  dir_node = leo_dir_node_init();
	  dir_node->name = strdup(host_name);
	  tbx_htable_add(directory->node_htable, host_name, dir_node);
	  tbx_slist_append(directory->node_slist, dir_node);
	}

      process_slist = expand_sbracket_modifier(object->modifier, 0);

      tbx_slist_ref_to_head(process_slist);
      do
	{
	  p_ntbx_process_t     process         = NULL;
	  ntbx_process_lrank_t node_local_rank =   -1;
	  
	  {
	    int *p_val = NULL;

	    p_val = tbx_slist_ref_get(process_slist);
	    node_local_rank = *p_val;

	    TRACE_VAL("======== Process number", node_local_rank);
	  }
	  
	  {
	    p_ntbx_process_info_t node_process_info = NULL;

	    node_process_info = ntbx_pc_get_local(dir_node->pc,
						  node_local_rank);
	  
	    if (!node_process_info)
	      {
		p_leo_process_specific_t process_specific = NULL;
	      
		process_specific = leo_process_specific_init();
		ntbx_tcp_client_init(process_specific->client);

		process_specific->current_loader_name      = network_loader;
		process_specific->current_spawn_group_name = network_name;
		process_specific->current_loader_priority  =
		  network_loader_priority;

		process              = ntbx_process_cons();
		process->ref         = leo_htable_init();
		process->specific    = process_specific;
		process->global_rank =
		  tbx_slist_get_length(directory->process_slist);
	      
		tbx_slist_append(directory->process_slist, process);

		ntbx_pc_add(dir_node->pc, process, node_local_rank,
			    dir_node, "node", NULL);

		tbx_slist_append(spawn_group->process_slist, process);
	      }
	    else
	      {
		p_leo_process_specific_t process_specific = NULL;
	      
		process          = node_process_info->process;
		process_specific = process->specific;
		
		if (process_specific->
		    current_loader_priority < network_loader_priority)
		  {
		    p_leo_spawn_group_t previous_spawn_group = NULL;
	    
		    previous_spawn_group = 
		      tbx_htable_get(spawn_groups->htable,
				     process_specific->
				     current_spawn_group_name);
		    
		    tbx_slist_search_and_extract(previous_spawn_group
						 ->process_slist, NULL,
						 process);

		    

		    process_specific->current_loader_name      =
		      network_loader;
		    process_specific->current_spawn_group_name = network_name;
		    process_specific->current_loader_priority  =
		      network_loader_priority;

		    tbx_slist_append(spawn_group->process_slist, process);

		  }
		else if (network_loader_priority == leo_loader_priority_high
			 && strcmp(process_specific->current_loader_name,
				   network_loader))
		  FAILURE("conflicting loaders");
	      }
	  }

	  {
	    p_ntbx_process_info_t channel_process_info = NULL;

	    channel_process_info = ntbx_pc_get_global(dir_channel->pc,
						      process->global_rank);
	  
	    if (!channel_process_info)
	      {
		p_leo_dir_channel_process_specific_t cp_specific = NULL;

		cp_specific = leo_dir_channel_process_specific_init();
		cp_specific->adapter_name = strdup(adapter_name);
		
		ntbx_pc_add(dir_channel->pc, process,
			    channel_local_rank, dir_channel,
			    channel_reference_name,
			    cp_specific);
	      }
	    else
	      {
		leo_terminate("duplicate process in channel process list");
	      }
	  }

	  {
	    p_ntbx_process_info_t driver_process_info = NULL;

	    driver_process_info = ntbx_pc_get_global(dir_driver->pc,
						     process->global_rank);

	    if (!driver_process_info)
	      {
		p_leo_dir_driver_process_specific_t dp_specific = NULL;
		p_leo_dir_adapter_t                 dir_adapter = NULL;

		dp_specific = leo_dir_driver_process_specific_init();
		dir_adapter = leo_dir_adapter_init();

		dir_adapter->name      = strdup(adapter_name);
		dir_adapter->selector  = strdup(adapter_selector);
		dir_adapter->parameter = NULL;

		tbx_htable_add(dp_specific->adapter_htable,
			       dir_adapter->name, dir_adapter);
		tbx_slist_append(dp_specific->adapter_slist, dir_adapter);
		
		ntbx_pc_add(dir_driver->pc, process, -1,
			    dir_driver, driver_reference_name,
			    dp_specific);
	      }
	    else
	      {
		p_leo_dir_driver_process_specific_t dp_specific = NULL;
		p_leo_dir_adapter_t                 dir_adapter = NULL;

		dp_specific = driver_process_info->specific;
		dir_adapter = tbx_htable_get(dp_specific->adapter_htable,
					 adapter_name);
		
		if (!dir_adapter)
		  {
		    dir_adapter = leo_dir_adapter_init();

		    dir_adapter->name      = strdup(adapter_name);
		    dir_adapter->selector  = strdup(adapter_selector);
		    dir_adapter->parameter = NULL;

		    tbx_htable_add(dp_specific->adapter_htable,
				   dir_adapter->name, dir_adapter);
		    tbx_slist_append(dp_specific->adapter_slist, dir_adapter);
		  }
	      }
	  }

	  channel_local_rank++;
	}
      while (tbx_slist_ref_forward(process_slist));

      TBX_FREE(host_name);
    }
  while (tbx_slist_ref_forward(channel_host_slist));
  
  dir_channel->ttable =
    ntbx_topology_table_init(dir_channel->pc,
			     ntbx_topology_regular,
			     NULL);

  tbx_htable_add(directory->channel_htable, dir_channel->name, dir_channel);
  tbx_slist_append(directory->channel_slist, dir_channel);
  LOG_OUT();
}

#ifdef LEO_VCHANNELS
static
void
process_vchannel(p_tbx_htable_t vchannel)
{
  char                       *vchannel_name           = NULL;
  p_tbx_slist_t               vchannel_channel_slist  = NULL;
  p_leo_dir_vchannel_t        dir_vchannel            = NULL;
  p_ntbx_process_container_t  dir_vchannel_pc         = NULL;
  p_tbx_slist_t               dir_channel_slist       = NULL;
  p_tbx_slist_t               dir_fchannel_slist      = NULL;
  char                       *vchannel_reference_name = NULL;

  LOG_IN();
  vchannel_name = leoparse_read_id(vchannel, "name");
  TRACE_STR("====== Processing vchannel", vchannel_name);

  vchannel_channel_slist = leoparse_read_slist(vchannel, "channels");
  dir_vchannel           = leo_dir_vchannel_init();
  dir_vchannel->name     = tbx_strdup(vchannel_name);
  dir_vchannel_pc        = dir_vchannel->pc;
  dir_channel_slist      = dir_vchannel->dir_channel_slist;
  dir_fchannel_slist     = dir_vchannel->dir_fchannel_slist;

  vchannel_reference_name =
    TBX_MALLOC(strlen("vchannel") + strlen(vchannel_name) + 2);
  sprintf(vchannel_reference_name, "%s:%s", "vchannel", vchannel_name);
  
  if (tbx_slist_is_nil(vchannel_channel_slist))
    {
      leo_terminate("vchannel has empty real channel list");
    }
  
  TRACE("====== Process list construction");
  tbx_slist_ref_to_head(vchannel_channel_slist);
  do
    {
      p_leoparse_object_t        object         = NULL;      
      char                      *channel_name   = NULL;
      p_leo_dir_channel_t        dir_channel    = NULL;
      p_leo_dir_fchannel_t       dir_fchannel   = NULL;
      p_ntbx_process_container_t dir_channel_pc = NULL;
      ntbx_process_lrank_t       rank           =    0;

      object = tbx_slist_ref_get(vchannel_channel_slist);
      channel_name = leoparse_get_id(object);
      TRACE_STR("======== processing real channel", channel_name);

      dir_channel = tbx_htable_get(directory->channel_htable, channel_name);
      if (!dir_channel)
	FAILURE("real channel not found");
      
      TRACE("real channel found");

      if (!dir_channel->public)
	FAILURE("real channel already in use");

      dir_channel->public = tbx_false;
      TRACE("real channel locked");

      dir_channel_pc = dir_channel->pc;
      if (!ntbx_pc_first_local_rank(dir_channel_pc, &rank))
	continue;

      tbx_slist_append(dir_channel_slist, dir_channel);
      dir_fchannel = make_fchannel(dir_channel);
      tbx_slist_append(dir_fchannel_slist, dir_fchannel);
      
      do
	{
	  p_ntbx_process_info_t process_info = NULL;
	  p_ntbx_process_t      process      = NULL;
	  ntbx_process_grank_t  global_rank  =    0;
	  p_ntbx_process_info_t tmp_pi       = NULL;
		
	  process_info = ntbx_pc_get_local(dir_channel_pc, rank);
		
	  process     = process_info->process;
	  global_rank = process->global_rank;
	  TRACE_VAL("======== adding process", global_rank);
		
	  tmp_pi = ntbx_pc_get_global(dir_vchannel_pc, global_rank);
		  
	  if (tmp_pi)
	    {
	      TRACE("process already added");
	    }
	  else
	    {
	      p_leo_dir_vchannel_process_specific_t dvps = NULL;
		      
	      TRACE("process is new for this vchannel");
		      
	      dvps = leo_dir_vchannel_process_specific_init();
		  
	      ntbx_pc_add(dir_vchannel_pc, process, -1,
			  dir_vchannel, vchannel_reference_name, dvps);
	    } 
	}
      while (ntbx_pc_next_local_rank(dir_channel_pc, &rank));
    }
  while (tbx_slist_ref_forward(vchannel_channel_slist));

  if (tbx_slist_is_nil(dir_channel_slist))
    {
      TRACE("vchannel is empty");      
      goto end;
    }

  TRACE("====== Routing table generation");  
  {
    typedef struct s_leo_path
    {
      ntbx_process_grank_t dest_rank;
      p_leo_dir_channel_t  dir_channel;
      p_leo_dir_fchannel_t dir_fchannel;
      tbx_bool_t           last;
    } leo_path_t, *p_leo_path_t;

    p_leo_path_t         *routing_table_current =     NULL;
    p_leo_path_t         *routing_table_next    =     NULL;
    ntbx_process_lrank_t  table_size            =        0;
    ntbx_process_grank_t  g_rank_src            =        0;
    int                   pass                  =        0;
    int                   pass_number           =        0;

    TRACE("====== Table initialization");  
    table_size = dir_vchannel_pc->count;

    routing_table_current =
      TBX_CALLOC(table_size * table_size, sizeof(p_leo_path_t));

    routing_table_next =
      TBX_CALLOC(table_size * table_size, sizeof(p_leo_path_t));

    tbx_slist_ref_to_head(dir_channel_slist);
    tbx_slist_ref_to_head(dir_fchannel_slist);
    do
      {
	p_leo_dir_channel_t        dir_channel         = NULL;
	p_ntbx_process_container_t dir_channel_pc      = NULL;
	p_ntbx_topology_table_t    dir_channel_ttable  = NULL;
	p_leo_dir_fchannel_t       dir_fchannel        = NULL;

	dir_channel  = tbx_slist_ref_get(dir_channel_slist);
	TRACE_STR("======== channel", dir_channel->name);  
	dir_channel_pc     = dir_channel->pc;
	dir_channel_ttable = dir_channel->ttable;

	dir_fchannel = tbx_slist_ref_get(dir_fchannel_slist);

	ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_src);
	do
	  {
	    ntbx_process_grank_t g_rank_dst =  0;
	    ntbx_process_lrank_t c_rank_src = -1;
	    ntbx_process_lrank_t l_rank_src = -1;

	    c_rank_src = ntbx_pc_global_to_local(dir_channel_pc, g_rank_src);
	    if (c_rank_src < 0)
	      continue;
	    
	    l_rank_src = ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_src);

	    TRACE_VAL("======== source", l_rank_src);  

	    ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_dst);
	    do
	      {
		ntbx_process_lrank_t c_rank_dst =   -1;
		ntbx_process_lrank_t l_rank_dst =   -1;
		p_leo_path_t         path       = NULL;

		c_rank_dst = ntbx_pc_global_to_local(dir_channel_pc,
						     g_rank_dst);
		if (c_rank_dst < 0)
		  continue;

		if (!ntbx_topology_table_get(dir_channel_ttable,
					     c_rank_src, c_rank_dst))
		  continue;
		
		l_rank_dst = ntbx_pc_global_to_local(dir_vchannel_pc,
						     g_rank_dst);
		TRACE_VAL("======== destination", l_rank_dst);
		
		{ 
		  const ntbx_process_lrank_t location = 
		    l_rank_src * table_size + l_rank_dst;

		  path = routing_table_current[location];

		  if (path)
		    continue;
		
		  path = TBX_MALLOC(sizeof(leo_path_t));
		
		  path->dest_rank    = g_rank_dst;
		  path->dir_channel  = dir_channel;
		  path->dir_fchannel = dir_fchannel;
		  path->last         = tbx_true;
		
		  routing_table_current[location] = path;
		}
	      }
	    while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_dst));
	  }
	while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_src));
      }
    while (   tbx_slist_ref_forward(dir_channel_slist)
	   && tbx_slist_ref_forward(dir_fchannel_slist));

    pass        = 0;
    pass_number = tbx_slist_get_length(dir_channel_slist);

    do
      {
	tbx_bool_t need_additional_pass  = tbx_false;
	tbx_bool_t process_is_converging = tbx_false;

	TRACE_VAL("====== Table filling, pass", pass);

	ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_src);
	do
	  {
	    ntbx_process_grank_t g_rank_dst =  0;
	    ntbx_process_lrank_t l_rank_src = -1;

	    l_rank_src =
	      ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_src);

	    TRACE_VAL("======== source", l_rank_src);  

	    ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_dst);

	    do
	      {
		ntbx_process_lrank_t l_rank_dst =   -1;
		ntbx_process_grank_t g_rank_med =   -1;
	    
		l_rank_dst = ntbx_pc_global_to_local(dir_vchannel_pc,
						     g_rank_dst);
		TRACE_VAL("======== destination", l_rank_dst);
		
		{ 
		  p_leo_path_t               path     = NULL;
		  const ntbx_process_lrank_t location = 
		    l_rank_src * table_size + l_rank_dst;

		  path = routing_table_current[location];

		  if (path)
		    {
		      routing_table_next[location] = path;
		      continue;
		    }
		  
		  ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_med);
		  do
		    {
		      ntbx_process_lrank_t l_rank_med = -1;
		      p_leo_path_t         path_src   = NULL;
		      p_leo_path_t         path_dst   = NULL;
	      
		      l_rank_med = ntbx_pc_global_to_local(dir_vchannel_pc,
							   g_rank_med);

		      path_src   =
			routing_table_current[l_rank_src * table_size +
					     l_rank_med];

		      if (!path_src)
			continue;
		      
		      path_dst =
			routing_table_current[l_rank_med * table_size +
					     l_rank_dst];

		      if (!path_dst)
			continue;
		      
		      TRACE_VAL("found path through", l_rank_med);

		      path = TBX_MALLOC(sizeof(leo_path_t));

		      *path = *path_src;
		      path->last = tbx_false;
		      
		      break;
		    }
		  while
		    (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_med));

		  if (path)
		    {
		      routing_table_next[location] = path;
		      process_is_converging        = tbx_true;
		    }
		  else
		    {
		      routing_table_next[location] = NULL;
		      need_additional_pass         = tbx_true;
		    }
		}
	      }
	    while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_dst));
	  }
	while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_src));
	
	if (!process_is_converging)
	  {
	    leo_terminate("routing table generation process "
			  "is not converging");
	  }

	{
	  p_leo_path_t *temp = NULL;
	  
	  temp                  = routing_table_current;
	  routing_table_current = routing_table_next;
	  routing_table_next    = temp;
	}

	if (!need_additional_pass)
	  break;
      }
    while (++pass < pass_number);
    TRACE_STR("====== Routing table ready for virtual channel", vchannel_name);

    TRACE_STR("====== Transferring routing table to the internal directory",
	     vchannel_name);
    ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_src);
    do
      {
	ntbx_process_grank_t                  g_rank_dst  =    0;
	ntbx_process_lrank_t                  l_rank_src  =   -1;
	p_ntbx_process_info_t                 pi          = NULL;
	p_leo_dir_vchannel_process_specific_t pi_specific = NULL;
	p_ntbx_process_container_t            pc          = NULL;
	p_ntbx_process_t                      process_src = NULL;
	
	l_rank_src  = ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_src);
	TRACE_VAL("======== source", l_rank_src);  
	pi          = ntbx_pc_get_global(dir_vchannel_pc, g_rank_src);
	process_src = pi->process;
	pi_specific = pi->specific;
	pc          = pi_specific->pc;
	
	ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_dst);
	do
	  {
	    p_leo_dir_vchannel_process_routing_table_t  rtable      = NULL;
	    ntbx_process_lrank_t                        l_rank_dst  =   -1;
	    p_ntbx_process_t                            process_dst = NULL;
	    char                                       *ref_name    = NULL;

	    l_rank_dst = ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_dst);
	    TRACE_VAL("======== destination", l_rank_dst);
		
	    { 
	      p_leo_path_t               path     = NULL;
	      const ntbx_process_lrank_t location = 
		l_rank_src * table_size + l_rank_dst;
	      
	      path   = routing_table_current[location];
	      rtable = leo_dir_vchannel_process_routing_table_init();

	      if (path->last)
		{
		  rtable->channel_name = strdup(path->dir_channel->name);
		}
	      else
		{
		  rtable->channel_name = strdup(path->dir_fchannel->name);
		}
	      rtable->destination_rank = path->dest_rank;
	    }

	    process_dst = ntbx_pc_get_local_process(dir_vchannel_pc,
						    l_rank_dst);
	    {
	      size_t len = 0;
	      
	      len = strlen(dir_vchannel->name) + 8;
	      ref_name = TBX_MALLOC(len);
	      
	      sprintf(ref_name, "%s:%d", dir_vchannel->name, l_rank_src);
	    }
	    
	    ntbx_pc_add(pc, process_dst, l_rank_dst,
			process_src, ref_name, rtable);
	    free(ref_name);
	    ref_name = NULL;
	  }
	while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_dst));
      }
    while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_src));

    TRACE("====== Transfer complete");
  }
  
  tbx_htable_add(directory->vchannel_htable, dir_vchannel->name, dir_vchannel);
  tbx_slist_append(directory->vchannel_slist, dir_vchannel);
  TRACE("====== Processing done");

 end:
  LOG_OUT();
}
#endif // LEO_VCHANNELS

static
void
process_application(p_tbx_htable_t application)
{
  p_tbx_slist_t include_slist  = NULL;
  p_tbx_slist_t channel_slist  = NULL;
  p_tbx_slist_t vchannel_slist = NULL;
  
  LOG_IN();
  application_name     = leoparse_read_id(application, "name");  
  TRACE_STR("==== Application name", application_name);
  application_flavor   = leoparse_read_id(application, "flavor");
  TRACE_STR("==== Application flavor", application_flavor);
  application_networks = leoparse_read_htable(application, "networks");

  include_slist = leoparse_read_slist(application_networks, "include"); 
  if (!include_slist || tbx_slist_is_nil(include_slist))
    {
      leo_terminate("parse error : no network include list");      
    }

  TRACE("==== Including Network configuration files");
  include_network_files(networks, include_slist);
  
  channel_slist = leoparse_read_slist(application_networks, "channels");
  if (!channel_slist || tbx_slist_is_nil(channel_slist))
    {
      leo_terminate("parse error : no channel list");
    }

  tbx_slist_ref_to_head(channel_slist);
      
  do
    {
      p_leoparse_object_t object  = NULL;
      p_tbx_htable_t      channel = NULL;
      
      object  = tbx_slist_ref_get(channel_slist);
      channel = leoparse_get_htable(object);
      
      process_channel(channel);
    }
  while (tbx_slist_ref_forward(channel_slist));

  vchannel_slist = leoparse_try_read_slist(application_networks, "vchannels");
  if (vchannel_slist)
    {
      if (tbx_slist_is_nil(vchannel_slist))
	{
	  leo_terminate("parse error : empty virtual channel list");
	}

      tbx_slist_ref_to_head(vchannel_slist);
      
      do
	{
	  p_leoparse_object_t object   = NULL;
	  p_tbx_htable_t      vchannel = NULL;
	  
	  object   = tbx_slist_ref_get(vchannel_slist);
	  vchannel = leoparse_get_htable(object);
	  
	  process_vchannel(vchannel);
	}
      while (tbx_slist_ref_forward(vchannel_slist));      
    }
  else
    {
      TRACE("no vchannel_slist");
    }
  TRACE("==== Internal directory generation completed");
  LOG_OUT();
}

#ifndef LEO_NO_SPAWN
static
void
connect_processes(p_tbx_slist_t process_slist)
{
  p_tbx_htable_t node_htable = NULL;
	
  if (tbx_slist_is_nil(process_slist))
    return;
  
  node_htable = leo_htable_init();
  
  tbx_slist_ref_to_head(process_slist);
  do
    {
      p_ntbx_process_t  process   = NULL;
      p_leo_dir_node_t  dir_node  = NULL;
      char             *host_name = NULL;
      p_tbx_slist_t     slist     = NULL;

      process   = tbx_slist_ref_get(process_slist);
      dir_node  = tbx_htable_get(process->ref, "node");
      host_name = dir_node->name;

      slist = tbx_htable_get(node_htable, host_name);
      
      if (!slist)
	{
	  slist = tbx_slist_nil();
	  tbx_htable_add(node_htable, host_name, slist);
	}

      tbx_slist_append(slist, process);
    }
  while (tbx_slist_ref_forward(process_slist));
  
  do
    {
      p_ntbx_client_t           client           = NULL;
      p_tbx_slist_t             slist            = NULL;
      char                     *host_name        = NULL;
      char                     *true_name        = NULL;
      p_ntbx_process_t          process          = NULL;
      p_leo_process_specific_t  process_specific = NULL;
      int                       status           = ntbx_failure;

      client = ntbx_client_cons();
      ntbx_tcp_client_init(client);
      status = ntbx_tcp_server_accept(net_server, client);
      if (status == ntbx_failure)
	FAILURE("client failed to connect");

      TRACE("Incoming connection detected");
      
      host_name = leo_receive_string(client);

      TRACE_STR("process location", host_name);
      TRACE_STR("client remote host", client->remote_host);

      slist = tbx_htable_get(node_htable, host_name);
      if (slist)
	goto found;
      
      true_name = ntbx_true_name(host_name);
      TRACE_STR("trying", true_name);
      slist     = tbx_htable_get(node_htable, true_name);
      if (slist)
	{
	  TBX_FREE(host_name);
	  host_name = true_name;
	  true_name = NULL;
	  goto found;
	}

      if (!tbx_slist_is_nil(client->remote_alias))
	{
	  TRACE("client provided hostname not found, trying aliases");
	  tbx_slist_ref_to_head(client->remote_alias);
	  do
	    {
	      char *alias = NULL;
	      
	      alias = tbx_slist_ref_get(client->remote_alias);
	      TRACE_STR("trying", alias);
	      
	      slist = tbx_htable_get(node_htable, alias);
	      if (slist)
		{
		  TBX_FREE(host_name);
		  host_name = tbx_strdup(alias);
		  goto found;
		}
	      
	      true_name = ntbx_true_name(alias);
	      TRACE_STR("trying", true_name);
	      slist = tbx_htable_get(node_htable, true_name);
	      if (slist)
		{
		  TBX_FREE(host_name);
		  host_name = true_name;
		  true_name = NULL;
		  goto found;
		}

	      TRACE("alias mismatched");
	    }
	  while (tbx_slist_ref_forward(client->remote_alias));
	  
	  FAILURE("client hostname not found");
	}
      else
	FAILURE("invalid client answer");
      
    found:
      TRACE_STR("retained host name", host_name);
      process = tbx_slist_extract(slist);

      if (tbx_slist_is_nil(slist))
	{
	  slist = tbx_htable_extract(node_htable, host_name);
	  tbx_slist_free(slist);
	  slist = NULL;
	}

      process_specific = process->specific;
      process_specific->client = client;

      leo_send_int(client, process->global_rank);

      TBX_FREE(host_name);
      TRACE_VAL("connected process", process->global_rank);
    }
  while (!tbx_htable_empty(node_htable));

  tbx_htable_free(node_htable);
}

static
void
spawn_processes(void)
{
  LOG_IN();
  tbx_slist_ref_to_head(spawn_groups->slist);
  do
    {
      p_leo_spawn_group_t spawn_group = NULL;
      p_leo_loader_t      loader      = NULL;

      spawn_group = tbx_slist_ref_get(spawn_groups->slist);

      if (tbx_slist_is_nil(spawn_group->process_slist))
	continue;

      loader = tbx_htable_get(loaders, spawn_group->loader_name);
      if (!loader)
	FAILURE("loader unavailable");

      loader->loader_func(application_name,
			  application_flavor,
			  net_server,
			  spawn_group->process_slist);

      connect_processes(spawn_group->process_slist);
    }
  while (tbx_slist_ref_forward(spawn_groups->slist));
  LOG_OUT();
}

static
void
send_directory(void)
{
  p_tbx_slist_nref_t process_ref = NULL;

  LOG_IN();
  TRACE("Directory transmission");

  process_ref = tbx_slist_nref_alloc(directory->process_slist);

  tbx_slist_nref_to_head(process_ref);
  do
    {
      p_tbx_slist_t            slist            = NULL;
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
      int                      len              = 0;
  
      process          = tbx_slist_nref_get(process_ref);
      TRACE_VAL("Transmitting to", process->global_rank);

      process_specific = process->specific;
      client           = process_specific->client;
      
      // Processes
      TRACE("Sending processes");
      slist = directory->process_slist;
      len = tbx_slist_get_length(slist);

      if (len <= 0)
	FAILURE("invalid number of processes");
      
      leo_send_int(client, len);

      tbx_slist_ref_to_head(slist);
      do
	{
	  p_ntbx_process_t dir_process = NULL;
	  
	  dir_process = tbx_slist_ref_get(slist);
	  
	  TRACE_VAL("Process", dir_process->global_rank);
	  leo_send_int(client, dir_process->global_rank);
	}
      while (tbx_slist_ref_forward(slist));
      
      leo_send_string(client, "end{processes}");

      // Nodes
      TRACE("Sending nodes");
      slist = directory->node_slist;
      len = tbx_slist_get_length(slist);

      if (len <= 0)
	FAILURE("invalid number of nodes");
      
      leo_send_int(client, len);

      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_node_t           dir_node    = NULL;
	  p_ntbx_process_container_t pc          = NULL;
	  ntbx_process_grank_t       global_rank =   -1;

	  dir_node = tbx_slist_ref_get(slist);
	  pc = dir_node->pc;
	  
	  TRACE_STR("Node", dir_node->name);
	  leo_send_string(client, dir_node->name);
	  
	  if (ntbx_pc_first_global_rank(pc, &global_rank))
	    {
	      do
		{
		  ntbx_process_lrank_t local_rank = -1;

		  leo_send_int(client, global_rank);
		  local_rank = ntbx_pc_global_to_local(pc, global_rank);
		  
		  leo_send_int(client, local_rank);
		}
	      while (ntbx_pc_next_global_rank(pc, &global_rank));
	    }

	  leo_send_int(client, -1);
	}
      while (tbx_slist_ref_forward(slist));
      leo_send_string(client, "end{nodes}");

      // Drivers
      TRACE("Sending drivers");
      slist = directory->driver_slist;
      len = tbx_slist_get_length(slist);

      if (len <= 0)
	FAILURE("invalid number of drivers");
      
      leo_send_int(client, len);

      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_driver_t         dir_driver  = NULL;
	  p_ntbx_process_container_t pc          = NULL;
	  ntbx_process_grank_t       global_rank =   -1;
	  
	  dir_driver = tbx_slist_ref_get(slist);
	  pc = dir_driver->pc;
	  
	  TRACE_STR("Driver", dir_driver->name);
	  leo_send_string(client, dir_driver->name);
	  
	  if (ntbx_pc_first_global_rank(pc, &global_rank))
	    {
	      do
		{
		  p_ntbx_process_info_t               process_info   = NULL;
		  p_leo_dir_driver_process_specific_t pi_specific    = NULL;
		  int                                 adapter_number =    0;
		  p_tbx_slist_t                       adapter_slist  = NULL;

		  leo_send_int(client, global_rank);
		  process_info = ntbx_pc_get_global(pc, global_rank);
		  pi_specific = process_info->specific;

		  leo_send_int(client, process_info->local_rank);

		  adapter_slist  = pi_specific->adapter_slist;
		  adapter_number = tbx_slist_get_length(adapter_slist);
		  leo_send_int(client, adapter_number);

		  tbx_slist_ref_to_head(adapter_slist);
		  do
		    {
		      p_leo_dir_adapter_t adapter = NULL;

		      adapter = tbx_slist_ref_get(adapter_slist);

		      leo_send_string(client, adapter->name);
		      leo_send_string(client, adapter->selector);
		    }
		  while (tbx_slist_ref_forward(adapter_slist));

		}
	      while (ntbx_pc_next_global_rank(pc, &global_rank));
	    }

	  leo_send_int(client, -1);
	}
      while (tbx_slist_ref_forward(slist));

      leo_send_string(client, "end{drivers}");

      // Channels
      TRACE("Sending channels");
      slist = directory->channel_slist;
      len = tbx_slist_get_length(slist);

      if (len <= 0)
	FAILURE("invalid number of channels");
      
      leo_send_int(client, len);

      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_channel_t        dir_channel = NULL;
	  p_ntbx_process_container_t pc          = NULL;
	  p_ntbx_topology_table_t    ttable      = NULL;
	  ntbx_process_grank_t       global_rank =   -1;
	  ntbx_process_lrank_t       l_rank_src  =   -1;
	  
	  dir_channel = tbx_slist_ref_get(slist);
	  pc     = dir_channel->pc;
	  ttable = dir_channel->ttable;
	  
	  TRACE_STR("Channel", dir_channel->name);
	  leo_send_string(client, dir_channel->name);
	  leo_send_unsigned_int(client, dir_channel->public);
	  leo_send_string(client, dir_channel->driver->name);
	  
	  if (ntbx_pc_first_global_rank(pc, &global_rank))
	    {
	      do
		{
		  p_ntbx_process_info_t                process_info = NULL;
		  p_leo_dir_channel_process_specific_t pi_specific  = NULL;

		  leo_send_int(client, global_rank);
		  process_info = ntbx_pc_get_global(pc, global_rank);
		  pi_specific = process_info->specific;

		  leo_send_int(client, process_info->local_rank);
		  leo_send_string(client, pi_specific->adapter_name);
		}
	      while (ntbx_pc_next_global_rank(pc, &global_rank));
	    }
	  leo_send_int(client, -1);
	  
	  if (ntbx_pc_first_local_rank(pc, &l_rank_src))
	    {
	      do
		{
		  ntbx_process_lrank_t l_rank_dst = -1;

		  ntbx_pc_first_local_rank(pc, &l_rank_dst);
		  do
		    {
		      if (ntbx_topology_table_get(ttable,
						 l_rank_src,
						 l_rank_dst))
			{
			  leo_send_int(client, 1);
			  TRACE_CHAR('+');
			}
		      else
			{
			  leo_send_int(client, 0);
			  TRACE_CHAR(' ');
			}
		    }
		  while (ntbx_pc_next_local_rank(pc, &l_rank_dst));
		  TRACE_CHAR('\n');
		}
	      while (ntbx_pc_next_local_rank(pc, &l_rank_src));
	    }
	}
      while (tbx_slist_ref_forward(slist));
      leo_send_string(client, "end{channels}");

      // Forwarding channels
      TRACE("Sending fchannels");
      slist = directory->fchannel_slist;
      len = tbx_slist_get_length(slist);

      if (len < 0)
	FAILURE("invalid number of forwarding channels");
      
      leo_send_int(client, len);

      if (!tbx_slist_is_nil(slist))
	{
	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_leo_dir_fchannel_t dir_fchannel = NULL;
	      
	      dir_fchannel = tbx_slist_ref_get(slist);
	      
	      TRACE_STR("Fchannel", dir_fchannel->name);
	      leo_send_string(client, dir_fchannel->name);
	      leo_send_string(client, dir_fchannel->channel_name);
	    }
	  while (tbx_slist_ref_forward(slist));
	}
      leo_send_string(client, "end{fchannels}");

      // Virtual channels
      TRACE("Sending vchannels");
      slist = directory->vchannel_slist;
      len = tbx_slist_get_length(slist);

      if (len < 0)
	FAILURE("invalid number of virtual channels");
      
      leo_send_int(client, len);

      if (!tbx_slist_is_nil(slist))
	{
	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_leo_dir_vchannel_t       dir_vchannel           = NULL;
	      p_ntbx_process_container_t pc                     = NULL;
	      ntbx_process_grank_t       g_rank_src             =   -1;
	      p_tbx_slist_t              dir_channel_slist      = NULL;
	      int                        dir_channel_slist_len  =    0;
	      p_tbx_slist_t              dir_fchannel_slist     = NULL;
	      int                        dir_fchannel_slist_len =    0;
	  
	      dir_vchannel = tbx_slist_ref_get(slist);
	      pc = dir_vchannel->pc;
	  
	      TRACE_STR("Vchannel", dir_vchannel->name);
	      leo_send_string(client, dir_vchannel->name);
	  
	      dir_channel_slist     = dir_vchannel->dir_channel_slist;
	      dir_channel_slist_len = tbx_slist_get_length(dir_channel_slist);
	      leo_send_int(client, dir_channel_slist_len);

	      tbx_slist_ref_to_head(dir_channel_slist);
	      do
		{
		  p_leo_dir_channel_t dir_channel = NULL;
	      
		  dir_channel = tbx_slist_ref_get(dir_channel_slist);
		  leo_send_string(client, dir_channel->name);
		}
	      while (tbx_slist_ref_forward(dir_channel_slist));
	  
	      dir_fchannel_slist     = dir_vchannel->dir_fchannel_slist;
	      dir_fchannel_slist_len = tbx_slist_get_length(dir_fchannel_slist);
	      leo_send_int(client, dir_fchannel_slist_len);

	      tbx_slist_ref_to_head(dir_fchannel_slist);
	      do
		{
		  p_leo_dir_fchannel_t dir_fchannel = NULL;
	      
		  dir_fchannel = tbx_slist_ref_get(dir_fchannel_slist);
		  leo_send_string(client, dir_fchannel->name);
		}
	      while (tbx_slist_ref_forward(dir_fchannel_slist));
  
	      TRACE("Virtual channel routing table");
	      if (ntbx_pc_first_global_rank(pc, &g_rank_src))
		{
		  do
		    {
		      p_leo_dir_vchannel_process_specific_t pi_specific  = NULL;
		      p_ntbx_process_container_t            ppc          = NULL;
		      ntbx_process_grank_t                  g_rank_dst   =   -1;

		      leo_send_int(client, g_rank_src);
		      pi_specific = ntbx_pc_get_global_specific(pc, g_rank_src);
		      ppc = pi_specific->pc;

		      if (ntbx_pc_first_global_rank(ppc, &g_rank_dst))
			{
			  do
			    {
			      p_leo_dir_vchannel_process_routing_table_t rtable =
				NULL;

			      leo_send_int(client, g_rank_dst);

			      rtable = ntbx_pc_get_global_specific(ppc, g_rank_dst);
			      TRACE("Process %d to %d: using channel %s through process %d",
				    g_rank_src, g_rank_dst, rtable->channel_name,
				    rtable->destination_rank);
			  
			      leo_send_string(client, rtable->channel_name);
			      leo_send_int(client,    rtable->destination_rank);
			    }
			  while
			    (ntbx_pc_next_global_rank(ppc, &g_rank_dst));
			}

		      leo_send_int(client, -1);
		    }
		  while (ntbx_pc_next_global_rank(pc, &g_rank_src));
		}

	      leo_send_int(client, -1);
	    }
	  while (tbx_slist_ref_forward(slist));
	}
      leo_send_string(client, "end{vchannels}");
  
      // End
      leo_send_string(client, "end{directory}");

     }
  while (tbx_slist_nref_forward(process_ref));
  
  tbx_slist_nref_free(process_ref);

  TRACE("Directory transmission completed");
  LOG_OUT();
}


static
void
init_drivers(void)
{
  p_tbx_slist_nref_t process_ref = NULL;

  LOG_IN();  
  TRACE("Initializing drivers");

  process_ref = tbx_slist_nref_alloc(directory->process_slist);

  TRACE("First pass");
  tbx_slist_nref_to_head(process_ref);
  do
    {
      p_tbx_slist_t            slist            = NULL;
      p_ntbx_process_t         process          = NULL;
      ntbx_process_grank_t     global_rank      =   -1;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
  
      process          = tbx_slist_nref_get(process_ref);
      global_rank      = process->global_rank;
      process_specific = process->specific;
      client           = process_specific->client;

      TRACE_VAL("Transmitting to", global_rank);
      TRACE("Sending drivers");
      slist = directory->driver_slist;

      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_driver_t                  dir_driver     = NULL;
	  p_ntbx_process_container_t          pc             = NULL;
	  p_ntbx_process_info_t               process_info   = NULL;
	  p_leo_dir_driver_process_specific_t pi_specific    = NULL;
	  p_tbx_slist_t                       adapter_slist  = NULL;
	  
	  dir_driver = tbx_slist_ref_get(slist);
	  pc         = dir_driver->pc;
	  
	  TRACE_STR("Driver", dir_driver->name);

	  process_info = ntbx_pc_get_global(pc, global_rank);

	  if (!process_info)
	    continue;
	  
	  leo_send_string(client, dir_driver->name);
	  pi_specific   = process_info->specific;
	  adapter_slist = pi_specific->adapter_slist;
	    
	  tbx_slist_ref_to_head(adapter_slist);
	  do
	    {
	      p_leo_dir_adapter_t dir_adapter = NULL;
		
	      dir_adapter = tbx_slist_ref_get(adapter_slist);
	      TRACE_STR("Adapter", dir_adapter->name);
	      leo_send_string(client, dir_adapter->name);	  
	      dir_adapter->parameter = leo_receive_string(client);
	      dir_adapter->mtu       = leo_receive_unsigned_int(client);
	      TRACE_STR("Parameter", dir_adapter->parameter);
	    }
	  while (tbx_slist_ref_forward(adapter_slist));

	  leo_send_string(client, "-");	  
	}
      while (tbx_slist_ref_forward(slist));

      leo_send_string(client, "-");	  
    }
  while (tbx_slist_nref_forward(process_ref));
  
  TRACE("Second pass");
  tbx_slist_nref_to_head(process_ref);
  do
    {
      p_tbx_slist_t            slist            = NULL;
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
  
      process          = tbx_slist_nref_get(process_ref);
      TRACE_VAL("Transmitting to", process->global_rank);

      process_specific = process->specific;
      client           = process_specific->client;

      TRACE("Sending drivers");
      slist = directory->driver_slist;

      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_driver_t         dir_driver  = NULL;
	  p_ntbx_process_container_t pc          = NULL;
	  ntbx_process_grank_t       global_rank =   -1;
	  
	  dir_driver = tbx_slist_ref_get(slist);
	  pc = dir_driver->pc;
	  
	  TRACE_STR("Driver", dir_driver->name);
	  leo_send_string(client, dir_driver->name);

	  if (ntbx_pc_first_global_rank(pc, &global_rank))
	    {
	      do
		{
		  p_ntbx_process_info_t               process_info   = NULL;
		  p_leo_dir_driver_process_specific_t pi_specific    = NULL;
		  p_tbx_slist_t                       adapter_slist  = NULL;

#if 0
		  if (global_rank == process->global_rank)
		    continue;
#endif // 0	      
		  leo_send_int(client, global_rank);
		  process_info  = ntbx_pc_get_global(pc, global_rank);
		  pi_specific   = process_info->specific;
		  adapter_slist = pi_specific->adapter_slist;

		  tbx_slist_ref_to_head(adapter_slist);
		  do
		    {
		      p_leo_dir_adapter_t adapter = NULL;

		      adapter = tbx_slist_ref_get(adapter_slist);
		      leo_send_string(client, adapter->name);	  

		      leo_send_string(client, adapter->parameter);
		      leo_send_unsigned_int(client, adapter->mtu);
		    }
		  while (tbx_slist_ref_forward(adapter_slist));

		  leo_send_string(client, "-");	  
		}
	      while (ntbx_pc_next_global_rank(pc, &global_rank));
	    }

	  leo_send_int(client, -1); 
	}
      while (tbx_slist_ref_forward(slist));

      leo_send_string(client, "-");	  
    }
  while (tbx_slist_nref_forward(process_ref));

  tbx_slist_nref_free(process_ref);      
  LOG_OUT();
}


static
void
init_channels(void)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  // Channels
  slist = directory->channel_slist;
  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_channel_t        dir_channel = NULL;
	  p_ntbx_process_container_t pc          = NULL;
	  p_ntbx_topology_table_t    ttable      = NULL;
	  ntbx_process_grank_t       global_rank =   -1;
	  ntbx_process_lrank_t       l_rank_src  =   -1;

	  dir_channel = tbx_slist_ref_get(slist);
	  pc     = dir_channel->pc;
	  ttable = dir_channel->ttable;

	  TRACE_STR("Channel", dir_channel->name);
	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t         process          = NULL;
	      p_leo_process_specific_t process_specific = NULL;
	      p_ntbx_client_t          client           = NULL;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 1 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      leo_send_string(client, dir_channel->name);
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	  ntbx_pc_first_local_rank(pc, &l_rank_src);
	  do
	    {
	      p_ntbx_process_t         process_src          = NULL;
	      p_leo_process_specific_t process_specific_src = NULL;
	      p_ntbx_client_t          client_src           = NULL;
	      ntbx_process_lrank_t     l_rank_dst           =   -1;

	      process_src = ntbx_pc_get_local_process(pc,
							 l_rank_src);
	      process_specific_src = process_src->specific;
	      client_src           = process_specific_src->client;

	      ntbx_pc_first_local_rank(pc, &l_rank_dst);
	      do
		{
		  p_ntbx_process_t         process_dst          =
		    NULL;
		  p_leo_process_specific_t process_specific_dst =
		    NULL;
		  p_ntbx_client_t          client_dst           =
		    NULL;
		  int                      ack_src              = 0;
		  int                      ack_dst              = 0;

		  if (!ntbx_topology_table_get(ttable, l_rank_src,
					       l_rank_dst))
		    continue;
		  
		  TRACE("Initializing connection: %d to %d",
		       l_rank_src, l_rank_dst);
		  
		  process_dst = ntbx_pc_get_local_process(pc,
							     l_rank_dst);
		  process_specific_dst = process_dst->specific;
		  client_dst           =
		    process_specific_dst->client;

		  leo_send_int(client_src, 0);
		  leo_send_int(client_src, l_rank_dst);
		  leo_send_int(client_dst, 1);
		  leo_send_int(client_dst, l_rank_src);

		  ack_src = leo_receive_int(client_src);
		  if (ack_src != -1)
		    FAILURE("synchronization error");
		  
		  ack_dst = leo_receive_int(client_dst);
		  if (ack_dst != -1)
		    FAILURE("synchronization error");
		}
	      while (ntbx_pc_next_local_rank(pc, &l_rank_dst));
	    }
	  while (ntbx_pc_next_local_rank(pc, &l_rank_src));

	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t          process          = NULL;
	      p_leo_process_specific_t  process_specific = NULL;
	      p_ntbx_client_t           client           = NULL;
	      int                       ack              =    0;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 2 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      leo_send_int(client, -1);
	      ack = leo_receive_int(client);
	      if (ack != -1)
		FAILURE("synchronization error");
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	  ntbx_pc_first_local_rank(pc, &l_rank_src);
	  do
	    {
	      p_ntbx_process_t         process_src          = NULL;
	      p_leo_process_specific_t process_specific_src = NULL;
	      p_ntbx_client_t          client_src           = NULL;
	      ntbx_process_lrank_t     l_rank_dst           =   -1;

	      process_src = ntbx_pc_get_local_process(pc,
							 l_rank_src);
	      process_specific_src = process_src->specific;
	      client_src           = process_specific_src->client;

	      ntbx_pc_first_local_rank(pc, &l_rank_dst);
	      do
		{
		  p_ntbx_process_t         process_dst          =
		    NULL;
		  p_leo_process_specific_t process_specific_dst =
		    NULL;
		  p_ntbx_client_t          client_dst           =
		    NULL;
		  int                      ack_src              = 0;
		  int                      ack_dst              = 0;

		  if (!ntbx_topology_table_get(ttable, l_rank_src,
					       l_rank_dst))
		    continue;
		  
		  TRACE("Connecting %d to %d", l_rank_src, l_rank_dst);
		  
		  process_dst = ntbx_pc_get_local_process(pc, l_rank_dst);
		  process_specific_dst = process_dst->specific;
		  client_dst           =
		    process_specific_dst->client;

		  leo_send_int(client_src, 0);
		  leo_send_int(client_src, l_rank_dst);
		  leo_send_int(client_dst, 1);
		  leo_send_int(client_dst, l_rank_src);

		  ack_src = leo_receive_int(client_src);
		  if (ack_src != -1)
		    FAILURE("synchronization error");
		  
		  ack_dst = leo_receive_int(client_dst);
		  if (ack_dst != -1)
		    FAILURE("synchronization error");
		}
	      while (ntbx_pc_next_local_rank(pc, &l_rank_dst));
	    }
	  while (ntbx_pc_next_local_rank(pc, &l_rank_src));

	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t          process          = NULL;
	      p_leo_process_specific_t  process_specific = NULL;
	      p_ntbx_client_t           client           = NULL;
	      int                       ack              =    0;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 3 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      leo_send_int(client, -1);
	      ack = leo_receive_int(client);
	      if (ack != -1)
		FAILURE("synchronization error");
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));
	}
      while (tbx_slist_ref_forward(slist));
    }
  
  slist = directory->process_slist;
  tbx_slist_ref_to_head(slist);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
      
      process          = tbx_slist_ref_get(slist);
      process_specific = process->specific;
      client           = process_specific->client;
      
      leo_send_string(client, "-");
    }
  while (tbx_slist_ref_forward(slist));

  // Forwarding channels
  slist = directory->fchannel_slist;
  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_fchannel_t       dir_fchannel = NULL;
	  p_leo_dir_channel_t        dir_channel  = NULL;
	  p_ntbx_topology_table_t    ttable       = NULL;
	  p_ntbx_process_container_t pc           = NULL;
	  ntbx_process_grank_t       global_rank  =   -1;
	  ntbx_process_lrank_t       l_rank_src   =   -1;

	  dir_fchannel = tbx_slist_ref_get(slist);
	  dir_channel  = tbx_htable_get(directory->channel_htable,
					dir_fchannel->channel_name);
	  
	  pc = dir_channel->pc;
	  ttable = dir_channel->ttable;
	  TRACE_STR("Forwarding channel", dir_fchannel->name);
	  ntbx_pc_first_global_rank(pc, &global_rank);
	  TRACE_VAL("global_rank", global_rank);

	  do
	    {
	      p_ntbx_process_t         process          = NULL;
	      p_leo_process_specific_t process_specific = NULL;
	      p_ntbx_client_t          client           = NULL;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 1 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      leo_send_string(client, dir_fchannel->name);
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	  ntbx_pc_first_local_rank(pc, &l_rank_src);
	  do
	    {
	      p_ntbx_process_t         process_src          = NULL;
	      p_leo_process_specific_t process_specific_src = NULL;
	      p_ntbx_client_t          client_src           = NULL;
	      ntbx_process_lrank_t     l_rank_dst           =   -1;

	      process_src = ntbx_pc_get_local_process(pc,
							 l_rank_src);
	      process_specific_src = process_src->specific;
	      client_src           = process_specific_src->client;

	      ntbx_pc_first_local_rank(pc, &l_rank_dst);
	      do
		{
		  p_ntbx_process_t         process_dst          =
		    NULL;
		  p_leo_process_specific_t process_specific_dst =
		    NULL;
		  p_ntbx_client_t          client_dst           =
		    NULL;
		  int                      ack_src              = 0;
		  int                      ack_dst              = 0;

		  if (!ntbx_topology_table_get(ttable, l_rank_src,
					       l_rank_dst))
		    continue;
		  
		  TRACE("Initializing connection: %d to %d",
		       l_rank_src, l_rank_dst);
		  
		  process_dst = ntbx_pc_get_local_process(pc,
							     l_rank_dst);
		  process_specific_dst = process_dst->specific;
		  client_dst           =
		    process_specific_dst->client;

		  leo_send_int(client_src, 0);
		  leo_send_int(client_src, l_rank_dst);
		  leo_send_int(client_dst, 1);
		  leo_send_int(client_dst, l_rank_src);

		  ack_src = leo_receive_int(client_src);
		  if (ack_src != -1)
		    FAILURE("synchronization error");
		  
		  ack_dst = leo_receive_int(client_dst);
		  if (ack_dst != -1)
		    FAILURE("synchronization error");
		}
	      while (ntbx_pc_next_local_rank(pc, &l_rank_dst));
	    }
	  while (ntbx_pc_next_local_rank(pc, &l_rank_src));

	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t          process          = NULL;
	      p_leo_process_specific_t  process_specific = NULL;
	      p_ntbx_client_t           client           = NULL;
	      int                       ack              =    0;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 2 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      leo_send_int(client, -1);
	      ack = leo_receive_int(client);
	      if (ack != -1)
		FAILURE("synchronization error");
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	  ntbx_pc_first_local_rank(pc, &l_rank_src);
	  do
	    {
	      p_ntbx_process_t         process_src          = NULL;
	      p_leo_process_specific_t process_specific_src = NULL;
	      p_ntbx_client_t          client_src           = NULL;
	      ntbx_process_lrank_t     l_rank_dst           =   -1;

	      process_src = ntbx_pc_get_local_process(pc,
							 l_rank_src);
	      process_specific_src = process_src->specific;
	      client_src           = process_specific_src->client;

	      ntbx_pc_first_local_rank(pc, &l_rank_dst);
	      do
		{
		  p_ntbx_process_t         process_dst          =
		    NULL;
		  p_leo_process_specific_t process_specific_dst =
		    NULL;
		  p_ntbx_client_t          client_dst           =
		    NULL;
		  int                      ack_src              = 0;
		  int                      ack_dst              = 0;

		  if (!ntbx_topology_table_get(ttable, l_rank_src,
					       l_rank_dst))
		    continue;
		  
		  TRACE("Connecting %d to %d", l_rank_src, l_rank_dst);
		  
		  process_dst = ntbx_pc_get_local_process(pc, l_rank_dst);
		  process_specific_dst = process_dst->specific;
		  client_dst           =
		    process_specific_dst->client;

		  leo_send_int(client_src, 0);
		  leo_send_int(client_src, l_rank_dst);
		  leo_send_int(client_dst, 1);
		  leo_send_int(client_dst, l_rank_src);

		  ack_src = leo_receive_int(client_src);
		  if (ack_src != -1)
		    FAILURE("synchronization error");
		  
		  ack_dst = leo_receive_int(client_dst);
		  if (ack_dst != -1)
		    FAILURE("synchronization error");
		}
	      while (ntbx_pc_next_local_rank(pc, &l_rank_dst));
	    }
	  while (ntbx_pc_next_local_rank(pc, &l_rank_src));

	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t          process          = NULL;
	      p_leo_process_specific_t  process_specific = NULL;
	      p_ntbx_client_t           client           = NULL;
	      int                       ack              =    0;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 3 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      leo_send_int(client, -1);
	      ack = leo_receive_int(client);
	      if (ack != -1)
		FAILURE("synchronization error");
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	}
      while (tbx_slist_ref_forward(slist));
    }
  
  slist = directory->process_slist;
  tbx_slist_ref_to_head(slist);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
      
      process          = tbx_slist_ref_get(slist);
      process_specific = process->specific;
      client           = process_specific->client;
      
      leo_send_string(client, "-");
    }
  while (tbx_slist_ref_forward(slist));


  // Virtual channels
  slist = directory->vchannel_slist;
  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_vchannel_t       dir_vchannel = NULL;
	  p_ntbx_process_container_t pc           = NULL;
	  ntbx_process_grank_t       global_rank  =   -1;

	  dir_vchannel = tbx_slist_ref_get(slist);
	  pc = dir_vchannel->pc;
	  TRACE_STR("Virtual channel", dir_vchannel->name);
	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t         process          = NULL;
	      p_leo_process_specific_t process_specific = NULL;
	      p_ntbx_client_t          client           = NULL;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 1 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      leo_send_string(client, dir_vchannel->name);
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t          process          = NULL;
	      p_leo_process_specific_t  process_specific = NULL;
	      p_ntbx_client_t           client           = NULL;
	      char                     *msg              = NULL;
	  
	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 2 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;

	      msg = leo_receive_string(client);
	      if (strcmp(msg, "ok"))
		FAILURE("synchronization error");
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));
	}
      while (tbx_slist_ref_forward(slist));
    }
  
  slist = directory->process_slist;
  tbx_slist_ref_to_head(slist);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
      
      process          = tbx_slist_ref_get(slist);
      process_specific = process->specific;
      client           = process_specific->client;
      
      leo_send_string(client, "-");
    }
  while (tbx_slist_ref_forward(slist));
  LOG_OUT();
}

static
void
exit_session(void)
{
  p_ntbx_client_t  *client_array = NULL;
  int               nb_clients   =    0;
  int               i            =    0;
  
  LOG_IN();
  nb_clients    = tbx_slist_get_length(directory->process_slist);
  client_array  = TBX_CALLOC(nb_clients, sizeof(ntbx_client_t));

  tbx_slist_ref_to_head(directory->process_slist);
  do
    {
      p_ntbx_process_t          process          = NULL;
      p_leo_process_specific_t  process_specific = NULL;
      p_ntbx_client_t           client           = NULL;
      
      process           = tbx_slist_ref_get(directory->process_slist);
      process_specific  = process->specific;
      client            = process_specific->client;
      client_array[i++] = client;
    }
  while (tbx_slist_ref_forward(directory->process_slist));

  while (nb_clients)
    {
      int status = -1;
      
      status = ntbx_tcp_read_poll(nb_clients, client_array);
      
      if (status > 0)
	{
	  int j = 0;
	  
	  while (status && (j < nb_clients))
	    {
	      
	      p_ntbx_client_t          client           = NULL;
	      int                      read_status      = ntbx_failure;
	      int                      data             = 0;
	      ntbx_pack_buffer_t       pack_buffer;

	      client = client_array[j];
	      TRACE("status = %d, nb_clients = %d, j = %d", status,
		   nb_clients, j);

	      if (client->state != ntbx_client_state_data_ready)
		{
		  j++;
		  continue;
		}
	      
	      read_status = ntbx_tcp_read_pack_buffer(client, &pack_buffer);
	      if (read_status == -1)
		FAILURE("control link failure");

	      data = ntbx_unpack_int(&pack_buffer);

	      switch (data)
		{
		case leo_command_end:
		  {
		    status--;
		    nb_clients--;
		    memmove(client_array + j, client_array + j + 1,
			    (nb_clients - j) * sizeof(p_ntbx_client_t));
		  }
		  break;
		case leo_command_print:
		  {
		    char *string = NULL;
		    
		    status--;
		    string = leo_receive_string(client);
		    DISP("%s", string);
		    free(string);
		    string = NULL;
		  }
		  break;
		default:
		  FAILURE("synchronization error");
		}
	      
	    }
	  
	  if (status)
	    FAILURE("incoherent behaviour");
	}
    }
  
  tbx_slist_ref_to_head(directory->process_slist);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
      int                      data             =    0;
      
      process           = tbx_slist_ref_get(directory->process_slist);
      process_specific  = process->specific;
      client            = process_specific->client;
      leo_send_int(client, 1);

      // Waiting for "Forward send" threads to shutdown
      data = leo_receive_int(client);
      
      if (data != -1)
	FAILURE("synchronization error");
    }
  while (tbx_slist_ref_forward(directory->process_slist));

  // Shuting down "Forward receive" threads
  if (!tbx_slist_is_nil(directory->vchannel_slist))
    {
      p_tbx_slist_t vslist = NULL;

      vslist = directory->vchannel_slist;
      tbx_slist_ref_to_head(vslist);
      do
	{
	  p_leo_dir_vchannel_t vchannel = NULL;
	  p_tbx_slist_t        slist = NULL;
	  
	  vchannel = tbx_slist_ref_get(vslist);

	  // Regular channels
	  slist = vchannel->dir_channel_slist;
	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_leo_dir_channel_t         channel   = NULL;
	      p_ntbx_topology_table_t     ttable    = NULL;
	      ntbx_process_lrank_t        dst_lrank =   -1;
	      p_ntbx_process_container_t  pc        = NULL;
	      
	      channel = tbx_slist_ref_get(slist);
	      ttable  = channel->ttable;
	      pc      = channel->pc;

	      if (ntbx_pc_first_local_rank(pc, &dst_lrank))
		{
		  do
		    {
		      ntbx_process_lrank_t src_lrank =   -1;
		      
		      ntbx_pc_first_local_rank(pc, &src_lrank);
		      do
			{
			  p_ntbx_client_t src_client = NULL;
			  p_ntbx_client_t dst_client = NULL;

			  if (src_lrank == dst_lrank)
			    continue;
			  
			  if (!ntbx_topology_table_get(ttable, src_lrank,
						       dst_lrank))
			    continue;
			  
			  {
			    p_ntbx_process_t         process          = NULL;
			    p_leo_process_specific_t process_specific = NULL;
      
			    process          =
			      ntbx_pc_get_local_process(pc, src_lrank);
			    process_specific = process->specific;
			    src_client       = process_specific->client;
			  }
			  
			  {
			    p_ntbx_process_t         process          = NULL;
			    p_leo_process_specific_t process_specific = NULL;
      
			    process          =
			      ntbx_pc_get_local_process(pc, dst_lrank);
			    process_specific = process->specific;
			    dst_client       = process_specific->client;
			  }

			  leo_send_string(src_client, vchannel->name);
			  leo_send_string(src_client, channel->name);
			  leo_send_int(src_client, dst_lrank);
			  leo_send_string(dst_client, vchannel->name);
			  leo_send_string(dst_client, channel->name);
			  leo_send_int(dst_client, -1);
			  
			  {
			    int data = 0;

			    data = leo_receive_int(src_client);
			    if (data != -1)
			      FAILURE("synchronization error");
			    
			    data = leo_receive_int(dst_client);
			    if (data != -1)
			      FAILURE("synchronization error");
			  }
			  
			  break;
			}
		      while (ntbx_pc_next_local_rank(pc, &src_lrank));
		    }
		  while (ntbx_pc_next_local_rank(pc, &dst_lrank));
		}
	    }
	  while (tbx_slist_ref_forward(slist));
	  
	  // Forwarding channels
	  slist = vchannel->dir_fchannel_slist;
	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_leo_dir_fchannel_t        fchannel  = NULL;
	      p_ntbx_topology_table_t     ttable    = NULL;
	      p_leo_dir_channel_t         channel   = NULL;
	      ntbx_process_lrank_t        dst_lrank =   -1;
	      p_ntbx_process_container_t  pc        = NULL;
	      
	      fchannel = tbx_slist_ref_get(slist);
	      channel  = tbx_htable_get(directory->channel_htable,
					fchannel->channel_name);	      
	      ttable   = channel->ttable;
	      pc       = channel->pc;

	      if (ntbx_pc_first_local_rank(pc, &dst_lrank))
		{
		  do
		    {
		      ntbx_process_lrank_t src_lrank =   -1;
		      
		      ntbx_pc_first_local_rank(pc, &src_lrank);
		      do
			{
			  p_ntbx_client_t src_client = NULL;
			  p_ntbx_client_t dst_client = NULL;

			  if (src_lrank == dst_lrank)
			    continue;
			  
			  if (!ntbx_topology_table_get(ttable, src_lrank,
						       dst_lrank))
			    continue;
			  
			  {
			    p_ntbx_process_t         process          = NULL;
			    p_leo_process_specific_t process_specific = NULL;
      
			    process          =
			      ntbx_pc_get_local_process(pc, src_lrank);
			    process_specific = process->specific;
			    src_client       = process_specific->client;
			  }
			  
			  {
			    p_ntbx_process_t         process          = NULL;
			    p_leo_process_specific_t process_specific = NULL;
      
			    process          =
			      ntbx_pc_get_local_process(pc, dst_lrank);
			    process_specific = process->specific;
			    dst_client       = process_specific->client;
			  }

			  leo_send_string(src_client, vchannel->name);
			  leo_send_string(src_client, fchannel->name);
			  leo_send_int(src_client, dst_lrank);
			  leo_send_string(dst_client, vchannel->name);
			  leo_send_string(dst_client, fchannel->name);
			  leo_send_int(dst_client, -1);
			  
			  {
			    int data = 0;

			    data = leo_receive_int(src_client);
			    if (data != -1)
			      FAILURE("synchronization error");
			    
			    data = leo_receive_int(dst_client);
			    if (data != -1)
			      FAILURE("synchronization error");
			  }

			  break;
			}
		      while (ntbx_pc_next_local_rank(pc, &src_lrank));
		    }
		  while (ntbx_pc_next_local_rank(pc, &dst_lrank));
		}
	    }
	  while (tbx_slist_ref_forward(slist));
	  
	}
      while (tbx_slist_ref_forward(vslist));
      
    }
  
  tbx_slist_ref_to_head(directory->process_slist);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
      
      process           = tbx_slist_ref_get(directory->process_slist);
      process_specific  = process->specific;
      client            = process_specific->client;
      leo_send_string(client, "-");
    }
  while (tbx_slist_ref_forward(directory->process_slist));

  LOG_OUT();
}

#endif // NO_SPAWN

int
main(int    argc,
     char **argv)
{
  p_tbx_htable_t application_file_htable = NULL;
  p_tbx_htable_t application_htable      = NULL;

  LOG_IN();
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  
  TRACE("== Initializing parser");
  leoparse_init(argc, argv);
  leoparse_purge_cmd_line(&argc, argv);

  TRACE("== Registering loaders");
  loaders = leo_loaders_register();

  
  TRACE("== Processing command line");
  argc--; argv++;

  if (argc != 1)
    leo_usage();

  TRACE("== Initializing internal structures");
  net_server    = leo_net_server_init();
  networks      = leo_networks_init();
  directory     = leo_directory_init();
  spawn_groups  = leo_spawn_groups_init();
  
  TRACE("== Parsing configuration file");
  application_file_htable = leoparse_parse_local_file(*argv);
  application_htable      = leoparse_read_htable(application_file_htable,
						 "application");
  TRACE("== Processing configuration");
  process_application(application_htable);

#ifndef LEO_NO_SPAWN
  TRACE("== Launching processes");
  spawn_processes();
  TRACE("== Transmitting directory");
  send_directory();
  init_drivers();
  init_channels();
  exit_session();

  getchar();
  leonie_processes_cleanup();
#endif // LEO_NO_SPAWN
  LOG_OUT();

  TRACE("== Leonie server shutdown");
  return 0;
}
