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


#ifndef __SYS_MARCEL_PRIVATEDEFS_H__
#define __SYS_MARCEL_PRIVATEDEFS_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "marcel_config.h"
#include "marcel_types.h"


#ifdef __MARCEL_KERNEL__


/** Internal functions **/
#ifdef ENABLE_STACK_JUMPING
static __tbx_inline__ void marcel_prepare_stack_jump(void *stack);
static __tbx_inline__ void marcel_set_stack_jump(marcel_t m);
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_PRIVATEDEFS_H__ **/
