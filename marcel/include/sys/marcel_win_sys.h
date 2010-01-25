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


#ifndef __SYS_MARCEL_WIN_SYS_H__
#define __SYS_MARCEL_WIN_SYS_H__


#ifdef __MINGW32__
#define ENOTSUP ENOSYS
#define ETIMEDOUT EAGAIN
#endif

void marcel_win_sys_init(void);

#ifdef WIN_SYS

#include <sys/time.h>

#ifdef __CYGWIN__
#include <sys/mman.h>
#else

#define MAP_FAILED NULL
#ifndef __WIN_MMAP_KERNEL__
#define mmap(a, l, p, f, fd, o)    win_mmap(a, l)
#define munmap(a, l)               win_munmap(a, l)
#endif

extern void * win_mmap(void * __addr, size_t __len);

extern int win_munmap(void * __addr, size_t __len);

#endif

#endif


#endif /** __SYS_MARCEL_WIN_SYS_H__ **/

