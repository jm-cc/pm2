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
 * Mad_directory.c
 * ===============
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

#include "madeleine.h"

static
void
mad_dir_process_get(p_mad_madeleine_t madeleine)
{
  p_mad_session_t    session        = NULL;
  p_mad_directory_t  dir            = NULL;
  p_ntbx_client_t    client         = NULL;
  p_tbx_darray_t     process_darray = NULL;
  p_tbx_slist_t      process_slist  = NULL;
  char              *command_string = NULL;
  int                number         =    0;

  LOG_IN();
  session = madeleine->session;
  dir     = madeleine->dir;
  client  = session->leonie_link;

  process_darray = dir->process_darray;
  process_slist  = dir->process_slist;

  number = mad_ntbx_receive_int(client);
  if (number <= 0)
    FAILURE("invalid number of processes");
  
  TRACE_VAL("Number of processes", number);
  
  while (number--)
    {
      ntbx_process_grank_t global_rank =   -1;
      p_ntbx_process_t     process     = NULL;

      global_rank = mad_ntbx_receive_int(client);
      TRACE_VAL("Received process", global_rank);

      process = ntbx_process_cons();
      process->global_rank = global_rank;
      process->ref         = tbx_htable_empty_table();

      tbx_darray_expand_and_set(process_darray, global_rank, process);
      tbx_slist_append(process_slist, process);
    }
  
  command_string = mad_ntbx_receive_string(client);
  
  if (strcmp(command_string, "end{processes}"))
    FAILURE("synchronisation error");
  
  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}

static
void
mad_dir_node_get(p_mad_madeleine_t madeleine)
{
  p_mad_session_t    session        = NULL;
  p_mad_directory_t  dir            = NULL;
  p_ntbx_client_t    client         = NULL;
  p_tbx_darray_t     process_darray = NULL;
  p_tbx_htable_t     node_htable    = NULL;
  p_tbx_slist_t      node_slist     = NULL;
  char              *command_string = NULL;
  int                number         =    0;

  LOG_IN();
  session = madeleine->session;
  dir     = madeleine->dir;
  client  = session->leonie_link;

  process_darray = dir->process_darray;
  node_htable    = dir->node_htable;
  node_slist     = dir->node_slist;

  number = mad_ntbx_receive_int(client);

  if (number <= 0)
    FAILURE("invalid number of nodes");
  
  TRACE_VAL("Number of nodes", number);
  
  while (number--)
    {
      p_mad_dir_node_t dir_node = NULL;
      
      dir_node = mad_dir_node_cons();
      dir_node->name = mad_ntbx_receive_string(client);
      TRACE_STR("Node name", dir_node->name);

      tbx_htable_add(node_htable, dir_node->name, dir_node);
      dir_node->id = tbx_slist_get_length(node_slist);
      tbx_slist_append(node_slist, dir_node);

      while (1)
	{
	  p_ntbx_process_t     process          = NULL;
	  ntbx_process_grank_t node_global_rank =   -1;
	  ntbx_process_lrank_t node_local_rank  =   -1;

	  node_global_rank = mad_ntbx_receive_int(client);
	  if (node_global_rank == -1)
	    break;
	  
	  node_local_rank = mad_ntbx_receive_int(client);
	  process = tbx_darray_get(process_darray, node_global_rank);
	  
	  ntbx_pc_add(dir_node->pc, process, node_local_rank,
		      dir_node, "node", NULL);
	  
	}
    }
  
  command_string = mad_ntbx_receive_string(client);
  
  if (strcmp(command_string, "end{nodes}"))
    FAILURE("synchronisation error");
  
  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}

static
void
mad_dir_driver_get(p_mad_madeleine_t madeleine)
{
  p_mad_session_t    session        = NULL;
  p_mad_directory_t  dir            = NULL;
  p_ntbx_client_t    client         = NULL;
  p_tbx_darray_t     process_darray = NULL;
  p_tbx_htable_t     driver_htable  = NULL;
  p_tbx_slist_t      driver_slist   = NULL;
  char              *command_string = NULL;
  int                number         =    0;

  LOG_IN();
  session = madeleine->session;
  dir     = madeleine->dir;
  client  = session->leonie_link;

  process_darray = dir->process_darray;
  driver_htable  = dir->driver_htable;
  driver_slist   = dir->driver_slist;

  number = mad_ntbx_receive_int(client);

  if (number <= 0)
    FAILURE("invalid number of driver");
  
  TRACE_VAL("Number of driver", number);
  
  while (number--)
    {
      p_mad_dir_driver_t  dir_driver            = NULL;
      char               *driver_reference_name = NULL;

      dir_driver = mad_dir_driver_cons();
      dir_driver->name = mad_ntbx_receive_string(client);
      TRACE_STR("Driver name", dir_driver->name);
      
      driver_reference_name =
	TBX_MALLOC(strlen("driver") + strlen(dir_driver->name) + 2);
      CTRL_ALLOC(driver_reference_name);
      sprintf(driver_reference_name, "%s:%s", "driver", dir_driver->name);

      tbx_htable_add(driver_htable, dir_driver->name, dir_driver);
      dir_driver->id = tbx_slist_get_length(driver_slist);
      tbx_slist_append(driver_slist, dir_driver);

      while (1)
	{
	  p_ntbx_process_t                    process            = NULL;
	  p_mad_dir_driver_process_specific_t pi_specific        = NULL;
	  ntbx_process_grank_t                driver_global_rank =   -1;
	  ntbx_process_lrank_t                driver_local_rank  =   -1;
	  int                                 adapter_number     =    0;
	  p_tbx_slist_t                       adapter_slist      = NULL;
	  p_tbx_htable_t                      adapter_htable     = NULL;

	  driver_global_rank = mad_ntbx_receive_int(client);
	  if (driver_global_rank == -1)
	    break;
	  
	  driver_local_rank = mad_ntbx_receive_int(client);
	  process = tbx_darray_get(process_darray, driver_global_rank);

	  adapter_number = mad_ntbx_receive_int(client);

	  pi_specific    = mad_dir_driver_process_specific_cons();
	  adapter_slist  = pi_specific->adapter_slist;
	  adapter_htable = pi_specific->adapter_htable;

	  TRACE("Adapters for local process %d:", driver_local_rank);
	  while (adapter_number--)
	    {
	      p_mad_dir_adapter_t adapter = NULL;
	      
	      adapter            = mad_dir_adapter_cons();
	      adapter->name      = mad_ntbx_receive_string(client);
	      adapter->selector  = mad_ntbx_receive_string(client);
	      TRACE_STR("- name", adapter->name);

	      adapter->id = tbx_slist_get_length(adapter_slist);
	      tbx_slist_append(adapter_slist, adapter);
	      tbx_htable_add(adapter_htable, adapter->name, adapter);
	    }

	  ntbx_pc_add(dir_driver->pc, process, driver_local_rank,
		      dir_driver, dir_driver->name, pi_specific);
	}

      TBX_FREE(driver_reference_name);
    }


  command_string = mad_ntbx_receive_string(client);
  
  if (strcmp(command_string, "end{drivers}"))
    FAILURE("synchronisation error");
  
  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}

static
void
mad_dir_channel_get(p_mad_madeleine_t madeleine)
{
  p_mad_session_t    session        = NULL;
  p_mad_directory_t  dir            = NULL;
  p_ntbx_client_t    client         = NULL;
  p_tbx_darray_t     process_darray = NULL;
  p_tbx_htable_t     driver_htable  = NULL;
  p_tbx_htable_t     channel_htable = NULL;
  p_tbx_slist_t      channel_slist  = NULL;
  char              *command_string = NULL;
  int                number         =    0;

  LOG_IN();
  session = madeleine->session;
  dir     = madeleine->dir;
  client  = session->leonie_link;
  
  process_darray = dir->process_darray;
  driver_htable  = dir->driver_htable;
  channel_htable = dir->channel_htable;
  channel_slist  = dir->channel_slist;

  number = mad_ntbx_receive_int(client);

  if (number <= 0)
    FAILURE("invalid number of channels");
  
  TRACE_VAL("Number of channels", number);
  
  while (number--)
    {
      p_mad_dir_channel_t         dir_channel            = NULL;
      char                       *channel_reference_name = NULL;
      char                       *driver_name            = NULL;
      p_ntbx_process_container_t  pc                     = NULL;
      p_ntbx_topology_table_t     ttable                 = NULL;
      ntbx_process_lrank_t        l_rank_src             =   -1;

      dir_channel       = mad_dir_channel_cons();
      pc                = dir_channel->pc;
      dir_channel->name = mad_ntbx_receive_string(client);
      TRACE_STR("Channel name", dir_channel->name);
      
      channel_reference_name =
	TBX_MALLOC(strlen("channel") + strlen(dir_channel->name) + 2);
      CTRL_ALLOC(channel_reference_name);
      sprintf(channel_reference_name, "%s:%s", "channel", dir_channel->name);

      dir_channel->public = mad_ntbx_receive_unsigned_int(client);

      driver_name = mad_ntbx_receive_string(client);      
      dir_channel->driver = tbx_htable_get(driver_htable, driver_name);
      if (!dir_channel->driver)
	FAILURE("driver not found");

      TRACE_STR("Channel driver", dir_channel->driver->name);
      TBX_FREE(driver_name);
      
      tbx_htable_add(channel_htable, dir_channel->name, dir_channel);
      dir_channel->id = tbx_slist_get_length(channel_slist);
      tbx_slist_append(channel_slist, dir_channel);

      TRACE("Adapters:");     
      while (1)
	{
	  p_ntbx_process_t                     process             = NULL;
	  p_mad_dir_channel_process_specific_t cp_specific         = NULL;
	  ntbx_process_grank_t                 channel_global_rank =   -1;
	  ntbx_process_lrank_t                 channel_local_rank  =   -1;

	  channel_global_rank = mad_ntbx_receive_int(client);
	  if (channel_global_rank == -1)
	    break;
	  
	  channel_local_rank = mad_ntbx_receive_int(client);
	  process = tbx_darray_get(process_darray, channel_global_rank);

	  cp_specific = mad_dir_channel_process_specific_cons();
	  cp_specific->adapter_name = mad_ntbx_receive_string(client);
	  TRACE("local process %d: %s", channel_local_rank,
	       cp_specific->adapter_name);
	  
	  ntbx_pc_add(pc, process, channel_local_rank,
		      dir_channel, dir_channel->name, cp_specific);
	}

      ttable =
	ntbx_topology_table_init(pc, ntbx_topology_empty, NULL);

      if (ntbx_pc_first_local_rank(pc, &l_rank_src))
	{
	  do
	    {
	      ntbx_process_lrank_t l_rank_dst = -1;

	      ntbx_pc_first_local_rank(pc, &l_rank_dst);
	      do
		{
		  int value = 0;
		  
		  value = mad_ntbx_receive_int(client);
		  if (value)
		    {
		      ntbx_topology_table_set(ttable,
					      l_rank_src,
					      l_rank_dst);
		      TRACE_CHAR('+');
		    }
		  else
		    {
		      TRACE_CHAR(' ');
		    }
		}
	      while (ntbx_pc_next_local_rank(pc, &l_rank_dst));
	      TRACE_CHAR('\n');
	    }
	  while (ntbx_pc_next_local_rank(pc, &l_rank_src));
	}      

      dir_channel->ttable = ttable;
      TBX_FREE(channel_reference_name);
    }

  command_string = mad_ntbx_receive_string(client);
  
  if (strcmp(command_string, "end{channels}"))
    FAILURE("synchronisation error");
  
  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}

