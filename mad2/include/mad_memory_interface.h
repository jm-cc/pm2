
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

#ifndef __MAD_MEMORY_MANAGEMENT_H
#define __MAD_MEMORY_MANAGEMENT_H

void
mad_memory_manager_init(int    argc,
			char **argv);

void
mad_memory_manager_clean(void);

p_mad_buffer_t
mad_alloc_buffer_struct(void);

void
mad_free_buffer_struct(p_mad_buffer_t buffer);

p_mad_buffer_t
mad_alloc_buffer(size_t length);

void
mad_free_buffer(p_mad_buffer_t buf);

p_mad_buffer_group_t
mad_alloc_buffer_group_struct(void);

void
mad_free_buffer_group_struct(p_mad_buffer_group_t buffer_group);

void
mad_foreach_free_buffer_group_struct(void *object);

void
mad_foreach_free_buffer(void *object);

p_mad_buffer_pair_t
mad_alloc_buffer_pair_struct(void);

void
mad_free_buffer_pair_struct(p_mad_buffer_pair_t buffer_pair);

void
mad_foreach_free_buffer_pair_struct(void *object);

#endif /* __MAD_MEMORY_MANAGEMENT_H */
