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
 * Mad_udp.h
 * =========
 */
#ifndef MAD_UDP_H
#define MAD_UDP_H


/*
 * Mad_udp functions 
 * -----------------
 */

char *
mad_udp_register(p_mad_driver_interface_t);

void
mad_udp_driver_init(p_mad_driver_t, int *, char ***);

void
mad_udp_adapter_init(p_mad_adapter_t);

void
mad_udp_channel_init(p_mad_channel_t);

void
mad_udp_connection_init(p_mad_connection_t,
			p_mad_connection_t);

void
mad_udp_link_init(p_mad_link_t);

void
mad_udp_accept(p_mad_connection_t,
	       p_mad_adapter_info_t);

void
mad_udp_connect(p_mad_connection_t,
		       p_mad_adapter_info_t);

void
mad_udp_disconnect(p_mad_connection_t);

void
mad_udp_channel_exit(p_mad_channel_t);

void
mad_udp_adapter_exit(p_mad_adapter_t);

void
mad_udp_new_message(p_mad_connection_t);

void
mad_udp_finalize_message(p_mad_connection_t);

#if 0
#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_udp_poll_message(p_mad_channel_t);
#endif /* MAD_MESSAGE_POLLING */
#endif /* 0 */

p_mad_connection_t
mad_udp_receive_message(p_mad_channel_t);

void
mad_udp_message_received(p_mad_connection_t);

void
mad_udp_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);
  
void
mad_udp_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);

void
mad_udp_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);

void
mad_udp_receive_sub_buffer_group(p_mad_link_t,
				 tbx_bool_t,
				 p_mad_buffer_group_t);


#endif /* MAD_UDP_H */