static
void
mad_dir_fchannel_get(p_mad_madeleine_t madeleine)
{
  p_mad_session_t    session         = NULL;
  p_mad_directory_t  dir             = NULL;
  p_ntbx_client_t    client          = NULL;
  p_tbx_darray_t     process_darray  = NULL;
  p_tbx_htable_t     driver_htable   = NULL;
  p_tbx_htable_t     fchannel_htable = NULL;
  p_tbx_slist_t      fchannel_slist  = NULL;
  char              *command_string  = NULL;
  int                number          =    0;

  LOG_IN();
  session = madeleine->session;
  dir     = madeleine->dir;
  client  = session->leonie_link;
  
  process_darray  = dir->process_darray;
  driver_htable   = dir->driver_htable;
  fchannel_htable = dir->fchannel_htable;
  fchannel_slist  = dir->fchannel_slist;

  number = mad_ntbx_receive_int(client);

  if (number < 0)
    FAILURE("invalid number of forwarding channels");
  
  TRACE_VAL("Number of forwarding channels", number);
  
  while (number--)
    {
      p_mad_dir_fchannel_t  dir_fchannel            = NULL;
      char                 *fchannel_reference_name = NULL;

      dir_fchannel = mad_dir_fchannel_cons();
      dir_fchannel->name         = mad_ntbx_receive_string(client);
      dir_fchannel->channel_name = mad_ntbx_receive_string(client);
      TRACE_STR("Forwarding channel name", dir_fchannel->name);
      
      fchannel_reference_name =
	TBX_MALLOC(strlen("fchannel") + strlen(dir_fchannel->name) + 2);
      CTRL_ALLOC(fchannel_reference_name);
      sprintf(fchannel_reference_name,
	      "%s:%s", "fchannel", dir_fchannel->name);

      tbx_htable_add(fchannel_htable, dir_fchannel->name, dir_fchannel);
      dir_fchannel->id = tbx_slist_get_length(fchannel_slist);
      tbx_slist_append(fchannel_slist, dir_fchannel);
      TBX_FREE(fchannel_reference_name);
    }

  command_string = mad_ntbx_receive_string(client);
  
  if (strcmp(command_string, "end{fchannels}"))
    FAILURE("synchronisation error");
  
  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}

static
void
mad_dir_vchannel_get(p_mad_madeleine_t madeleine)
{
  p_mad_session_t    session         = NULL;
  p_mad_directory_t  dir             = NULL;
  p_ntbx_client_t    client          = NULL;
  p_tbx_darray_t     process_darray  = NULL;
  p_tbx_htable_t     driver_htable   = NULL;
  p_tbx_htable_t     channel_htable  = NULL;
  p_tbx_htable_t     fchannel_htable = NULL;
  p_tbx_htable_t     vchannel_htable = NULL;
  p_tbx_slist_t      vchannel_slist  = NULL;
  char              *command_string  = NULL;
  int                number          =    0;

  LOG_IN();
  session = madeleine->session;
  dir     = madeleine->dir;
  client  = session->leonie_link;
  
  process_darray  = dir->process_darray;
  driver_htable   = dir->driver_htable;
  channel_htable  = dir->channel_htable;
  fchannel_htable = dir->fchannel_htable;
  vchannel_htable = dir->vchannel_htable;
  vchannel_slist  = dir->vchannel_slist;

  number = mad_ntbx_receive_int(client);

  if (number < 0)
    FAILURE("invalid number of virtual channels");
  
  TRACE_VAL("Number of virtual channels", number);
  
  while (number--)
    {
      p_mad_dir_vchannel_t  dir_vchannel            = NULL;
      char                 *vchannel_reference_name = NULL;
      p_tbx_slist_t         dir_channel_slist       = NULL;
      int                   dir_channel_slist_len   =    0;
      p_tbx_slist_t         dir_fchannel_slist      = NULL;
      int                   dir_fchannel_slist_len  =    0;
      
      dir_vchannel = mad_dir_vchannel_cons();
      dir_vchannel->name = mad_ntbx_receive_string(client);
      TRACE_STR("Virtual channel name", dir_vchannel->name);
      
      vchannel_reference_name =
	TBX_MALLOC(strlen("vchannel") + strlen(dir_vchannel->name) + 2);
      CTRL_ALLOC(vchannel_reference_name);
      sprintf(vchannel_reference_name,
	      "%s:%s", "vchannel", dir_vchannel->name);

      dir_channel_slist     = dir_vchannel->dir_channel_slist;
      dir_channel_slist_len = mad_ntbx_receive_int(client);

      TRACE("Supporting channels:");
      do 
	{
	  p_mad_dir_channel_t  dir_channel      = NULL;
	  char                *dir_channel_name = NULL;

	  dir_channel_name = mad_ntbx_receive_string(client);
	  dir_channel = tbx_htable_get(channel_htable,
				       dir_channel_name);
	  if (!dir_channel)
	    FAILURE("channel not found");
	  
	  TRACE_STR("- Channel", dir_channel->name);

	  tbx_slist_append(dir_channel_slist, dir_channel);
	  TBX_FREE(dir_channel_name);
	}
      while (--dir_channel_slist_len);

      dir_fchannel_slist     = dir_vchannel->dir_fchannel_slist;
      dir_fchannel_slist_len = mad_ntbx_receive_int(client);

      TRACE("Supporting forwarding channels:");
      do 
	{
	  p_mad_dir_fchannel_t  dir_fchannel      = NULL;
	  char                 *dir_fchannel_name = NULL;
	  
	  dir_fchannel_name = mad_ntbx_receive_string(client);
	  dir_fchannel = tbx_htable_get(fchannel_htable,
				       dir_fchannel_name);
	  if (!dir_fchannel)
	    FAILURE("channel not found");
	  
	  TRACE_STR("- Forwarding channel", dir_fchannel->name);

	  tbx_slist_append(dir_fchannel_slist, dir_fchannel);
	  TBX_FREE(dir_fchannel_name);
	}
      while (--dir_fchannel_slist_len);

      tbx_htable_add(vchannel_htable, dir_vchannel->name, dir_vchannel);
      dir_vchannel->id = tbx_slist_get_length(vchannel_slist);
      tbx_slist_append(vchannel_slist, dir_vchannel);

      TRACE("Virtual channel routing table");
      while (1)
	{
	  p_ntbx_process_t                      process     = NULL;
	  ntbx_process_grank_t                  g_rank_src  =   -1;
	  p_mad_dir_vchannel_process_specific_t pi_specific = NULL;
	  p_ntbx_process_container_t            ppc         = NULL;

	  g_rank_src = mad_ntbx_receive_int(client);
	  if (g_rank_src == -1)
	    break;
	  
	  process = tbx_darray_get(process_darray, g_rank_src);

	  pi_specific = mad_dir_vchannel_process_specific_cons();
	  ppc = pi_specific->pc;

	  while (1)
	    {
	      p_ntbx_process_t                            pprocess   = NULL;
	      ntbx_process_grank_t                        g_rank_dst =   -1;
	      p_mad_dir_vchannel_process_routing_table_t  rtable     = NULL;
	      char                                       *ref_name   = NULL;
	      
	      g_rank_dst = mad_ntbx_receive_int(client);
	      if (g_rank_dst == -1)
		break;

	      pprocess = tbx_darray_get(process_darray, g_rank_dst);
	      
	      rtable = mad_dir_vchannel_process_routing_table_cons();

	      rtable->channel_name     = mad_ntbx_receive_string(client);
	      rtable->destination_rank = mad_ntbx_receive_int(client);
	      TRACE("Process %d to %d: using channel %s through process %d",
		   g_rank_src, g_rank_dst, rtable->channel_name,
		   rtable->destination_rank);

	      {
		size_t len = 0;
		
		len = strlen(dir_vchannel->name) + 8;
		ref_name = malloc(len);
		CTRL_ALLOC(ref_name);

		sprintf(ref_name, "%s:%d", dir_vchannel->name,
			g_rank_src);
	      }

	      ntbx_pc_add(ppc, pprocess, g_rank_dst, process,
			  ref_name, rtable);
	    }
	  
	  ntbx_pc_add(dir_vchannel->pc, process, g_rank_src,
		      dir_vchannel, dir_vchannel->name, pi_specific);
	}

      TBX_FREE(vchannel_reference_name);
    }

  command_string = mad_ntbx_receive_string(client);
  
  if (strcmp(command_string, "end{vchannels}"))
    FAILURE("synchronisation error");
  
  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}

