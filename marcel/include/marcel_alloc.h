
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

#section variables
#depend "marcel_allocator.h[marcel_types]"
extern ma_allocator_t *marcel_mapped_slot_allocator, *marcel_unmapped_slot_allocator;

#section functions
#depend "tbx_compiler.h"

TBX_FMALLOC void *marcel_slot_alloc(void);
void marcel_slot_free(void *addr);
void marcel_slot_exit(void);
#define marcel_slot_alloc() ma_obj_alloc(marcel_mapped_slot_allocator)
#define marcel_slot_free(addr) ma_obj_free(marcel_mapped_slot_allocator, addr)


/* ======= MT-Safe functions from standard library ======= */

#section macros
#define tmalloc(size)          marcel_malloc(size, __FILE__, __LINE__)
#define trealloc(ptr, size)    marcel_realloc(ptr, size, __FILE__, __LINE__)
#define tcalloc(nelem, elsize) marcel_calloc(nelem, elsize, __FILE__, __LINE__)
#define tfree(ptr)             marcel_free(ptr, __FILE__, __LINE__)

#section functions
TBX_FMALLOC void *marcel_malloc(unsigned size, char *file, unsigned line);
TBX_FMALLOC void *marcel_realloc(void *ptr, unsigned size, char * __restrict file, unsigned line);
TBX_FMALLOC void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line);
void marcel_free(void *ptr, char * __restrict file, unsigned line);

