
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

/*
 * tbx_safe_malloc
 * ---------------
 */
typedef enum 
{
  tbx_safe_malloc_ERRORS_ONLY,
  tbx_safe_malloc_VERBOSE
} tbx_safe_malloc_mode_t, *p_tbx_safe_malloc_mode_t;


/*
 * tbx_malloc
 * ----------
 */
typedef struct s_tbx_memory
{
  TBX_SHARED; 
  void    *first_mem;
  void    *current_mem;
  size_t   block_len;
  long     mem_len;
  void    *first_free;
  long     first_new;
} tbx_memory_t;

#endif /* TBX_MALLOC_H */