void
mad_dir_driver_init(p_mad_madeleine_t madeleine)
{
  p_mad_session_t      session             = NULL;
  p_mad_directory_t    dir                 = NULL;
  p_ntbx_client_t      client              = NULL;
  p_tbx_htable_t       mad_driver_htable   = NULL;
  p_tbx_htable_t       dir_driver_htable   = NULL;
  ntbx_process_grank_t global_process_rank =   -1;
  mad_driver_id_t      mad_driver_id       =    0;
  
  LOG_IN();
  session           = madeleine->session;
  dir               = madeleine->dir;
  mad_driver_htable = madeleine->driver_htable;

  client              = session->leonie_link;
  global_process_rank = session->process_rank;

  dir_driver_htable = dir->driver_htable;


  TRACE("Driver initialization: first pass");
  while (1)
    {
      p_mad_driver_t                       mad_driver         = NULL;
      p_mad_driver_interface_t             interface          = NULL;
      p_tbx_htable_t                       mad_adapter_htable = NULL;
      p_mad_dir_driver_t                   dir_driver         = NULL;
      p_ntbx_process_container_t           pc                 = NULL;
      p_ntbx_process_info_t                process_info       = NULL;
      p_mad_dir_driver_process_specific_t  pi_specific        = NULL;
      p_tbx_htable_t                       dir_adapter_htable = NULL;
      char                                *driver_name        = NULL;
      mad_adapter_id_t                     mad_adapter_id     =    0;

      driver_name = mad_ntbx_receive_string(client);
      if (!strcmp(driver_name, "-"))
	break;

      dir_driver = tbx_htable_get(dir_driver_htable, driver_name);
      if (!dir_driver)
	FAILURE("driver not found");

      mad_driver = tbx_htable_get(mad_driver_htable, driver_name);
      if (!mad_driver)
	FAILURE("driver not available");      

      TRACE_STR("Initializing driver", driver_name);
      mad_driver->id         = mad_driver_id++;
      mad_driver->dir_driver = dir_driver;
      
      interface = mad_driver->interface;
      interface->driver_init(mad_driver);

      mad_adapter_htable = mad_driver->adapter_htable;

      pc                 = dir_driver->pc;
      process_info       = ntbx_pc_get_global(pc, global_process_rank);
      pi_specific        = process_info->specific;
      dir_adapter_htable = pi_specific->adapter_htable;

      TRACE("Adapter initialization");
      while (1)
	{
	  p_mad_dir_adapter_t  dir_adapter  = NULL;
	  p_mad_adapter_t      mad_adapter  = NULL;
	  char                *adapter_name = NULL;

	  adapter_name = mad_ntbx_receive_string(client);
	  if (!strcmp(adapter_name, "-"))
	    break;

	  dir_adapter =
	    tbx_htable_get(dir_adapter_htable, adapter_name);
	  if (!dir_adapter)
	    FAILURE("adapter not found");

	  TRACE_STR("Initializing adapter", adapter_name);
	  mad_adapter = mad_adapter_cons();

	  mad_adapter->driver         = mad_driver;
	  mad_adapter->id             = mad_adapter_id++;
	  mad_adapter->selector       =
	    tbx_strdup(dir_adapter->selector);
	  mad_adapter->channel_htable = tbx_htable_empty_table();
	  mad_adapter->dir_adapter    = dir_adapter;

	  tbx_htable_add(mad_adapter_htable, dir_adapter->name,
			 mad_adapter);

	  TRACE_STR("Adapter selector", mad_adapter->selector);
	  interface->adapter_init(mad_adapter);
	  TRACE_STR("Adapter connection parameter",
		   mad_adapter->parameter);
	  mad_ntbx_send_string(client, mad_adapter->parameter);
	  if (!mad_adapter->mtu)
	    {
	      mad_adapter->mtu = 1UL << 31;
	    }
	  mad_ntbx_send_unsigned_int(client, mad_adapter->mtu);
	}
    }

  TRACE("Driver initialization: second pass");
  while (1)
    {
      p_mad_dir_driver_t          dir_driver  = NULL;
      p_ntbx_process_container_t  pc          = NULL;
      char                       *driver_name = NULL;

      driver_name = mad_ntbx_receive_string(client);
      if (!strcmp(driver_name, "-"))
	break;

      dir_driver = tbx_htable_get(dir_driver_htable, driver_name);
      if (!dir_driver)
	FAILURE("driver not found");
      pc = dir_driver->pc;

      TRACE_STR("Initializing driver", driver_name);
      while (1)
	{
	  p_ntbx_process_info_t                process_info       = NULL;
	  p_mad_dir_driver_process_specific_t  pi_specific        = NULL;
	  ntbx_process_grank_t                 global_rank        =   -1;
	  p_tbx_htable_t                       adapter_htable     = NULL;

	  global_rank = mad_ntbx_receive_int(client);
	  if (global_rank == -1)
	    break;

	  TRACE_VAL("Process", global_rank);
	  
	  process_info   = ntbx_pc_get_global(pc, global_rank);
	  pi_specific    = process_info->specific;
	  adapter_htable = pi_specific->adapter_htable;

	  while (1)
	    {
	      p_mad_dir_adapter_t  dir_adapter  = NULL;
	      char                *adapter_name = NULL;
	      
	      adapter_name = mad_ntbx_receive_string(client);
	      if (!strcmp(adapter_name, "-"))
		break;

	      dir_adapter = tbx_htable_get(adapter_htable,
					   adapter_name);
	      if (!dir_adapter)
		FAILURE("adapter not found");
	      
	      TRACE_STR("Adapter", adapter_name);	      
	      dir_adapter->parameter = mad_ntbx_receive_string(client);
	      TRACE_STR("- parameter", dir_adapter->parameter);
	      dir_adapter->mtu = mad_ntbx_receive_unsigned_int(client);
	      TRACE_VAL("- mtu", dir_adapter->mtu);
	      if (!dir_adapter->mtu)
		FAILURE("invalid mtu");
	    }
	}
    }

#ifdef MARCEL
  {
    p_mad_driver_t           forwarding_driver = NULL;
    p_mad_driver_interface_t interface         = NULL;
    
    forwarding_driver  =
      tbx_htable_get(madeleine->driver_htable, "forward");

    interface = forwarding_driver->interface;
    interface->driver_init(forwarding_driver);
  }
#endif // MARCEL
  LOG_OUT();
}

static
unsigned int
mad_dir_point_to_point_mtu(p_mad_dir_channel_t  channel,
			   ntbx_process_grank_t src,
			   ntbx_process_grank_t dst)
{
  p_mad_dir_driver_t driver  = NULL;
  unsigned int       src_mtu = 0;
  unsigned int       dst_mtu = 0;
  unsigned int       mtu     = 0;

  LOG_IN();
  driver = channel->driver;

  {
    p_mad_dir_driver_process_specific_t  dps     = NULL;
    p_mad_dir_channel_process_specific_t cps     = NULL;
    p_mad_dir_adapter_t                  adapter = NULL;

    dps     = ntbx_pc_get_global_specific(driver->pc, src);
    cps     = ntbx_pc_get_global_specific(channel->pc, src);
    adapter = tbx_htable_get(dps->adapter_htable, cps->adapter_name);

    src_mtu = adapter->mtu;
    if (!src_mtu)
      FAILURE("invalid mtu");
  }

  {
    p_mad_dir_driver_process_specific_t  dps     = NULL;
    p_mad_dir_channel_process_specific_t cps     = NULL;
    p_mad_dir_adapter_t                  adapter = NULL;

    dps     = ntbx_pc_get_global_specific(driver->pc, dst);
    cps     = ntbx_pc_get_global_specific(channel->pc, dst);
    adapter = tbx_htable_get(dps->adapter_htable, cps->adapter_name);

    dst_mtu = adapter->mtu;
    if (!dst_mtu)
      FAILURE("invalid mtu");
  }

  mtu = min(src_mtu, dst_mtu);
  LOG_OUT();

  return mtu;
}

static
unsigned int
mad_dir_mtu_calculation(p_mad_directory_t           dir,
			p_ntbx_process_container_t  src_pc,
			ntbx_process_grank_t        src,
			ntbx_process_grank_t        dst)
{
  p_mad_dir_vchannel_process_specific_t      vps    = NULL;
  p_ntbx_process_container_t                 dst_pc = NULL;
  p_mad_dir_vchannel_process_routing_table_t rtable = NULL;
  ntbx_process_grank_t                       med    =   -1;
  unsigned int                               mtu    =    0;

  LOG_IN();
  vps    = ntbx_pc_get_global_specific(src_pc, src);
  dst_pc = vps->pc;
  rtable = ntbx_pc_get_global_specific(dst_pc, dst);
  med    = rtable->destination_rank;

  if (med == dst)
    {
      p_mad_dir_channel_t channel = NULL;
      
      channel = tbx_htable_get(dir->channel_htable, rtable->channel_name);

      mtu = mad_dir_point_to_point_mtu(channel, src, dst);
    }
  else
    {
      p_mad_dir_fchannel_t fchannel = NULL;
      p_mad_dir_channel_t  channel  = NULL;
      unsigned int         mtu1     =    0;
      unsigned int         mtu2     =    0;
      
      fchannel = tbx_htable_get(dir->fchannel_htable, rtable->channel_name);
      channel  = tbx_htable_get(dir->channel_htable, fchannel->channel_name);

      mtu1 = mad_dir_point_to_point_mtu(channel, src, med);
      mtu2 = mad_dir_mtu_calculation(dir, src_pc, med, dst);
      mtu  = min(mtu1, mtu2);
    }
  LOG_OUT();

  return mtu;
}

