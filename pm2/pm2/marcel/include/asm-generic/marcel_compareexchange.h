
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

#section marcel_variables
extern ma_spinlock_t compareexchange_spinlock;

#section marcel_functions
static __inline__ unsigned long
pm2_compareexchange(volatile void *ptr, unsigned long old,
		unsigned long new, int size) ;
#section marcel_inline
#depend "linux_spinlock.h"
#depend "asm/linux_types.h"
static __inline__ unsigned long
pm2_compareexchange(volatile void *ptr, unsigned long old,
		unsigned long new, int size)
{
	unsigned long prev;
	ma_spin_lock_softirq(&compareexchange_spinlock);
	switch (size) {
	case 1: {
			volatile ma_u8 *p = ptr;
			if (old == (prev=*p))
				*p = new;
			break;
		}
	case 2: {
			volatile ma_u16 *p = ptr;
			if (old == (prev=*p))
				*p = new;
			break;
		}
	case 4:
		{
			volatile ma_u32 *p = ptr;
			if (old == (prev=*p))
				*p = new;
			break;
		}
	}
	ma_spin_unlock_softirq(&compareexchange_spinlock);
	return prev;
}

#section marcel_macros
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))pm2_compareexchange((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
