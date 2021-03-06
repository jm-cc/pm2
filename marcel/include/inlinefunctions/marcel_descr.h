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


#ifndef __INLINEFUNCTIONS_MARCEL_DESCR_H__
#define __INLINEFUNCTIONS_MARCEL_DESCR_H__


#include "marcel_descr.h"
#ifndef MA__SELF_VAR
#include "asm/marcel_archdep.h"
#include "marcel_threads.h"
#endif


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#ifdef MA__SELF_VAR
static inline TBX_NOINST marcel_t ma_self(void)
{
	return __ma_self;
}
#else
static inline TBX_NOINST marcel_t ma_self(void)
{
	marcel_t self;
	register unsigned long sp = get_sp();

#ifdef STANDARD_MAIN
	if (sp >= ma_main_stacklimit)
		self = &__main_thread_struct;
	else
#endif
	{
#ifdef ENABLE_STACK_JUMPING
		self = *((marcel_t *) (((sp & ~(THREAD_SLOT_SIZE - 1)) +
					THREAD_SLOT_SIZE - sizeof(void *))));
#else
		self = ma_slot_sp_task(sp);
#endif
		/* Detect stack overflow. If you encounter
		 * this, increase THREAD_SLOT_SIZE */
		MA_BUG_ON(sp >= (unsigned long) self && sp < (unsigned long) (self + 1));
	}
	return self;
}
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_DESCR_H__ **/
