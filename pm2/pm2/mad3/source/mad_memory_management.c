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
 * mad_memory_management.c
 * -----------------------
 */

#include "madeleine.h"

/*
 * Macros
 * ------
 */
#define INITIAL_BUFFER_NUM                 1024
#define INITIAL_BUFFER_GROUP_NUM             64
#define INITIAL_BUFFER_PAIR_NUM              64
#define INITIAL_BUFFER_SLICE_PARAMETER_NUM   16

/*
 * Static variables
 * ----------------
 */
static p_tbx_memory_t mad_buffer_memory                 = NULL;
static p_tbx_memory_t mad_buffer_group_memory           = NULL;
static p_tbx_memory_t mad_buffer_pair_memory            = NULL;
static p_tbx_memory_t mad_buffer_slice_parameter_memory = NULL;

/*
 * Functions
 * ---------
 */

/* tbx_memory_manager_init:
   initialize mad memory manager */
void
mad_memory_manager_init(int    argc TBX_UNUSED,
			char **argv TBX_UNUSED)
{
  tbx_malloc_init(&mad_buffer_memory,
		  sizeof(mad_buffer_t),
		  INITIAL_BUFFER_NUM);
  tbx_malloc_init(&mad_buffer_group_memory,
		  sizeof(mad_buffer_group_t),
		  INITIAL_BUFFER_GROUP_NUM);
  tbx_malloc_init(&mad_buffer_pair_memory,
		  sizeof(mad_buffer_pair_t),
		  INITIAL_BUFFER_PAIR_NUM);
  tbx_malloc_init(&mad_buffer_slice_parameter_memory,
		  sizeof(mad_buffer_slice_parameter_t),
		  INITIAL_BUFFER_SLICE_PARAMETER_NUM);
}

void
mad_memory_manager_exit(void)
{
  tbx_malloc_clean(mad_buffer_memory);
  tbx_malloc_clean(mad_buffer_group_memory);
  tbx_malloc_clean(mad_buffer_pair_memory);
  tbx_malloc_clean(mad_buffer_slice_parameter_memory);
}

p_mad_buffer_t
mad_alloc_buffer_struct(void)
{
  p_mad_buffer_t buffer = NULL;

  buffer = tbx_malloc(mad_buffer_memory);
  memset(buffer, 0, sizeof(mad_buffer_t));

  return buffer;
}

void
mad_free_buffer_struct(p_mad_buffer_t buffer)
{
  if (buffer->parameter_slist)
    {
      while (!tbx_slist_is_nil(buffer->parameter_slist))
        {
          p_mad_buffer_slice_parameter_t param = NULL;

          param = tbx_slist_extract(buffer->parameter_slist);
          mad_free_slice_parameter(param);
        }

      tbx_slist_clear_and_free(buffer->parameter_slist);
      buffer->parameter_slist = NULL;
    }

  tbx_free(mad_buffer_memory, buffer);
}

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

p_mad_buffer_group_t
mad_alloc_buffer_group_struct(void)
{
  return tbx_malloc(mad_buffer_group_memory);
}

void
mad_free_buffer_group_struct(p_mad_buffer_group_t buffer_group)
{
  tbx_free(mad_buffer_group_memory, buffer_group);
}

void
mad_foreach_free_buffer_group_struct(void *object)
{
  tbx_free(mad_buffer_group_memory, object);
}

void
mad_foreach_free_buffer(void *object)
{
  p_mad_buffer_t buffer = object;

  mad_free_buffer(buffer);
}

p_mad_buffer_pair_t
mad_alloc_buffer_pair_struct()
{
  p_mad_buffer_pair_t bp = NULL;
  bp = tbx_malloc(mad_buffer_pair_memory);
  memset(bp, 0, sizeof(mad_buffer_pair_t));
  return bp;
}

void
mad_free_buffer_pair_struct(p_mad_buffer_pair_t buffer_pair)
{
  tbx_free(mad_buffer_pair_memory, buffer_pair);
}

void
mad_foreach_free_buffer_pair_struct(void *object)
{
  tbx_free(mad_buffer_pair_memory, object);
}

p_mad_buffer_slice_parameter_t
mad_alloc_slice_parameter(void)
{
  p_mad_buffer_slice_parameter_t slice_parameter = NULL;

  slice_parameter = tbx_malloc(mad_buffer_slice_parameter_memory);
  slice_parameter->length = 0;
  slice_parameter->offset = 0;
  slice_parameter->opcode = 0;
  slice_parameter->value  = 0;

  return slice_parameter;
}

void
mad_free_slice_parameter(p_mad_buffer_slice_parameter_t slice_parameter)
{
  tbx_free(mad_buffer_slice_parameter_memory, slice_parameter);
}

