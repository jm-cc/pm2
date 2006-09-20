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
 * Mad_buffers.c
 * =============
 */

#include "madeleine.h"

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
  p_mad_buffer_t buffer = (p_mad_buffer_t)object;

  mad_free_buffer(buffer);
}
