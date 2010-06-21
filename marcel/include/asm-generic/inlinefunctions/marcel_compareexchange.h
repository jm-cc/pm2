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


#ifndef __ASM_GENERIC_INLINEFUNCTIONS_MARCEL_COMPAREEXCHANGE_H__
#define __ASM_GENERIC_INLINEFUNCTIONS_MARCEL_COMPAREEXCHANGE_H__


#include "asm/marcel_compareexchange.h"
#include <stdlib.h>
#include "tbx_compiler.h"
#include "tbx_intdef.h"
#include "asm/marcel_testandset.h"
#include "linux_spinlock.h"
#include "asm/linux_types.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
static __tbx_inline__ unsigned long
pm2_exchange(volatile void *ptr, unsigned long repl, int size)
{
	unsigned long prev;
	ma_spin_lock_softirq(&ma_compareexchange_spinlock);
	switch (size) {
	case 1:{
			volatile uint8_t *p = ptr;
			prev = *p;
			*p = repl;
			break;
		}
	case 2:{
			volatile uint16_t *p = ptr;
			prev = *p;
			*p = repl;
			break;
		}
	case 4:
		{
			volatile uint32_t *p = ptr;
			prev = *p;
			*p = repl;
			break;
		}
	case 8:
		{
			volatile uint64_t *p = ptr;
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
pm2_compareexchange(volatile void *ptr, unsigned long old, unsigned long repl, int size)
{
	unsigned long prev;
	ma_spin_lock_softirq(&ma_compareexchange_spinlock);
	switch (size) {
	case 1:{
			volatile uint8_t *p = ptr;
			if (old == (prev = *p))
				*p = repl;
			break;
		}
	case 2:{
			volatile uint16_t *p = ptr;
			if (old == (prev = *p))
				*p = repl;
			break;
		}
	case 4:
		{
			volatile uint32_t *p = ptr;
			if (old == (prev = *p))
				*p = repl;
			break;
		}
	case 8:
		{
			volatile uint64_t *p = ptr;
			if (old == (prev = *p))
				*p = repl;
			break;
		}
	default:
		abort();
	}
	ma_spin_unlock_softirq(&ma_compareexchange_spinlock);
	return prev;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_INLINEFUNCTIONS_MARCEL_COMPAREEXCHANGE_H__ **/
