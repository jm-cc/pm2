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
 * mad_multi_ping.c
 * ================
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


// Types
//......................
typedef struct s_mad_ping_result
{
  ntbx_process_lrank_t lrank_src;
  ntbx_process_lrank_t lrank_dst;
  int                  size;
  int                  nb_pack;
  int                  send;
  int                  recv;
  int                  dir;
} mad_ping_result_t, *p_mad_ping_result_t;

// Static variables
//......................

static int param_send_mode         =  mad_send_CHEAPER;
static int param_receive_mode      =  mad_receive_CHEAPER;
static const int param_nb_samples        =  2;
static const int param_min_size          =  MAD_LENGTH_ALIGNMENT;
static const int param_max_size          =  1024*1024*4;
static const int param_step              =  0; /* 0 = progression log. */
static const int param_min_pack_number   =  1;
static const int param_max_pack_number   =  4;
static const int param_nb_tests          =  1;
static const int param_no_zero           =  1;
static const int param_fill_buffer       =  1;
static const int param_fill_buffer_value =  1;
static int param_one_way           =  1;

static ntbx_process_grank_t process_grank = -1;
static ntbx_process_lrank_t process_lrank = -1;
static ntbx_process_lrank_t master_lrank  = -1;
static tbx_bool_t           status        = tbx_true;

#ifndef STARTUP_ONLY
static unsigned char *main_buffer = NULL;
#endif // STARTUP_ONLY


// Functions
//......................

#ifndef STARTUP_ONLY
static
void
send_int(p_mad_channel_t c,
	 int             dest,
	 int             val)
{
  p_mad_connection_t out = NULL;
  ntbx_pack_buffer_t buffer;

  ntbx_pack_int(val, &buffer);

  out = mad_begin_packing(c, dest);
  mad_pack(out, &buffer, sizeof(buffer),
	   mad_send_CHEAPER, mad_receive_CHEAPER);
  mad_end_packing(out);
}

static
int
receive_int(p_mad_channel_t c)
{
  p_mad_connection_t in  = NULL;
  int                val = 0;
  ntbx_pack_buffer_t buffer;

  in = mad_begin_unpacking(c);
  mad_unpack(in, &buffer, sizeof(buffer),
	   mad_send_CHEAPER, mad_receive_CHEAPER);
  mad_end_unpacking(in);

  val = ntbx_unpack_int(&buffer);

  return val;
}

static
void
send_int_pair(p_mad_channel_t c,
	      int             dest,
	      int             val1,
	      int             val2)
{
  p_mad_connection_t out = NULL;
  ntbx_pack_buffer_t buffer1;
  ntbx_pack_buffer_t buffer2;

  ntbx_pack_int(val1, &buffer1);
  ntbx_pack_int(val2, &buffer2);

  out = mad_begin_packing(c, dest);
  mad_pack(out, &buffer1, sizeof(buffer1),
	   mad_send_CHEAPER, mad_receive_CHEAPER);
  mad_pack(out, &buffer2, sizeof(buffer2),
	   mad_send_CHEAPER, mad_receive_CHEAPER);
  mad_end_packing(out);
}

static
void
receive_int_pair(p_mad_channel_t  c,
		 int             *pval1,
		 int             *pval2)
{
  p_mad_connection_t in = NULL;
  ntbx_pack_buffer_t buffer1;
  ntbx_pack_buffer_t buffer2;

  in = mad_begin_unpacking(c);
  mad_unpack(in, &buffer1, sizeof(buffer1),
	   mad_send_CHEAPER, mad_receive_CHEAPER);
  mad_unpack(in, &buffer2, sizeof(buffer2),
	   mad_send_CHEAPER, mad_receive_CHEAPER);
  mad_end_unpacking(in);

  *pval1 = ntbx_unpack_int(&buffer1);
  *pval2 = ntbx_unpack_int(&buffer2);
}



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
	  LDISP("####........ Bad data at byte %X: expected %X, received %X", i, v1, v2);
	  ok = tbx_false;
	}
    }

  if (!ok)
    {
      LDISP("####........ %d bytes reception failed", len);
      status = tbx_false;
    }
}

