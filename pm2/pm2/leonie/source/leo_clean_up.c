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
 * leonie_clean_up.c
 * =================
 *
 * - session disconnect routines
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

// dir_vchannel_exit: manages virtual channels data structures clean-up
static
void
dir_vchannel_exit(p_leo_directory_t dir)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = dir->vchannel_slist;

  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_vchannel_t       dir_vchannel = NULL;
	  p_ntbx_process_container_t pc           = NULL;
	  ntbx_process_grank_t       global_rank  =   -1;
	  ntbx_process_lrank_t       l_rank_src   =   -1;

	  dir_vchannel = tbx_slist_ref_get(slist);
	  pc           = dir_vchannel->pc;

	  TRACE_STR("Vchannel", dir_vchannel->name);
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

		  TRACE("Closing connection: %d to %d",
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

		  TRACE("Freeing connection from %d to %d",
			l_rank_src, l_rank_dst);

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
 LOG_OUT();
}

static
void
dir_xchannel_exit(p_leo_directory_t dir)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = dir->xchannel_slist;

  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_xchannel_t       dir_xchannel = NULL;
	  p_ntbx_process_container_t pc           = NULL;
	  ntbx_process_grank_t       global_rank  =   -1;
	  ntbx_process_lrank_t       l_rank_src   =   -1;

	  dir_xchannel = tbx_slist_ref_get(slist);
	  pc           = dir_xchannel->pc;

	  TRACE_STR("Xchannel", dir_xchannel->name);
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

	      leo_send_string(client, dir_xchannel->name);
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

		  TRACE("Closing connection: %d to %d",
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

		  TRACE("Freeing connection from %d to %d",
			l_rank_src, l_rank_dst);

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
 LOG_OUT();
}

// dir_fchannel_exit: manages forwarding channels data structures clean-up
static
void
dir_fchannel_exit(p_leo_directory_t dir)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = dir->fchannel_slist;

  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  p_leo_dir_fchannel_t       dir_fchannel = NULL;
	  p_leo_dir_channel_t        dir_channel  = NULL;
	  p_ntbx_process_container_t pc           = NULL;
	  p_ntbx_topology_table_t    ttable       = NULL;
	  ntbx_process_grank_t       global_rank  =   -1;
	  ntbx_process_lrank_t       l_rank_src   =   -1;

	  dir_fchannel = tbx_slist_ref_get(slist);
	  dir_channel  = tbx_htable_get(dir->channel_htable,
					dir_fchannel->channel_name);
	  pc           = dir_channel->pc;
	  ttable       = dir_channel->ttable;

	  TRACE_STR("Channel", dir_fchannel->name);
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

		  TRACE("Freeing connection: %d to %d",
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

		  TRACE("Disconnecting %d to %d", l_rank_src, l_rank_dst);

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
  LOG_OUT();
}

// dir_channel_exit: manages regular channels data structures clean-up
static
void
dir_channel_exit(p_leo_directory_t dir)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = dir->channel_slist;
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

		  TRACE("Freeing connection: %d to %d",
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

		  TRACE("Disconnecting %d to %d", l_rank_src, l_rank_dst);

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
  LOG_OUT();
}

// dir_driver_exit: manages driver data structures clean-up
static
void
dir_driver_exit(p_leo_directory_t dir)
{
  p_tbx_slist_nref_t process_ref = NULL;

  LOG_IN();
  TRACE("Freeing drivers");

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
		TRACE_STR("Parameter", dir_adapter->parameter);
	      }
	    while (tbx_slist_ref_forward(adapter_slist));

	    leo_send_string(client, "-");
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

  tbx_slist_nref_free(process_ref);
  LOG_OUT();
}

