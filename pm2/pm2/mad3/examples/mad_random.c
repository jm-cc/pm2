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

#ifdef MARCEL

#define DISP_LEVEL 0
#define DATA_CHECK
//#define UNIFORM_RANDOM_LENGTH

#ifdef UNIFORM_RANDOM_LENGTH
#define MIN_BUF		4
#define MAX_BUF		(16*1024*1024)
#else
#define MIN_LENGTH_POWER 2
#define MAX_LENGTH_POWER 16
#endif
#define MAX_PACKS		4
#define NB_PACKS_OFFSET		4
#define MAX_MSGS		4
#define NB_MSGS_OFFSET		4
#define NB_SEND_MODES		 3
#define NB_RECEIVE_MODES	 2


#if 0
#define LOCK(L) do{DISP("%d:locking", __LINE__);marcel_mutex_lock((L));DISP("%d:locked", __LINE__);}while(0);
#define UNLOCK(L) do{DISP("%d:unlocking", __LINE__);marcel_mutex_unlock((L));DISP("%d:unlocked", __LINE__);}while(0);
#else
#define LOCK(L) do{marcel_mutex_lock((L));}while(0);
#define UNLOCK(L) do{marcel_mutex_unlock((L));}while(0);
#endif

#if (DISP_LEVEL >= 3)
#define DISP3(...) DISP(__VA_ARGS__)
#else
#define DISP3(...)
#endif

#if (DISP_LEVEL >= 2)
#define DISP2(...) DISP(__VA_ARGS__)
#else
#define DISP2(...)
#endif

#if (DISP_LEVEL >= 1)
#define DISP1(...) DISP(__VA_ARGS__)
#else
#define DISP1(...)
#endif

typedef struct s_listener
{
        p_mad_madeleine_t	 madeleine;
        char			*name;
        marcel_mutex_t		 channel_ready;
        p_tbx_darray_t		 receivers;
        volatile int		 nb_active_receivers;
        marcel_mutex_t		 active_receiver_lock;
        marcel_t		 thread;
} listener_t, *p_listener_t;

typedef struct s_receiver
{
        p_mad_madeleine_t	 madeleine;
        char			*name;
        ntbx_process_lrank_t     local_lrank;
        ntbx_process_lrank_t     remote_lrank;
        marcel_mutex_t           data_ready;
        p_listener_t 	 	 listener;
        p_mad_connection_t 	 connection;
        long int		 random_seed;
        marcel_t                 thread;
} receiver_t, *p_receiver_t;

typedef struct s_sender
{
        p_mad_madeleine_t	 madeleine;
        char			*name;
        ntbx_process_lrank_t     local_lrank;
        ntbx_process_lrank_t     remote_lrank;
        long int		 random_seed;
        marcel_t                 thread;
} sender_t, *p_sender_t;

#ifdef DATA_CHECK
typedef struct s_pack_info
{
        long int	 num;
        long int	 length;
        long int	 send_mode_value;
        long int	 receive_mode_value;
        void		*buffer;
        void		*check_buffer;
} pack_info_t, *p_pack_info_t;
#endif /* DATA_CHECK */

static ntbx_process_grank_t 	process_rank	= -1;
static long int			next_seed_val	=  0;

static mad_send_mode_t send_mode_array[] = {
        mad_send_SAFER,
        mad_send_LATER,
        mad_send_CHEAPER
};

static const char * send_mode_array_string[] = {
        "mad_send_SAFER",
        "mad_send_LATER",
        "mad_send_CHEAPER"
};

static mad_receive_mode_t receive_mode_array[] = {
        mad_receive_EXPRESS,
        mad_receive_CHEAPER
};

static const char * receive_mode_array_string[] = {
        "mad_receive_EXPRESS",
        "mad_receive_CHEAPER"
};

static
long int
rand48(struct drand48_data *p_rand_buffer,
       long int             max_value) {
        double  rand_value	= 0.0;
        int	rand_status	=   0;

        rand_status = drand48_r(p_rand_buffer, &rand_value);
        return (long int)(rand_value * max_value);
}

