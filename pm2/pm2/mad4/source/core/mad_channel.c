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
 * Mad_channel.c
 * =============
 */
/* #define DEBUG */
/* #define TRACING */

/*
 * Headers
 * -------
 */
#include "madeleine.h"

/*
 * Functions
 * ---------
 */

p_mad_channel_t
mad_get_channel(p_mad_madeleine_t  madeleine,
		char              *name)
{
  p_mad_channel_t channel = NULL;

  LOG_IN();
  channel = tbx_htable_get(madeleine->channel_htable, name);
  if (!channel)
    FAILURE("channel not found");

  if (!channel->not_private)
    FAILURE("invalid channel");

  LOG_OUT();

  return channel;
}

p_mad_channel_t
mad_get_sub_channel(p_mad_channel_t channel,
		    unsigned int    sub)
{
  p_mad_channel_t sub_channel = NULL;
  p_tbx_darray_t  darray  = NULL;

  LOG_IN();
  darray = channel->sub_channel_darray;

  TBX_LOCK_SHARED(darray);
  if (sub >= channel->max_sub)
    FAILURE("not enough resources to allocate anonymous sub channel");

  sub_channel = tbx_darray_expand_and_get(darray, sub);

  if (!sub_channel)
    {
      p_mad_driver_interface_t interface = NULL;

      interface = channel->adapter->driver->interface;

      if (interface->get_sub_channel)
	{
	  sub_channel = interface->get_sub_channel(channel, sub);
	}
      else
	FAILURE("anonymous sub channels unsupported by the driver");

      tbx_darray_set(darray, sub, sub_channel);
    }
  TBX_UNLOCK_SHARED(darray);

  LOG_OUT();

  return sub_channel;
}