// dir_vchannel_cleanup: manages local virtual channels data structures
// clean-up
static
void
dir_vchannel_cleanup(p_leo_directory_t dir)
{
  p_tbx_htable_t     vchannel_htable = NULL;
  p_tbx_slist_t      vchannel_slist  = NULL;

  LOG_IN();
  vchannel_htable = dir->vchannel_htable;
  vchannel_slist  = dir->vchannel_slist;

  while (!tbx_slist_is_nil(vchannel_slist))
    {
      p_leo_dir_vchannel_t       dir_vchannel   = NULL;
      p_tbx_slist_t              channel_slist  = NULL;
      p_tbx_slist_t              fchannel_slist = NULL;
      p_ntbx_process_container_t pc             = NULL;
      ntbx_process_grank_t       g_rank_src     =   -1;

      dir_vchannel   = tbx_slist_extract(vchannel_slist);
      tbx_htable_extract(vchannel_htable, dir_vchannel->name);

      channel_slist  = dir_vchannel->dir_channel_slist;
      fchannel_slist = dir_vchannel->dir_fchannel_slist;

      while (!tbx_slist_is_nil(channel_slist))
	{
	  tbx_slist_extract(channel_slist);
	}

      tbx_slist_free(channel_slist);
      channel_slist                   = NULL;
      dir_vchannel->dir_channel_slist = NULL;

      while (!tbx_slist_is_nil(fchannel_slist))
	{
	  tbx_slist_extract(fchannel_slist);
	}

      tbx_slist_free(fchannel_slist);
      fchannel_slist                   = NULL;
      dir_vchannel->dir_fchannel_slist = NULL;

      pc = dir_vchannel->pc;

      if (ntbx_pc_first_global_rank(pc, &g_rank_src))
	{
	  do
	    {
	      p_leo_dir_vchannel_process_specific_t vps        = NULL;
	      p_ntbx_process_container_t            ppc        = NULL;
	      ntbx_process_grank_t                  g_rank_dst =   -1;

	      vps = ntbx_pc_get_global_specific(pc, g_rank_src);
	      ppc = vps->pc;

	      ntbx_pc_first_global_rank(ppc, &g_rank_dst);
	      do
		{
		  p_leo_dir_vchannel_process_routing_table_t rtable = NULL;

		  rtable = ntbx_pc_get_global_specific(ppc, g_rank_dst);
		  TBX_FREE(rtable->channel_name);
		  rtable->channel_name = NULL;
		}
	      while (ntbx_pc_next_global_rank(ppc, &g_rank_dst));

	      ntbx_pc_dest(ppc, tbx_default_specific_dest);
	    }
	  while (ntbx_pc_next_global_rank(pc, &g_rank_src));
	}

      ntbx_pc_dest(pc, tbx_default_specific_dest);
      dir_vchannel->pc   = NULL;

      TBX_FREE(dir_vchannel->name);
      dir_vchannel->name = NULL;

      TBX_FREE(dir_vchannel);
    }

  tbx_slist_free(dir->vchannel_slist);
  dir->vchannel_slist = NULL;

  tbx_htable_free(dir->vchannel_htable);
  dir->vchannel_htable = NULL;
  LOG_OUT();
}

// dir_vchannel_cleanup: manages local virtual channels data structures
// clean-up
static
void
dir_xchannel_cleanup(p_leo_directory_t dir)
{
  p_tbx_htable_t     xchannel_htable = NULL;
  p_tbx_slist_t      xchannel_slist  = NULL;

  LOG_IN();
  xchannel_htable = dir->xchannel_htable;
  xchannel_slist  = dir->xchannel_slist;

  while (!tbx_slist_is_nil(xchannel_slist))
    {
      p_leo_dir_xchannel_t       dir_xchannel   = NULL;
      p_tbx_slist_t              channel_slist  = NULL;
      p_ntbx_process_container_t pc             = NULL;
      ntbx_process_grank_t       g_rank_src     =   -1;

      dir_xchannel   = tbx_slist_extract(xchannel_slist);
      tbx_htable_extract(xchannel_htable, dir_xchannel->name);

      channel_slist  = dir_xchannel->dir_channel_slist;

      while (!tbx_slist_is_nil(channel_slist))
	{
	  tbx_slist_extract(channel_slist);
	}

      tbx_slist_free(channel_slist);
      channel_slist                   = NULL;
      dir_xchannel->dir_channel_slist = NULL;

      pc = dir_xchannel->pc;

      if (ntbx_pc_first_global_rank(pc, &g_rank_src))
	{
	  do
	    {
	      p_leo_dir_xchannel_process_specific_t vps        = NULL;
	      p_ntbx_process_container_t            ppc        = NULL;
	      ntbx_process_grank_t                  g_rank_dst =   -1;

	      vps = ntbx_pc_get_global_specific(pc, g_rank_src);
	      ppc = vps->pc;

	      ntbx_pc_first_global_rank(ppc, &g_rank_dst);
	      do
		{
		  p_leo_dir_xchannel_process_routing_table_t rtable = NULL;

		  rtable = ntbx_pc_get_global_specific(ppc, g_rank_dst);
		  TBX_FREE(rtable->channel_name);
		  rtable->channel_name = NULL;
		}
	      while (ntbx_pc_next_global_rank(ppc, &g_rank_dst));

	      ntbx_pc_dest(ppc, tbx_default_specific_dest);
	    }
	  while (ntbx_pc_next_global_rank(pc, &g_rank_src));
	}

      ntbx_pc_dest(pc, tbx_default_specific_dest);
      dir_xchannel->pc   = NULL;

      TBX_FREE(dir_xchannel->name);
      dir_xchannel->name = NULL;

      TBX_FREE(dir_xchannel);
    }

  tbx_slist_free(dir->xchannel_slist);
  dir->xchannel_slist = NULL;

  tbx_htable_free(dir->xchannel_htable);
  dir->xchannel_htable = NULL;
  LOG_OUT();
}

