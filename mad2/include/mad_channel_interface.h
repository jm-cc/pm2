
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
#ifdef MAD_FORWARDING
p_mad_user_channel_t
mad_open_channel(p_mad_madeleine_t   madeleine,
                 char               *name);
#else /* MAD_FORWARDING */
p_mad_channel_t
mad_open_channel(p_mad_madeleine_t   madeleine,
		 mad_adapter_id_t    adapter);
#endif /* MAD_FORWARDING */

void
mad_close_channels(p_mad_madeleine_t madeleine);

#endif /* MAD_CHANNEL_INTERFACE_H */
