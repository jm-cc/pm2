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
p_tbx_memory_t mad_buffer_memory                 = NULL;
p_tbx_memory_t mad_buffer_group_memory           = NULL;
p_tbx_memory_t mad_buffer_pair_memory            = NULL;
p_tbx_memory_t mad_buffer_slice_parameter_memory = NULL;

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
		  INITIAL_BUFFER_NUM,
                  "madeleine/buffers");
  tbx_malloc_init(&mad_buffer_group_memory,
		  sizeof(mad_buffer_group_t),
		  INITIAL_BUFFER_GROUP_NUM,
                  "madeleine/buffer groups");
  tbx_malloc_init(&mad_buffer_pair_memory,
		  sizeof(mad_buffer_pair_t),
		  INITIAL_BUFFER_PAIR_NUM,
                  "madeleine/buffer pairs");
  tbx_malloc_init(&mad_buffer_slice_parameter_memory,
		  sizeof(mad_buffer_slice_parameter_t),
		  INITIAL_BUFFER_SLICE_PARAMETER_NUM,
                  "madeleine/buffer slice parameters");
}

void
mad_memory_manager_exit(void)
{
  tbx_malloc_clean(mad_buffer_memory);
  tbx_malloc_clean(mad_buffer_group_memory);
  tbx_malloc_clean(mad_buffer_pair_memory);
  tbx_malloc_clean(mad_buffer_slice_parameter_memory);
}

