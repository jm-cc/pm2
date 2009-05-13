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
 * mad_mini.c
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

// Maximum number of anonymous subchannels to test
#define MAX_SUB 8

// Buffer size
#define BUFFER_SIZE 16000

// Display of argv contents at program startup
//#define ECHO_ARGS

// Static variables
//......................

/* The globally unique rank of the process in the session */
static ntbx_process_grank_t process_rank = -1;


// Functions
//......................

static
p_mad_connection_t
select_next_process_connection(p_mad_channel_t      channel,
                               ntbx_process_lrank_t my_local_rank) {
  p_ntbx_process_container_t pc             = NULL;
  p_mad_connection_t         out            = NULL;
  ntbx_process_lrank_t       its_local_rank = my_local_rank;
  tbx_bool_t                 looped         = tbx_false;

  pc = channel->pc;

  do
    {
    again:
      if (!ntbx_pc_next_local_rank(pc, &its_local_rank)) {
        ntbx_pc_first_local_rank(pc, &its_local_rank);
        looped = tbx_true;
      }

      if (its_local_rank == my_local_rank) {
        goto again;
      }

      out = mad_begin_packing(channel, its_local_rank);
    }
  while (!out && !looped);

  return out;
}

static
char *
build_sender_string(ntbx_process_lrank_t my_local_rank)
{
  p_tbx_string_t  string = NULL;
  char           *s      = NULL;

  string  = tbx_string_init_to_cstring("the sender is ");
  tbx_string_append_int(string, my_local_rank);
  s = tbx_string_to_cstring(string);
  tbx_string_free(string);

  return s;
}

