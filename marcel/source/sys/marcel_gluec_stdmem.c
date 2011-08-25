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


#include "marcel.h"
#include "marcel_pmarcel.h"


DEF_MARCEL_PMARCEL(void*, calloc, (size_t nmemb, size_t size), (nmemb, size),
{
	MA_CALL_LIBC_FUNC(void*, libc_calloc, nmemb, size);
})
DEF_C(void*, calloc, (size_t nmemb, size_t size), (nmemb, size))

DEF_MARCEL_PMARCEL(void*, malloc, (size_t size), (size),
{
	MA_CALL_LIBC_FUNC(void*, libc_malloc, size);
})
DEF_C(void*, malloc, (size_t size), (size))

DEF_MARCEL_PMARCEL(void*, valloc, (size_t size), (size),
{
	MA_CALL_LIBC_FUNC(void*, libc_valloc, size);
})
DEF_C(void*, valloc, (size_t size), (size))

DEF_MARCEL_PMARCEL(void, free, (void *ptr), (ptr),
{
	marcel_extlib_protect();
	libc_free(ptr);
	marcel_extlib_unprotect();
})
DEF_C(void, free, (void *ptr), (ptr))

DEF_MARCEL_PMARCEL(void*, realloc, (void *ptr, size_t size), (ptr, size),
{
	MA_CALL_LIBC_FUNC(void*, libc_realloc, ptr, size);
})
DEF_C(void*, realloc, (void *ptr, size_t size), (ptr, size))

DEF_MARCEL_PMARCEL(void*, mmap, (void *addr, size_t length, int prot, int flags, int fd, off_t offset),
		   (addr, length, prot, flags, fd, offset),
{
	MA_CALL_LIBC_FUNC(void *, MA_REALSYM(mmap), addr, length, prot, flags, fd, offset);
})
DEF_C(void*, mmap, (void *addr, size_t length, int prot, int flags, int fd, off_t offset),
	 (addr, length, prot, flags, fd, offset))

DEF_MARCEL_PMARCEL(int, munmap, (void *addr, size_t length), (addr, length),
{
	MA_CALL_LIBC_FUNC(int, MA_REALSYM(munmap), addr, length);
})
DEF_C(int, munmap, (void *addr, size_t length), (addr, length))

DEF_MARCEL_PMARCEL(char*, strdup, (const char *s), (s),
{
	return marcel_strndup(s, strlen(s+1));
})
DEF_C(char*, strdup, (const char *s), (s))

DEF_MARCEL_PMARCEL(char*, strndup, (const char *s, size_t n), (s, n),
{
	char *copy;

	copy = NULL;
	if (s && n) {
		copy = marcel_malloc(n+1);
		if (copy) {
			strncpy(copy, s, n);
			copy[n] = 0;
		}
	}
	
	return copy;
})
DEF_C(char*, strndup, (const char *s, size_t n), (s, n))

#if HAVE_DECL_MEMALIGN
DEF_MARCEL_PMARCEL(void*, memalign, (size_t boundary, size_t size), (boundary, size),
{
	MA_CALL_LIBC_FUNC(void*, libc_memalign, boundary, size);
})
DEF_C(void*, memalign, (size_t boundary, size_t size), (boundary, size))

#if HAVE_DECL_POSIX_MEMALIGN
DEF_MARCEL_PMARCEL(int, posix_memalign, (void **memptr, size_t alignment, size_t size),
		   (memptr, alignment, size),
{
	if (memptr) {
		*memptr = marcel_memalign(alignment, size);
		if ((NULL == *memptr)) {
			if (((0 != alignment % 2) || (0 != size % sizeof(void *))))
				return EINVAL;
			return ENOMEM;
		}
		
		return 0;
	}

	return EINVAL;
})
DEF_C(int, posix_memalign, (void **memptr, size_t alignment, size_t size), (memptr, alignment, size))
#endif
#endif
