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
 * Mad_mx.h
 * =========
 */
#ifndef MAD_MX_H
#define MAD_MX_H

/*
 * Structures
 * ----------
 */

/*
 * Functions
 * ---------
 */

void
mad_mx_register(p_mad_driver_t);

void
mad_mx_driver_init(p_mad_driver_t);

void
mad_mx_adapter_init(p_mad_adapter_t);

void
mad_mx_adapter_configuration_init(p_mad_adapter_t);

void
mad_mx_channel_init(p_mad_channel_t);

void
mad_mx_connection_init(p_mad_connection_t,
			p_mad_connection_t);

void
mad_mx_link_init(p_mad_link_t);

void
mad_mx_accept(p_mad_connection_t,
	       p_mad_adapter_info_t);

void
mad_mx_connect(p_mad_connection_t,
	       p_mad_adapter_info_t);

void
mad_mx_disconnect(p_mad_connection_t);

void
mad_mx_link_exit(p_mad_link_t);
void
mad_mx_connection_exit(p_mad_connection_t,
                       p_mad_connection_t);

void
mad_mx_channel_exit(p_mad_channel_t);

void
mad_mx_adapter_exit(p_mad_adapter_t);

void
mad_mx_driver_exit(p_mad_driver_t);

p_mad_link_t
mad_mx_choice(p_mad_connection_t,
	       size_t,
	       mad_send_mode_t,
	       mad_receive_mode_t);

void
mad_mx_new_message(p_mad_connection_t);

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_mx_poll_message(p_mad_channel_t);
#endif /* MAD_MESSAGE_POLLING */

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t);

void
mad_mx_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);

void
mad_mx_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);

void
mad_mx_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);

void
mad_mx_receive_sub_buffer_group(p_mad_link_t,
				 tbx_bool_t,
				 p_mad_buffer_group_t);


#endif /* MAD_MX_H */
