
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

typedef struct s_mad_channel
{
  TBX_SHARED;
  mad_channel_id_t          id ;
  p_mad_adapter_t           adapter;
#ifdef MAD_FORWARDING
  tbx_bool_t                is_forward;
  struct s_mad_user_channel *user_channel;
#endif //MAD_FORWARDING
  volatile tbx_bool_t       reception_lock;
  p_mad_connection_t        input_connection; 
  p_mad_connection_t        output_connection;
  p_mad_driver_specific_t   specific;
} mad_channel_t;

#ifdef MAD_FORWARDING
typedef struct s_mad_user_channel
{
  TBX_SHARED;
  p_mad_channel_t  *channels;
  p_mad_channel_t  *forward_channels;
  mad_adapter_id_t *adapter_ids;
  ntbx_host_id_t   *gateways;

  int               nb_active_channels;
  marcel_sem_t      sem_message_ready;
  marcel_sem_t      sem_lock_msgs;
  marcel_sem_t      sem_msg_handled;
  p_mad_connection_t msg_connection;

  char* name;
} mad_user_channel_t;
#endif //MAD_FORWARDING

#endif /* MAD_CHANNEL_H */
