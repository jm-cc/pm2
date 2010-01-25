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


#ifndef __ASM_SPARC_MARCEL_TESTANDSET_H__
#define __ASM_SPARC_MARCEL_TESTANDSET_H__


#include "tbx_compiler.h"


/** Public macros **/
#define MA_HAVE_TESTANDSET 1


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define pm2_spinlock_release(spinlock) \
  __asm__ __volatile__("stbar\n\tstb %1,%0" : "=m"(*(spinlock)) : "r"(0):"memory")


/** Internal functions **/
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_SPARC_MARCEL_TESTANDSET_H__ **/
