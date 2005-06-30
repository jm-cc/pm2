
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


#ifndef DSM_BITMAP_IS_DEF
#define DSM_BITMAP_IS_DEF

#include "sys/bitmap.h"
#include "marcel.h" // tmalloc

typedef unsigned int* dsm_bitmap_t;

/* For the following function, size is in bits */

#define dsm_bitmap_alloc(size) \
  ((dsm_bitmap_t) tcalloc(((size) % 8) ? ((size) / 8 + 1) : ((size) / 8), 1))
  
#define dsm_bitmap_free(b) tfree(b)

#define dsm_bitmap_mark_dirty(offset, length, bitmap) set_bits_to_1(offset, length, bitmap)

#define dsm_bitmap_is_empty(b, size) bitmap_is_empty(b, size)

#define dsm_bitmap_clear(b,size) clear_bitmap(b, size)

#endif
