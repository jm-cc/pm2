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


#ifndef __ASM_GENERIC_MARCEL_TESTANDSET_H__
#define __ASM_GENERIC_MARCEL_TESTANDSET_H__


#include "tbx_compiler.h"
#include "asm/marcel_compareexchange.h"


/** Public macros **/
#ifdef MA_HAVE_COMPAREEXCHANGE
#define MA_HAVE_TESTANDSET 1
#endif


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#ifdef MA_HAVE_COMPAREEXCHANGE
#define pm2_spinlock_testandset(spinlock) ma_cmpxchg(spinlock, 0, 1)
#define pm2_spinlock_release(spinlock) (void)ma_cmpxchg(spinlock, 1, 0)
#else
#define pm2_spinlock_release(spinlock) do { ma_mb(); (*(spinlock) = 0); } while(0)
#endif


/** Internal global variables **/
#ifndef MA_HAVE_COMPAREEXCHANGE
extern ma_spinlock_t testandset_spinlock;
#endif


/** Internal functions **/
#ifndef MA_HAVE_COMPAREEXCHANGE
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
    __tbx_deprecated__;
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_MARCEL_TESTANDSET_H__ **/
