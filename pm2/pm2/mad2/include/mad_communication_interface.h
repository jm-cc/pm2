
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
 * Mad_communication_interface.h
 * =============================
 */

#ifndef MAD_COMMUNICATION_INTERFACE_H
#define MAD_COMMUNICATION_INTERFACE_H

/*
 * Functions 
 * ---------
 */

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_message_ready(p_mad_channel_t channel);
#endif /* MAD_MESSAGE_POLLING */

#ifdef MAD_FORWARDING
p_mad_connection_t
mad_begin_packing(p_mad_user_channel_t   user_channel,
                  ntbx_host_id_t    remote_host_id);

p_mad_connection_t
mad_begin_unpacking(p_mad_user_channel_t user_channel);
#else /* MAD_FORWARDING */
p_mad_connection_t
mad_begin_packing(p_mad_channel_t   channel,
		  ntbx_host_id_t    remote_host_id);

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel);
#endif /* MAD_FORWARDING */

void
mad_end_packing(p_mad_connection_t connection);

void
mad_end_unpacking(p_mad_connection_t connection);

void
mad_pack(p_mad_connection_t   connection,
	 void                *buffer,
	 size_t               buffer_length,
	 mad_send_mode_t      send_mode,
	 mad_receive_mode_t   receive_mode
	 );

void
mad_unpack(p_mad_connection_t   connection,
	 void                  *buffer,
	 size_t                 buffer_length,
	 mad_send_mode_t        send_mode,
	 mad_receive_mode_t     receive_mode
	 );

#endif /* MAD_COMMUNICATION_INTERFACE_H */
