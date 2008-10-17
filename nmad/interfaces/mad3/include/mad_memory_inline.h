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
 * mad_memory_inline.h
 * -----------------------
 */

#ifndef MAD_MEMORY_INLINE_H
#define MAD_MEMORY_INLINE_H

#include "tbx_compiler.h"

extern p_tbx_memory_t mad_buffer_memory;
extern p_tbx_memory_t mad_buffer_group_memory;
extern p_tbx_memory_t mad_buffer_pair_memory;
extern p_tbx_memory_t mad_buffer_slice_parameter_memory;

TBX_FMALLOC
static
__inline__
p_mad_buffer_t
mad_alloc_buffer_struct(void)
{
  p_mad_buffer_t buffer = NULL;

  buffer = (p_mad_buffer_t)tbx_malloc(mad_buffer_memory);
  memset(buffer, 0, sizeof(mad_buffer_t));

  return buffer;
}

static
__inline__
void
mad_free_buffer_struct(p_mad_buffer_t buffer)
{
  if (buffer->parameter_slist)
    {
      while (!tbx_slist_is_nil(buffer->parameter_slist))
        {
          p_mad_buffer_slice_parameter_t param = NULL;

          param = (p_mad_buffer_slice_parameter_t)tbx_slist_extract(buffer->parameter_slist);
          mad_free_slice_parameter(param);
        }

      tbx_slist_clear_and_free(buffer->parameter_slist);
      buffer->parameter_slist = NULL;
    }

  tbx_free(mad_buffer_memory, buffer);
}

TBX_FMALLOC
static
__inline__
p_mad_buffer_t
mad_alloc_buffer(size_t length)
{
  p_mad_buffer_t buffer = NULL;

  buffer = mad_alloc_buffer_struct();

  buffer->buffer = tbx_aligned_malloc(length, MAD_ALIGNMENT);

  buffer->length          = length;
  buffer->bytes_read      = 0;
  buffer->bytes_written   = 0;
  buffer->type            = mad_dynamic_buffer;
  buffer->parameter_slist = NULL;
  buffer->specific        = NULL;

  return buffer;
}

static
__inline__
void
mad_free_buffer(p_mad_buffer_t buffer)
{
  if (buffer->type == mad_dynamic_buffer)
    {
      if (buffer->buffer)
	{
	  tbx_aligned_free(buffer->buffer, MAD_ALIGNMENT);
	}
    }
  mad_free_buffer_struct(buffer);
}

TBX_FMALLOC
static
__inline__
p_mad_buffer_group_t
mad_alloc_buffer_group_struct(void)
{
  return (p_mad_buffer_group_t)tbx_malloc(mad_buffer_group_memory);
}

static
__inline__
void
mad_free_buffer_group_struct(p_mad_buffer_group_t buffer_group)
{
  tbx_free(mad_buffer_group_memory, buffer_group);
}

static
__inline__
void
mad_foreach_free_buffer_group_struct(void *object)
{
  tbx_free(mad_buffer_group_memory, object);
}

static
__inline__
void
mad_foreach_free_buffer(void *object)
{
  p_mad_buffer_t buffer = (p_mad_buffer_t)object;

  mad_free_buffer(buffer);
}

TBX_FMALLOC
static
__inline__
p_mad_buffer_pair_t
mad_alloc_buffer_pair_struct()
{
  p_mad_buffer_pair_t bp = NULL;
  bp = (p_mad_buffer_pair_t)tbx_malloc(mad_buffer_pair_memory);
  memset(bp, 0, sizeof(mad_buffer_pair_t));
  return bp;
}

static
__inline__
void
mad_free_buffer_pair_struct(p_mad_buffer_pair_t buffer_pair)
{
  tbx_free(mad_buffer_pair_memory, buffer_pair);
}

static
__inline__
void
mad_foreach_free_buffer_pair_struct(void *object)
{
  tbx_free(mad_buffer_pair_memory, object);
}

TBX_FMALLOC
static
__inline__
p_mad_buffer_slice_parameter_t
mad_alloc_slice_parameter(void)
{
  p_mad_buffer_slice_parameter_t slice_parameter = NULL;

  slice_parameter = (p_mad_buffer_slice_parameter_t)tbx_malloc(mad_buffer_slice_parameter_memory);
  slice_parameter->length = 0;
  slice_parameter->offset = 0;
  slice_parameter->opcode = 0;
  slice_parameter->value  = 0;

  return slice_parameter;
}

static
__inline__
void
mad_free_slice_parameter(p_mad_buffer_slice_parameter_t slice_parameter)
{
  tbx_free(mad_buffer_slice_parameter_memory, slice_parameter);
}

#endif /* MAD_MEMORY_INLINE_H */
