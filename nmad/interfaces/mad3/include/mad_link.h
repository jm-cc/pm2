
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
 * Mad_link.h
 * ==========
 */

#ifndef MAD_LINK_H
#define MAD_LINK_H

/*
 * Structures
 * ----------
 */
typedef struct s_mad_link
{
  // Common use fields
  p_mad_connection_t        connection;
  mad_link_id_t             id;

  // Internal use fields
  mad_link_mode_t           link_mode;
  mad_buffer_mode_t         buffer_mode;
  mad_group_mode_t          group_mode;
  p_tbx_list_t              buffer_list;
  size_t                    cumulated_length;
  p_tbx_list_t              user_buffer_list;

  // Driver specific data
  p_mad_driver_specific_t   specific;
} mad_link_t;

#endif /* MAD_LINK_H */
