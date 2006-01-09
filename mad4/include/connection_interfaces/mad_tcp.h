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
 * Mad_tcp.h
 * =========
 */
#ifndef MAD_TCP_H
#define MAD_TCP_H

/*
 * Structures
 * ----------
 */

/*
 * Functions
 * ---------
 */

char *
mad_tcp_register(p_mad_driver_interface_t);

void
mad_tcp_driver_init(p_mad_driver_t, int *, char ***);

void
mad_tcp_adapter_init(p_mad_adapter_t);

void
mad_tcp_connection_init(p_mad_connection_t,
			p_mad_connection_t);

void
mad_tcp_track_set_init(p_mad_track_set_t);

void
mad_tcp_track_init(p_mad_adapter_t, uint32_t);

void
mad_tcp_accept(p_mad_connection_t,
	       p_mad_adapter_info_t);

void
mad_tcp_connect(p_mad_connection_t,
                p_mad_adapter_info_t);

void
mad_tcp_disconnect(p_mad_connection_t);

void
mad_tcp_connection_exit(p_mad_connection_t,
			p_mad_connection_t);

void
mad_tcp_channel_exit(p_mad_channel_t);

void
mad_tcp_adapter_exit(p_mad_adapter_t);

void
mad_tcp_driver_exit(p_mad_driver_t);

void
mad_tcp_close_track(p_mad_track_t);

void
mad_tcp_new_message(p_mad_connection_t);

p_mad_connection_t
mad_tcp_receive_message(p_mad_channel_t);

tbx_bool_t
mad_tcp_need_rdv(p_mad_iovec_t);

tbx_bool_t
mad_tcp_buffer_need_rdv(size_t);

//void
//mad_tcp_isend(p_mad_track_t,
//              ntbx_process_lrank_t,
//              struct iovec *, uint32_t);
//
//void
//mad_tcp_irecv(p_mad_track_t,
//              struct iovec *, uint32_t);
void
mad_tcp_isend(p_mad_track_t, p_mad_iovec_t);

void
mad_tcp_irecv(p_mad_track_t, p_mad_iovec_t);

//tbx_bool_t
//mad_tcp_s_test(p_mad_track_t);
tbx_bool_t
mad_tcp_s_test(p_mad_track_set_t);

p_mad_iovec_t
mad_tcp_r_test(p_mad_track_t);


void
mad_tcp_init_pre_posted(p_mad_adapter_t adapter,
                        p_mad_track_t track);

void
mad_tcp_replace_pre_posted(p_mad_adapter_t adapter,
                           p_mad_track_t track,
                           int port_id);

//void
//mad_tcp_add_pre_posted(p_mad_adapter_t, p_mad_track_t);
//mad_tcp_add_pre_posted(p_mad_adapter_t, p_mad_track_set_t);


void
mad_tcp_remove_all_pre_posted(p_mad_adapter_t);
#endif /* MAD_TCP_H */