static
p_mad_dir_vchannel_process_routing_table_t
mad_dir_rtable_get(p_ntbx_process_container_t pc,
		   ntbx_process_grank_t       src,
		   ntbx_process_grank_t       dst)
{
  p_mad_dir_vchannel_process_routing_table_t rtable = NULL;
  p_mad_dir_vchannel_process_specific_t      ps     = NULL;
  p_ntbx_process_container_t                 ppc    = NULL;

  LOG_IN();
  ps     = ntbx_pc_get_global_specific(pc, src);
  ppc    = ps->pc;
  rtable = ntbx_pc_get_global_specific(ppc, dst);
  LOG_OUT();
  
  return rtable;
}

static
p_mad_dir_vchannel_process_routing_table_t
mad_dir_reverse_routing(p_ntbx_process_container_t  pc,
			ntbx_process_grank_t       *src,
			ntbx_process_grank_t        dst)
{
  p_mad_dir_vchannel_process_routing_table_t rtable = NULL;

  LOG_IN();
  rtable = mad_dir_rtable_get(pc, *src, dst);

  if (rtable->destination_rank != dst)
    {
      *src   = rtable->destination_rank;
      rtable = mad_dir_reverse_routing(pc, src, dst);
    }
  LOG_OUT();
  
  return rtable;
}