// dir_fchannel_cleanup: manages local forwarding channels data structures
// clean-up
static
void
dir_fchannel_cleanup(p_leo_directory_t dir)
{
  p_tbx_htable_t    fchannel_htable = NULL;
  p_tbx_slist_t     fchannel_slist  = NULL;

  LOG_IN();
  fchannel_htable = dir->fchannel_htable;
  fchannel_slist  = dir->fchannel_slist;

  while (!tbx_slist_is_nil(fchannel_slist))
    {
      p_leo_dir_fchannel_t dir_fchannel = NULL;

      dir_fchannel = tbx_slist_extract(fchannel_slist);
      tbx_htable_extract(fchannel_htable, dir_fchannel->name);

      TBX_FREE(dir_fchannel->name);
      dir_fchannel->name = NULL;

      TBX_FREE(dir_fchannel->channel_name);
      dir_fchannel->channel_name = NULL;

      TBX_FREE(dir_fchannel);
    }

  tbx_slist_free(dir->fchannel_slist);
  dir->fchannel_slist = NULL;

  tbx_htable_free(dir->fchannel_htable);
  dir->fchannel_htable = NULL;
  LOG_OUT();
}

// dir_channel_cleanup: manages local regular channels data structures
// clean-up
static
void
dir_channel_cleanup(p_leo_directory_t dir)
{
  p_tbx_htable_t    channel_htable = NULL;
  p_tbx_slist_t     channel_slist  = NULL;

  LOG_IN();
  channel_htable = dir->channel_htable;
  channel_slist  = dir->channel_slist;

  while (!tbx_slist_is_nil(channel_slist))
    {
      p_leo_dir_channel_t        dir_channel = NULL;
      p_ntbx_process_container_t pc          = NULL;
      ntbx_process_lrank_t       l_rank      =   -1;

      dir_channel = tbx_slist_extract(channel_slist);
      tbx_htable_extract(channel_htable, dir_channel->name);

      ntbx_topology_table_exit(dir_channel->ttable);
      dir_channel->ttable = NULL;

      pc = dir_channel->pc;

      if (ntbx_pc_first_local_rank(pc, &l_rank))
	{
	  do
	    {
	      p_leo_dir_channel_process_specific_t cps = NULL;

	      cps = ntbx_pc_get_local_specific(pc, l_rank);

	      if (cps && cps->adapter_name)
		{
		  TBX_FREE(cps->adapter_name);
		  cps->adapter_name = NULL;
		}
	    }
	  while (ntbx_pc_next_global_rank(pc, &l_rank));
	}

      ntbx_pc_dest(dir_channel->pc, tbx_default_specific_dest);
      dir_channel->pc = NULL;

      TBX_FREE(dir_channel->name);
      dir_channel->name = NULL;

      dir_channel->driver = NULL;
      dir_channel->public = tbx_false;

      TBX_FREE(dir_channel);
    }

  tbx_slist_free(dir->channel_slist);
  dir->channel_slist = NULL;

  tbx_htable_free(dir->channel_htable);
  dir->channel_htable = NULL;
  LOG_OUT();
}

