
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

#section common
#include "tbx_compiler.h"
#depend "asm/marcel_testandset.h"

#section macros
#section marcel_variables
extern ma_spinlock_t ma_compareexchange_spinlock;

#section marcel_functions
static __tbx_inline__ unsigned long
pm2_compareexchange(volatile void *ptr, unsigned long old,
		unsigned long repl, int size) ;
#section marcel_inline
#depend "linux_spinlock.h[marcel_macros]"
#depend "asm/linux_types.h[marcel_types]"
#depend "[marcel_variables]"
#include <stdlib.h>
static __tbx_inline__ unsigned long
pm2_exchange(volatile void *ptr,
		unsigned long repl, int size)
{
	unsigned long prev;
	ma_spin_lock_softirq(&ma_compareexchange_spinlock);
	switch (size) {
	case 1: {
			volatile ma_u8 *p = ptr;
			prev = *p;
			*p = repl;
			break;
		}
	case 2: {
			volatile ma_u16 *p = ptr;
			prev = *p;
			*p = repl;
			break;
		}
	case 4:
		{
			volatile ma_u32 *p = ptr;
			prev = *p;
			*p = repl;
			break;
		}
        case 8:
                {
                        volatile ma_u64 *p = ptr;
			prev = *p;
			*p = repl;
                        break;
                }
	default:
		abort();
	}
	ma_spin_unlock_softirq(&ma_compareexchange_spinlock);
	return prev;
}
#define ma_xchg(ptr,v) ((__typeof__(*(ptr)))pm2_exchange((ptr),(unsigned long)(v),sizeof(*(ptr))))
static __tbx_inline__ unsigned long
pm2_compareexchange(volatile void *ptr, unsigned long old,
		unsigned long repl, int size)
{
	unsigned long prev;
	ma_spin_lock_softirq(&ma_compareexchange_spinlock);
	switch (size) {
	case 1: {
			volatile ma_u8 *p = ptr;
			if (old == (prev=*p))
				*p = repl;
			break;
		}
	case 2: {
			volatile ma_u16 *p = ptr;
			if (old == (prev=*p))
				*p = repl;
			break;
		}
	case 4:
		{
			volatile ma_u32 *p = ptr;
			if (old == (prev=*p))
				*p = repl;
			break;
		}
        case 8:
                {
                        volatile ma_u64 *p = ptr;
                        if (old == (prev=*p))
                                *p = repl;
                        break;
                }
	default:
		abort();
	}
	ma_spin_unlock_softirq(&ma_compareexchange_spinlock);
	return prev;
}

#section marcel_macros
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))pm2_compareexchange((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