static void
fill_buffer(void)
{
  memset(main_buffer, param_fill_buffer_value, param_max_size);
}

static
void
process_results(p_mad_channel_t     channel,
		p_mad_ping_result_t results)
{
  LOG_IN();
  if (process_lrank == master_lrank)
    {
      mad_ping_result_t tmp_results = { 0, 0, 0, 0 };

      if (!results)
	{
	  p_mad_connection_t in = NULL;
	  ntbx_pack_buffer_t buffer;

	  in = mad_begin_unpacking(channel);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_SAFER, mad_receive_EXPRESS);
	  tmp_results.lrank_src       = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_SAFER, mad_receive_EXPRESS);
	  tmp_results.lrank_dst       = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_SAFER, mad_receive_EXPRESS);
	  tmp_results.size            = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_SAFER, mad_receive_EXPRESS);
	  tmp_results.nb_pack         = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_SAFER, mad_receive_EXPRESS);
	  tmp_results.send            = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_SAFER, mad_receive_EXPRESS);
	  tmp_results.recv            = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_SAFER, mad_receive_EXPRESS);
	  tmp_results.dir             = ntbx_unpack_int(&buffer);

	  mad_end_unpacking(in);

	  results = &tmp_results;
	}

      {
	char *snd = NULL;
	char *rcv = NULL;
	char *dir = NULL;

      switch (results->send)
	{
	case  mad_send_SAFER:
	  snd = "s_SAFER";
	  break;

	case  mad_send_LATER:
	  snd = "s_LATER";
	  break;

	case  mad_send_CHEAPER:
	  snd = "s_CHEAPER";
	  break;

	default:
	  FAILURE(" unknown send mode");
	}

	  switch (results->recv)
	    {
	    case  mad_receive_EXPRESS:
	      rcv = "r_EXPRESS";
	      break;

	    case  mad_receive_CHEAPER:
	      rcv = "r_CHEAPER";
	      break;

	    default:
	      FAILURE("unknown receive mode");
	    }


	      switch (results->dir)
		{
		case 0:
		  dir = "2-way";
		  break;

		case 1:
		  dir = "1-way";
		  break;

		default:
		  FAILURE("invalid test mode");
		}


	LDISP("####%s, %s, %s: %d, %d, %d, %d -- test completed",
	      snd,
	      rcv,
	      dir,
	      results->lrank_src,
	      results->lrank_dst,
	      results->size,
	      results->nb_pack);
      }
    }
  else if (results)
    {
      p_mad_connection_t out = NULL;
      ntbx_pack_buffer_t buffer;

      out = mad_begin_packing(channel, master_lrank);

      ntbx_pack_int(results->lrank_src, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);

      ntbx_pack_int(results->lrank_dst, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);

      ntbx_pack_int(results->size, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);

      ntbx_pack_int(results->nb_pack, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);

      ntbx_pack_int(results->send, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);

      ntbx_pack_int(results->recv, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);

      ntbx_pack_int(results->dir, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);

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

  for (size  = param_min_size;
       size <= param_max_size;
       size  = param_step?size + param_step:size * 2)
    {
      int pack = 0;

      for (pack  = param_min_pack_number;
	   pack <= param_max_pack_number;
	   pack *= 2)
	{
	  const int         _length = (!size && param_no_zero)?1:size;
	  const int         _size   = _length / pack;
	  mad_ping_result_t results = { 0, 0, 0, 0 };
	  double            sum     = 0.0;
	  tbx_tick_t        t1;
	  tbx_tick_t        t2;

	  if (_size < MAD_LENGTH_ALIGNMENT)
	    goto skip;

	  if (param_one_way)
	    {
	      int nb_tests = param_nb_tests * param_nb_samples;

	      TBX_GET_TICK(t1);
	      mark_buffer(_length);

	      while (nb_tests--)
		{
		  p_mad_connection_t  out = NULL;
		  void               *ptr = main_buffer;
		  int                 i   = 0;

		  out = mad_begin_packing(channel, lrank_dst);
		  for (i = 0; i < pack; i++)
		    {
		      mad_pack(out, ptr, _size,
			       param_send_mode, param_receive_mode);
		      ptr += _size;
		    }
		  mad_end_packing(out);
		}

	      clear_buffer();
	      {
		p_mad_connection_t in = NULL;

		in = mad_begin_unpacking(channel);
		mad_unpack(in, main_buffer, _length,
			   param_send_mode, param_receive_mode);
		mad_end_unpacking(in);
		control_buffer(_length);
	      }
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
		      mark_buffer(_length);

		      {
			p_mad_connection_t  out = NULL;
			void               *ptr = main_buffer;
			int                 i   = 0;

			out = mad_begin_packing(channel, lrank_dst);
			for (i = 0; i < pack; i++)
			  {
			    mad_pack(out, ptr, _size,
				     param_send_mode,
				     param_receive_mode);
			    ptr += _size;
			  }
			mad_end_packing(out);
		      }

		      clear_buffer();

		      {
			p_mad_connection_t  in  = NULL;
			void               *ptr = main_buffer;
			int                 i   = 0;

			in = mad_begin_unpacking(channel);
			for (i = 0; i < pack; i++)
			  {
			    mad_unpack(in, ptr, _size,
				       param_send_mode,
				       param_receive_mode);
			    ptr += _size;
			  }
			mad_end_unpacking(in);
		      }

		      control_buffer(_length);
		    }
		  TBX_GET_TICK(t2);

		  sum += TBX_TIMING_DELAY(t1, t2);
		}
	    }

	skip:
	  results.lrank_src       = process_lrank;
	  results.lrank_dst       = lrank_dst;
	  results.size            = _length;
	  results.nb_pack         = pack;
	  results.send            = param_send_mode;
	  results.recv            = param_receive_mode;
	  results.dir             = param_one_way;
	  process_results(channel, &results);
	}
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
      int pack = 0;

      for (pack  = param_min_pack_number;
	   pack <= param_max_pack_number;
	   pack *= 2)
	{
	  const int _length = (!size && param_no_zero)?1:size;
	  const int _size   = _length / pack;

	  if (_size < MAD_LENGTH_ALIGNMENT)
	    goto skip;

	  if (param_one_way)
	    {
	      int nb_tests = param_nb_tests * param_nb_samples;

	      while (nb_tests--)
		{
		  p_mad_connection_t  in  = NULL;
		  void               *ptr = main_buffer;
		  int i = 0;

		  in = mad_begin_unpacking(channel);
		  for (i = 0; i < pack; i++)
		    {
		      mad_unpack(in, ptr, _size,
				 param_send_mode,
				 param_receive_mode);
		      ptr += _size;
		    }
		  mad_end_unpacking(in);
		}

	      {
		p_mad_connection_t out = NULL;

		out = mad_begin_packing(channel, lrank_dst);
		mad_pack(out, main_buffer, _length,
			 param_send_mode, param_receive_mode);
		mad_end_packing(out);
	      }
	    }
	  else
	    {
	      int nb_tests = param_nb_tests;

	      while (nb_tests--)
		{
		  int nb_samples = param_nb_samples;

		  while (nb_samples--)
		    {
		      {
			p_mad_connection_t  in  = NULL;
			void               *ptr = main_buffer;
			int                 i   = 0;

			in = mad_begin_unpacking(channel);
			for (i = 0; i < pack; i++)
			  {
			    mad_unpack(in, ptr, _size,
				       param_send_mode,
				       param_receive_mode);
			    ptr += _size;
			  }
			mad_end_unpacking(in);
		      }

		      {
			p_mad_connection_t  out = NULL;
			void               *ptr = main_buffer;
			int                 i   =    0;

			out = mad_begin_packing(channel, lrank_dst);
			for (i = 0; i < pack; i++)
			  {
			    mad_pack(out, ptr, _size,
				     param_send_mode,
				     param_receive_mode);
			    ptr += _size;
			  }
			mad_end_packing(out);
		      }
		    }
		}
	    }

	skip:
	  process_results(channel, NULL);
	}
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

  channel = tbx_htable_get(madeleine->channel_htable, name);
  if (!channel)
    {
      return;
    }

  pc = channel->pc;

  status = ntbx_pc_first_local_rank(pc, &master_lrank);
  if (!status)
    return;

  process_lrank = ntbx_pc_global_to_local(pc, process_grank);

  if (process_lrank == master_lrank)
    {
      // Master
      ntbx_process_lrank_t lrank_src = -1;

      LDISP("#### __________ channel <%s> __________", name);

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

	      if (!ntbx_pc_next_local_rank(pc, &lrank_dst))
		break;
	    }

	  do
	    {
	      if (lrank_dst == lrank_src)
		continue;

	      if (lrank_src == master_lrank)
		{
		  send_int_pair(channel, lrank_dst, lrank_src, 0);
		  receive_int(channel);

		  ping(channel, lrank_dst);
		}
	      else if (lrank_dst == master_lrank)
		{
		  send_int_pair(channel, lrank_src, lrank_dst, 1);
		  receive_int(channel);

		  pong(channel, lrank_src);
		}
	      else
		{
		  send_int_pair(channel, lrank_dst, lrank_src, 0);
		  receive_int(channel);

		  send_int_pair(channel, lrank_src, lrank_dst, 1);
		  receive_int(channel);

		  master_loop(channel);
		}
	    }
	  while (ntbx_pc_next_local_rank(pc, &lrank_dst));
	}
      while (ntbx_pc_next_local_rank(pc, &lrank_src));

      ntbx_pc_first_local_rank(pc, &lrank_src);

      while (ntbx_pc_next_local_rank(pc, &lrank_src))
	{
	  send_int_pair(channel, lrank_src, lrank_src, 0);
	}
    }
  else
    {
      /* Slaves */
      while (1)
	{
	  ntbx_process_lrank_t its_local_rank = -1;
	  int                  direction      =  0;

	  receive_int_pair(channel, &its_local_rank, &direction);

	  if (its_local_rank == process_lrank)
	    return;

	  send_int(channel, 0, 0);

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
marcel_main(int    argc,
            char **argv)
{
  p_mad_madeleine_t madeleine = NULL;
  p_mad_session_t   session   = NULL;
#ifndef STARTUP_ONLY
  p_tbx_slist_t     slist     = NULL;
#endif // STARTUP_ONLY

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);

  madeleine     = mad_get_madeleine();
  session       = madeleine->session;
  process_grank = session->process_rank;

#ifndef STARTUP_ONLY
 {
   int             i             = 0;
   mad_send_mode_t send_array[3] = {mad_send_SAFER,
				    mad_send_LATER,
				    mad_send_CHEAPER};

   for (i = 0; i < 3; i++)
    {
      int             j             = 0;
      mad_send_mode_t recv_array[2] = {mad_receive_EXPRESS,
				       mad_receive_CHEAPER};

      param_send_mode = send_array[i];

      for (j = 0; j < 2; j++)
	{
	  param_receive_mode = recv_array[j];

	  for (param_one_way = 0; param_one_way <= 1; param_one_way++)
	    {
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
	    }
	}
    }
 }
#endif // STARTUP_ONLY

  if (status)
    {
      LDISP("success");
    }
  else
    {
      LDISP("failure");
    }

  common_exit(NULL);

  return 0;
}
