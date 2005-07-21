
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
 * mad_memory_management.h
 * -----------------------
 */

#ifndef MAD_MEMORY_MANAGEMENT_H
#define MAD_MEMORY_MANAGEMENT_H

#include "tbx_compiler.h"

void
mad_memory_manager_init(int  TBX_UNUSED   argc,
			char TBX_UNUSED **argv);

void
mad_memory_manager_exit(void);

TBX_FMALLOC
p_mad_buffer_t
mad_alloc_buffer_struct(void);

void
mad_free_buffer_struct(p_mad_buffer_t buffer);

void
mad_free_buffer(p_mad_buffer_t buf);

TBX_FMALLOC
p_mad_buffer_group_t
mad_alloc_buffer_group_struct(void);

void
mad_free_buffer_group_struct(p_mad_buffer_group_t buffer_group);

void
mad_foreach_free_buffer_group_struct(void *object);

void
mad_foreach_free_buffer(void *object);

TBX_FMALLOC
p_mad_buffer_pair_t
mad_alloc_buffer_pair_struct(void);

void
mad_free_buffer_pair_struct(p_mad_buffer_pair_t buffer_pair);

void
mad_foreach_free_buffer_pair_struct(void *object);

TBX_FMALLOC
p_mad_buffer_slice_parameter_t
mad_alloc_slice_parameter(void);

void
mad_free_slice_parameter(p_mad_buffer_slice_parameter_t slice_parameter);

#endif /* MAD_MEMORY_MANAGEMENT_H */
