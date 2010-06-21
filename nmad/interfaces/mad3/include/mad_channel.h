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
 * Mad_channel.h
 * =============
 */

#ifndef MAD_CHANNEL_H
#define MAD_CHANNEL_H

typedef enum e_mad_channel_type
{
  mad_channel_type_uninitialized = 0,
  mad_channel_type_regular,
  mad_channel_type_forwarding,
  mad_channel_type_virtual,
  mad_channel_type_mux,
} mad_channel_type_t, *p_mad_channel_type_t;


typedef struct s_mad_channel
{
  ntbx_process_lrank_t        process_lrank;
  mad_channel_type_t          type;
  mad_channel_id_t            id;
  char                       *name;
  p_ntbx_process_container_t  pc;
  tbx_bool_t                  not_private;
  tbx_bool_t                  mergeable; 
  p_mad_dir_channel_t         dir_channel;
  p_mad_dir_channel_t         cloned_dir_channel;
  p_mad_adapter_t             adapter;
  p_tbx_darray_t              in_connection_darray;
  p_tbx_darray_t              out_connection_darray;
  char                       *parameter;
#ifdef MARCEL
  marcel_mutex_t              reception_lock_mutex;
#else // MARCEL
  volatile tbx_bool_t         reception_lock;
#endif // MARCEL

  unsigned int                max_sub;
  unsigned int                sub;
  p_tbx_darray_t              sub_channel_darray;

  /* Forwarding/Mux only */
#ifdef MARCEL
  p_tbx_darray_t              sub_list_darray;
  unsigned int                max_mux;
  unsigned int                mux;
  p_tbx_darray_t              mux_list_darray;
  p_tbx_darray_t              mux_channel_darray;
  p_tbx_slist_t               channel_slist;
  p_tbx_slist_t               fchannel_slist;

  marcel_sem_t                ready_to_receive;
  marcel_sem_t                message_ready;
  marcel_sem_t                message_received;
  volatile tbx_bool_t         can_receive;
  volatile tbx_bool_t         a_message_is_ready;
  p_mad_connection_t          active_connection;
  marcel_t                    polling_thread;
#endif // MARCEL

  p_mad_driver_specific_t specific;
} mad_channel_t;

#endif /* MAD_CHANNEL_H */
