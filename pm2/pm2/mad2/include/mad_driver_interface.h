
/*
 * Mad_driver_interface.h
 * ======================
 */

#ifndef MAD_DRIVER_INTERFACE_H
#define MAD_DRIVER_INTERFACE_H

typedef struct s_mad_driver_interface
{
  /* Driver initialization */
  void
  (*driver_init)(p_mad_driver_t);
  

  /* Adapter initialization */
  void
  (*adapter_init)(p_mad_adapter_t);

  void
  (*adapter_configuration_init)(p_mad_adapter_t); 


  /* Channel/Connection/Link initialization */
  void
  (*channel_init)(p_mad_channel_t);

  void
  (*before_open_channel)(p_mad_channel_t);

  void
  (*connection_init)(p_mad_connection_t,
		     p_mad_connection_t);
  
  void
  (*link_init)(p_mad_link_t);
  
  /* Point-to-point connection */
  void
  (*accept)(p_mad_channel_t);
  
  void
  (*connect)(p_mad_connection_t);

  /* Channel clean-up functions */
  void
  (*after_open_channel)(p_mad_channel_t);

  void 
  (*before_close_channel)(p_mad_channel_t);

  /* Connection clean-up function */
  void
  (*disconnect)(p_mad_connection_t);
  
  void 
  (*after_close_channel)(p_mad_channel_t);

  /* Deallocation functions */
  void
  (*link_exit)(p_mad_link_t);
  
  void
  (*connection_exit)(p_mad_connection_t,
		     p_mad_connection_t);
  
  void
  (*channel_exit)(p_mad_channel_t);

  void
  (*adapter_exit)(p_mad_adapter_t);
  
  void
  (*driver_exit)(p_mad_driver_t);

  /* Dynamic paradigm selection */
  p_mad_link_t
  (*choice)(p_mad_connection_t,
	    size_t,
            mad_send_mode_t,
	    mad_receive_mode_t);

  /* Static buffers management */
  p_mad_buffer_t
  (*get_static_buffer)(p_mad_link_t);

  void
  (*return_static_buffer)(p_mad_link_t, p_mad_buffer_t);

  /* Message transfer */
  void
  (*new_message)(p_mad_connection_t);

  p_mad_connection_t
  (*receive_message)(p_mad_channel_t);

#ifdef MAD_MESSAGE_POLLING
  p_mad_connection_t
  (*poll_message)(p_mad_channel_t);
#endif /* MAD_MESSAGE_POLLING */
  
  /* Buffer transfer */
  void
  (*send_buffer)(p_mad_link_t,
		 p_mad_buffer_t);
  
  void
  (*receive_buffer)(p_mad_link_t,
		    p_mad_buffer_t *);
  
  /* Buffer group transfer */
  void
  (*send_buffer_group)(p_mad_link_t,
		       p_mad_buffer_group_t);
  
  void
  (*receive_sub_buffer_group)(p_mad_link_t,
			      tbx_bool_t,
			      p_mad_buffer_group_t);

  void
  (*finalize_sub_buffer_group_reception)(p_mad_link_t);

  /* External spawn support */
  void
  (*external_spawn_init)(p_mad_adapter_t, int *, char **);

  void
  (*configuration_init)(p_mad_adapter_t, p_mad_configuration_t);

  void
  (*send_adapter_parameter)(p_mad_adapter_t, ntbx_host_id_t, char *);

  void
  (*receive_adapter_parameter)(p_mad_adapter_t, char **);
  
} mad_driver_interface_t;

#endif /* MAD_DRIVER_INTERFACE_H */
