
/*
 * Mad_link.h
 * ==========
 */

#ifndef MAD_LINK_H
#define MAD_LINK_H
/*
 * link
 * ----
 */

typedef struct s_mad_link
{
  /* Common use fields */
  p_mad_connection_t        connection;
  mad_link_id_t             id;

  /* Internal use fields */
  mad_link_mode_t           link_mode;
  mad_buffer_mode_t         buffer_mode;
  mad_group_mode_t          group_mode;
  tbx_list_t                buffer_list;
  size_t                    cumulated_length;
  tbx_list_t                user_buffer_list;

  /* Driver specific data */
  p_mad_driver_specific_t   specific;
} mad_link_t;

#endif /* MAD_LINK_H */
