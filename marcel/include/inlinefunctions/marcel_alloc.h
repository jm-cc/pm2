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


#ifndef __INLINEFUNCTIONS_MARCEL_ALLOC_H__
#define __INLINEFUNCTIONS_MARCEL_ALLOC_H__


#include "marcel_alloc.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL
/** Internal inline functions **/
static __tbx_inline__ void ma_free_task_stack(marcel_t task)
{
	switch (task->stack_kind) {
		/* TODO free them somewhere more appropriate than the current level? */
	case MA_DYNAMIC_STACK:
		marcel_tls_slot_free(marcel_stackbase(task), NULL);
		break;
	case MA_STATIC_STACK:
		marcel_tls_detach(task);
		break;
	case MA_NO_STACK:
		ma_obj_free(marcel_thread_seed_allocator, task, NULL);
		break;
	default:
		MA_BUG();
	}
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/
#endif /** __INLINEFUNCTIONS_MARCEL_ALLOC_H__ **/
