
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
 * Mad_buffer_interface.h
 * ======================
 */

#ifndef MAD_BUFFER_INTERFACE_H
#define MAD_BUFFER_INTERFACE_H

p_mad_buffer_t
mad_get_user_send_buffer(void   *ptr, 
			 size_t  length);

p_mad_buffer_t
mad_get_user_receive_buffer(void   *ptr, 
			    size_t  length);

p_mad_buffer_t
mad_alloc_buffer(size_t length);

tbx_bool_t
mad_buffer_full(p_mad_buffer_t buffer);

tbx_bool_t
mad_more_data(p_mad_buffer_t buffer);

size_t
mad_copy_length(p_mad_buffer_t source, 
		p_mad_buffer_t destination);

size_t
mad_copy_buffer(p_mad_buffer_t source,
		p_mad_buffer_t destination);

size_t
mad_pseudo_copy_buffer(p_mad_buffer_t source, 
		       p_mad_buffer_t destination);

p_mad_buffer_pair_t
mad_make_sub_buffer_pair(p_mad_buffer_t source,
			 p_mad_buffer_t destination);

p_mad_buffer_t
mad_duplicate_buffer(p_mad_buffer_t source);

void
mad_make_buffer_group(p_mad_buffer_group_t buffer_group,
		      p_tbx_list_t         buffer_list, 
		      p_mad_link_t         lnk);

p_mad_buffer_t
mad_split_buffer(p_mad_buffer_t buffer,
		 size_t         limit);

size_t
mad_append_buffer_to_list(p_tbx_list_t   list,
			  p_mad_buffer_t buffer,
			  size_t         position,
			  size_t         limit);

#endif /* MAD_BUFFER_INTERFACE_H */
