
/*
 * Mad_via.h
 * ---------
 */

#ifndef __MAD_VIA_H
#define __MAD_VIA_H

void mad_via_register(p_mad_driver_t driver);
void mad_via_driver_init(p_mad_driver_t driver);
void mad_via_adapter_init(p_mad_adapter_t adapter);
void mad_via_adapter_configuration_init(p_mad_adapter_t adapter);
void mad_via_channel_init(p_mad_channel_t);
void mad_via_before_open_channel(p_mad_channel_t);
void mad_via_connection_init(p_mad_connection_t,
			       p_mad_connection_t);
void mad_via_link_init(p_mad_link_t);
void mad_via_accept(p_mad_channel_t);
void mad_via_connect(p_mad_connection_t);
void mad_via_after_open_channel(p_mad_channel_t);
void mad_via_before_close_channel(p_mad_channel_t);
void mad_via_disconnect(p_mad_connection_t);
void mad_via_after_close_channel(p_mad_channel_t);
void mad_via_link_exit(p_mad_link_t);
void mad_via_connection_exit(p_mad_connection_t,
			     p_mad_connection_t);
void mad_via_channel_exit(p_mad_channel_t);
void mad_via_adapter_exit(p_mad_adapter_t);
void mad_via_driver_exit(p_mad_driver_t);
p_mad_link_t mad_via_choice(p_mad_connection_t,
			      size_t,
			      mad_send_mode_t,
			      mad_receive_mode_t);

void mad_via_new_message(p_mad_connection_t);
p_mad_connection_t mad_via_receive_message(p_mad_channel_t);
void mad_via_send_buffer(p_mad_link_t,
			   p_mad_buffer_t);
void mad_via_receive_buffer(p_mad_link_t,
			      p_mad_buffer_t *);
void mad_via_send_buffer_group(p_mad_link_t,
				 p_mad_buffer_group_t);
void mad_via_receive_sub_buffer_group(p_mad_link_t,
					tbx_bool_t,
					p_mad_buffer_group_t);
p_mad_buffer_t mad_via_get_static_buffer(p_mad_link_t);
void mad_via_return_static_buffer(p_mad_link_t,
				    p_mad_buffer_t);
#endif /* __MAD_VIA_H */