#ifndef UNIFORM_RANDOM_LENGTH
static
int
brand48(struct drand48_data *p_rand_buffer) {
        double  rand_value	= 0.0;
        int	rand_status	=   0;

        rand_status = drand48_r(p_rand_buffer, &rand_value);

        return (int)(rand_value < 0.5);
}
#endif /* UNIFORM_RANDOM_LENGTH */

static
void *
listener_thread(void *_arg)
{
        p_listener_t	arg	= _arg;
        p_mad_channel_t channel	= NULL;

        channel = tbx_htable_get(arg->madeleine->channel_htable, arg->name);
        if (!channel) {
                DISP1("listener: channel not found, leaving");
                return NULL;
        }

        while (1) {
                p_mad_connection_t	in		= NULL;
                ntbx_process_lrank_t	remote_lrank	= -1;
                p_receiver_t		receiver	= NULL;

                LOCK(&(arg->active_receiver_lock));
                if (!arg->nb_active_receivers)
                        break;

                UNLOCK(&(arg->active_receiver_lock));

                in		= mad_begin_unpacking(channel);
                remote_lrank	= in->remote_rank;
                DISP1("message from %d", remote_lrank);

                receiver	= tbx_darray_get(arg->receivers, remote_lrank);

                UNLOCK(&(receiver->data_ready));
                LOCK(&(arg->channel_ready));
        }

        DISP1("listener: leaving");
        return NULL;
}

