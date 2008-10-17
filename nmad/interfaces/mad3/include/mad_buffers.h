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
 * Mad_buffers.h
 * =============
 */

#ifndef MAD_BUFFERS_H
#define MAD_BUFFERS_H

/* Default buffer alignment */
#define MAD_ALIGNMENT        4
#define MAD_LENGTH_ALIGNMENT 4
#define MAD_ALIGNED          TBX_ALIGN(MAD_ALIGNMENT)
#define MAD_LENGTH_ALIGNED   TBX_ALIGN(MAD_SIZE_ALIGNMENT)

/* mad_buffer_type: indicates the category of the buffer */
typedef enum
{
  mad_user_buffer,   /* buffer given by the user */
  mad_static_buffer, /* buffer allocated by the low level layer */
  mad_dynamic_buffer, /* buffer allocated by the high level layer */
  mad_internal_buffer /* buffer to be used by the internal routines only */
} mad_buffer_type_t, *p_mad_buffer_type_t;

typedef struct s_mad_buffer_slice_parameter
{
  void   *base;
  size_t  offset;
  size_t  length;
  int     opcode;
  int     value;
} mad_buffer_slice_parameter_t, *p_mad_buffer_slice_parameter_t;

/* mad_buffer: generic buffer object */
typedef struct s_mad_buffer
{
  void                    *buffer;
  size_t                   length;
  size_t                   bytes_written;
  size_t                   bytes_read;
  mad_buffer_type_t        type;
  p_tbx_slist_t  	   parameter_slist; /* may be used by the
                                               application to attach
                                               driver-dependent settings
                                               to buffer slices */
  p_mad_driver_specific_t  specific; /* may be used by connection
                                        layer to store data
                                        related to static buffers */
} mad_buffer_t;

/* pair of dynamic/static buffer for 'send_later' handling
   with static buffer links */
typedef struct s_mad_buffer_pair
{
  mad_buffer_t dynamic_buffer;
  mad_buffer_t static_buffer;
} mad_buffer_pair_t;

typedef struct s_mad_buffer_group
{
  tbx_list_t   buffer_list;
  p_mad_link_t link; /* associated link */
  size_t       length;
} mad_buffer_group_t;

#endif /* MAD_BUFFERS_H */
