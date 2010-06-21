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


#ifndef __MARCEL_GLUE_PTHREAD_H__
#define __MARCEL_GLUE_PTHREAD_H__


#include "sys/marcel_flags.h"
#include <sched.h>
#include <pthread.h>


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal global variables **/
extern unsigned long int ma_fork_generation;


/** Internal functions **/
#ifdef MA__LIBPTHREAD

/** Convert an OS cpuset into a Marcel vpset */
int marcel_cpuset2vpset(size_t cpusetsize, const cpu_set_t * cpuset,
			marcel_vpset_t * vpset);
/** Convert a Marcel vpset into an OS cpuset */
int marcel_vpset2cpuset(const marcel_vpset_t * vpset, size_t cpusetsize,
			cpu_set_t * cpuset);

/* pthread_cleanup_push_defer/pop_restore() handlers for glibc.  */
void _pthread_cleanup_push_defer(struct _pthread_cleanup_buffer *buffer,
				 void (*routine) (void *), void *arg);
void _pthread_cleanup_pop_restore(struct _pthread_cleanup_buffer *buffer, int execute);

/*
 * When statically linked, this is called directly by the libc.
 * When dynamically linked, it is called by our constructor.
 */
void __pthread_initialize_minimal(void);
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_GLUE_PTHREAD_H__ **/
