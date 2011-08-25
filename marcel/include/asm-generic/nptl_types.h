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


#ifndef __ASM_GENERIC_NPTL_TYPES_H__
#define __ASM_GENERIC_NPTL_TYPES_H__


#include "sys/marcel_fastlock.h"
#include "sys/marcel_types.h"


/** Public data structures **/
/* Data structure for read-write lock variable handling.  The
   structure of the attribute type is not exposed on purpose.  */
typedef union {
	struct {
		struct _lpt_fastlock __lock;
		unsigned int __nr_readers;
		struct _lpt_fastlock __readers_wakeup;
		struct _lpt_fastlock __writer_wakeup;
		unsigned int __nr_readers_queued;
		unsigned int __nr_writers_queued;
		unsigned char __flags;
		unsigned char __shared;
		marcel_t __writer;
	} __data;
	long int __align;
} lpt_rwlock_t;


typedef union {
	struct marcel_rwlockattr __attr;
	long int __align;
} lpt_rwlockattr_t;


#endif /** __ASM_GENERIC_NPTL_TYPES_H__ **/
