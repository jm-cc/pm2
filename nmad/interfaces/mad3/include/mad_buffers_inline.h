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
 * Mad_buffers_inline.h
 * ====================
 */

#ifndef MAD_BUFFERS_INLINE_H
#define MAD_BUFFERS_INLINE_H

#include "tbx_compiler.h"

TBX_FMALLOC
static
__inline__
p_mad_buffer_t
mad_get_user_send_buffer(void   *ptr,
			 size_t  length)
{
  p_mad_buffer_t buffer = NULL;

  buffer = mad_alloc_buffer_struct();

  buffer->buffer          = ptr;
  buffer->length          = length;
  buffer->bytes_written   = length ;
  buffer->bytes_read      = 0;
  buffer->type            = mad_user_buffer;
  buffer->specific        = NULL;
  buffer->parameter_slist = NULL;

  return buffer;
}

TBX_FMALLOC
static
__inline__
p_mad_buffer_t
mad_get_user_receive_buffer(void   *ptr,
			    size_t  length)
{
  p_mad_buffer_t buffer = NULL;

  buffer = mad_alloc_buffer_struct();

  buffer->buffer          = ptr;
  buffer->length          = length;
  buffer->bytes_written   = 0 ;
  buffer->bytes_read      = 0;
  buffer->type            = mad_user_buffer;
  buffer->specific        = NULL;
  buffer->parameter_slist = NULL;

  return buffer;
}

static
__inline__
tbx_bool_t
mad_buffer_full(p_mad_buffer_t buffer)
{
  if (buffer->bytes_written < buffer->length)
    {
      return tbx_false ;
    }
  else
    {
      return tbx_true ;
    }
}

static
__inline__
tbx_bool_t
mad_more_data(p_mad_buffer_t buffer)
{
  if (buffer->bytes_read < buffer->bytes_written)
    {
      return tbx_true ;
    }
  else
    {
      return tbx_false ;
    }
}

static
__inline__
size_t
mad_copy_length(p_mad_buffer_t source,
		p_mad_buffer_t destination)
{
  return tbx_min((source->bytes_written - source->bytes_read),
                 (destination->length - destination->bytes_written));
}

static
__inline__
void
mad_copy_buffer_parameters(p_mad_buffer_t source,
                           p_mad_buffer_t destination)
{
  size_t        length          =    0;
  p_tbx_slist_t param_slist_src = NULL;

  length = mad_copy_length(source, destination);
  param_slist_src = source->parameter_slist;

  if (!param_slist_src || tbx_slist_is_nil(param_slist_src))
    return;

  tbx_slist_ref_to_head(param_slist_src);
  do
    {
      p_mad_buffer_slice_parameter_t src_param    = NULL;
      p_mad_buffer_slice_parameter_t dst_param    = NULL;
      size_t                         param_offset =    0;
      size_t                         param_length =    0;

      src_param = (p_mad_buffer_slice_parameter_t)tbx_slist_ref_get(param_slist_src);

      if (src_param->offset >= source->bytes_read + length)
        continue;

      if (src_param->offset + src_param->length <= source->bytes_read)
        continue;

      if (src_param->offset > source->bytes_read)
        {
          param_offset = src_param->offset - source->bytes_read;
          param_length = tbx_min(length-param_offset, src_param->length);
        }
      else
        {
          param_offset = 0;
          param_length =
            tbx_min(length,
                    src_param->length-(source->bytes_read-src_param->offset));
        }

      dst_param = mad_alloc_slice_parameter();

      dst_param->base   = destination->buffer;
      dst_param->offset = param_offset;
      dst_param->length = param_length;
      dst_param->opcode = src_param->opcode;
      dst_param->value  = src_param->value;

      if (!destination->parameter_slist)
        {
          destination->parameter_slist = tbx_slist_nil();
        }

      tbx_slist_append(destination->parameter_slist, dst_param);
    }
  while (tbx_slist_ref_forward(param_slist_src));
}

static
__inline__
size_t
mad_copy_buffer(p_mad_buffer_t source,
		p_mad_buffer_t destination)
{
  size_t length = 0;

  mad_copy_buffer_parameters(source, destination);

  length = mad_copy_length(source, destination);

  memcpy((char*)destination->buffer + destination->bytes_written,
	 (char*)source->buffer + source->bytes_read, length);

  source->bytes_read         += length ;
  destination->bytes_written += length ;

  return length;
}

