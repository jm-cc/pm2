#ifndef MAD_CONSTRUCTOR_H
#define MAD_CONSTRUCTOR_H


p_mad_dir_node_t
mad_dir_node_cons(void);

p_mad_dir_adapter_t
mad_dir_adapter_cons(void);

p_mad_dir_driver_process_specific_t
mad_dir_driver_process_specific_cons(void);

p_mad_dir_driver_t
mad_dir_driver_cons(void);

p_mad_dir_channel_process_specific_t
mad_dir_channel_process_specific_cons(void);

p_mad_dir_channel_t
mad_dir_channel_cons(void);

p_mad_dir_vchannel_process_routing_table_t
mad_dir_vchannel_process_routing_table_cons(void);

p_mad_dir_vchannel_process_specific_t
mad_dir_vchannel_process_specific_cons(void);

p_mad_dir_fchannel_t
mad_dir_fchannel_cons(void);

p_mad_dir_vchannel_t
mad_dir_vchannel_cons(void);

p_mad_directory_t
mad_directory_cons(void);

p_mad_session_t
mad_session_cons(void);

p_mad_settings_t
mad_settings_cons(void);

/*
p_mad_configuration_t
mad_configuration_cons(void);
*/

p_mad_driver_interface_t
mad_driver_interface_cons(void);

p_mad_driver_t
mad_driver_cons(void);

p_mad_adapter_t
mad_adapter_cons(void);

p_mad_channel_t
mad_channel_cons(void);

p_mad_connection_t
mad_connection_cons(void);

p_mad_link_t
mad_link_cons(void);

p_mad_madeleine_t
mad_madeleine_cons(void);

#endif // MAD_CONSTRUCTOR_H
