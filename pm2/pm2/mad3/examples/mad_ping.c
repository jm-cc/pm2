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
 * mad_ping.c
 * ==========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

// #include "madeleine.h"

// Setup
//......................

// #define STARTUP_ONLY

// Macros
//......................

#define MAX 16

// Types
//......................
typedef struct s_mad_ping_result
{
  ntbx_process_lrank_t lrank_src;
  ntbx_process_lrank_t lrank_dst;
  int                  size;
  double               latency;
  double               bandwidth_mbit;
  double               bandwidth_mbyte;  
} mad_ping_result_t, *p_mad_ping_result_t;

// Static variables
//......................

static const int param_control_receive   = 0;
static const int param_send_mode         = mad_send_CHEAPER;
static const int param_receive_mode      = mad_receive_CHEAPER;
static const int param_nb_samples        = 1000;
static const int param_min_size          = 0;
static const int param_max_size          = 256;
static const int param_step              = 4; /* 0 = progression log. */
static const int param_nb_tests          = 5;
static const int param_no_zero           = 1;
static const int param_fill_buffer       = 1;
static const int param_fill_buffer_value = 1;
static const int param_one_way           = 1;

static ntbx_process_grank_t process_grank = -1;
static ntbx_process_lrank_t process_lrank = -1;
static ntbx_process_lrank_t master_lrank  = -1;

#ifndef STARTUP_ONLY
static unsigned char *main_buffer = NULL;
#endif // STARTUP_ONLY

// Functions
//......................

#ifndef STARTUP_ONLY
static void
mark_buffer(int len) TBX_UNUSED; 

static void
mark_buffer(int len)
{
  unsigned int n = 0;
  int          i = 0;

  for (i = 0; i < len; i++)
    {
      unsigned char c = 0;

      n += 7;
      c = (unsigned char)(n % 256);

      main_buffer[i] = c;      
    }
}

static void
clear_buffer(void) TBX_UNUSED; 

static void
clear_buffer(void)
{
  memset(main_buffer, 0, param_max_size);
}

static void
fill_buffer(void)
{
  memset(main_buffer, param_fill_buffer_value, param_max_size);
}

static void
control_buffer(int len) TBX_UNUSED; 

static void
control_buffer(int len)
{
  tbx_bool_t   ok = tbx_true;
  unsigned int n  = 0;
  int          i  = 0;

  for (i = 0; i < len; i++)
    {
      unsigned char c = 0;

      n += 7;
      c = (unsigned char)(n % 256);

      if (main_buffer[i] != c)
	{
	  int v1 = 0;
	  int v2 = 0;

	  v1 = c;
	  v2 = main_buffer[i];
	  DISP("Bad data at byte %X: expected %X, received %X", i, v1, v2);
	  ok = tbx_false;
	}
    }

  if (!ok)
    {
      DISP("%d bytes reception failed", len);
    }
}

