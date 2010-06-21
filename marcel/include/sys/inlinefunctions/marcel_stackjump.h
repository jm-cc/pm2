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


#ifndef __INLINEFUNCTIONS_SYS_MARCEL_STACKJUMP_H__
#define __INLINEFUNCTIONS_SYS_MARCEL_STACKJUMP_H__


#include "sys/marcel_stackjump.h"
#include "asm/marcel_archdep.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#if defined(ENABLE_STACK_JUMPING) && !defined(MA__SELF_VAR)
static __tbx_inline__ void marcel_prepare_stack_jump(void *stack)
{
	char *s = (char *) stack;
	*(marcel_t *) (s + THREAD_SLOT_SIZE - sizeof(char *)) = marcel_self();
}

static __tbx_inline__ void marcel_set_stack_jump(marcel_t m)
{
	register unsigned long sp = get_sp();
	*(marcel_t *) ((sp & ~(THREAD_SLOT_SIZE - 1)) + THREAD_SLOT_SIZE -
		       sizeof(void *)) = m;

}
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_SYS_MARCEL_STACKJUMP_H__ **/
