
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
 * Mad_channel_inteface
 * ====================
 */

#ifndef MAD_CHANNEL_INTERFACE_H
#define MAD_CHANNEL_INTERFACE_H

/*
 * Functions
 * ---------
 */

p_mad_channel_t
mad_get_channel(p_mad_madeleine_t   madeleine,
		char               *name);

p_mad_channel_t
mad_get_sub_channel(p_mad_channel_t channel,
		    unsigned int    sub);

tbx_bool_t
  channel_reopen(p_mad_madeleine_t  madeleine);

tbx_bool_t
    connection_init(p_mad_channel_t mad_channel);

tbx_bool_t
    connection_open(p_mad_channel_t mad_channel);

void
    mad_channel_merge_done(p_mad_madeleine_t madeleine);

tbx_bool_t
   mad_channel_is_merged(p_mad_madeleine_t madeleine);

tbx_bool_t
    mad_expand_channel(p_mad_madeleine_t madeleine, char *name);

void
    mad_channel_split_done(p_mad_madeleine_t madeleine);

tbx_bool_t
    mad_shrink_channel(p_mad_madeleine_t madeleine, char *name);

void
    common_channel_exit2(p_mad_channel_t mad_channel);

tbx_bool_t
    connection_disconnect2(p_mad_channel_t ch);


#endif /* MAD_CHANNEL_INTERFACE_H */