static
void
process_results(p_mad_channel_t     channel,
		p_mad_ping_result_t results)
{
  LOG_IN();
  if (process_lrank == master_lrank)
    {
      mad_ping_result_t tmp_results = { 0, 0, 0, 0.0, 0.0, 0.0 };

      if (!results)
	{
	  p_mad_connection_t in = NULL;
	  ntbx_pack_buffer_t buffer;

	  in = mad_begin_unpacking(channel);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.lrank_src       = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.lrank_dst       = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.size            = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.latency         = ntbx_unpack_double(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.bandwidth_mbit  = ntbx_unpack_double(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.bandwidth_mbyte = ntbx_unpack_double(&buffer);

	  mad_end_unpacking(in);
	  
	  results = &tmp_results;
	}

      LDISP("%3d %3d %12d %12.3f %8.3f %8.3f",
	    results->lrank_src,
	    results->lrank_dst,
	    results->size,
	    results->latency,
	    results->bandwidth_mbit,
	    results->bandwidth_mbyte);
    }
  else if (results)
    {
      p_mad_connection_t out = NULL;
      ntbx_pack_buffer_t buffer;

      out = mad_begin_packing(channel, master_lrank);
      
      ntbx_pack_int(results->lrank_src, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_int(results->lrank_dst, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_int(results->size, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_double(results->latency, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_double(results->bandwidth_mbit, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_double(results->bandwidth_mbyte, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      mad_end_packing(out);
    }
  
  LOG_OUT();
}

static
void
ping(p_mad_channel_t      channel,
     ntbx_process_lrank_t lrank_dst)
{
  int size = 0;

  LOG_IN();
  if (param_fill_buffer)
    {
      fill_buffer();
    }

  for (size = param_min_size;
       size <= param_max_size;
       size = param_step?size + param_step:size * 2)
    {
      const int         _size   = (!size && param_no_zero)?1:size;
      int               dummy   = 0;
      mad_ping_result_t results = { 0, 0, 0, 0.0, 0.0, 0.0 };
      double            sum     = 0.0;
      tbx_tick_t        t1;
      tbx_tick_t        t2;

      sum = 0;
      
      if (param_one_way)
	{
	  p_mad_connection_t connection = NULL;
	  int                nb_tests   = param_nb_tests * param_nb_samples;

	  TBX_GET_TICK(t1);
	  while (nb_tests--)
	    {
	      connection = mad_begin_packing(channel, lrank_dst);
	      mad_pack(connection, main_buffer, _size,
		       param_send_mode, param_receive_mode);
	      mad_end_packing(connection);
	    }

	  connection = mad_begin_unpacking(channel);
	  mad_unpack(connection, &dummy, sizeof(dummy),
		     param_send_mode, param_receive_mode);
	  mad_end_unpacking(connection);
	  TBX_GET_TICK(t2);       
	  
	  sum += TBX_TIMING_DELAY(t1, t2);	  
	}
      else
	{
	  int nb_tests = param_nb_tests;
	  
	  while (nb_tests--)
	    {
	      int nb_samples = param_nb_samples;

	      TBX_GET_TICK(t1);
	      while (nb_samples--)
		{
		  p_mad_connection_t connection = NULL;

		  connection = mad_begin_packing(channel, lrank_dst);
		  mad_pack(connection, main_buffer, _size,
			   param_send_mode, param_receive_mode);
		  mad_end_packing(connection);

		  connection = mad_begin_unpacking(channel);
		  mad_unpack(connection, main_buffer, _size,
			     param_send_mode, param_receive_mode);
		  mad_end_unpacking(connection);
		}
	      TBX_GET_TICK(t2);

	      sum += TBX_TIMING_DELAY(t1, t2);
	    }
	}

      results.lrank_src       = process_lrank;
      results.lrank_dst       = lrank_dst;
      results.size            = _size;
      results.bandwidth_mbit  =
	(_size * (double)param_nb_tests * (double)param_nb_samples)
	/ (sum / (2 - param_one_way));

      results.bandwidth_mbyte = results.bandwidth_mbit / 1.048576;
      results.latency         =
	sum / param_nb_tests / param_nb_samples / (2 - param_one_way);

      process_results(channel, &results);
    }
  LOG_OUT();
}

static
void
pong(p_mad_channel_t      channel, 
     ntbx_process_lrank_t lrank_dst)
{
  int size = 0;
  
  LOG_IN();
  for (size = param_min_size;
       size <= param_max_size;
       size = param_step?size + param_step:size * 2)
    {
      const int _size = (!size && param_no_zero)?1:size;
      
      if (param_one_way)
	{
	  p_mad_connection_t connection = NULL;
	  int                nb_tests   =
	    param_nb_tests * param_nb_samples;
          int                dummy      = 0;

	  while (nb_tests--)
	    {
	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection, main_buffer, _size,
			 param_send_mode, param_receive_mode);	      
	      mad_end_unpacking(connection);
	    }	  

	  connection = mad_begin_packing(channel, lrank_dst);
	  mad_pack(connection, &dummy, sizeof(dummy),
		   param_send_mode, param_receive_mode);
	  mad_end_packing(connection);
	}
      else
	{
	  int nb_tests = param_nb_tests;

	  while (nb_tests--)
	    {
	      int nb_samples = param_nb_samples;

	      while (nb_samples--)
		{
		  p_mad_connection_t connection = NULL;

		  connection = mad_begin_unpacking(channel);
		  mad_unpack(connection, main_buffer, _size,
			     param_send_mode, param_receive_mode);
		  mad_end_unpacking(connection);

		  connection = mad_begin_packing(channel, lrank_dst);
		  mad_pack(connection, main_buffer, _size,
			   param_send_mode, param_receive_mode);
		  mad_end_packing(connection);
		}
	    }	  
	}

      process_results(channel, NULL);
    }  
  LOG_OUT();
}

static
void
master_loop(p_mad_channel_t channel)
{
  int size = 0;

  LOG_IN();
  for (size = param_min_size;
       size <= param_max_size;
       size = param_step?size + param_step:size * 2)
    {
      process_results(channel, NULL);
    }
  LOG_OUT();
}

static
void
play_with_channel(p_mad_madeleine_t  madeleine,
		  char              *name)
{
  p_mad_channel_t            channel = NULL;
  p_ntbx_process_container_t pc      = NULL;
  tbx_bool_t                 status  = tbx_false;
  ntbx_pack_buffer_t         buffer;
  
  DISP_STR("Channel", name);
  channel = tbx_htable_get(madeleine->channel_htable, name);
  if (!channel)
    {
      DISP("I don't belong to this channel");

      return;
    }

  pc = channel->pc;

  status = ntbx_pc_first_local_rank(pc, &master_lrank);
  if (!status)
    return;  

  process_lrank = ntbx_pc_global_to_local(pc, process_grank);
  DISP_VAL("My local channel rank is", process_lrank);
  
  if (process_lrank == master_lrank)
    {
      // Master
	   ntbx_process_lrank_t lrank_src = -1;
      
      LDISP_STR("Channel", name);
      LDISP("src|dst|size        |latency     |Mb/s    |MB/s    |");

      ntbx_pc_first_local_rank(pc, &lrank_src);
      do
	{
	  ntbx_process_lrank_t lrank_dst = -1;
      
	  if (param_one_way)
	    {
	      ntbx_pc_first_local_rank(pc, &lrank_dst);
	    }
	  else
	    {
	      lrank_dst = lrank_src;

	      if (!ntbx_pc_first_local_rank(pc, &lrank_dst))
		break;
	    }
	  
	  do
	    {
	      if (lrank_dst == lrank_src)
		continue;

	      if (lrank_src == master_lrank)
		{
		  p_mad_connection_t out = NULL;
		  p_mad_connection_t in  = NULL;
		  
		  out = mad_begin_packing(channel, lrank_dst);
		  ntbx_pack_int(lrank_src, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  ntbx_pack_int(0, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_packing(out);

		  in = mad_begin_unpacking(channel);
		  mad_unpack(in, &buffer, sizeof(buffer),
			     mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_unpacking(in);

		  ping(channel, lrank_dst);
		}
	      else if (lrank_dst == master_lrank)
		{
		  p_mad_connection_t out = NULL;
		  p_mad_connection_t in  = NULL;		  

		  out = mad_begin_packing(channel, lrank_src);
		  ntbx_pack_int(lrank_dst, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  ntbx_pack_int(1, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_packing(out);

		  in = mad_begin_unpacking(channel);
		  mad_unpack(in, &buffer, sizeof(buffer),
			     mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_unpacking(in);

		  pong(channel, lrank_src);
		}
	      else
		{
		  p_mad_connection_t out = NULL;
		  p_mad_connection_t in  = NULL;
		  
		  out = mad_begin_packing(channel, lrank_dst);
		  ntbx_pack_int(lrank_src, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  ntbx_pack_int(0, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_packing(out);

		  in = mad_begin_unpacking(channel);
		  mad_unpack(in, &buffer, sizeof(buffer),
			     mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_unpacking(in);

		  out = mad_begin_packing(channel, lrank_src);
		  ntbx_pack_int(lrank_dst, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  ntbx_pack_int(1, &buffer);
		  mad_pack(out, &buffer, sizeof(buffer),
			   mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_packing(out);

		  in = mad_begin_unpacking(channel);
		  mad_unpack(in, &buffer, sizeof(buffer),
			     mad_send_CHEAPER, mad_receive_EXPRESS);
		  mad_end_unpacking(in);

		  master_loop(channel);
		}
	    }
	  while (ntbx_pc_next_local_rank(pc, &lrank_dst));
	}
      while (ntbx_pc_next_local_rank(pc, &lrank_src));

      DISP("test series completed");
      ntbx_pc_first_local_rank(pc, &lrank_src);

      while (ntbx_pc_next_local_rank(pc, &lrank_src))
	{
	  p_mad_connection_t out = NULL;
		  
	  out = mad_begin_packing(channel, lrank_src);
	  ntbx_pack_int(lrank_src, &buffer);
	  mad_pack(out, &buffer, sizeof(buffer),
		   mad_send_CHEAPER, mad_receive_EXPRESS);
	  ntbx_pack_int(0, &buffer);
	  mad_pack(out, &buffer, sizeof(buffer),
		   mad_send_CHEAPER, mad_receive_EXPRESS);
	  mad_end_packing(out);
	}
    }
  else
    {
      /* Slaves */
      while (1)
	{	       
	  p_mad_connection_t   out            = NULL;
	  p_mad_connection_t   in             = NULL;
	  ntbx_process_lrank_t its_local_rank =   -1;
	  int                  direction      =    0;

	  in = mad_begin_unpacking(channel);
	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  its_local_rank = ntbx_unpack_int(&buffer);
	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  direction = ntbx_unpack_int(&buffer);
	  mad_end_unpacking(in);

	  if (its_local_rank == process_lrank)
	    return;
      
	  out = mad_begin_packing(channel, 0);
	  mad_pack(out, &buffer, sizeof(buffer),
		   mad_send_CHEAPER, mad_receive_EXPRESS);
	  mad_end_packing(out);

	  if (direction)
	    {
	      /* Ping */
	      ping(channel, its_local_rank);
	    }
	  else
	    {
	      /* Pong */
	      pong(channel, its_local_rank);	  
	    }
	}      
    }
}  
#endif // STARTUP_ONLY

int
tbx_main(int argc, char **argv)
{
  p_mad_madeleine_t madeleine = NULL;
  p_mad_session_t   session   = NULL;
#ifndef STARTUP_ONLY
  p_tbx_slist_t     slist     = NULL;
#endif // STARTUP_ONLY

#ifdef USE_MAD_INIT
  madeleine = mad_init(&argc, argv);
  TRACE("Returned from mad_init");
#else // USE_MAD_INIT

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  TRACE("Returned from common_init");
  madeleine = mad_get_madeleine();
#endif // USE_MAD_INIT

  session       = madeleine->session;
  process_grank = session->process_rank;
  DISP_VAL("My global rank is", process_grank);
  DISP_VAL("The configuration size is",
	   tbx_slist_get_length(madeleine->dir->process_slist));

#ifndef STARTUP_ONLY
  slist = madeleine->public_channel_slist;
  if (!tbx_slist_is_nil(slist))
    {
      main_buffer = tbx_aligned_malloc(param_max_size, MAD_ALIGNMENT);

      tbx_slist_ref_to_head(slist);
      do
	{
	  char *name = NULL;

	  name = tbx_slist_ref_get(slist);

	  play_with_channel(madeleine, name);
	}
      while (tbx_slist_ref_forward(slist));

      tbx_aligned_free(main_buffer, MAD_ALIGNMENT);
    }
  else
    {
      DISP("No channels");
    }
#endif // STARTUP_ONLY

  DISP("Exiting");  
  
  mad_exit(madeleine);

  return 0;
}