void
mad_dir_channel_init(p_mad_madeleine_t madeleine)
{
  p_mad_session_t      session                  = NULL;
  p_mad_directory_t    dir                      = NULL;
  p_ntbx_client_t      client                   = NULL;
  p_tbx_htable_t       mad_driver_htable        = NULL;
  p_tbx_htable_t       mad_channel_htable       = NULL;
  p_tbx_slist_t        mad_public_channel_slist = NULL;
  p_tbx_htable_t       dir_driver_htable        = NULL;
  p_tbx_htable_t       dir_channel_htable       = NULL;
  p_tbx_htable_t       dir_fchannel_htable      = NULL;
  p_tbx_htable_t       dir_vchannel_htable      = NULL;
  mad_channel_id_t     channel_id               =    0;
  ntbx_process_grank_t process_rank             =   -1;
  ntbx_process_grank_t process_rank_max         =   -1;
#ifdef MARCEL
  p_mad_adapter_t      forwarding_adapter       = NULL;
#endif // MARCEL

  LOG_IN();
  session                  = madeleine->session;
  dir                      = madeleine->dir;
  mad_driver_htable        = madeleine->driver_htable;
  mad_channel_htable       = madeleine->channel_htable;
  mad_public_channel_slist = madeleine->public_channel_slist;
  client                   = session->leonie_link;
  process_rank             = session->process_rank;
  dir_driver_htable        = dir->driver_htable;
  dir_channel_htable       = dir->channel_htable;
  dir_fchannel_htable      = dir->fchannel_htable;
  dir_vchannel_htable      = dir->vchannel_htable;
  process_rank_max         = tbx_darray_length(dir->process_darray);

  // Channels
  
  TRACE("Opening channels");
  while (1)
    {
      p_mad_dir_channel_t       dir_channel  = NULL;
      p_mad_channel_t           mad_channel  = NULL;
      p_mad_adapter_t           mad_adapter  = NULL;
      p_mad_driver_t            mad_driver   = NULL;
      p_mad_dir_driver_t        dir_driver   = NULL;
      p_mad_driver_interface_t  interface    = NULL;
      p_tbx_darray_t            in_darray    = NULL;
      p_tbx_darray_t            out_darray   = NULL;
      char                     *channel_name = NULL;

      channel_name = mad_ntbx_receive_string(client);
      if (!strcmp(channel_name, "-"))
	break;
      
      dir_channel = tbx_htable_get(dir_channel_htable, channel_name);
      TRACE_STR("Channel", channel_name);

      if (!dir_channel)
	FAILURE("channel not found");

      mad_driver = tbx_htable_get(madeleine->driver_htable,
				  dir_channel->driver->name);
      if (!mad_driver)
	FAILURE("driver not found");
      
      dir_driver = mad_driver->dir_driver;
      interface  = mad_driver->interface;

      {
	p_ntbx_process_info_t                pi          = NULL;
	p_mad_dir_channel_process_specific_t cp_specific = NULL;
	
	pi = ntbx_pc_get_global(dir_channel->pc, session->process_rank);
	cp_specific = pi->specific;
	mad_adapter =
	  tbx_htable_get(mad_driver->adapter_htable,
			 cp_specific->adapter_name);
	if (!mad_adapter)
	  FAILURE("adapter not found");
      }
      
      mad_channel                = mad_channel_cons();
      mad_channel->process_lrank =
	ntbx_pc_global_to_local(dir_channel->pc, process_rank);
      mad_channel->type          = mad_channel_type_regular;
      mad_channel->id            = channel_id++;
      mad_channel->name          = dir_channel->name;
      mad_channel->pc            = dir_channel->pc;
      mad_channel->public        = dir_channel->public;
      mad_channel->dir_channel   = dir_channel;
      mad_channel->adapter       = mad_adapter;

      if (interface->channel_init)
	interface->channel_init(mad_channel);
      
      in_darray  = tbx_darray_init();
      out_darray = tbx_darray_init();
      
      while (1)
	{
	  int                  command     = -1;
	  ntbx_process_lrank_t remote_rank = -1;
	  
	  command = mad_ntbx_receive_int(client);
	  if (command == -1)
	    break;

	  remote_rank = mad_ntbx_receive_int(client);	  

	  if (command)
	    {
	      // Output connection
	      p_mad_connection_t out = NULL;

	      TRACE_VAL("Initializing connection to", remote_rank);

	      if (mad_driver->connection_type ==
		  mad_bidirectional_connection)
		{
		  out =
		    tbx_darray_expand_and_get(out_darray,
					      remote_rank);

		  if (!out)
		    {
		      p_mad_connection_t in      = NULL;
		      mad_link_id_t      link_id =   -1;
		      
		      in  = mad_connection_cons();
		      out = mad_connection_cons();

		      in->nature  = mad_connection_nature_regular;
		      out->nature = mad_connection_nature_regular;

		      in->remote_rank  = remote_rank;
		      out->remote_rank = remote_rank;
		      
		      in->channel  = mad_channel;
		      out->channel = mad_channel;

		      in->reverse  = out;
		      out->reverse = in;

		      in->regular  = NULL;
		      out->regular = NULL;

		      in->way  = mad_incoming_connection;
		      out->way = mad_outgoing_connection;
		      
		      if (interface->connection_init)
			{
			  interface->connection_init(in, out);

			  if ((in->nb_link  <= 0)
			      || (out->nb_link <= 0)
			      || (in->nb_link  != out->nb_link))
			    FAILURE("mad_open_channel: invalid link number");
			}
		      else
			{
			  in->nb_link  = 1;
			  out->nb_link = 1;
			}

		      in->link_array  =
			TBX_CALLOC(in->nb_link,
				   sizeof(p_mad_link_t));
		      out->link_array =
			TBX_CALLOC(out->nb_link,
				   sizeof(p_mad_link_t));

		      in->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      in->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      out->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      out->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));      
		      out->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      for (link_id = 0; link_id < in->nb_link; link_id++)
			{
			  p_mad_link_t in_link  = NULL;
			  p_mad_link_t out_link = NULL;
	  
			  in_link  = mad_link_cons();
			  out_link = mad_link_cons();
	  
			  in_link->connection = in;
			  in_link->id         = link_id;

			  out_link->connection = out;
			  out_link->id         = link_id;

			  in->link_array[link_id]  = in_link;
			  out->link_array[link_id] = out_link;

			  if (interface->link_init)
			    {
			      interface->link_init(in_link);
			      interface->link_init(out_link);
			    }

			  in_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  in_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			}

		      tbx_darray_set(out_darray, remote_rank, out);
		      tbx_darray_expand_and_set(in_darray, remote_rank, in);
		    }
		}
	      else
		{
		  mad_link_id_t link_id = -1;

		  out = mad_connection_cons();

		  out->nature      = mad_connection_nature_regular;
		  out->remote_rank = remote_rank;
		  out->channel     = mad_channel;
		  out->reverse     =
		    tbx_darray_expand_and_get(in_darray,
					      remote_rank);
		  if (out->reverse)
		    {
		      out->reverse->reverse = out;
		    }

		  out->regular = NULL;
		  out->way     = mad_outgoing_connection;
		      
		  if (interface->connection_init)
		    {
		      interface->connection_init(NULL, out);

		      if (out->nb_link <= 0)
			FAILURE("mad_open_channel: invalid link number");
		    }
		  else
		    {
		      out->nb_link = 1;
		    }

		  out->link_array =
		    TBX_CALLOC(out->nb_link,
			       sizeof(p_mad_link_t));

		  out->user_buffer_list_reference =
		    TBX_MALLOC(sizeof(tbx_list_reference_t));
		  out->user_buffer_list           =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  out->buffer_list                =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  out->buffer_group_list          =
		    TBX_MALLOC(sizeof(tbx_list_t));      
		  out->pair_list                  =
		    TBX_MALLOC(sizeof(tbx_list_t));

		  for (link_id = 0;
		       link_id < out->nb_link;
		       link_id++)
		    {
		      p_mad_link_t out_link = NULL;
	  
		      out_link = mad_link_cons();
	  
		      out_link->connection = out;
		      out_link->id         = link_id;
		      out->link_array[link_id] = out_link;

		      if (interface->link_init)
			{
			  interface->link_init(out_link);
			}

		      out_link->buffer_list      =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out_link->user_buffer_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		    }

		  tbx_darray_expand_and_set(out_darray, remote_rank,
					    out);
		}
	    }
	  else
	    {
	      // Input connection
	      p_mad_connection_t in = NULL;

	      TRACE_VAL("Initializing connection from", remote_rank);

	      if (mad_driver->connection_type ==
		  mad_bidirectional_connection)
		{
		  in = tbx_darray_expand_and_get(in_darray,
						 remote_rank);

		  if (!in)
		    {
		      p_mad_connection_t out     = NULL;
		      mad_link_id_t      link_id =   -1;

		      in  = mad_connection_cons();
		      out = mad_connection_cons();

		      in->nature  = mad_connection_nature_regular;
		      out->nature = mad_connection_nature_regular;

		      in->remote_rank  = remote_rank;
		      out->remote_rank = remote_rank;
		      
		      in->channel  = mad_channel;
		      out->channel = mad_channel;

		      in->reverse  = out;
		      out->reverse = in;

		      in->regular  = NULL;
		      out->regular = NULL;

		      in->way  = mad_incoming_connection;
		      out->way = mad_outgoing_connection;
		      
		      if (interface->connection_init)
			{
			  interface->connection_init(in, out);

			  if ((in->nb_link  <= 0)
			      || (out->nb_link <= 0)
			      || (in->nb_link  != out->nb_link))
			    FAILURE("mad_open_channel: invalid link number");
			}
		      else
			{
			  in->nb_link  = 1;
			  out->nb_link = 1;
			}

		      in->link_array  =
			TBX_CALLOC(in->nb_link,
				   sizeof(p_mad_link_t));
		      out->link_array =
			TBX_CALLOC(out->nb_link,
				   sizeof(p_mad_link_t));

		      in->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      in->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      out->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      out->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));      
		      out->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      for (link_id = 0; link_id < in->nb_link; link_id++)
			{
			  p_mad_link_t in_link  = NULL;
			  p_mad_link_t out_link = NULL;
	  
			  in_link  = mad_link_cons();
			  out_link = mad_link_cons();
	  
			  in_link->connection = in;
			  in_link->id         = link_id;

			  out_link->connection = out;
			  out_link->id         = link_id;

			  in->link_array[link_id]  = in_link;
			  out->link_array[link_id] = out_link;

			  if (interface->link_init)
			    {
			      interface->link_init(in_link);
			      interface->link_init(out_link);
			    }

			  in_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  in_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			}

		      tbx_darray_set(in_darray, remote_rank, in);
		      tbx_darray_expand_and_set(out_darray, remote_rank, out);
		    }
		}
	      else
		{
		  mad_link_id_t link_id = -1;

		  in = mad_connection_cons();
		      
		  in->nature      = mad_connection_nature_regular;
		  in->remote_rank = remote_rank;
		  in->channel     = mad_channel;
		  in->reverse     =
		    tbx_darray_expand_and_get(out_darray,
					      remote_rank);
		  if (in->reverse)
		    {
		      in->reverse->reverse = in;
		    }
		      
		  in->regular = NULL;
		  in->way     = mad_incoming_connection;
		      
		  if (interface->connection_init)
		    {
		      interface->connection_init(in, NULL);

		      if (in->nb_link <= 0)
			FAILURE("mad_open_channel: invalid link number");
		    }
		  else
		    {
		      in->nb_link = 1;
		    }

		  in->link_array  =
		    TBX_CALLOC(in->nb_link,
			       sizeof(p_mad_link_t));

		  in->user_buffer_list_reference =
		    TBX_MALLOC(sizeof(tbx_list_reference_t));
		  in->user_buffer_list           =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  in->buffer_list                =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  in->buffer_group_list          =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  in->pair_list                  =
		    TBX_MALLOC(sizeof(tbx_list_t));

		  for (link_id = 0; link_id < in->nb_link; link_id++)
		    {
		      p_mad_link_t in_link  = NULL;
	  
		      in_link = mad_link_cons();
	  
		      in_link->connection = in;
		      in_link->id         = link_id;
		      in->link_array[link_id] = in_link;

		      if (interface->link_init)
			{
			  interface->link_init(in_link);
			}

		      in_link->buffer_list      =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in_link->user_buffer_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		    }

		  tbx_darray_expand_and_set(in_darray, remote_rank,
					    in);
		}
	    }

	  mad_ntbx_send_int(client, -1);
	}
      
      mad_ntbx_send_int(client, -1);

      mad_channel->in_connection_darray  = in_darray;
      mad_channel->out_connection_darray = out_darray;
      
      if (interface->before_open_channel)
	interface->before_open_channel(mad_channel);

      while (1)
	{
	  int                  command     = -1;
	  ntbx_process_lrank_t remote_rank = -1;
	  mad_adapter_info_t   remote_adapter_info;
	  
	  command = mad_ntbx_receive_int(client);
	  if (command == -1)
	    break;

	  remote_rank = mad_ntbx_receive_int(client);
	  if (!interface->accept || !interface->connect)
	    goto channel_connection_established;  

	  {
	    p_ntbx_process_info_t                remote_pi      = NULL;
	    p_ntbx_process_t                     remote_process = NULL;
	    p_mad_dir_channel_process_specific_t cp_specific    = NULL;
	    p_mad_dir_driver_process_specific_t  dp_specific    = NULL;

	    remote_pi      = ntbx_pc_get_local(dir_channel->pc, remote_rank);
	    remote_process = remote_pi->process;
	    cp_specific    = remote_pi->specific;

	    remote_adapter_info.dir_node = tbx_htable_get(remote_process->ref,
							  "node");
	    
	    dp_specific =
	      ntbx_pc_get_global_specific(dir_driver->pc,
					  remote_process->global_rank);
	    
	    remote_adapter_info.dir_adapter = 
	      tbx_htable_get(dp_specific->adapter_htable,
			     cp_specific->adapter_name);
	    
	    if (!remote_adapter_info.dir_adapter)
	      FAILURE("adapter not found");
	  }

	  if (command)
	    {
	      // Output connection
	      p_mad_connection_t out = NULL;

	      TRACE_VAL("Connection to", remote_rank);
	      out = tbx_darray_get(out_darray, remote_rank);

	      if (mad_driver->connection_type == mad_bidirectional_connection)
		{
		  if (out->connected)
		    goto channel_connection_established;

		  interface->connect(out, &remote_adapter_info);
		  out->reverse->connected = tbx_true;
		}
	      else
		{
		  interface->connect(out, &remote_adapter_info);
		}

	      out->connected = tbx_true;
	    }
	  else
	    {
	      // Input connection
	      p_mad_connection_t in = NULL;

	      TRACE_VAL("Accepting connection from", remote_rank);
	      in = tbx_darray_expand_and_get(in_darray, remote_rank);

	      if (mad_driver->connection_type == mad_bidirectional_connection)
		{
		  if (in->connected)
		    goto channel_connection_established;

		  interface->accept(in, &remote_adapter_info);
		  in->reverse->connected = tbx_true;
		}
	      else
		{
		  interface->accept(in, &remote_adapter_info);
		}

	      in->connected = tbx_true;
	    }

	channel_connection_established:
	  mad_ntbx_send_int(client, -1);
	}
      
      if (interface->after_open_channel)
	interface->after_open_channel(mad_channel);

      tbx_htable_add(mad_adapter->channel_htable,
		     dir_channel->name, mad_channel);
      
      tbx_htable_add(madeleine->channel_htable,
		     dir_channel->name, mad_channel);      

      if (mad_channel->public)
	{
	  tbx_slist_append(mad_public_channel_slist, mad_channel->name);
	}
	  
      mad_ntbx_send_int(client, -1);
    }
  
  // Forwarding channels

  TRACE("Opening forwarding channels");
  while (1)
    {
      p_mad_dir_fchannel_t      dir_fchannel = NULL;
      p_mad_dir_channel_t       dir_channel  = NULL;
      p_mad_channel_t           mad_channel  = NULL;
      p_mad_adapter_t           mad_adapter  = NULL;
      p_mad_driver_t            mad_driver   = NULL;
      p_mad_dir_driver_t        dir_driver   = NULL;
      p_mad_driver_interface_t  interface    = NULL;
      p_tbx_darray_t            in_darray    = NULL;
      p_tbx_darray_t            out_darray   = NULL;
      char                     *channel_name = NULL;

      channel_name = mad_ntbx_receive_string(client);
      if (!strcmp(channel_name, "-"))
	break;
      
      dir_fchannel = tbx_htable_get(dir_fchannel_htable, channel_name);
      TRACE_STR("Channel", channel_name);

      if (!dir_fchannel)
	FAILURE("channel not found");

      dir_channel = tbx_htable_get(dir_channel_htable,
				   dir_fchannel->channel_name);
      mad_driver = tbx_htable_get(madeleine->driver_htable,
				  dir_channel->driver->name);
      if (!mad_driver)
	FAILURE("driver not found");
      
      dir_driver = mad_driver->dir_driver;
      interface  = mad_driver->interface;

      {
	p_ntbx_process_info_t                pi          = NULL;
	p_mad_dir_channel_process_specific_t cp_specific = NULL;
	
	pi = ntbx_pc_get_global(dir_channel->pc, session->process_rank);
	cp_specific = pi->specific;
	mad_adapter =
	  tbx_htable_get(mad_driver->adapter_htable,
			 cp_specific->adapter_name);
	if (!mad_adapter)
	  FAILURE("adapter not found");
      }
      
      mad_channel                = mad_channel_cons();
      mad_channel->process_lrank =
	ntbx_pc_global_to_local(dir_channel->pc, process_rank);
      mad_channel->type          = mad_channel_type_forwarding;
      mad_channel->id            = channel_id++;
      mad_channel->name          = dir_fchannel->name;
      mad_channel->pc            = dir_channel->pc;
      mad_channel->public        = tbx_false;
      mad_channel->dir_channel   = dir_channel;
      mad_channel->dir_fchannel  = dir_fchannel;
      mad_channel->adapter       = mad_adapter;

      if (interface->channel_init)
	interface->channel_init(mad_channel);
      
      in_darray  = tbx_darray_init();
      out_darray = tbx_darray_init();
      
      while (1)
	{
	  int                  command     = -1;
	  ntbx_process_lrank_t remote_rank = -1;
	  
	  command = mad_ntbx_receive_int(client);
	  if (command == -1)
	    break;

	  remote_rank = mad_ntbx_receive_int(client);	  

	  if (command)
	    {
	      // Output connection
	      p_mad_connection_t out = NULL;

	      TRACE_VAL("Initializing connection to", remote_rank);

	      if (mad_driver->connection_type ==
		  mad_bidirectional_connection)
		{
		  out =
		    tbx_darray_expand_and_get(out_darray,
					      remote_rank);

		  if (!out)
		    {
		      p_mad_connection_t in      = NULL;
		      mad_link_id_t      link_id =   -1;
		      
		      in  = mad_connection_cons();
		      out = mad_connection_cons();
		      
		      in->nature  = mad_connection_nature_regular;
		      out->nature = mad_connection_nature_regular;

		      in->remote_rank  = remote_rank;
		      out->remote_rank = remote_rank;
		      
		      in->channel  = mad_channel;
		      out->channel = mad_channel;

		      in->reverse  = out;
		      out->reverse = in;

		      in->way  = mad_incoming_connection;
		      out->way = mad_outgoing_connection;
		      
		      if (interface->connection_init)
			{
			  interface->connection_init(in, out);

			  if ((in->nb_link  <= 0)
			      || (out->nb_link <= 0)
			      || (in->nb_link  != out->nb_link))
			    FAILURE("mad_open_channel: invalid link number");
			}
		      else
			{
			  in->nb_link  = 1;
			  out->nb_link = 1;
			}

		      in->link_array  =
			TBX_CALLOC(in->nb_link,
				   sizeof(p_mad_link_t));
		      out->link_array =
			TBX_CALLOC(out->nb_link,
				   sizeof(p_mad_link_t));

		      in->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      in->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      out->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      out->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));      
		      out->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      for (link_id = 0; link_id < in->nb_link; link_id++)
			{
			  p_mad_link_t in_link  = NULL;
			  p_mad_link_t out_link = NULL;
	  
			  in_link  = mad_link_cons();
			  out_link = mad_link_cons();
	  
			  in_link->connection = in;
			  in_link->id         = link_id;

			  out_link->connection = out;
			  out_link->id         = link_id;

			  in->link_array[link_id]  = in_link;
			  out->link_array[link_id] = out_link;

			  if (interface->link_init)
			    {
			      interface->link_init(in_link);
			      interface->link_init(out_link);
			    }

			  in_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  in_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			}

		      tbx_darray_set(out_darray, remote_rank, out);
		      tbx_darray_expand_and_set(in_darray, remote_rank, in);
		    }
		}
	      else
		{
		  mad_link_id_t link_id = -1;

		  out = mad_connection_cons();
		      
		  out->nature      = mad_connection_nature_regular;
		  out->remote_rank = remote_rank;
		  out->channel     = mad_channel;
		  out->reverse     =
		    tbx_darray_expand_and_get(in_darray,
					      remote_rank);
		  if (out->reverse)
		    {
		      out->reverse->reverse = out;
		    }

		  out->way = mad_outgoing_connection;
		      
		  if (interface->connection_init)
		    {
		      interface->connection_init(NULL, out);

		      if (out->nb_link <= 0)
			FAILURE("mad_open_channel: invalid link number");
		    }
		  else
		    {
		      out->nb_link = 1;
		    }

		  out->link_array =
		    TBX_CALLOC(out->nb_link,
			       sizeof(p_mad_link_t));

		  out->user_buffer_list_reference =
		    TBX_MALLOC(sizeof(tbx_list_reference_t));
		  out->user_buffer_list           =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  out->buffer_list                =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  out->buffer_group_list          =
		    TBX_MALLOC(sizeof(tbx_list_t));      
		  out->pair_list                  =
		    TBX_MALLOC(sizeof(tbx_list_t));

		  for (link_id = 0;
		       link_id < out->nb_link;
		       link_id++)
		    {
		      p_mad_link_t out_link = NULL;
	  
		      out_link = mad_link_cons();
	  
		      out_link->connection = out;
		      out_link->id         = link_id;
		      out->link_array[link_id] = out_link;

		      if (interface->link_init)
			{
			  interface->link_init(out_link);
			}

		      out_link->buffer_list      =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out_link->user_buffer_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		    }

		  tbx_darray_expand_and_set(out_darray, remote_rank,
					    out);
		}
	    }
	  else
	    {
	      // Input connection
	      p_mad_connection_t in = NULL;

	      TRACE_VAL("Initializing connection from", remote_rank);

	      if (mad_driver->connection_type ==
		  mad_bidirectional_connection)
		{
		  in = tbx_darray_expand_and_get(in_darray,
						 remote_rank);

		  if (!in)
		    {
		      p_mad_connection_t out     = NULL;
		      mad_link_id_t      link_id =   -1;

		      in  = mad_connection_cons();
		      out = mad_connection_cons();

		      in->nature  = mad_connection_nature_regular;
		      out->nature = mad_connection_nature_regular;

		      in->remote_rank  = remote_rank;
		      out->remote_rank = remote_rank;
		      
		      in->channel  = mad_channel;
		      out->channel = mad_channel;

		      in->reverse  = out;
		      out->reverse = in;

		      in->way  = mad_incoming_connection;
		      out->way = mad_outgoing_connection;
		      
		      if (interface->connection_init)
			{
			  interface->connection_init(in, out);

			  if ((in->nb_link  <= 0)
			      || (out->nb_link <= 0)
			      || (in->nb_link  != out->nb_link))
			    FAILURE("mad_open_channel: invalid link number");
			}
		      else
			{
			  in->nb_link  = 1;
			  out->nb_link = 1;
			}

		      in->link_array  =
			TBX_CALLOC(in->nb_link,
				   sizeof(p_mad_link_t));
		      out->link_array =
			TBX_CALLOC(out->nb_link,
				   sizeof(p_mad_link_t));

		      in->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      in->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      out->user_buffer_list_reference =
			TBX_MALLOC(sizeof(tbx_list_reference_t));
		      out->user_buffer_list  =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_list       =
			TBX_MALLOC(sizeof(tbx_list_t));
		      out->buffer_group_list =
			TBX_MALLOC(sizeof(tbx_list_t));      
		      out->pair_list         =
			TBX_MALLOC(sizeof(tbx_list_t));

		      for (link_id = 0; link_id < in->nb_link; link_id++)
			{
			  p_mad_link_t in_link  = NULL;
			  p_mad_link_t out_link = NULL;
	  
			  in_link  = mad_link_cons();
			  out_link = mad_link_cons();
	  
			  in_link->connection = in;
			  in_link->id         = link_id;

			  out_link->connection = out;
			  out_link->id         = link_id;

			  in->link_array[link_id]  = in_link;
			  out->link_array[link_id] = out_link;

			  if (interface->link_init)
			    {
			      interface->link_init(in_link);
			      interface->link_init(out_link);
			    }

			  in_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  in_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->buffer_list      =
			    TBX_MALLOC(sizeof(tbx_list_t));
			  out_link->user_buffer_list =
			    TBX_MALLOC(sizeof(tbx_list_t));
			}

		      tbx_darray_set(in_darray, remote_rank, in);
		      tbx_darray_expand_and_set(out_darray, remote_rank, out);
		    }
		}
	      else
		{
		  mad_link_id_t link_id = -1;

		  in = mad_connection_cons();
		      
		  in->nature      = mad_connection_nature_regular;
		  in->remote_rank = remote_rank;
		  in->channel     = mad_channel;
		  in->reverse     =
		    tbx_darray_expand_and_get(out_darray,
					      remote_rank);
		  if (in->reverse)
		    {
		      in->reverse->reverse = in;
		    }
		      
		  in->way = mad_incoming_connection;
		      
		  if (interface->connection_init)
		    {
		      interface->connection_init(in, NULL);

		      if (in->nb_link <= 0)
			FAILURE("mad_open_channel: invalid link number");
		    }
		  else
		    {
		      in->nb_link = 1;
		    }

		  in->link_array  =
		    TBX_CALLOC(in->nb_link,
			       sizeof(p_mad_link_t));

		  in->user_buffer_list_reference =
		    TBX_MALLOC(sizeof(tbx_list_reference_t));
		  in->user_buffer_list           =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  in->buffer_list                =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  in->buffer_group_list          =
		    TBX_MALLOC(sizeof(tbx_list_t));
		  in->pair_list                  =
		    TBX_MALLOC(sizeof(tbx_list_t));

		  for (link_id = 0; link_id < in->nb_link; link_id++)
		    {
		      p_mad_link_t in_link  = NULL;
	  
		      in_link = mad_link_cons();
	  
		      in_link->connection = in;
		      in_link->id         = link_id;
		      in->link_array[link_id] = in_link;

		      if (interface->link_init)
			{
			  interface->link_init(in_link);
			}

		      in_link->buffer_list      =
			TBX_MALLOC(sizeof(tbx_list_t));
		      in_link->user_buffer_list =
			TBX_MALLOC(sizeof(tbx_list_t));
		    }

		  tbx_darray_expand_and_set(in_darray, remote_rank,
					    in);
		}
	    }

	  mad_ntbx_send_int(client, -1);
	}
      
      mad_ntbx_send_int(client, -1);

      mad_channel->in_connection_darray  = in_darray;
      mad_channel->out_connection_darray = out_darray;
      
      if (interface->before_open_channel)
	interface->before_open_channel(mad_channel);

      while (1)
	{
	  int                  command     = -1;
	  ntbx_process_lrank_t remote_rank = -1;
	  mad_adapter_info_t   remote_adapter_info;
	  
	  command = mad_ntbx_receive_int(client);
	  if (command == -1)
	    break;

	  remote_rank = mad_ntbx_receive_int(client);
	  if (!interface->accept || !interface->connect)
	    goto fchannel_connection_established;  

	  {
	    p_ntbx_process_info_t                remote_pi      = NULL;
	    p_ntbx_process_t                     remote_process = NULL;
	    p_mad_dir_channel_process_specific_t cp_specific    = NULL;
	    p_mad_dir_driver_process_specific_t  dp_specific    = NULL;

	    remote_pi      = ntbx_pc_get_local(dir_channel->pc, remote_rank);
	    remote_process = remote_pi->process;
	    cp_specific    = remote_pi->specific;

	    remote_adapter_info.dir_node = tbx_htable_get(remote_process->ref,
							  "node");
	    
	    dp_specific =
	      ntbx_pc_get_global_specific(dir_driver->pc,
					  remote_process->global_rank);
	    
	    remote_adapter_info.dir_adapter = 
	      tbx_htable_get(dp_specific->adapter_htable,
			     cp_specific->adapter_name);
	    
	    if (!remote_adapter_info.dir_adapter)
	      FAILURE("adapter not found");
	  }

	  if (command)
	    {
	      // Output connection
	      p_mad_connection_t out = NULL;

	      TRACE_VAL("Connection to", remote_rank);
	      out = tbx_darray_get(out_darray, remote_rank);

	      if (mad_driver->connection_type == mad_bidirectional_connection)
		{
		  if (out->connected)
		    goto fchannel_connection_established;

		  interface->connect(out, &remote_adapter_info);
		  out->reverse->connected = tbx_true;
		}
	      else
		{
		  interface->connect(out, &remote_adapter_info);
		}

	      out->connected = tbx_true;
	    }
	  else
	    {
	      // Input connection
	      p_mad_connection_t in = NULL;

	      TRACE_VAL("Accepting connection from", remote_rank);
	      in = tbx_darray_expand_and_get(in_darray, remote_rank);

	      if (mad_driver->connection_type == mad_bidirectional_connection)
		{
		  if (in->connected)
		    goto fchannel_connection_established;

		  interface->accept(in, &remote_adapter_info);
		  in->reverse->connected = tbx_true;
		}
	      else
		{
		  interface->accept(in, &remote_adapter_info);
		}

	      in->connected = tbx_true;
	    }

	fchannel_connection_established:
	  mad_ntbx_send_int(client, -1);
	}
      
      if (interface->after_open_channel)
	interface->after_open_channel(mad_channel);

      tbx_htable_add(mad_adapter->channel_htable,
		     dir_fchannel->name, mad_channel);
      
      tbx_htable_add(madeleine->channel_htable,
		     dir_fchannel->name, mad_channel);      

      mad_ntbx_send_int(client, -1);
    }

  // Virtual channels
  TRACE("Opening virtual channels");
