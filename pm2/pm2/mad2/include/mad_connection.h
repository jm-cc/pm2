
/*
 * Mad_connection.h
 * ================
 */

#ifndef MAD_CONNECTION_H
#define MAD_CONNECTION_H

/*
 * Structures
 * ----------
 */
typedef enum
{
  mad_incoming_connection,
  mad_outgoing_connection
} mad_connection_way_t, *p_mad_connection_way_t;

typedef struct s_mad_connection
{
  /* Common use fields */
           ntbx_host_id_t          remote_host_id;
#ifdef MAD_FORWARDING
           ntbx_host_id_t          actual_destination_id;
           tbx_bool_t              active;
#endif /* MAD_FORWARDING */
           p_mad_channel_t         channel;
           p_mad_connection_t      reverse;
           mad_connection_way_t    way;

  /* Internal use fields */
           int                     nb_link; 
           p_mad_link_t            link;
#ifdef MAD_FORWARDING
           p_mad_link_t            fwd_link;
#endif /* MAD_FORWARDING */
           tbx_list_t              user_buffer_list; 
           tbx_list_reference_t    user_buffer_list_reference; 
           tbx_list_t              buffer_list;
           size_t                  cumulated_length;
           tbx_list_t              buffer_group_list;
           tbx_list_t              pair_list;
           p_mad_link_t            last_link;
           mad_link_mode_t         last_link_mode;

  /* Flags */
  volatile tbx_bool_t              lock;
           tbx_bool_t              send;
           tbx_bool_t              delayed_send; 
           tbx_bool_t              flushed;
           tbx_bool_t              pair_list_used;
           tbx_bool_t              first_sub_buffer_group;
           tbx_bool_t              more_data;
#ifdef MAD_FORWARDING
           tbx_bool_t              is_being_forwarded;
#endif /* MAD_FORWARDING */

  /* Driver specific data */
  p_mad_driver_specific_t          specific;
} mad_connection_t;

#endif /* MAD_CONNECTION_H */
