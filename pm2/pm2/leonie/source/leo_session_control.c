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
 * leo_session_control.c
 * =====================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

// init_drivers: initializes the selected network drivers on each session
// process.
void
init_drivers(p_leonie_t leonie)
{
  p_leo_directory_t  dir         = NULL;
  p_tbx_slist_nref_t process_ref = NULL;

  LOG_IN();
  dir = leonie->directory;

  TRACE("Initializing drivers");

  process_ref = tbx_slist_nref_alloc(dir->process_slist);

  TRACE("First pass");
  {
    p_tbx_slist_t slist = NULL;

    slist = dir->driver_slist;

    tbx_slist_ref_to_head(slist);
    do
      {
	p_leo_dir_driver_t         dir_driver = NULL;
	p_ntbx_process_container_t pc         = NULL;

	dir_driver = tbx_slist_ref_get(slist);
	pc         = dir_driver->pc;

	TRACE_STR("Driver", dir_driver->name);

	// Name
	tbx_slist_nref_to_head(process_ref);
	do
	  {
	    p_ntbx_process_t         process          = NULL;
	    ntbx_process_grank_t     global_rank      =   -1;
	    p_leo_process_specific_t process_specific = NULL;
	    p_ntbx_client_t          client           = NULL;
	    p_ntbx_process_info_t    process_info     = NULL;

	    process      = tbx_slist_nref_get(process_ref);
	    global_rank  = process->global_rank;
	    process_info = ntbx_pc_get_global(pc, global_rank);

	    if (!process_info)
	      continue;

	    process_specific = process->specific;
	    client           = process_specific->client;

	    TRACE_VAL("Transmitting to", global_rank);
	    TRACE("Sending drivers");

	    leo_send_string(client, dir_driver->name);
	  }
	while (tbx_slist_nref_forward(process_ref));

	// Ack
	tbx_slist_nref_to_head(process_ref);
	do
	  {
	    p_ntbx_process_t         process          = NULL;
	    ntbx_process_grank_t     global_rank      =   -1;
	    p_leo_process_specific_t process_specific = NULL;
	    p_ntbx_client_t          client           = NULL;
	    p_ntbx_process_info_t    process_info     = NULL;
	    int                      ack              =    0;

	    process      = tbx_slist_nref_get(process_ref);
	    global_rank  = process->global_rank;
	    process_info = ntbx_pc_get_global(pc, global_rank);

	    if (!process_info)
	      continue;

	    process_specific = process->specific;
	    client           = process_specific->client;

	    TRACE_VAL("Transmitting to", global_rank);
	    TRACE("Receiving ack");
	    ack = leo_receive_int(client);

	    if (ack != 1)
	      FAILURE("synchronisation error");
	  }
	while (tbx_slist_nref_forward(process_ref));

	// Adapters
	tbx_slist_nref_to_head(process_ref);
	do
	  {
	    p_ntbx_process_t                    process          = NULL;
	    ntbx_process_grank_t                global_rank      =   -1;
	    p_leo_process_specific_t            process_specific = NULL;
	    p_ntbx_client_t                     client           = NULL;
	    p_ntbx_process_info_t               process_info     = NULL;
	    p_leo_dir_driver_process_specific_t pi_specific      = NULL;
	    p_tbx_slist_t                       adapter_slist    = NULL;

	    process      = tbx_slist_nref_get(process_ref);
	    global_rank  = process->global_rank;
	    process_info = ntbx_pc_get_global(pc, global_rank);

	    if (!process_info)
	      continue;

	    process_specific = process->specific;
	    client           = process_specific->client;

	    TRACE_VAL("Transmitting to", global_rank);
	    TRACE("Sending adapters");
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
	while (tbx_slist_nref_forward(process_ref));

      }
    while (tbx_slist_ref_forward(slist));
  }

  tbx_slist_nref_to_head(process_ref);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;

      process          = tbx_slist_nref_get(process_ref);
      process_specific = process->specific;
      client           = process_specific->client;

      TRACE_VAL("Transmitting to", process->global_rank);
      TRACE("Sending drivers");

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
      slist = dir->driver_slist;

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

// init_channels: initiates the Madeleine III channels network connexions over
// the session. This function also initiates connections for forwarding
// channels and virtual channels.
void
init_channels(p_leonie_t leonie)
{
  p_leo_directory_t dir   = NULL;
  p_tbx_slist_t     slist = NULL;

  LOG_IN();
  dir = leonie->directory;

  // Channels
  slist = dir->channel_slist;
  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_channel_t        dir_channel = NULL;
	  p_leo_dir_channel_common_t dir_cc      = NULL;
	  p_ntbx_process_container_t pc          = NULL;
	  p_ntbx_process_container_t cpc         = NULL;
	  p_ntbx_topology_table_t    ttable      = NULL;
	  ntbx_process_grank_t       global_rank =   -1;
	  ntbx_process_lrank_t       l_rank_src  =   -1;

	  dir_channel = tbx_slist_ref_get(slist);
	  dir_cc      = dir_channel->common;
	  pc          = dir_channel->pc;
	  cpc         = dir_cc->pc;
	  ttable      = dir_channel->ttable;

	  TRACE_STR("Channel", dir_channel->name);
	  ntbx_pc_first_global_rank(pc, &global_rank);
	  do
	    {
	      p_ntbx_process_t                            process          = NULL;
	      p_leo_process_specific_t                    process_specific = NULL;
	      p_ntbx_client_t                             client           = NULL;
	      p_leo_dir_channel_common_process_specific_t ccps             = NULL;

	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 1 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;
	      ccps = ntbx_pc_get_global_specific(cpc, global_rank);

	      leo_send_string(client, dir_channel->name);
	      ccps->parameter = leo_receive_string(client);
	      TRACE_STR("Channel connection parameter", ccps->parameter);
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	  ntbx_pc_first_local_rank(pc, &l_rank_src);
	  do
	    {
	      p_ntbx_process_t         process_src          = NULL;
	      p_leo_process_specific_t process_specific_src = NULL;
	      p_leo_dir_channel_common_process_specific_t ccps_src = NULL;
	      p_ntbx_client_t          client_src           = NULL;
	      ntbx_process_lrank_t     l_rank_dst           =   -1;

	      process_src = ntbx_pc_get_local_process(pc, l_rank_src);
	      process_specific_src = process_src->specific;
	      client_src           = process_specific_src->client;
	      ccps_src = ntbx_pc_get_local_specific(cpc, l_rank_src);

	      ntbx_pc_first_local_rank(pc, &l_rank_dst);
	      do
		{
		  p_ntbx_process_t         process_dst          = NULL;
		  p_leo_process_specific_t process_specific_dst = NULL;
		  p_ntbx_client_t          client_dst           = NULL;
		  p_leo_dir_channel_common_process_specific_t ccps_dst = NULL;
		  int                      ack_src              =    0;
		  int                      ack_dst              =    0;

		  if (!ntbx_topology_table_get(ttable, l_rank_src, l_rank_dst))
		    continue;

		  TRACE("Initializing connection: %d to %d",
		       l_rank_src, l_rank_dst);

		  process_dst = ntbx_pc_get_local_process(pc, l_rank_dst);
		  process_specific_dst = process_dst->specific;
		  client_dst           = process_specific_dst->client;
		  ccps_dst = ntbx_pc_get_local_specific(cpc, l_rank_dst);

		  leo_send_int(client_src, 0);
		  leo_send_int(client_src, l_rank_dst);
		  leo_send_int(client_dst, 1);
		  leo_send_int(client_dst, l_rank_src);

		  {
		    char *src_cnx_parameter = NULL;
		    char *dst_cnx_parameter = NULL;

		    src_cnx_parameter = leo_receive_string(client_src);
		    tbx_darray_expand_and_set(ccps_src->
					      out_connection_parameter_darray,
					      l_rank_dst,
					      src_cnx_parameter);
		    TRACE_STR("Src connection parameter", src_cnx_parameter);

		    dst_cnx_parameter = leo_receive_string(client_dst);
		    tbx_darray_expand_and_set(ccps_dst->
					      in_connection_parameter_darray,
					      l_rank_src,
					      dst_cnx_parameter);
		    TRACE_STR("Dst connection parameter", dst_cnx_parameter);

		    leo_send_string(client_src, ccps_dst->parameter);
		    leo_send_string(client_src, dst_cnx_parameter);
		    leo_send_string(client_dst, ccps_src->parameter);
		    leo_send_string(client_dst, src_cnx_parameter);
		  }

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

  slist = dir->process_slist;
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
  slist = dir->fchannel_slist;
  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_fchannel_t       dir_fchannel = NULL;
	  p_leo_dir_channel_t        dir_channel  = NULL;
	  p_leo_dir_channel_common_t dir_cc       = NULL;
	  p_ntbx_topology_table_t    ttable       = NULL;
	  p_ntbx_process_container_t pc           = NULL;
	  p_ntbx_process_container_t cpc          = NULL;
	  ntbx_process_grank_t       global_rank  =   -1;
	  ntbx_process_lrank_t       l_rank_src   =   -1;

	  dir_fchannel = tbx_slist_ref_get(slist);
	  dir_cc       = dir_fchannel->common;
	  dir_channel  = tbx_htable_get(dir->channel_htable,
					dir_fchannel->channel_name);
	  cpc          = dir_cc->pc;

	  pc = dir_channel->pc;
	  ttable = dir_channel->ttable;
	  TRACE_STR("Forwarding channel", dir_fchannel->name);
	  ntbx_pc_first_global_rank(pc, &global_rank);
	  TRACE_VAL("global_rank", global_rank);

	  do
	    {
	      p_ntbx_process_t                            process          = NULL;
	      p_leo_process_specific_t                    process_specific = NULL;
	      p_ntbx_client_t                             client           = NULL;
	      p_leo_dir_channel_common_process_specific_t ccps             = NULL;

	      process = ntbx_pc_get_global_process(pc, global_rank);
	      TRACE_VAL("Pass 1 - Process", process->global_rank);
	      process_specific = process->specific;
	      client           = process_specific->client;
	      ccps = ntbx_pc_get_global_specific(cpc, global_rank);

	      leo_send_string(client, dir_fchannel->name);
	      ccps->parameter = leo_receive_string(client);
	      TRACE_STR("Channel connection parameter", ccps->parameter);
	    }
	  while (ntbx_pc_next_global_rank(pc, &global_rank));

	  ntbx_pc_first_local_rank(pc, &l_rank_src);
	  do
	    {
	      p_ntbx_process_t         process_src          = NULL;
	      p_leo_process_specific_t process_specific_src = NULL;
	      p_leo_dir_channel_common_process_specific_t ccps_src = NULL;
	      p_ntbx_client_t          client_src           = NULL;
	      ntbx_process_lrank_t     l_rank_dst           =   -1;

	      process_src = ntbx_pc_get_local_process(pc, l_rank_src);
	      process_specific_src = process_src->specific;
	      client_src           = process_specific_src->client;
	      ccps_src = ntbx_pc_get_local_specific(cpc, l_rank_src);

	      ntbx_pc_first_local_rank(pc, &l_rank_dst);
	      do
		{
		  p_ntbx_process_t         process_dst          = NULL;
		  p_leo_process_specific_t process_specific_dst = NULL;
		  p_ntbx_client_t          client_dst           = NULL;
		  p_leo_dir_channel_common_process_specific_t ccps_dst = NULL;
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
		  ccps_dst = ntbx_pc_get_local_specific(cpc, l_rank_dst);

		  leo_send_int(client_src, 0);
		  leo_send_int(client_src, l_rank_dst);
		  leo_send_int(client_dst, 1);
		  leo_send_int(client_dst, l_rank_src);

		  {
		    char *src_cnx_parameter = NULL;
		    char *dst_cnx_parameter = NULL;

		    src_cnx_parameter = leo_receive_string(client_src);
		    tbx_darray_expand_and_set(ccps_src->
					      out_connection_parameter_darray,
					      l_rank_dst,
					      src_cnx_parameter);
		    TRACE_STR("Src connection parameter", src_cnx_parameter);

		    dst_cnx_parameter = leo_receive_string(client_dst);
		    tbx_darray_expand_and_set(ccps_dst->
					      in_connection_parameter_darray,
					      l_rank_src,
					      dst_cnx_parameter);
		    TRACE_STR("Dst connection parameter", dst_cnx_parameter);

		    leo_send_string(client_src, ccps_dst->parameter);
		    leo_send_string(client_src, dst_cnx_parameter);
		    leo_send_string(client_dst, ccps_src->parameter);
		    leo_send_string(client_dst, src_cnx_parameter);
		  }

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

  slist = dir->process_slist;
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
  slist = dir->vchannel_slist;
  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_vchannel_t       dir_vchannel = NULL;
	  p_ntbx_process_container_t pc           = NULL;
	  ntbx_process_grank_t       global_rank  =   -1;

	  dir_vchannel = tbx_slist_ref_get(slist);
	  pc           = dir_vchannel->pc;

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

  slist = dir->process_slist;
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

// exit_session: has to purposes:
// - MadIII/Leonie interactions functions (data display, synchronizations)
// - session termination synchronization.
void
exit_session(p_leonie_t leonie)
{
  p_leo_directory_t  dir           = NULL;
  p_ntbx_client_t   *client_array  = NULL;
  int                nb_clients    =    0;
  int                i             =    0;
  tbx_bool_t         finishing     = tbx_false;
  int                barrier_count =    0;

  LOG_IN();
  dir = leonie->directory;

  nb_clients    = tbx_slist_get_length(dir->process_slist);
  client_array  = TBX_CALLOC(nb_clients, sizeof(ntbx_client_t));

  tbx_slist_ref_to_head(dir->process_slist);
  do
    {
      p_ntbx_process_t          process          = NULL;
      p_leo_process_specific_t  process_specific = NULL;
      p_ntbx_client_t           client           = NULL;

      process           = tbx_slist_ref_get(dir->process_slist);
      process_specific  = process->specific;
      client            = process_specific->client;
      client_array[i++] = client;
    }
  while (tbx_slist_ref_forward(dir->process_slist));

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
		    finishing = tbx_true;
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

		case leo_command_barrier:
		  {
		    if (finishing)
		      FAILURE("barrier request during session clean-up");

		    barrier_count++;

		    if (barrier_count >= nb_clients)
		      {
			if (barrier_count > nb_clients)
			  FAILURE("incoherent behaviour");

			tbx_slist_ref_to_head(dir->process_slist);
			do
			  {
			    p_ntbx_process_t          process          = NULL;
			    p_leo_process_specific_t  process_specific = NULL;
			    p_ntbx_client_t           tmp_client       = NULL;

			    process           =
			      tbx_slist_ref_get(dir->process_slist);
			    process_specific  = process->specific;
			    tmp_client        = process_specific->client;
			    leo_send_int(tmp_client,
					 leo_command_barrier_passed);
			  }
			while (tbx_slist_ref_forward(dir->
						     process_slist));

			barrier_count = 0;
		      }

		    status --;
		  }
		  break;

		default:
		  FAILURE("unknown command or synchronization error");
		}
	    }

	  if (status)
	    FAILURE("incoherent behaviour");
	}
    }

  LOG_OUT();
}

// dir_vchannel_disconnect: manages virtual channels disconnection
void
dir_vchannel_disconnect(p_leonie_t leonie)
{
  p_leo_directory_t dir = NULL;

  LOG_IN();
  dir = leonie->directory;

  tbx_slist_ref_to_head(dir->process_slist);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;
      int                      data             =    0;

      process           = tbx_slist_ref_get(dir->process_slist);
      process_specific  = process->specific;
      client            = process_specific->client;
      leo_send_int(client, 1);

      // Waiting for "Forward send" threads to shutdown
      data = leo_receive_int(client);

      if (data != -1)
	FAILURE("synchronization error");
    }
  while (tbx_slist_ref_forward(dir->process_slist));

  // Shuting down "Forward receive" threads
  if (!tbx_slist_is_nil(dir->vchannel_slist))
    {
      p_tbx_slist_t vslist = NULL;

      vslist = dir->vchannel_slist;
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
	      channel  = tbx_htable_get(dir->channel_htable,
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

  tbx_slist_ref_to_head(dir->process_slist);
  do
    {
      p_ntbx_process_t         process          = NULL;
      p_leo_process_specific_t process_specific = NULL;
      p_ntbx_client_t          client           = NULL;

      process           = tbx_slist_ref_get(dir->process_slist);
      process_specific  = process->specific;
      client            = process_specific->client;
      leo_send_string(client, "-");
    }
  while (tbx_slist_ref_forward(dir->process_slist));


  LOG_OUT();
}