// dir_driver_cleanup: manages local driver data structures clean-up
static
void
dir_driver_cleanup(p_leo_directory_t dir)
{
  p_tbx_htable_t    driver_htable = NULL;
  p_tbx_slist_t     driver_slist  = NULL;

  LOG_IN();
  driver_htable = dir->driver_htable;
  driver_slist  = dir->driver_slist;

  while (!tbx_slist_is_nil(driver_slist))
    {
      p_leo_dir_driver_t         dir_driver = NULL;
      p_ntbx_process_container_t pc         = NULL;
      ntbx_process_lrank_t       l_rank     =   -1;

      dir_driver = tbx_slist_extract(driver_slist);
      tbx_htable_extract(driver_htable, dir_driver->name);

      pc = dir_driver->pc;

      if (ntbx_pc_first_local_rank(pc, &l_rank))
	{
	  do
	    {
	      p_leo_dir_driver_process_specific_t dps = NULL;

	      dps = ntbx_pc_get_local_specific(pc, l_rank);

	      if (dps)
		{
		  p_tbx_htable_t adapter_htable = NULL;
		  p_tbx_slist_t  adapter_slist  = NULL;

		  adapter_htable = dps->adapter_htable;
		  adapter_slist  = dps->adapter_slist;

		  while (!tbx_slist_is_nil(adapter_slist))
		    {
		      p_leo_dir_adapter_t dir_adapter = NULL;

		      dir_adapter = tbx_slist_extract(adapter_slist);
		      tbx_htable_extract(adapter_htable, dir_adapter->name);

		      TBX_FREE(dir_adapter->name);
		      dir_adapter->name = NULL;

		      if (dir_adapter->selector)
			{
			  TBX_FREE(dir_adapter->selector);
			  dir_adapter->selector = NULL;
			}

		      if (dir_adapter->parameter)
			{
			  TBX_FREE(dir_adapter->parameter);
			  dir_adapter->parameter = NULL;
			}

		      dir_adapter->mtu = 0;

		      TBX_FREE(dir_adapter);
		    }

		  tbx_slist_free(adapter_slist);
		  dps->adapter_slist = NULL;

		  tbx_htable_free(adapter_htable);
		  dps->adapter_htable = NULL;
		}
	    }
	  while (ntbx_pc_next_global_rank(pc, &l_rank));
	}

      ntbx_pc_dest(dir_driver->pc, tbx_default_specific_dest);
      dir_driver->pc = NULL;

      TBX_FREE(dir_driver->name);
      dir_driver->name = NULL;

      TBX_FREE(dir_driver);
    }

  tbx_slist_free(dir->driver_slist);
  dir->driver_slist = NULL;

  tbx_htable_free(dir->driver_htable);
  dir->driver_htable = NULL;
  LOG_OUT();
}

// dir_node_cleanup: manages local node data structures clean-up
static
void
dir_node_cleanup(p_leo_directory_t dir)
{
  p_tbx_htable_t    node_htable = NULL;
  p_tbx_slist_t     node_slist  = NULL;

  LOG_IN();
  node_htable = dir->node_htable;
  node_slist  = dir->node_slist;

  while (!tbx_slist_is_nil(node_slist))
    {
      p_leo_dir_node_t dir_node = NULL;

      dir_node = tbx_slist_extract(node_slist);
      tbx_htable_extract(node_htable, dir_node->name);

      ntbx_pc_dest(dir_node->pc, tbx_default_specific_dest);
      dir_node->pc   = NULL;

      TBX_FREE(dir_node->name);
      dir_node->name = NULL;

      TBX_FREE(dir_node);
    }

  tbx_slist_free(dir->node_slist);
  dir->node_slist = NULL;

  tbx_htable_free(dir->node_htable);
  dir->node_htable = NULL;
  LOG_OUT();
}

// dir_process_cleanup: manages local process data structures clean-up
static
void
dir_process_cleanup(p_leo_directory_t dir)
{
  p_tbx_slist_t process_slist = NULL;

  LOG_IN();
  process_slist = dir->process_slist;

  while (!tbx_slist_is_nil(process_slist))
    {
      p_ntbx_process_t process = NULL;
      p_tbx_htable_t   ref     = NULL;
      p_tbx_slist_t    keys    = NULL;

      process = tbx_slist_extract(process_slist);
      ref     = process->ref;
      keys    = tbx_htable_get_key_slist(ref);

      while (!tbx_slist_is_nil(keys))
	{
	  char *key = NULL;

	  key = tbx_slist_extract(keys);
	  tbx_htable_extract(ref, key);
	  TBX_FREE(key);
	}

      tbx_slist_free(keys);
      tbx_htable_free(ref);
    }

  tbx_slist_free(dir->process_slist);
  dir->process_slist = NULL;
  LOG_OUT();
}

// directory_exit: local directory data structure clean-up
void
directory_exit(p_leonie_t leonie)
{
  p_leo_directory_t dir   = NULL;
  p_tbx_slist_t     slist = NULL;

  LOG_IN();
  dir = leonie->directory;

  dir_vchannel_exit(dir);
  dir_xchannel_exit(dir);
  dir_fchannel_exit(dir);
  dir_channel_exit(dir);
  dir_driver_exit(dir);
  TRACE("Directory clean-up");

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
      leo_send_string(client, "clean{directory}");
    }
  while (tbx_slist_ref_forward(slist));

  dir_vchannel_cleanup(dir);
  dir_xchannel_cleanup(dir);
  dir_fchannel_cleanup(dir);
  dir_channel_cleanup(dir);
  dir_driver_cleanup(dir);
  dir_node_cleanup(dir);
  dir_process_cleanup(dir);
  LOG_OUT();
}

