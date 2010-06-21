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
 * Mad_connection.h
 * ================
 */

#ifndef MAD_CONNECTION_H
#define MAD_CONNECTION_H

/*
 * Structures / enums
 * -------------------
 */
typedef enum e_mad_connection_nature
{
  mad_connection_nature_uninitialized = 0,
  mad_connection_nature_regular,
  mad_connection_nature_direct_virtual,
  mad_connection_nature_indirect_virtual,
  mad_connection_nature_mux,
} mad_connection_nature_t, *p_mad_connection_nature_t;

typedef enum e_mad_connection_way
{
  mad_unknown_connection = 0,
  mad_incoming_connection,
  mad_outgoing_connection,
} mad_connection_way_t, *p_mad_connection_way_t;

typedef struct s_mad_connection
{
  /* Common use fields */
  mad_connection_nature_t  nature;
  ntbx_process_lrank_t     remote_rank;
  p_mad_channel_t          channel;
  p_mad_connection_t       reverse;
  mad_connection_way_t     way;
  tbx_bool_t               connected;
  p_mad_connection_t       regular;
  char                    *parameter;

  /* Forwarding connections only */
#ifdef MARCEL
  marcel_t                 forwarding_thread;
  p_tbx_slist_t            forwarding_block_list;
  marcel_sem_t             something_to_forward;
  p_tbx_darray_t           block_queues;
  p_tbx_slist_t            message_queue;
  volatile tbx_bool_t      closing;

  /* Virtual connections only */
  unsigned int             mtu;
  tbx_bool_t               new_msg;
  unsigned int             first_block_length;
  unsigned int             first_block_is_a_group;
#ifdef MAD_FORWARD_FLOW_CONTROL
  marcel_sem_t             ack;
#endif // MAD_FORWARD_FLOW_CONTROL
#endif // MARCEL

  /* Internal use fields */
  int                      nb_link;
  p_mad_link_t            *link_array;
  p_tbx_list_reference_t   user_buffer_list_reference;
  p_tbx_list_t             user_buffer_list;
  p_tbx_list_t             buffer_list;
  p_tbx_list_t             buffer_group_list;
  p_tbx_list_t             pair_list;
  p_tbx_slist_t            parameter_slist;
  p_mad_link_t             last_link;
  mad_link_mode_t          last_link_mode;

  /* Flags */
#ifdef MARCEL
  marcel_mutex_t           lock_mutex;
#else // MARCEL
  volatile tbx_bool_t      lock;
#endif // MARCEL
  tbx_bool_t               delayed_send;
  tbx_bool_t               flushed;
  tbx_bool_t               pair_list_used;
  tbx_bool_t               first_sub_buffer_group;
  tbx_bool_t               more_data;

  /* Driver specific data */
  p_mad_driver_specific_t  specific;
} mad_connection_t;

#endif /* MAD_CONNECTION_H */
