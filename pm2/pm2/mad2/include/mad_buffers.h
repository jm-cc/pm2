
/*
 * Mad_buffers.h
 * =============
 */

#ifndef MAD_BUFFERS_H
#define MAD_BUFFERS_H

/* mad_buffer_type: indicates the category of the buffer */
typedef enum
{
  mad_user_buffer,   /* buffer given by the user */
  mad_static_buffer, /* buffer allocated by the low level layer */
  mad_dynamic_buffer, /* buffer allocated by the high level layer */
  mad_internal_buffer /* buffer to be used by the internal routines only */
} mad_buffer_type_t, *p_mad_buffer_type_t;

#ifdef MAD_FORWARDING
typedef struct s_mad_buffer_info
{
  mad_send_mode_t    send_mode;
  mad_receive_mode_t recv_mode;
  
} mad_buffer_info_t;
#endif //MAD_FORWARDING

/* mad_buffer: generic buffer object */
typedef struct s_mad_buffer
{
  void                     *buffer;
  size_t                    length;
  size_t                    bytes_written;
  size_t                    bytes_read;
  mad_buffer_type_t         type;
#ifdef MAD_FORWARDING
  mad_buffer_info_t         informations;
#endif //MAD_FORWARDING
  p_mad_driver_specific_t   specific; /* may be used by connection
				         layer to store data
				         related to static buffers */
} mad_buffer_t;

/* pair of dynamic/static buffer for 'send_later' handling
   with static buffer links */
typedef struct s_mad_buffer_pair
{
  mad_buffer_t dynamic_buffer;
  mad_buffer_t static_buffer;
} mad_buffer_pair_t;

typedef struct s_mad_buffer_group
{
  tbx_list_t      buffer_list;
  p_mad_link_t    link; /* associated link */
  size_t          length;
} mad_buffer_group_t;

#endif /* MAD_BUFFERS_H */
