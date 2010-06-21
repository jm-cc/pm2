/*! \file tbx_malloc.h
 *  \brief TBX memory allocators data structures.
 *
 *  This file provides TBX memory allocators data structures:
 *  - The fast memory allocator:   a fast specialized allocator dedicated
 *    to allocation of memory blocks of identical size.
 *
 */

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
 * tbx_malloc.h
 * ------------
 */

#ifndef TBX_MALLOC_H
#define TBX_MALLOC_H

/** \defgroup malloc_interface memory allocation interface
 *
 * This is the memory allocation interface
 *
 * @{
 */

/*
 * tbx_malloc
 * ----------
 */
typedef struct s_tbx_memory {
	int is_critical;
	void *first_mem;
	void *current_mem;
	size_t block_len;
	long mem_len;
	void *first_free;
	long first_new;
	long nb_allocated;
	const char *name;
} tbx_memory_t;

/* @} */

#endif				/* TBX_MALLOC_H */
