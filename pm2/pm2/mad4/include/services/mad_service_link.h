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
 * Mad_service_link.h
 * =============================
 */

#ifndef MAD_SERVICE_LINK_H
#define MAD_SERVICE_LINK_H


/*
 * Functions
 * --------- 
 */

p_mad_link_t
  mad_link_default_choice(p_mad_connection_t   connection,
			  size_t               msg_size,
			  p_mad_send_mode_t    send_mode    TBX_UNUSED,
			  size_t               smode_size   TBX_UNUSED,
			  p_mad_receive_mode_t receive_mode TBX_UNUSED,
			  size_t               rmode_size   TBX_UNUSED) ;

#endif /* MAD_SERVICE_LINK_H */
