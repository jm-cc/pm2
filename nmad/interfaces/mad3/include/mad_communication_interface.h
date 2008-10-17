
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
#if defined CONFIG_SCHED_OPT
p_mad_connection_t
mad_nmad_begin_packing(p_mad_channel_t      ch,
                       ntbx_process_lrank_t remote_rank);

p_mad_connection_t
mad_nmad_begin_unpacking_from(p_mad_channel_t      ch,
                              ntbx_process_lrank_t remote_rank);

void
mad_nmad_end_packing(p_mad_connection_t out);

void
mad_nmad_end_unpacking(p_mad_connection_t in);

void
mad_nmad_pack(p_mad_connection_t   out,
              void                *user_buffer,
              size_t               user_buffer_length,
              mad_send_mode_t      send_mode,
              mad_receive_mode_t   receive_mode);

void
mad_nmad_unpack(p_mad_connection_t   in,
                void                *user_buffer,
                size_t               user_buffer_length,
                mad_send_mode_t      send_mode,
                mad_receive_mode_t   receive_mode);

#else
void
mad_free_parameter_slist(p_tbx_slist_t parameter_slist);

#  ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_message_ready(p_mad_channel_t channel);
#  endif /* MAD_MESSAGE_POLLING */

p_mad_connection_t
mad_begin_packing(p_mad_channel_t      channel,
		  ntbx_process_lrank_t remote_rank);

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel);

void
mad_end_packing(p_mad_connection_t connection);

void
mad_end_unpacking(p_mad_connection_t connection);

p_tbx_slist_t
mad_end_unpacking_ext(p_mad_connection_t connection);

void
mad_pack(p_mad_connection_t   connection,
	 void                *buffer,
	 size_t               buffer_length,
	 mad_send_mode_t      send_mode,
	 mad_receive_mode_t   receive_mode
	 );

void
mad_pack_ext(p_mad_connection_t   connection,
             void                *buffer,
             size_t               buffer_length,
             mad_send_mode_t      send_mode,
             mad_receive_mode_t   receive_mode,
             ...);

void
mad_unpack(p_mad_connection_t   connection,
	 void                  *buffer,
	 size_t                 buffer_length,
	 mad_send_mode_t        send_mode,
	 mad_receive_mode_t     receive_mode
	 );

p_tbx_slist_t
mad_unpack_ext(p_mad_connection_t   connection,
               void                  *buffer,
               size_t                 buffer_length,
               mad_send_mode_t        send_mode,
               mad_receive_mode_t     receive_mode);
#endif

#ifdef MARCEL
void
mad_forward_memory_manager_init(int    argc,
				char **argv);

void
mad_forward_memory_manager_exit(void);

TBX_INTERNAL char *
mad_forward_register(p_mad_driver_interface_t driver_interface);

TBX_INTERNAL void
mad_forward_driver_init(p_mad_driver_t   driver,
                        int              *argc,
                        char           ***argv);

