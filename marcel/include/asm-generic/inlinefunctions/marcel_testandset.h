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


#ifndef __ASM_GENERIC_INLINEFUNCTIONS_MARCEL_TESTANDSET_H__
#define __ASM_GENERIC_INLINEFUNCTIONS_MARCEL_TESTANDSET_H__


#include "asm/marcel_testandset.h"
#include "tbx_compiler.h"
#ifndef MA_HAVE_COMPAREEXCHANGE
#include "linux_spinlock.h"
#include "asm/linux_types.h"
#endif


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
#ifndef MA_HAVE_COMPAREEXCHANGE
static __tbx_inline__ unsigned __tbx_deprecated__ 
pm2_spinlock_testandset(volatile unsigned *spinlock)
{
	unsigned ret;
	ma_spin_lock_softirq(&testandset_spinlock);
	if (!(ret = *spinlock))
		*spinlock = 1;
	ma_spin_unlock_softirq(&testandset_spinlock);
	return ret;
}
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_INLINEFUNCTIONS_MARCEL_TESTANDSET_H__ **/