#ifdef MARCEL
  {
    p_mad_driver_t forwarding_driver = NULL;
    
    forwarding_driver  =
      tbx_htable_get(madeleine->driver_htable, "forward");
    forwarding_adapter =
      tbx_htable_get(forwarding_driver->adapter_htable, "forward");
  }
#endif // MARCEL
  
  while (1)
    {
      char                                  *vchannel_name = NULL;
#ifdef MARCEL
      p_mad_dir_vchannel_t                   dir_vchannel  = NULL;
      p_ntbx_process_container_t             vchannel_pc   = NULL;
      p_mad_dir_vchannel_process_specific_t  vchannel_ps   = NULL;
      p_ntbx_process_container_t             vchannel_ppc  = NULL;
      p_mad_channel_t                        mad_channel   = NULL;
      ntbx_process_lrank_t                   process_lrank =   -1;
      ntbx_process_grank_t                   g_rank_dst    =   -1;
      p_tbx_darray_t                         in_darray     = NULL;
      p_tbx_darray_t                         out_darray    = NULL;
      p_mad_driver_interface_t               interface     = NULL;
#endif // MARCEL

      vchannel_name = mad_ntbx_receive_string(client);
      if (!strcmp(vchannel_name, "-"))
	break;

#ifdef MARCEL      
      dir_vchannel =
	tbx_htable_get(dir_vchannel_htable, vchannel_name);
      TRACE_STR("Virtual channel", vchannel_name);

      if (!dir_vchannel)
	FAILURE("virtual channel not found");

      vchannel_pc                 = dir_vchannel->pc;
      mad_channel                 = mad_channel_cons();
      mad_channel->process_lrank  =
	ntbx_pc_global_to_local(vchannel_pc, process_rank);
      mad_channel->type           = mad_channel_type_virtual;
      mad_channel->id             = channel_id++;
      mad_channel->name           = dir_vchannel->name;
      mad_channel->pc             = dir_vchannel->pc;
      mad_channel->public         = tbx_true;
      mad_channel->dir_vchannel   = dir_vchannel;
      mad_channel->adapter        = forwarding_adapter;

      mad_channel->channel_slist  = tbx_slist_nil();
      {	
	tbx_slist_ref_to_head(dir_vchannel->dir_channel_slist);
	do
	  {
	    p_mad_dir_channel_t dir_channel     = NULL;
	    p_mad_channel_t     regular_channel = NULL;

	    dir_channel = tbx_slist_ref_get(dir_vchannel->dir_channel_slist);
	    regular_channel = tbx_htable_get(madeleine->channel_htable,
					     dir_channel->name);
	    if (regular_channel)
	      {
		tbx_slist_append(mad_channel->channel_slist, regular_channel);
	      }
	  }
	while (tbx_slist_ref_forward(dir_vchannel->dir_channel_slist));
      }

      mad_channel->fchannel_slist = tbx_slist_nil();
      {	
	tbx_slist_ref_to_head(dir_vchannel->dir_fchannel_slist);
	do
	  {
	    p_mad_dir_fchannel_t dir_fchannel     = NULL;
	    p_mad_channel_t      regular_channel = NULL;

	    dir_fchannel = tbx_slist_ref_get(dir_vchannel->dir_fchannel_slist);
	    regular_channel = tbx_htable_get(madeleine->channel_htable,
					     dir_fchannel->name);
	    if (regular_channel)
	      {
		tbx_slist_append(mad_channel->fchannel_slist, regular_channel);
	      }
	  }
	while (tbx_slist_ref_forward(dir_vchannel->dir_fchannel_slist));
      }      
      interface = forwarding_adapter->driver->interface;

      if (interface->channel_init)
	interface->channel_init(mad_channel);
      
      in_darray  = tbx_darray_init();
      out_darray = tbx_darray_init();      

      process_lrank = ntbx_pc_global_to_local(vchannel_pc, process_rank);

      // virtual connections construction
      vchannel_ps  = ntbx_pc_get_global_specific(vchannel_pc, process_rank);
      vchannel_ppc = vchannel_ps->pc;
      
      ntbx_pc_first_global_rank(vchannel_ppc, &g_rank_dst);
      do
	{
	  p_mad_dir_vchannel_process_routing_table_t rtable = NULL;
	  p_mad_connection_t   in         = NULL;
	  p_mad_connection_t   out        = NULL;
	  mad_link_id_t        link_id    =   -1;
	  ntbx_process_lrank_t l_rank_dst =   -1;
	  
	  l_rank_dst = ntbx_pc_global_to_local(vchannel_ppc,
					       g_rank_dst);

	  in  = mad_connection_cons();
	  out = mad_connection_cons();

	  in->remote_rank  = l_rank_dst;
	  out->remote_rank = l_rank_dst;
	  
	  in->channel  = mad_channel;
	  out->channel = mad_channel;

	  in->reverse  = out;
	  out->reverse = in;

	  in->way  = mad_incoming_connection;
	  out->way = mad_outgoing_connection;

	  // in->regular
	  rtable = mad_dir_rtable_get(vchannel_pc,
				      g_rank_dst,
				      process_rank);
	  if (rtable->destination_rank == process_rank)
	    {
	      // direct
	      p_mad_channel_t            regular_channel = NULL;
	      p_mad_dir_channel_t        dir_channel     = NULL;
	      p_ntbx_process_container_t rchannel_pc     = NULL;
	      ntbx_process_lrank_t       l_rank_rt       =   -1;

	      in->nature      = mad_connection_nature_direct_virtual;
	      regular_channel =
		tbx_htable_get(madeleine->channel_htable,
			       rtable->channel_name);
	      dir_channel = regular_channel->dir_channel;
	      rchannel_pc = dir_channel->pc;
	      l_rank_rt   =
		ntbx_pc_global_to_local(rchannel_pc, g_rank_dst);

	      in->regular = tbx_darray_get(regular_channel->
					   in_connection_darray,
					   l_rank_rt);
	      if (!in->regular)
		FAILURE("invalid connection");
	    }
	  else
	    {
	      // indirect
	      p_mad_channel_t            regular_channel = NULL;
	      p_mad_dir_channel_t        dir_channel     = NULL;
	      p_ntbx_process_container_t rchannel_pc     = NULL;
	      ntbx_process_grank_t       g_rank_rt       =   -1;
	      ntbx_process_lrank_t       l_rank_rt       =   -1;

	      in->nature      =
		mad_connection_nature_indirect_virtual;
	      g_rank_rt = rtable->destination_rank;
	      // g_rank_rt = g_rank_dst;
	      rtable    = mad_dir_reverse_routing(vchannel_pc,
						  &g_rank_rt,
						  process_rank);
	      regular_channel =
		tbx_htable_get(madeleine->channel_htable,
			       rtable->channel_name);
	      dir_channel = regular_channel->dir_channel;
	      rchannel_pc = dir_channel->pc;
	      l_rank_rt   =
		ntbx_pc_global_to_local(rchannel_pc, g_rank_rt);
	      in->regular = tbx_darray_get(regular_channel->
					   in_connection_darray,
					   l_rank_rt);
	      if (!in->regular)
		FAILURE("invalid connection");	      
	    }

	  // out->regular
	  out->mtu = mad_dir_mtu_calculation(dir, vchannel_pc, process_rank,
					     g_rank_dst);
	  if (!out->mtu)
	    FAILURE("invalid MTU");

	  rtable = ntbx_pc_get_global_specific(vchannel_ppc, g_rank_dst);
	  
	  if (rtable->destination_rank == g_rank_dst)
	    {
	      p_mad_channel_t            regular_channel = NULL;
	      p_mad_dir_channel_t        dir_channel     = NULL;
	      p_ntbx_process_container_t rchannel_pc     = NULL;
	      ntbx_process_lrank_t       l_rank_rt       =   -1;

	      out->nature = mad_connection_nature_direct_virtual;

	      regular_channel =
		tbx_htable_get(madeleine->channel_htable,
			       rtable->channel_name);
	      dir_channel = regular_channel->dir_channel;
	      rchannel_pc = dir_channel->pc;
	      l_rank_rt   =
		ntbx_pc_global_to_local(rchannel_pc,
					rtable->destination_rank);

	      out->regular = tbx_darray_get(regular_channel->
					    out_connection_darray,
					    l_rank_rt);
	    }
	  else
	    {
	      p_mad_channel_t            forward_channel = NULL;
	      p_mad_dir_channel_t        dir_channel     = NULL;
	      p_mad_dir_fchannel_t       dir_fchannel    = NULL;
	      p_ntbx_process_container_t rchannel_pc     = NULL;
	      ntbx_process_lrank_t       l_rank_rt       =   -1;

	      out->nature = mad_connection_nature_indirect_virtual;

	      forward_channel =
		tbx_htable_get(madeleine->channel_htable,
			       rtable->channel_name);
	      dir_fchannel = forward_channel->dir_fchannel;
	      dir_channel  = forward_channel->dir_channel;
	      rchannel_pc  = dir_channel->pc;
	      l_rank_rt    =
		ntbx_pc_global_to_local(rchannel_pc,
					rtable->destination_rank);

	      out->regular = tbx_darray_get(forward_channel->
					    out_connection_darray,
					    l_rank_rt);
	    }

	  if (interface->connection_init)
	    {
	      interface->connection_init(in, out);

	      if ((in->nb_link  <= 0)
		  || (out->nb_link <= 0)
		  || (in->nb_link  != out->nb_link))
		FAILURE("mad_open_channel: invalid link number");
	    }
	  else
	    {
	      in->nb_link  = 1;
	      out->nb_link = 1;
	    }

	  in->link_array  =
	    TBX_CALLOC(in->nb_link,
		       sizeof(p_mad_link_t));
	  out->link_array =
	    TBX_CALLOC(out->nb_link,
		       sizeof(p_mad_link_t));

	  in->user_buffer_list_reference =
	    TBX_MALLOC(sizeof(tbx_list_reference_t));
	  in->user_buffer_list  =
	    TBX_MALLOC(sizeof(tbx_list_t));
	  in->buffer_list       =
	    TBX_MALLOC(sizeof(tbx_list_t));
	  in->buffer_group_list =
	    TBX_MALLOC(sizeof(tbx_list_t));
	  in->pair_list         =
	    TBX_MALLOC(sizeof(tbx_list_t));

	  out->user_buffer_list_reference =
	    TBX_MALLOC(sizeof(tbx_list_reference_t));
	  out->user_buffer_list  =
	    TBX_MALLOC(sizeof(tbx_list_t));
	  out->buffer_list       =
	    TBX_MALLOC(sizeof(tbx_list_t));
	  out->buffer_group_list =
	    TBX_MALLOC(sizeof(tbx_list_t));      
	  out->pair_list         =
	    TBX_MALLOC(sizeof(tbx_list_t));

	  for (link_id = 0; link_id < in->nb_link; link_id++)
	    {
	      p_mad_link_t in_link  = NULL;
	      p_mad_link_t out_link = NULL;
	  
	      in_link  = mad_link_cons();
	      out_link = mad_link_cons();
	  
	      in_link->connection = in;
	      in_link->id         = link_id;

	      out_link->connection = out;
	      out_link->id         = link_id;

	      in->link_array[link_id]  = in_link;
	      out->link_array[link_id] = out_link;

	      if (interface->link_init)
		{
		  interface->link_init(in_link);
		  interface->link_init(out_link);
		}

	      in_link->buffer_list      =
		TBX_MALLOC(sizeof(tbx_list_t));
	      in_link->user_buffer_list =
		TBX_MALLOC(sizeof(tbx_list_t));
	      out_link->buffer_list      =
		TBX_MALLOC(sizeof(tbx_list_t));
	      out_link->user_buffer_list =
		TBX_MALLOC(sizeof(tbx_list_t));
	    }

	  tbx_darray_expand_and_set(in_darray, l_rank_dst, in);
	  tbx_darray_expand_and_set(out_darray, l_rank_dst, out);
	}
      while (ntbx_pc_next_global_rank(vchannel_ppc, &g_rank_dst));

      mad_channel->in_connection_darray  = in_darray;
      mad_channel->out_connection_darray = out_darray;

      if (interface->before_open_channel)
	interface->before_open_channel(mad_channel);

      if (interface->after_open_channel)
	interface->after_open_channel(mad_channel);

      tbx_htable_add(forwarding_adapter->channel_htable,
		     dir_vchannel->name, mad_channel);

      tbx_htable_add(madeleine->channel_htable,
		     dir_vchannel->name, mad_channel);

      tbx_slist_append(mad_public_channel_slist, mad_channel->name);
#endif // MARCEL

      // Virtual channel ready
      mad_ntbx_send_string(client, "ok");
    }

  LOG_OUT();
}

