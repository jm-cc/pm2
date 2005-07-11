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

//void
//mad_mx_register(p_mad_driver_t);
//
//void
//mad_mx_driver_init(p_mad_driver_t);

char *
mad_mx_register(p_mad_driver_interface_t);

void
mad_mx_driver_init(p_mad_driver_t, int *, char ***);

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

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_mx_poll_message(p_mad_channel_t);
#endif /* MAD_MESSAGE_POLLING */

void
mad_mx_new_message(p_mad_connection_t);

void
mad_mx_finalize_message(p_mad_connection_t);

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t);

void
mad_mx_send_iovec(p_mad_link_t,
                  p_mad_pipeline_t,
                  p_mad_on_going_t);

void
mad_mx_receive_iovec(p_mad_link_t,
                     p_mad_on_going_t);

void
mad_mx_s_make_progress(p_mad_adapter_t);

void
mad_mx_r_make_progress(p_mad_adapter_t, p_mad_channel_t);

tbx_bool_t
mad_mx_need_rdv(p_mad_iovec_t);

unsigned int
mad_mx_limit_size(void);

void
mad_mx_create_areas(p_mad_iovec_t);

void
mad_mx_sending_track_init(p_mad_adapter_t);

void
mad_mx_reception_track_init(p_mad_adapter_t);

void
mad_mx_sending_track_exit(p_mad_adapter_t);

void
mad_mx_reception_track_exit(p_mad_adapter_t);

#endif /* MAD_MX_H */