TBX_INTERNAL void
mad_forward_channel_init(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_connection_init(p_mad_connection_t in,
			    p_mad_connection_t out);

TBX_INTERNAL void
mad_forward_link_init(p_mad_link_t lnk);

TBX_INTERNAL void
mad_forward_before_open_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_after_open_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_disconnect(p_mad_connection_t connection);

TBX_INTERNAL void
mad_forward_new_message(p_mad_connection_t connection);

TBX_INTERNAL void
mad_forward_finalize_message(p_mad_connection_t connection);

#ifdef MAD_MESSAGE_POLLING
TBX_INTERNAL p_mad_connection_t
mad_forward_poll_message(p_mad_channel_t channel);
#endif // MAD_MESSAGE_POLLING

TBX_INTERNAL p_mad_connection_t
mad_forward_receive_message(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_message_received(p_mad_connection_t connection);

TBX_INTERNAL void
mad_forward_before_close_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_after_close_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_link_exit(p_mad_link_t link);

TBX_INTERNAL void
mad_forward_connection_exit(p_mad_connection_t in,
			    p_mad_connection_t out);

TBX_INTERNAL void
mad_forward_channel_exit(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_driver_exit(p_mad_driver_t driver);

TBX_INTERNAL p_mad_link_t
mad_forward_choice(p_mad_connection_t connection,
		   size_t             size,
		   mad_send_mode_t    send_mode,
		   mad_receive_mode_t receive_mode);

TBX_INTERNAL p_mad_buffer_t
mad_forward_get_static_buffer(p_mad_link_t lnk);


TBX_INTERNAL void
mad_forward_return_static_buffer(p_mad_link_t   lnk,
				 p_mad_buffer_t buffer);

TBX_INTERNAL void
mad_forward_send_buffer(p_mad_link_t   lnk,
			p_mad_buffer_t buffer);

TBX_INTERNAL void
mad_forward_receive_buffer(p_mad_link_t    lnk,
			   p_mad_buffer_t *buffer);

TBX_INTERNAL void
mad_forward_send_buffer_group(p_mad_link_t         lnk,
			      p_mad_buffer_group_t buffer_group);

TBX_INTERNAL void
mad_forward_receive_sub_buffer_group(p_mad_link_t         lnk,
				     tbx_bool_t __attribute__ ((unused)) first_sub_group,
				     p_mad_buffer_group_t buffer_group);

TBX_INTERNAL void
mad_forward_stop_direct_retransmit(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_stop_indirect_retransmit(p_mad_channel_t channel);

TBX_INTERNAL void
mad_forward_stop_reception(p_mad_channel_t      vchannel,
			   p_mad_channel_t      channel,
			   ntbx_process_lrank_t lrank);


void
mad_mux_memory_manager_init(int    argc,
				char **argv);

void
mad_mux_memory_manager_exit(void);

TBX_INTERNAL char *
mad_mux_register(p_mad_driver_interface_t interface);

TBX_INTERNAL void
mad_mux_driver_init(p_mad_driver_t driver,
                    int              *argc,
                    char           ***argv);

TBX_INTERNAL void
mad_mux_channel_init(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_connection_init(p_mad_connection_t in,
			    p_mad_connection_t out);

TBX_INTERNAL void
mad_mux_link_init(p_mad_link_t lnk);

TBX_INTERNAL void
mad_mux_before_open_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_after_open_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_disconnect(p_mad_connection_t connection);

TBX_INTERNAL void
mad_mux_new_message(p_mad_connection_t connection);

TBX_INTERNAL void
mad_mux_finalize_message(p_mad_connection_t connection);

#ifdef MAD_MESSAGE_POLLING
TBX_INTERNAL p_mad_connection_t
mad_mux_poll_message(p_mad_channel_t channel);
#endif // MAD_MESSAGE_POLLING

TBX_INTERNAL p_mad_connection_t
mad_mux_receive_message(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_message_received(p_mad_connection_t connection);

TBX_INTERNAL void
mad_mux_before_close_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_after_close_channel(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_link_exit(p_mad_link_t link);

TBX_INTERNAL void
mad_mux_connection_exit(p_mad_connection_t in,
			    p_mad_connection_t out);

TBX_INTERNAL void
mad_mux_channel_exit(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_driver_exit(p_mad_driver_t driver);

TBX_INTERNAL p_mad_link_t
mad_mux_choice(p_mad_connection_t connection,
		   size_t             size,
		   mad_send_mode_t    send_mode,
		   mad_receive_mode_t receive_mode);

TBX_INTERNAL p_mad_buffer_t
mad_mux_get_static_buffer(p_mad_link_t lnk);


TBX_INTERNAL void
mad_mux_return_static_buffer(p_mad_link_t   lnk,
				 p_mad_buffer_t buffer);

TBX_INTERNAL void
mad_mux_send_buffer(p_mad_link_t   lnk,
			p_mad_buffer_t buffer);

TBX_INTERNAL void
mad_mux_receive_buffer(p_mad_link_t    lnk,
			   p_mad_buffer_t *buffer);

TBX_INTERNAL void
mad_mux_send_buffer_group(p_mad_link_t         lnk,
			      p_mad_buffer_group_t buffer_group);

TBX_INTERNAL void
mad_mux_receive_sub_buffer_group(p_mad_link_t         lnk,
                                 tbx_bool_t __attribute__ ((unused))
                                 first_sub_group,
                                 p_mad_buffer_group_t buffer_group);

TBX_INTERNAL void
mad_mux_stop_indirect_retransmit(p_mad_channel_t channel);

TBX_INTERNAL void
mad_mux_stop_reception(p_mad_channel_t      xchannel,
		       p_mad_channel_t      channel,
		       ntbx_process_lrank_t lrank);

TBX_INTERNAL p_mad_channel_t
mad_mux_get_sub_channel(p_mad_channel_t xchannel,
			unsigned int    sub);

TBX_INTERNAL void
mad_mux_add_named_sub_channels(p_mad_channel_t xchannel);

#endif // MARCEL

#endif /* MAD_COMMUNICATION_INTERFACE_H */