void
mad_dir_vchannel_exit(p_mad_madeleine_t madeleine)
{
  p_mad_session_t   session            = NULL;
  p_ntbx_client_t   client             = NULL;
  p_mad_directory_t dir                = NULL;
#ifdef MARCEL
  p_tbx_slist_t     dir_vchannel_slist = NULL;
#endif // MARCEL
  p_tbx_htable_t    channel_htable     = NULL;

  LOG_IN();
  session        = madeleine->session;
  client         = session->leonie_link;
  dir            = madeleine->dir;
  channel_htable = madeleine->channel_htable;

  // "Forward send" threads shutdown
#ifdef MARCEL
  dir_vchannel_slist = dir->vchannel_slist;

  if (!tbx_slist_is_nil(dir_vchannel_slist))
    {
      tbx_slist_ref_to_head(dir_vchannel_slist);
      do
	{
	  p_mad_dir_vchannel_t dir_vchannel = NULL;
	  p_mad_channel_t      mad_vchannel = NULL;
	  p_tbx_slist_t        slist        = NULL;
      
	  dir_vchannel = tbx_slist_ref_get(dir_vchannel_slist);
	  mad_vchannel = tbx_htable_get(channel_htable, dir_vchannel->name);

	  // Regular channels
	       slist = mad_vchannel->channel_slist;
      
	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_mad_channel_t channel = NULL;

	      channel = tbx_slist_ref_get(slist);
	      mad_forward_stop_direct_retransmit(channel);
	    }
	  while (tbx_slist_ref_forward(slist));

	  // Forwarding channels
	       slist = mad_vchannel->fchannel_slist;
      
	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_mad_channel_t channel = NULL;

	      channel = tbx_slist_ref_get(slist);
	      mad_forward_stop_indirect_retransmit(channel);
	    }
	  while (tbx_slist_ref_forward(slist));

	}
      while (tbx_slist_ref_forward(dir_vchannel_slist));
    }  