static
void *
receiver_thread(void *_arg)
{
        p_receiver_t	arg		= _arg;
        int		rand_status	=    0;
        p_mad_channel_t channel		= NULL;
        p_mad_connection_t	in	= NULL;
        p_tbx_slist_t	buffer_slist	= NULL;
#ifdef DATA_CHECK
        p_tbx_slist_t	pack_info_slist		= NULL;
#endif /* DATA_CHECK */
        long int	i_msg		= 0;
        long int	nb_msgs		= 0;
        struct drand48_data rand_buffer;

        channel = tbx_htable_get(arg->madeleine->channel_htable, arg->name);
        if (!channel) {
                DISP1("receiver[%d]: channel not found, leaving", arg->remote_lrank);
                LOCK(&(arg->listener->active_receiver_lock));
                arg->listener->nb_active_receivers--;
                UNLOCK(&(arg->listener->active_receiver_lock));
                return NULL;
        }

        in = tbx_darray_get(channel->in_connection_darray, arg->remote_lrank);
        if (!in) {
                DISP1("receiver[%d]: connection not found, leaving", arg->remote_lrank);
                LOCK(&(arg->listener->active_receiver_lock));
                arg->listener->nb_active_receivers--;
                UNLOCK(&(arg->listener->active_receiver_lock));
                return NULL;
        }

        buffer_slist	= tbx_slist_nil();
#ifdef DATA_CHECK
        pack_info_slist		= tbx_slist_nil();
#endif /* DATA_CHECK */

        DISP1("receiver random seed: %d-->%d = %ld", arg->remote_lrank, arg->local_lrank, arg->random_seed);
        rand_status	= srand48_r(arg->random_seed, &rand_buffer);

        // rand: nb_msgs
        nb_msgs = rand48(&rand_buffer, MAX_MSGS) + NB_MSGS_OFFSET;
        DISP1("receiver nb messages: %d-->%d = %ld", arg->remote_lrank, arg->local_lrank, nb_msgs);
        for (i_msg = 0; i_msg < nb_msgs; i_msg++) {
                long int nb_packs	= 0;
                long int i_pack	= 0;

                // wait for a message
                LOCK(&(arg->data_ready));

                DISP("receiver new message %d-->%d, %ld remaining", arg->remote_lrank, arg->local_lrank, nb_msgs - i_msg - 1);

                // rand: nb_packs
                nb_packs = rand48(&rand_buffer, MAX_PACKS) + NB_PACKS_OFFSET;

                for (i_pack = 0; i_pack < nb_packs; i_pack++) {
                        long int 		length			= 0;
                        long int                send_mode_value		= 0;
                        long int		receive_mode_value	= 0;
                        mad_send_mode_t		send_mode		= mad_send_unknown;
                        mad_receive_mode_t	receive_mode		= mad_receive_unknown;

                        // rand: length
#ifdef UNIFORM_RANDOM_LENGTH
                        // rand: length
                        length = MIN_BUF+rand48(&rand_buffer, MAX_BUF-MIN_BUF+1);
#else /* UNIFORM_RANDOM_LENGTH */
                        {
                                long int len_pow = 0;

                                len_pow = MIN_LENGTH_POWER + rand48(&rand_buffer, MAX_LENGTH_POWER-MIN_LENGTH_POWER+1);
                                length = 1 << len_pow;

                                while (len_pow --) {
                                        if (brand48(&rand_buffer)) {
                                                length |= 1 << len_pow;
                                        }
                                }
                        }
#endif /* UNIFORM_RANDOM_LENGTH */

                        // rand: send mode
                        send_mode_value	= rand48(&rand_buffer, NB_SEND_MODES);
                        send_mode	= send_mode_array[send_mode_value];

                        // rand: receive mode
                        receive_mode_value	= rand48(&rand_buffer, NB_RECEIVE_MODES);
                        receive_mode	= receive_mode_array[receive_mode_value];

                        {
                                void *buffer = NULL;
#ifdef DATA_CHECK
                                p_pack_info_t pack_info	= NULL;
#endif /* DATA_CHECK */

                                buffer = TBX_MALLOC(length);
                                DISP2("new unpack: %ld bytes, %s, %s, %p", length, send_mode_array_string[send_mode_value], receive_mode_array_string[receive_mode_value], buffer);
#ifdef DATA_CHECK
                                pack_info = TBX_MALLOC(sizeof(pack_info_t));
                                pack_info->num		= i_pack;
                                pack_info->length	= length;
                                pack_info->send_mode_value	= send_mode_value;
                                pack_info->receive_mode_value	= receive_mode_value;
                                pack_info->buffer	= buffer;
                                pack_info->check_buffer	= TBX_MALLOC(length);

                                {
                                        long int i = 0;
                                        for (i = 0; i < length; i++) {
                                                // rand: buffer contents
                                                ((unsigned char *)pack_info->check_buffer)[i] = (unsigned char)rand48(&rand_buffer, 256);
                                        }
                                }
#endif /* DATA_CHECK */
                                mad_unpack(in, buffer, length, send_mode, receive_mode);
                                tbx_slist_append(buffer_slist, buffer);
#ifdef DATA_CHECK
                                if (receive_mode == mad_receive_EXPRESS) {
                                        {
                                                long int i 			= 0;
                                                long int cumulative_check	= 0;
                                                for (i = 0; i < length; i++) {
                                                        int check = 0;

                                                        check = ((unsigned char *)pack_info->buffer)[i] != ((unsigned char *)pack_info->check_buffer)[i];
                                                        cumulative_check	= cumulative_check || check;
                                                }

                                                if (cumulative_check) {
                                                        DISP("data check failed for message %d-->%d [%ld/%ld], pack %ld/%ld, %ld bytes, %s, %s", arg->remote_lrank, arg->local_lrank, i_msg+1, nb_msgs, i_pack+1, nb_packs, length, send_mode_array_string[send_mode_value], receive_mode_array_string[receive_mode_value]);

#if (DISP_LEVEL > 3)
                                                        for (i = 0; i < length; i++) {
                                                                unsigned char received = 0;
                                                                unsigned char expected = 0;

                                                                received = ((unsigned char *)pack_info->buffer)[i];
                                                                expected = ((unsigned char *)pack_info->check_buffer)[i];

                                                                DISP("%08lx: %02x (%02x expected)", i, received, expected);
                                                        }
#endif
                                                } else {
                                                        DISP3("data check successful for message %d-->%d [%ld/%ld], pack %ld/%ld, %ld bytes, %s, %s", arg->remote_lrank, arg->local_lrank, i_msg+1, nb_msgs, i_pack+1, nb_packs, length, send_mode_array_string[send_mode_value], receive_mode_array_string[receive_mode_value]);
                                                }
                                        }
                                        TBX_FREE(pack_info->check_buffer);
                                        TBX_FREE(pack_info);
                                } else {
                                        tbx_slist_append(pack_info_slist, pack_info);
                                }
#endif /* DATA_CHECK */
                        }
                }

                mad_end_unpacking(in);
                UNLOCK(&(arg->listener->channel_ready));

#ifdef DATA_CHECK
                while (!tbx_slist_is_nil(pack_info_slist)) {
                        p_pack_info_t pack_info	= NULL;

                        pack_info = tbx_slist_extract(pack_info_slist);

                        long int i 			= 0;
                        long int cumulative_check	= 0;

                        for (i = 0; i < pack_info->length; i++) {
                                int check = 0;

                                check = ((unsigned char *)pack_info->buffer)[i] != ((unsigned char *)pack_info->check_buffer)[i];
                                cumulative_check	= cumulative_check || check;
                        }

                        if (cumulative_check) {
                                DISP("data check failed for message %d-->%d [%ld/%ld], pack %ld/%ld, %ld bytes, %s, %s", arg->remote_lrank, arg->local_lrank, i_msg+1, nb_msgs, pack_info->num+1, nb_packs, pack_info->length, send_mode_array_string[pack_info->send_mode_value], receive_mode_array_string[pack_info->receive_mode_value]);

#if (DISP_LEVEL > 3)
                                for (i = 0; i < pack_info->length; i++) {
                                        unsigned char received = 0;
                                        unsigned char expected = 0;

                                        received = ((unsigned char *)pack_info->buffer)[i];
                                        expected = ((unsigned char *)pack_info->check_buffer)[i];

                                        DISP("%08lx: %02x (%02x expected)", i, received, expected);
                                }
#endif
                        } else {
                                DISP3("data check successful for message %d-->%d [%ld/%ld], pack %ld/%ld, %ld bytes, %s, %s", arg->remote_lrank, arg->local_lrank, i_msg+1, nb_msgs, pack_info->num+1, nb_packs, pack_info->length, send_mode_array_string[pack_info->send_mode_value], receive_mode_array_string[pack_info->receive_mode_value]);
                        }

                        TBX_FREE(pack_info->check_buffer);
                        TBX_FREE(pack_info);
                }
#endif /* DATA_CHECK */

                while (!tbx_slist_is_nil(buffer_slist)) {
                        void *buffer = NULL;
                        buffer = tbx_slist_extract(buffer_slist);
                        TBX_FREE(buffer);
                }
        }

        LOCK(&(arg->listener->active_receiver_lock));
        arg->listener->nb_active_receivers--;
        UNLOCK(&(arg->listener->active_receiver_lock));
        DISP1("receiver[%d]: leaving", arg->remote_lrank);
        return NULL;
}

