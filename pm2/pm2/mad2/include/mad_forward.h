
/*
 * Mad_forward.h
 * =============
 */

/* *** Note: Rajouter Lionel dans le copyright *** */
#ifndef MAD_FORWARD_H
#define MAD_FORWARD_H

typedef struct s_mad_msg_hdr_fwd
{
  size_t             length;  /* Pour un groupe, donne le nb de buffers
				 dans le groupe */
  mad_send_mode_t    send_mode;
  mad_receive_mode_t recv_mode;
  mad_link_id_t      in_link_id;
} mad_msg_hdr_fwd_t, *p_mad_msg_hdr_fwd_t;

typedef struct s_mad_group_hdr_fwd
{
  size_t             length; 
  /*  mad_send_mode_t    send_mode;
      mad_receive_mode_t recv_mode;*/
} mad_msg_group_hdr_fwd_t;

typedef struct s_mad_hdr_fwd 
{
  ntbx_host_id_t destination;
} mad_hdr_fwd_t, *p_mad_hdr_fwd_t;

#endif /* MAD_FORWARD_H */