#endif // MARCEL

  mad_ntbx_send_int(client, -1);

  // "Forward receive" threads shutdown
  for (;;)
    {
      ntbx_process_lrank_t  lrank         = -1;
#ifdef MARCEL
      p_mad_channel_t       channel       = NULL;
      p_mad_channel_t       vchannel      = NULL;
#endif // MARCEL
      char                 *channel_name  = NULL;
      char                 *vchannel_name = NULL;


      vchannel_name = mad_ntbx_receive_string(client);
      if (!strcmp(vchannel_name, "-"))
	break;

      channel_name = mad_ntbx_receive_string(client);
      lrank = mad_ntbx_receive_int(client);

#ifdef MARCEL      
      vchannel = tbx_htable_get(channel_htable, vchannel_name);
      if (!vchannel)
	FAILURE("channel not found");

      channel = tbx_htable_get(channel_htable, channel_name);
      if (!channel)
	FAILURE("channel not found");

      mad_forward_stop_reception(vchannel, channel, lrank);
#endif // MARCEL
      mad_ntbx_send_int(client, -1);
    }
  LOG_OUT();
}

void
mad_dir_directory_get(p_mad_madeleine_t madeleine)
{
  p_mad_session_t    session           = NULL;
  p_ntbx_client_t    client            = NULL;
  char              *command_string    = NULL;

  LOG_IN();
  TRACE("Receiving directory");  
  session = madeleine->session;
  client  = session->leonie_link;

  mad_dir_process_get(madeleine);
  mad_dir_node_get(madeleine);
  mad_dir_driver_get(madeleine);
  mad_dir_channel_get(madeleine);
  mad_dir_fchannel_get(madeleine);
  mad_dir_vchannel_get(madeleine);  

  command_string = mad_ntbx_receive_string(client);
  
  if (strcmp(command_string, "end{directory}"))
    FAILURE("synchronisation error");

  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}