static
void
send_token(p_mad_channel_t       channel,
           ntbx_process_lrank_t  my_local_rank,
           char                 *buf,
           char                 *dyn_buf)
{
  /* The buffer for converting data types to a heterogeneity-safe
     representation before network transmission */
  ntbx_pack_buffer_t pb MAD_ALIGNED;
  p_mad_connection_t out     = NULL;
  int                len     =    0;
  int                dyn_len =    0;

  out = select_next_process_connection(channel, my_local_rank);

  if (!out)
    TBX_FAILURE("invalid state");

  /* Make sure the buffer is fully initialized before we use it, in
     order to avoid 'Valgrind' complaining */
  memset(&pb, 0, sizeof(pb));

  /* Send the first string */
  len = strlen(buf);

  ntbx_pack_int(len, &pb);
  mad_pack(out, &pb, sizeof(pb),
           mad_send_SAFER, mad_receive_EXPRESS);
  mad_pack(out, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);

  /* Send the second string */
  dyn_len = strlen(dyn_buf) + 1;
  ntbx_pack_int(dyn_len, &pb);
  mad_pack(out, &pb, sizeof(pb),
           mad_send_SAFER, mad_receive_EXPRESS);
  mad_pack(out, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
  mad_end_packing(out);
}

static
void
receive_token(p_mad_channel_t   channel,
              char             *buf,
              char            **p_dyn_buf)
{
  /* The buffer for converting data types to a heterogeneity-safe
     representation before network transmission */
  ntbx_pack_buffer_t  pb MAD_ALIGNED;
  p_mad_connection_t  in      = NULL;
  int                 len     =    0;
  int                 dyn_len =    0;
  char               *dyn_buf = NULL;

#ifdef MAD_MESSAGE_POLLING
          /* Show the use of the 'message polling' feature. */
	  while (!(in = mad_message_ready(channel)));
#else // MAD_MESSAGE_POLLING
          /* Regular */
	  in = mad_begin_unpacking(channel);
#endif // MAD_MESSAGE_POLLING

  /* Make sure the buffer is fully initialized before we use it, in
     order to avoid 'Valgrind' complaining */
  memset(&pb, 0, sizeof(pb));

  /* Get the first string */
  mad_unpack(in, &pb, sizeof(pb), mad_send_SAFER, mad_receive_EXPRESS);
  len = ntbx_unpack_int(&pb);
  if ((len < 1) || (len > BUFFER_SIZE))
    TBX_FAILURE("invalid message length");

  mad_unpack(in, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);

  /* Get the second string */
  mad_unpack(in, &pb, sizeof(pb), mad_send_SAFER, mad_receive_EXPRESS);
  dyn_len = ntbx_unpack_int(&pb);
  if (dyn_len < 1)
    TBX_FAILURE("invalid message length");
  dyn_buf = TBX_CALLOC(1, dyn_len);

  mad_unpack(in, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);

  /* Flush any pending receive_CHEAPER */
  mad_end_unpacking(in);

  *p_dyn_buf = dyn_buf;
}

static
void
play_with_channel(p_mad_madeleine_t  madeleine,
		  char              *name)
{
  /* The current channel */
  p_mad_channel_t            channel        = NULL;

  /* The set of processes in the current channel */
  p_ntbx_process_container_t pc             = NULL;

  /* The 'local' rank of the process in the current channel */
  ntbx_process_lrank_t       my_local_rank  =   -1;

  /* The first 'local' rank in the channel which plays the role of the
     master */
  ntbx_process_lrank_t       first_local_rank =   -1;

  /* The current sub-channel number we are using */
  int                        sub            =    0;

  /* Tell the user which channel we are currently using */
  DISP_STR("Channel", name);

  /* Get a reference to the corresponding channel structure */
  channel = tbx_htable_get(madeleine->channel_htable, name);

  /* If that fails, it means that our process does not belong to the channel */
  if (!channel)
    {
      DISP("I don't belong to this channel");
      return;
    }

  /* Get the set of processes in the channel */
  pc = channel->pc;

  /* Convert the globally unique process rank to its _local_ rank
     in the current channel */
  my_local_rank = ntbx_pc_global_to_local(pc, process_rank);
  DISP_VAL("My local channel rank is", my_local_rank);

  /* Who's the master here ? */
  ntbx_pc_first_local_rank(pc, &first_local_rank);
  DISP_VAL("The master is", first_local_rank);

  {
    tbx_darray_index_t idx = 0;

    if (!tbx_darray_first_idx(channel->in_connection_darray, &idx)
        || (idx == my_local_rank && !tbx_darray_next_idx(channel->in_connection_darray, &idx)))
      {
        TBX_FAILURE("The ring is broken, "
                "I do not have any incoming connection on this channel");
      }

    if (!tbx_darray_first_idx(channel->out_connection_darray, &idx)
        || (idx == my_local_rank && !tbx_darray_next_idx(channel->out_connection_darray, &idx)))
      {
        TBX_FAILURE("The ring is broken, "
                "I do not have any outgoing connection on this channel");
      }
  }

  /* If the channel provides several multiplexed sub-channels, let's
     play with them, just for fun, ... but do not play to much */
  for (sub = 0; sub < ((tbx_min(channel->max_sub, MAX_SUB))?:1); sub++)
    {
      /* sub==0 is the channel itself, so we only call get_sub_channel
         for 'true' sub_channels */
      if (sub)
	{
	  channel = mad_get_sub_channel(channel, sub);
	  DISP_VAL("subchannel", sub);
	}

     if (my_local_rank != first_local_rank)
	{
          /* Not the master */
          char *dyn_buf = NULL;
          char  buf[BUFFER_SIZE] MAD_ALIGNED;

          /* receive */
	  memset(buf, 0, BUFFER_SIZE);
          receive_token(channel, buf, &dyn_buf);

	  DISP_STR("Hi, I just received", buf);
	  DISP_STR("Source", dyn_buf);

	  TBX_FREE(dyn_buf);
	  dyn_buf = NULL;

          /* send */
          dyn_buf = build_sender_string(my_local_rank);
          send_token(channel, my_local_rank, buf, dyn_buf);

	  TBX_FREE(dyn_buf);
	  dyn_buf = NULL;
	}
      else
	{
          /* The master */
	  char *msg     = NULL;
	  char *dyn_buf = NULL;
	  char  buf[BUFFER_SIZE] MAD_ALIGNED;

          /* send */
	  msg = tbx_strdup("token__");
          dyn_buf = build_sender_string(my_local_rank);
          send_token(channel, my_local_rank, msg, dyn_buf);

	  TBX_FREE(dyn_buf);
	  dyn_buf = NULL;

	  TBX_FREE(msg);
	  msg = NULL;

          /* receive */
	  memset(buf, 0, BUFFER_SIZE);
          receive_token(channel, buf, &dyn_buf);

	  DISP_STR("Hi, I just received", buf);
	  DISP_STR("Source", dyn_buf);

	  TBX_FREE(dyn_buf);
	  dyn_buf = NULL;
	}

      DISP("End of iteration");
    }
}

#ifdef ECHO_ARGS
static
void
disp_args(int argc, char **argv)
{
  argc --; argv ++;

  while (argc--)
    {
      DISP_STR("argv", *argv);
      argv++;
    }
}
#endif // ECHO_ARGS

void* thr_func(void* arg)
{

	int a=100;
  	p_mad_madeleine_t madeleine = NULL;

	while(a--) {
  		madeleine    = mad_get_madeleine();
		marcel_yield();
	}

	return NULL;
}

void do_calcul(void* arg)
{

  	p_mad_madeleine_t madeleine = NULL;
	volatile int *a=arg;
	*a += 1;
	madeleine    = mad_get_madeleine();
	while(*a!=-1) {
		;
	}
	madeleine    = mad_get_madeleine();

}

void* wait_func(void* arg)
{

	do_calcul(arg);
	return NULL;
}


void*communication(void* arg)
{
  p_mad_madeleine_t madeleine = NULL;
  p_mad_session_t   session   = NULL;
  p_tbx_slist_t     slist     = NULL;

  volatile int*a=arg;

  /*
   * Reference to the Madeleine object.
   */
  madeleine    = mad_get_madeleine();

  /*
   * Reference to the session information object
   */
  session      = madeleine->session;

  /*
   * Globally unique process rank.
   */
  process_rank = session->process_rank;
  DISP_VAL("My global rank is", process_rank);

  if (process_rank == 0) {
	  marcel_delay(2000);
	  while(*a!=3) ;
  }

  /*
   * How to obtain the configuration size.
   */
  DISP_VAL("The configuration size is",
	   tbx_slist_get_length(madeleine->dir->process_slist));

  DISP("----------");

  /*
   * Reference to the list of the _names_ of channels that are
   * available to the application.
   */
  slist = madeleine->public_channel_slist;

  /* If the channel name list is not empty... */
  if (!tbx_slist_is_nil(slist))
    {
      /* Put the internal reference of the list at the beginning */
      tbx_slist_ref_to_head(slist);
      do
	{
	  char *name = NULL;

          /*
           * Get the object which is currently referenced by the
           * internal list reference. This is a channel name.
           */
	  name = tbx_slist_ref_get(slist);

          /*
           * Let's do something with this channel.
           */
	  play_with_channel(madeleine, name);

          /*
           * Synchronize every node using Leonie.
           */
	  mad_leonie_barrier();
	  DISP("----------");
	}
      /* Move the reference to the next object in the list or end the loop */
      while (tbx_slist_ref_forward(slist));
    }
  else
    {
      /*
       * If we are here, then the configuration file did not define
       * any channel
       */
      DISP("No channels");
    }
  return NULL;
}


/*
 * Warning: this function is automatically renamed to marcel_main when
 * appropriate
 */
int
main(int    argc,
     char **argv)
{
  marcel_t calcul1, calcul2, calcul3, calcul4, comm;
  int stop=0;
  p_mad_madeleine_t madeleine = NULL;
  p_mad_session_t   session   = NULL;
  marcel_attr_t attr;

  
#ifdef ECHO_ARGS
  disp_args(argc, argv);
#endif // ECHO_ARGS

  /*
   * Initialization of various libraries.
   */
#ifdef PROFILE
  profile_activate(FUT_SETMASK, MARCEL_PROF_MASK|MAD_PROF_MASK|
		  FUT_GCC_INSTRUMENT_KEYMASK, 0);
#endif

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  TRACE("Returned from common_init");

  /*
   * Reference to the Madeleine object.
   */
  madeleine    = mad_get_madeleine();

  DISP_VAL("Madeleine", madeleine);
  /*
   * Reference to the session information object
   */
  session      = madeleine->session;

  /*
   * Globally unique process rank.
   */
  process_rank = session->process_rank;

  marcel_attr_init(&attr);
  marcel_attr_setname(&attr, "communication");
  marcel_attr_setprio(&attr, MA_RT_PRIO);
  marcel_create(&comm, &attr, communication, &stop);

  marcel_yield();

  if (process_rank==0) {
	  marcel_attr_init(&attr);
	  marcel_attr_setname(&attr, "calcul1");
	  marcel_create(&calcul1, &attr, wait_func, &stop);

	  marcel_attr_init(&attr);
	  marcel_attr_setname(&attr, "calcul2");
	  marcel_create(&calcul2, &attr, wait_func, &stop);

	  marcel_attr_init(&attr);
	  marcel_attr_setname(&attr, "calcul3");
	  marcel_create(&calcul3, &attr, wait_func, &stop);

	  marcel_attr_init(&attr);
	  marcel_attr_setname(&attr, "calcul4");
	  //marcel_create(&calcul4, &attr, wait_func, &stop);
  }


  marcel_join(comm, NULL);
  if (process_rank==0) {
	  stop=-1;
	  marcel_join(calcul1, NULL);
	  marcel_join(calcul2, NULL);
	  marcel_join(calcul3, NULL);
	  //marcel_join(calcul4, NULL);
  }
  DISP("Exiting");

#ifdef PROFILE
  profile_stop();
#endif

  // mad_exit(madeleine);
  /* Clean-up (almost...) everything.  */
  common_exit(NULL);

  return 0;
}
