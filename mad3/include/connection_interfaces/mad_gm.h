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
 * Mad_gm.h
 * =========
 */
#ifndef MAD_GM_H
#define MAD_GM_H

/*
 * Structures
 * ----------
 */

/*
 * Functions
 * ---------
 */


char *
mad_gm_register(p_mad_driver_interface_t);

void
mad_gm_driver_init(p_mad_driver_t d, int *, char ***);

void
mad_gm_adapter_init(p_mad_adapter_t a);

void
mad_gm_channel_init(p_mad_channel_t ch);

void
mad_gm_connection_init(p_mad_connection_t in,
		       p_mad_connection_t out);

void
mad_gm_link_init(p_mad_link_t l);

void
mad_gm_accept(p_mad_connection_t   in,
	      p_mad_adapter_info_t ai);

void
mad_gm_connect(p_mad_connection_t   out,
	       p_mad_adapter_info_t ai);
void
mad_gm_disconnect(p_mad_connection_t cnx);

void
mad_gm_new_message(p_mad_connection_t out);
#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_gm_poll_message(p_mad_channel_t channel);
#endif /*  MAD_MESSAGE_POLLING */

p_mad_connection_t
mad_gm_receive_message(p_mad_channel_t ch);

void
mad_gm_message_received(p_mad_connection_t in);

void
mad_gm_send_buffer(p_mad_link_t   l,
		   p_mad_buffer_t b);
void
mad_gm_receive_buffer(p_mad_link_t    l,
		      p_mad_buffer_t *b);


void
mad_gm_send_buffer_group(p_mad_link_t         l,
			 p_mad_buffer_group_t bg);
void
mad_gm_receive_sub_buffer_group(p_mad_link_t         l,
				tbx_bool_t TBX_UNUSED
                                first_sub_group,
				p_mad_buffer_group_t bg);

p_mad_buffer_t
mad_gm_get_static_buffer(p_mad_link_t l);

void
mad_gm_return_static_buffer(p_mad_link_t   l,
                            p_mad_buffer_t b);

void
mad_gm_link_exit(p_mad_link_t l);

void
mad_gm_connection_exit(p_mad_connection_t in,
                       p_mad_connection_t out);

void
mad_gm_channel_exit(p_mad_channel_t ch);

void
mad_gm_adapter_exit(p_mad_adapter_t a);

void
mad_gm_driver_exit(p_mad_driver_t d);

p_mad_link_t
mad_gm_choice(p_mad_connection_t connection,
              size_t             size,
              mad_send_mode_t    send_mode,
              mad_receive_mode_t receive_mode);

#endif /* MAD_GM_H */
