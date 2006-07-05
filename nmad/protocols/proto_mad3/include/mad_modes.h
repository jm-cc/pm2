
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
 * Mad_modes.h
 * ===========
 */

#ifndef MAD_MODES_H
#define MAD_MODES_H

/*
 * Send/Receive modes
 * ------------------
 */
typedef enum
{
  mad_send_unknown = 0,
  mad_send_SAFER   = 1,
  mad_send_LATER   = 2,
  mad_send_CHEAPER = 3,
} mad_send_mode_t, *p_mad_send_mode_t;

typedef enum
{
  mad_receive_unknown = 0,
  mad_receive_EXPRESS = 1,
  mad_receive_CHEAPER = 2,
} mad_receive_mode_t, *p_mad_receive_mode_t;

/*
 * Link mode
 * ----------
 */
typedef enum
{
  mad_link_mode_unknown = 0,
  mad_link_mode_buffer  = 1,      /* when possible, and buffer is full */
  mad_link_mode_buffer_group = 2,       /* as much data as possible
				XXX: WARNING !!!
				This mode may require special support
				and link buffering at the low level
				layer on the receiving side for EXPRESS
				data reception
				XXX: WARNING 2 !!!
				This mode is automatically selected
				for EXPRESS data after 'delayed_send'
				is set to True. */
  mad_link_mode_link_group = 3,   /* send all buffers in one unique chunk
			      	XXX: WARNING !!!
				This mode cannot be used for
			       	EXPRESS data (to prevent dead-locks)
			      	group mode for EXPRESS data will automatically
			      	be changed into chunk mode
			      	XXX: WARNING 2 !!!
			      	This mode is automatically selected
			      	for no-EXPRESS data after 'delayed_send'
			      	is set to True.
				XXX: WARNING 3 !!!
				This mode is automatically selected
				for no-EXPRESS/LATER data */

} mad_link_mode_t, *p_mad_link_mode_t;

typedef enum
{
  mad_buffer_mode_unknown = 0,
  mad_buffer_mode_dynamic = 1, /* buffers must be dynamically allocated
			      when needed (send_mode != LATER)
			      -> the buffer size corresponds to the user
			      data size	*/
  mad_buffer_mode_static  = 2, /* buffers are given by the low level layer
			    (connection layer) and may either be
			    system buffers, preallocated buffers or
			    buffers allocated on the fly by the connection
			    layer
			    -> the buffer size is fixed by the connection
			    layer and user data may be splitted onto one
			    one more consecutive buffer */
} mad_buffer_mode_t, *p_mad_buffer_mode_t;

typedef enum
{
  mad_group_mode_unknown   = 0,
  mad_group_mode_aggregate = 1,
  mad_group_mode_split     = 2,
} mad_group_mode_t, *p_mad_group_mode_t;

#endif /* MAD_MODES_H */
