
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

