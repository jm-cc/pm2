
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
 * Mad_types.h
 * ===========
 */

#ifndef MAD_TYPES_H
#define MAD_TYPES_H

typedef void *p_mad_driver_specific_t;

/*
 * Data structure elements
 * -----------------------
 */
typedef long    mad_link_id_t,              *p_mad_link_id_t;
typedef long    mad_channel_id_t,           *p_mad_channel_id_t;
typedef int     mad_adapter_id_t,           *p_mad_adapter_id_t;
typedef int     mad_buffer_alignment_t,     *p_mad_buffer_alignment_t;

#endif /* MAD_TYPES_H */