static
__inline__
size_t
mad_pseudo_copy_buffer(p_mad_buffer_t source,
		       p_mad_buffer_t destination)
{
  size_t length = 0;

  mad_copy_buffer_parameters(source, destination);

  length = mad_copy_length(source, destination);

  source->bytes_read         += length ;
  destination->bytes_written += length ;

  return length;
}

static
__inline__
p_mad_buffer_pair_t
mad_make_sub_buffer_pair(p_mad_buffer_t source,
			 p_mad_buffer_t destination)
{
  p_mad_buffer_pair_t pair   = NULL;
  size_t              length =    0;

  length = mad_copy_length(source, destination);
  pair   = mad_alloc_buffer_pair_struct();

  pair->dynamic_buffer.buffer        = (char*)source->buffer + source->bytes_read;
  pair->dynamic_buffer.length        = length;
  pair->dynamic_buffer.bytes_written = length;
  pair->dynamic_buffer.bytes_read    = 0;
  pair->dynamic_buffer.type          = mad_internal_buffer;
  pair->dynamic_buffer.specific      = NULL;

  pair->static_buffer.buffer         =
    (char*)destination->buffer + destination->bytes_written;
  pair->static_buffer.length         = length;
  pair->static_buffer.bytes_written  = 0;
  pair->static_buffer.bytes_read     = 0 ;
  pair->static_buffer.type           = mad_internal_buffer;
  pair->static_buffer.specific       = NULL;

  return pair;
}

TBX_FMALLOC
static
__inline__
p_mad_buffer_t
mad_duplicate_buffer(p_mad_buffer_t source)
{
  p_mad_buffer_t destination = NULL;

  destination = mad_alloc_buffer(source->length);
  mad_copy_buffer(source, destination);

  return destination;
}

static
__inline__
void
mad_make_buffer_group(p_mad_buffer_group_t buffer_group,
		      p_tbx_list_t         buffer_list,
		      p_mad_link_t         lnk)
{
  tbx_extract_sub_list(buffer_list, &(buffer_group->buffer_list));
  buffer_group->link = lnk;
}

TBX_FMALLOC
static
__inline__
p_mad_buffer_t
mad_split_buffer(p_mad_buffer_t buffer,
		 size_t         limit)
{
  if (buffer->length > limit)
    {
      p_mad_buffer_t new_buffer = NULL;

      new_buffer = mad_alloc_buffer_struct();
      new_buffer->buffer        = (char*)buffer->buffer + limit;
      new_buffer->length        = buffer->length - limit;
      new_buffer->bytes_written =
	(buffer->bytes_written > limit)?buffer->bytes_written - limit:0;
      new_buffer->bytes_read    =
	(buffer->bytes_read > limit)?buffer->bytes_read - limit:0;
      new_buffer->type          = mad_user_buffer;
      new_buffer->specific      = NULL;

      buffer->length = limit;

      if (buffer->bytes_written > limit)
	{
	  buffer->bytes_written = limit;
	}

      if (buffer->bytes_read > limit)
	{
	  buffer->bytes_read = limit;
	}

      return new_buffer;
    }
  else
    return NULL;
}

static
__inline__
size_t
mad_append_buffer_to_list(p_tbx_list_t   list,
			  p_mad_buffer_t buffer,
			  size_t         position,
			  size_t         limit)
{
  if (limit)
    {
      p_mad_buffer_t new_buffer = buffer;

      if (position)
	{
	  new_buffer = mad_split_buffer(buffer, limit - position);

	  tbx_append_list(list, buffer);

	  if (!new_buffer)
	    {
	      return position + buffer->length;
	    }
	}

      do
	{
	  buffer = new_buffer;
	  new_buffer = mad_split_buffer(buffer, limit);
	  tbx_append_list(list, buffer);
	}
      while (new_buffer);

      return buffer->length % limit;
    }
  else
    {
      tbx_append_list(list, buffer);
      return position + buffer->length;
    }
}

#endif /* MAD_BUFFERS_INLINE_H */

