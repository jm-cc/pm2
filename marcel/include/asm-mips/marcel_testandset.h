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


#ifndef __ASM_MIPS_MARCEL_TESTANDSET_H__
#define __ASM_MIPS_MARCEL_TESTANDSET_H__


#include "tbx_compiler.h"


/** Public macros **/
#define MA_HAVE_TESTANDSET 1


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define pm2_spinlock_release(spinlock) do { ma_mb(); (*(spinlock) = 0); } while(0)


/** Internal functions **/
static __tbx_inline__ long unsigned pm2_spinlock_testandset(volatile unsigned *spinlock);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_MIPS_MARCEL_TESTANDSET_H__ **/
