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
 * mad_test.c
 * ==========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

// #include "madeleine.h"

// Macros
//......................

#ifdef MAX
#undef MAX
#endif

#define MAX 16

// Static variables
//......................

static ntbx_process_grank_t process_rank = -1;
static const char *const    token        = "token__";

// Functions
//......................

static
tbx_bool_t
play_with_channel(p_mad_madeleine_t  madeleine,
		  char              *name)
{
  p_mad_channel_t            channel        = NULL;
  p_ntbx_process_container_t pc             = NULL;
  ntbx_process_lrank_t       my_local_rank  =   -1;
  ntbx_process_lrank_t       its_local_rank =   -1;
  tbx_bool_t                 status         = tbx_false;
  ntbx_pack_buffer_t         buffer MAD_ALIGNED;

  channel = tbx_htable_get(madeleine->channel_htable, name);
  if (!channel)
    {
      return tbx_true;
    }

  pc = channel->pc;

  my_local_rank = ntbx_pc_global_to_local(pc, process_rank);

  if (my_local_rank)
    {
      p_tbx_string_t      string  = NULL;
      p_mad_connection_t  in      = NULL;
      p_mad_connection_t  out     = NULL;
      int                 len     =    0;
      int                 dyn_len =    0;
      char               *dyn_buf = NULL;
      char                buf[MAX] MAD_ALIGNED;

      memset(buf, 0, MAX);

#ifdef MAD_MESSAGE_POLLING
      while (!(in = mad_begin_unpacking(channel)));
#else // MAD_MESSAGE_POLLING
      in = mad_begin_unpacking(channel);
#endif // MAD_MESSAGE_POLLING

      mad_unpack(in, &buffer, sizeof(buffer),
		 mad_send_SAFER, mad_receive_EXPRESS);

      len = ntbx_unpack_int(&buffer);
      if ((len < 1) || (len > MAX))
	FAILURE("invalid message length");

      mad_unpack(in, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);
      mad_unpack(in, &buffer, sizeof(buffer),
		 mad_send_SAFER, mad_receive_EXPRESS);
      dyn_len = ntbx_unpack_int(&buffer);
      if (dyn_len < 1)
	FAILURE("invalid message length");
      dyn_buf = TBX_CALLOC(1, dyn_len);

      mad_unpack(in, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
      mad_end_unpacking(in);

      if (tbx_streq(buf, token))
	status = tbx_true;

      TBX_FREE(dyn_buf);
      dyn_buf = NULL;

      string  = tbx_string_init_to_cstring("the sender is ");
      tbx_string_append_int(string, my_local_rank);
      dyn_buf = tbx_string_to_cstring(string);
      tbx_string_free(string);
      string  = NULL;

      its_local_rank = my_local_rank;

      do
	{
	  if (ntbx_pc_next_local_rank(pc, &its_local_rank))
	    {
	      out = mad_begin_packing(channel, its_local_rank);
	    }
	  else
	    {
	      goto last;
	    }
	}
      while (!out);

      ntbx_pack_int(len, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);
      mad_pack(out, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);

      dyn_len = strlen(dyn_buf) + 1;
      ntbx_pack_int(dyn_len, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);
      mad_pack(out, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
      mad_end_packing(out);
      TBX_FREE(dyn_buf);
      dyn_buf = NULL;
      return status;

    last:
      ntbx_pc_first_local_rank(pc, &its_local_rank);
      out = mad_begin_packing(channel, its_local_rank);
      while (!out);
      {
	if (!ntbx_pc_next_local_rank(pc, &its_local_rank))
	  FAILURE("inconsistent error");
      }

      ntbx_pack_int(len, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);
      mad_pack(out, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);
      dyn_len = strlen(dyn_buf) + 1;
      ntbx_pack_int(dyn_len, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);
      mad_pack(out, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
      mad_end_packing(out);
      TBX_FREE(dyn_buf);
      dyn_buf = NULL;
      return status;
    }
  else
    {
      p_tbx_string_t      string  = NULL;
      p_mad_connection_t  in      = NULL;
      p_mad_connection_t  out     = NULL;
      char               *msg     = NULL;
      int                 len     =    0;
      int                 dyn_len =    0;
      char               *dyn_buf = NULL;
      tbx_bool_t          status  = tbx_false;
      char                buf[MAX] MAD_ALIGNED;

      msg = tbx_strdup(token);

      its_local_rank = my_local_rank;
      string  = tbx_string_init_to_cstring("the sender is ");
      tbx_string_append_int(string, my_local_rank);
      dyn_buf = tbx_string_to_cstring(string);
      tbx_string_free(string);
      string  = NULL;

      do
	{
	  if (ntbx_pc_next_local_rank(pc, &its_local_rank))
	    {
	      out = mad_begin_packing(channel, its_local_rank);
	    }
	  else
	    return tbx_true;
	}
      while (!out);

      len = strlen(msg) + 1;
      ntbx_pack_int(len, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);
      mad_pack(out, msg, len, mad_send_CHEAPER, mad_receive_CHEAPER);
      dyn_len = strlen(dyn_buf) + 1;
      ntbx_pack_int(dyn_len, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_SAFER, mad_receive_EXPRESS);
      mad_pack(out, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
      mad_end_packing(out);

      TBX_FREE(msg);
      msg = NULL;

      TBX_FREE(dyn_buf);
      dyn_buf = NULL;
      memset(buf, 0, MAX);

#ifdef MAD_MESSAGE_POLLING
      while (!(in = mad_begin_unpacking(channel)));
#else // MAD_MESSAGE_POLLING
      in = mad_begin_unpacking(channel);
#endif // MAD_MESSAGE_POLLING

      mad_unpack(in, &buffer, sizeof(buffer),
		 mad_send_SAFER, mad_receive_EXPRESS);

      len = ntbx_unpack_int(&buffer);
      if ((len < 1) || (len > MAX))
	FAILURE("invalid message length");

      mad_unpack(in, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);
      mad_unpack(in, &buffer, sizeof(buffer),
		 mad_send_SAFER, mad_receive_EXPRESS);
      dyn_len = ntbx_unpack_int(&buffer);
      if (dyn_len < 1)
	FAILURE("invalid message length");
      dyn_buf = TBX_CALLOC(1, dyn_len);

      mad_unpack(in, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
      mad_end_unpacking(in);

      if (tbx_streq(buf, token))
	status = tbx_true;

      TBX_FREE(dyn_buf);
      dyn_buf = NULL;

      return status;
    }
}

/*
 * Warning: this function is automatically renamed to marcel_main when
 * appropriate
 */
int
main(int    argc,
     char **argv)
{
  p_mad_madeleine_t madeleine = NULL;
  p_mad_session_t   session   = NULL;
  p_tbx_slist_t     slist     = NULL;
  tbx_bool_t        status    = tbx_true;

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);

  madeleine    = mad_get_madeleine();
  session      = madeleine->session;
  process_rank = session->process_rank;
  slist = madeleine->public_channel_slist;

  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);
      do
	{
	  char *name = NULL;

	  name = tbx_slist_ref_get(slist);
	  status &= play_with_channel(madeleine, name);

	  mad_leonie_barrier();
	}
      while (tbx_slist_ref_forward(slist));
    }

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
