/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __SYS_MARCEL_GLUEC_STDMEM_H__
#define __SYS_MARCEL_GLUEC_STDMEM_H__


#include <stdlib.h>
#include <sys/mman.h>
#include "marcel_config.h"
#include "sys/marcel_alias.h"
#include "sys/marcel_gluec_realsym.h"
#include "tbx_types.h"


#ifndef MARCEL_LIBPTHREAD
#  define libc_calloc   calloc
#  define libc_malloc   malloc
#  define libc_valloc   valloc
#  define libc_free     free
#  define libc_realloc  realloc
#  define libc_memalign memalign
#endif


DEC_MARCEL(void*, calloc, (size_t nmemb, size_t size));
DEC_MARCEL(void*, malloc, (size_t size));
DEC_MARCEL(void*, valloc, (size_t size));
DEC_MARCEL(void, free, (void *ptr));
DEC_MARCEL(void*, realloc, (void *ptr, size_t size));
DEC_MARCEL(void*, mmap, (void *addr, size_t length, int prot, int flags, int fd, off_t offset));
DEC_MARCEL(int, munmap, (void *addr, size_t length));
DEC_MARCEL(char*, strdup, (const char *s));
DEC_MARCEL(char*, strndup, (const char *s, size_t n));
#if HAVE_DECL_MEMALIGN
DEC_MARCEL(void*, memalign, (size_t boundary, size_t size));
#if HAVE_DECL_POSIX_MEMALIGN
DEC_MARCEL(int, posix_memalign, (void **memptr, size_t alignment, size_t size));
#endif
#endif


#endif /** __SYS_MARCEL_GLUEC_STDMEM_H__ **/