static
void *
sender_thread(void *_arg)
{
        p_sender_t	arg		= _arg;
        int		rand_status	=    0;
        p_mad_channel_t channel		= NULL;
        p_mad_connection_t	out	= NULL;
        p_tbx_slist_t	buffer_slist	= NULL;
        long int	nb_msgs		= 0;
        struct drand48_data rand_buffer;

        channel = tbx_htable_get(arg->madeleine->channel_htable, arg->name);
        if (!channel) {
                return NULL;
        }

        out = tbx_darray_get(channel->out_connection_darray, arg->remote_lrank);
        if (!out) {
                return NULL;
        }


        buffer_slist	= tbx_slist_nil();
        DISP1("sender random seed: %d-->%d = %ld", arg->local_lrank, arg->remote_lrank, arg->random_seed);
        rand_status	= srand48_r(arg->random_seed, &rand_buffer);

        // rand: nb_msgs
        nb_msgs = rand48(&rand_buffer, MAX_MSGS) + NB_MSGS_OFFSET;
        DISP1("sender nb messages: %d-->%d = %ld", arg->local_lrank, arg->remote_lrank, nb_msgs);
        while (nb_msgs--) {
                long int nb_packs	= 0;
                long int i_pack		= 0;

                out = mad_begin_packing(channel, arg->remote_lrank);
                DISP2("sender new message %d-->%d, %ld remaining", arg->local_lrank, arg->remote_lrank, nb_msgs);

                // rand: nb_packs
                nb_packs = rand48(&rand_buffer, MAX_PACKS) + NB_PACKS_OFFSET;

                for (i_pack = 0; i_pack < nb_packs; i_pack++) {
                        long int 		length		= 0;
                        long int                send_mode_value		= 0;
                        long int		receive_mode_value	= 0;
                        mad_send_mode_t		send_mode	= mad_send_unknown;
                        mad_receive_mode_t	receive_mode	= mad_receive_unknown;

#ifdef UNIFORM_RANDOM_LENGTH
                        // rand: length
                         length = MIN_BUF+rand48(&rand_buffer, MAX_BUF-MIN_BUF+1);
#else /* UNIFORM_RANDOM_LENGTH */
                        {
                                long int len_pow = 0;

                                len_pow = MIN_LENGTH_POWER + rand48(&rand_buffer, MAX_LENGTH_POWER-MIN_LENGTH_POWER+1);
                                length = 1 << len_pow;

                                while (len_pow --) {
                                        if (brand48(&rand_buffer)) {
                                                length |= 1 << len_pow;
                                        }
                                }
                        }
#endif /* UNIFORM_RANDOM_LENGTH */

                        // rand: send mode
                        send_mode_value	= rand48(&rand_buffer, NB_SEND_MODES);
                        send_mode	= send_mode_array[send_mode_value];

                        // rand: receive mode
                        receive_mode_value	= rand48(&rand_buffer, NB_RECEIVE_MODES);
                        receive_mode	= receive_mode_array[receive_mode_value];

                        {
                                void *buffer = NULL;

                                buffer = TBX_MALLOC(length);
                                DISP2("new pack: %ld bytes, %s, %s, %p", length, send_mode_array_string[send_mode_value], receive_mode_array_string[receive_mode_value], buffer);
#ifdef DATA_CHECK
                                {
                                        long int i = 0;
                                        for (i = 0; i < length; i++) {
                                                // rand: buffer contents
                                                ((unsigned char *)buffer)[i] = (unsigned char)rand48(&rand_buffer, 256);
                                        }
                                }
#endif /* DATA_CHECK */
                                mad_pack(out, buffer, length, send_mode, receive_mode);
                                tbx_slist_append(buffer_slist, buffer);
                        }
                }

                mad_end_packing(out);

                while (!tbx_slist_is_nil(buffer_slist)) {
                        void *buffer = NULL;

                        buffer = tbx_slist_extract(buffer_slist);
                        TBX_FREE(buffer);
                }
        }

        return NULL;
}

