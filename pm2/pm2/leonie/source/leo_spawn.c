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
 * leo_spawn.c
 * ===========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

// connect_processes: establishes the network connections between a set of 
// processes of the session and the Leonie server.
// The 'process_slist' argument contains the list of the set of processes
// to connect to Leonie. For identifications purposes, each process has to
// send its host name to Leonie. The matching between the host name sent
// by the client and the host names availables in the internal directory
// is performed by this function, possibly by normalizing and/or trying 
// host name aliases.
static
void
connect_processes(p_leonie_t leonie,
		  p_tbx_slist_t process_slist)
{
  p_ntbx_server_t net_server  = NULL;
  p_tbx_htable_t  node_htable = NULL;
  
  if (tbx_slist_is_nil(process_slist))
    return;
  
  net_server  = leonie->net_server;
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
      
      {
	char           *ip_str = NULL;
	unsigned long   ip     = 0;
	struct hostent *h      = NULL;
	
	ip_str = leo_receive_string(client);
	ip     = strtoul(ip_str, &host_name, 16);
	h      = gethostbyaddr((char *)&ip, sizeof(unsigned long), AF_INET);
	host_name = tbx_strdup(h->h_name);
	TBX_FREE(ip_str);
      }

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

// spawn_processes: spawns the session processes by groups using the static
// variable 'spawn_groups'. Each group may use a different loader function.
void
spawn_processes(p_leonie_t leonie)
{
  p_leo_spawn_groups_t spawn_groups = NULL;
  p_tbx_htable_t       loaders      = NULL;
  p_leo_settings_t     settings     = NULL;
  p_ntbx_server_t      net_server   = NULL;

  LOG_IN();
  spawn_groups = leonie->spawn_groups;
  loaders      = leonie->loaders;
  settings     = leonie->settings;
  net_server   = leonie->net_server;

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

      loader->loader_func(settings,
			  net_server,
			  spawn_group->process_slist);

      connect_processes(leonie, spawn_group->process_slist);
    }
  while (tbx_slist_ref_forward(spawn_groups->slist));
  LOG_OUT();
}

// send_directory: sends the internal directory structure to each session
// process.
void
send_directory(p_leonie_t leonie)
{
  p_leo_directory_t  dir         = NULL;
  p_tbx_slist_nref_t process_ref = NULL;

  LOG_IN();
  TRACE("Directory transmission");

  dir = leonie->directory;

  process_ref = tbx_slist_nref_alloc(dir->process_slist);

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
      slist = dir->process_slist;
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
      slist = dir->node_slist;
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
	  {
	    char ip_str[11];

	    sprintf(ip_str,"%lx", dir_node->ip);
	    TRACE_STR("Node IP", ip_str);
	    leo_send_string(client, ip_str);
	  }
	  
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
      slist = dir->driver_slist;
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
      slist = dir->channel_slist;
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
      slist = dir->fchannel_slist;
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
      slist = dir->vchannel_slist;
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

