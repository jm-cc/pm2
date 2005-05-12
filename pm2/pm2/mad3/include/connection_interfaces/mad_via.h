
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
 * Mad_via.h
 * ---------
 */

#ifndef __MAD_VIA_H
#define __MAD_VIA_H

char *
mad_via_register(p_mad_driver_interface_t);

void
mad_via_driver_init(p_mad_driver_t);

void
mad_via_adapter_init(p_mad_adapter_t);

void
mad_via_channel_init(p_mad_channel_t);

void
mad_via_before_open_channel(p_mad_channel_t);

void
mad_via_connection_init(p_mad_connection_t,
			p_mad_connection_t);
  
void
mad_via_link_init(p_mad_link_t);
  
/* Point-to-point connection */
void
mad_via_accept(p_mad_connection_t,
	       p_mad_adapter_info_t);
  
void
mad_via_connect(p_mad_connection_t,
		p_mad_adapter_info_t);

/* Channel clean-up functions */
void
mad_via_after_open_channel(p_mad_channel_t);

void 
mad_via_before_close_channel(p_mad_channel_t);

/* Connection clean-up function */
void
mad_via_disconnect(p_mad_connection_t);
  
void 
mad_via_after_close_channel(p_mad_channel_t);

/* Deallocation functions */
void
mad_via_link_exit(p_mad_link_t);
  
void
mad_via_connection_exit(p_mad_connection_t,
			p_mad_connection_t);
  
void
mad_via_channel_exit(p_mad_channel_t);

void
mad_via_adapter_exit(p_mad_adapter_t);
  
void
mad_via_driver_exit(p_mad_driver_t);

/* Dynamic paradigm selection */
p_mad_link_t
mad_via_choice(p_mad_connection_t,
	       size_t,
	       mad_send_mode_t,
	       mad_receive_mode_t);

/* Static buffers management */
p_mad_buffer_t
mad_via_get_static_buffer(p_mad_link_t);

void
mad_via_return_static_buffer(p_mad_link_t, p_mad_buffer_t);

/* Message transfer */
void
mad_via_new_message(p_mad_connection_t);

void
mad_via_finalize_message(p_mad_connection_t);

p_mad_connection_t
mad_via_receive_message(p_mad_channel_t);

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_via_poll_message(p_mad_channel_t);
#endif /* MAD_MESSAGE_POLLING */
  
void
mad_via_message_received(p_mad_connection_t);

/* Buffer transfer */
void
mad_via_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);
  
void
mad_via_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);
  
/* Buffer group transfer */
void
mad_via_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);
  
void
mad_via_receive_sub_buffer_group(p_mad_link_t,
				 tbx_bool_t,
				 p_mad_buffer_group_t);


/* void mad_via_register(p_mad_driver_t driver); */
/* void mad_via_driver_init(p_mad_driver_t driver); */
/* void mad_via_adapter_init(p_mad_adapter_t adapter); */
/* void mad_via_adapter_configuration_init(p_mad_adapter_t adapter); */
/* void mad_via_channel_init(p_mad_channel_t); */
/* void mad_via_before_open_channel(p_mad_channel_t); */
/* void mad_via_connection_init(p_mad_connection_t, */
/* 			       p_mad_connection_t); */
/* void mad_via_link_init(p_mad_link_t); */
/* void mad_via_accept(p_mad_channel_t); */
/* void mad_via_connect(p_mad_connection_t); */
/* void mad_via_after_open_channel(p_mad_channel_t); */
/* void mad_via_before_close_channel(p_mad_channel_t); */
/* void mad_via_disconnect(p_mad_connection_t); */
/* void mad_via_after_close_channel(p_mad_channel_t); */
/* void mad_via_link_exit(p_mad_link_t); */
/* void mad_via_connection_exit(p_mad_connection_t, */
/* 			     p_mad_connection_t); */
/* void mad_via_channel_exit(p_mad_channel_t); */
/* void mad_via_adapter_exit(p_mad_adapter_t); */
/* void mad_via_driver_exit(p_mad_driver_t); */
/* p_mad_link_t mad_via_choice(p_mad_connection_t, */
/* 			      size_t, */
/* 			      mad_send_mode_t, */
/* 			      mad_receive_mode_t); */

/* void mad_via_new_message(p_mad_connection_t); */
/* p_mad_connection_t mad_via_receive_message(p_mad_channel_t); */
/* void mad_via_send_buffer(p_mad_link_t, */
/* 			   p_mad_buffer_t); */
/* void mad_via_receive_buffer(p_mad_link_t, */
/* 			      p_mad_buffer_t *); */
/* void mad_via_send_buffer_group(p_mad_link_t, */
/* 				 p_mad_buffer_group_t); */
/* void mad_via_receive_sub_buffer_group(p_mad_link_t, */
/* 					tbx_bool_t, */
/* 					p_mad_buffer_group_t); */
/* p_mad_buffer_t mad_via_get_static_buffer(p_mad_link_t); */
/* void mad_via_return_static_buffer(p_mad_link_t, */
/* 				    p_mad_buffer_t); */
#endif /* __MAD_VIA_H */