static
void
test(p_mad_madeleine_t   madeleine,
     char               *name)
{
  p_mad_dir_channel_t		dir_channel	= NULL;
  p_ntbx_process_container_t	pc		= NULL;
  ntbx_process_lrank_t		my_lrank	=   -1;
  ntbx_process_lrank_t		src_lrank	=   -1;
  ntbx_process_lrank_t		dst_lrank	=   -1;
  p_listener_t			listener	= NULL;
  p_tbx_darray_t		receivers	= NULL;
  p_tbx_slist_t			receiver_slist	= NULL;
  p_tbx_slist_t			sender_slist	= NULL;
  p_ntbx_topology_table_t	ttable		= NULL;

  receivers	= tbx_darray_init();

  receiver_slist	= tbx_slist_nil();
  sender_slist		= tbx_slist_nil();

  listener	= TBX_MALLOC(sizeof(listener_t));

  listener->madeleine	= madeleine;
  listener->name	= name;
  listener->receivers	= receivers;
  listener->nb_active_receivers	= 0;

  marcel_mutex_init(&(listener->active_receiver_lock), NULL);
  marcel_mutex_init(&(listener->channel_ready), NULL);
  LOCK(&(listener->channel_ready));

  dir_channel	= tbx_htable_get(madeleine->dir->channel_htable, name);
  ttable	= dir_channel->ttable;
  pc		= dir_channel->pc;

  my_lrank = ntbx_pc_global_to_local(pc, process_rank);

  ntbx_pc_first_local_rank(pc, &src_lrank);
  do {
          ntbx_pc_first_local_rank(pc, &dst_lrank);
          do {
                  long int seed_val = next_seed_val ++;

                  if (!ttable || ntbx_topology_table_get(ttable, src_lrank, dst_lrank)) {
                          if (dst_lrank == my_lrank) {
                                  p_receiver_t receiver = NULL;

                                  listener->nb_active_receivers++;

                                  receiver = TBX_MALLOC(sizeof(receiver_t));
                                  receiver->madeleine		= madeleine;
                                  receiver->name		= name;
                                  receiver->local_lrank		= dst_lrank;
                                  receiver->remote_lrank	= src_lrank;
                                  receiver->listener		= listener;
                                  marcel_mutex_init(&(receiver->data_ready), NULL);
                                  LOCK(&(receiver->data_ready));
                                  receiver->random_seed	= seed_val;

                                  tbx_darray_expand_and_set(receivers, src_lrank, receiver);

                                  marcel_create(&receiver->thread, NULL, receiver_thread, receiver);
                                  tbx_slist_append(receiver_slist, receiver);
                          }

                          if (src_lrank == my_lrank) {
                                  p_sender_t sender	= NULL;

                                  sender = TBX_MALLOC(sizeof(sender_t));
                                  sender->madeleine	= madeleine;
                                  sender->name		= name;
                                  sender->local_lrank	= src_lrank;
                                  sender->remote_lrank	= dst_lrank;
                                  sender->random_seed	= seed_val;
                                  marcel_create(&sender->thread, NULL, sender_thread, sender);
                                  tbx_slist_append(sender_slist, sender);
                          }
                  }
          } while (ntbx_pc_next_local_rank(pc, &dst_lrank));

  } while (ntbx_pc_next_local_rank(pc, &src_lrank));

  marcel_create(&listener->thread, NULL, listener_thread, listener);

  marcel_join(listener->thread, NULL);
  TBX_FREE(listener);

  tbx_darray_free(receivers);
  receivers	= NULL;

  while (!tbx_slist_is_nil(receiver_slist)) {
          p_receiver_t receiver = NULL;

          receiver = tbx_slist_extract(receiver_slist);
          marcel_join(receiver->thread, NULL);
          TBX_FREE(receiver);
  }

  tbx_slist_free(receiver_slist);

  while (!tbx_slist_is_nil(sender_slist)) {
          p_sender_t sender = NULL;

          sender = tbx_slist_extract(sender_slist);
          marcel_join(sender->thread, NULL);
          TBX_FREE(sender);
  }
  tbx_slist_free(sender_slist);
}

