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


#ifndef __MARCEL_COMPAREEXCHANGE_H__
#define __MARCEL_COMPAREEXCHANGE_H__


#include "tbx_compiler.h"
#include "asm/marcel_testandset.h"
#include "asm/linux_spinlock.h"


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))pm2_compareexchange((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))


/** Internal global variables **/
extern ma_spinlock_t ma_compareexchange_spinlock;


/** Internal functions **/
static __tbx_inline__ unsigned long
pm2_compareexchange(volatile void *ptr, unsigned long old,
		unsigned long repl, int size) ;


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_COMPAREEXCHANGE_H__ **/
