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


#ifndef __MARCEL_SYSDEP_H__
#define __MARCEL_SYSDEP_H__


#include "sys/marcel_flags.h"
#include "tbx_compiler.h"


#ifdef MA__LWPS
#  if defined(SOLARIS_SYS)
#    include <thread.h>
#  else /*  pas SOLARIS */
#    include <sched.h>
#  endif
#endif /*  MA__LWP */


/** Public functions **/
TBX_FMALLOC extern void *ma_malloc_node(size_t size, int node, char *file,
		unsigned line);
extern void ma_free_node(void *ptr, size_t size,
		char * __restrict file,  unsigned line);
extern void ma_migrate_mem(void *ptr, size_t size, int node);
extern int ma_is_numa_available(void);

#ifndef MM_HEAP_ENABLED
//refaire un malloc bas de gamme non numa
#define ma_malloc_node(size, node, file, line) marcel_malloc(size, file, line)
#define ma_free_node(ptr, size, file, line) marcel_free(ptr)
#define ma_migrate_mem(ptr, size, node) (void)0
#endif /* MM_HEAP_ENABLED */


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifndef MA__LWPS
#  define SCHED_YIELD()  do {} while(0)
#else /*  MA__LWP */
#  if defined(SOLARIS_SYS)
#    define SCHED_YIELD() \
do { \
	thr_yield(); \
} while (0)
#  else /*  pas SOLARIS */
#    define SCHED_YIELD() \
do { \
	sched_yield(); \
} while (0)
#  endif
#endif /*  MA__LWP */

#define ma_lwp_relax() SCHED_YIELD()


/** Internal global variables **/
#ifdef MA__NUMA
extern int ma_numa_not_available;
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_SYSDEP_H__ **/