int
main(int    argc,
     char **argv)
{
  p_mad_madeleine_t madeleine = NULL;
  p_mad_session_t   session   = NULL;
  p_tbx_slist_t     slist     = NULL;

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  TRACE("Returned from common_init");

  madeleine    = mad_get_madeleine();
  session      = madeleine->session;
  process_rank = session->process_rank;
  DISP_VAL("My global rank is", process_rank);
  DISP_VAL("The configuration size is",
	   tbx_slist_get_length(madeleine->dir->process_slist));
  DISP("----------");
  slist = madeleine->public_channel_slist;

  if (!tbx_slist_is_nil(slist))
    {

      tbx_slist_ref_to_head(slist);
      do
	{
	  char *name = NULL;

	  name = tbx_slist_ref_get(slist);
	  test(madeleine, name);
	  mad_leonie_barrier();
	  DISP("----------");
	}

      while (tbx_slist_ref_forward(slist));
    }
  else
    {
      DISP("No channels");
    }

  DISP("Exiting");

  common_exit(NULL);

  return 0;
}

#else	/* MARCEL */

int
main(int    argc,
     char **argv)
{
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  DISP("This program requires Marcel to be compiled in. Exiting.");
  common_exit(NULL);

  return 0;
}

#endif	/* MARCEL */
