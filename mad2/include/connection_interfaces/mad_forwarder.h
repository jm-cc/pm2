
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
 * Mad_forwarder.h
 * ===============
 */
#ifndef MAD_FORWARDER_H
#define MAD_FORWARDER_H

/*
 * Structures
 * ----------
 */

/*
 * Functions 
 * ---------
 */

void
mad_fwd_link_init(p_mad_link_t);

void
mad_fwd_new_message(p_mad_connection_t);

void
mad_fwd_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);

void
mad_fwd_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);

void
mad_fwd_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);

void
mad_fwd_receive_sub_buffer_group(p_mad_link_t,
				 tbx_bool_t,
				 p_mad_buffer_group_t);

#endif /* MAD_FORWARDER_H */
