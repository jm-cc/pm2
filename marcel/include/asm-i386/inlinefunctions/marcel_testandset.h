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


#ifndef __ASM_I386_INLINEFUNCTIONS_MARCEL_TESTANDSET_H__
#define __ASM_I386_INLINEFUNCTIONS_MARCEL_TESTANDSET_H__


#include "asm/marcel_testandset.h"
#include "asm/linux_atomic.h"
#include "tbx_compiler.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
static __tbx_inline__ unsigned __tbx_deprecated__
pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  unsigned ret;

  __asm__ __volatile__(
       MA_LOCK_PREFIX "xchgl %0, %1"
       : "=q"(ret), "=m"(*spinlock)
       : "0"(1), "m"(*spinlock)
       : "memory");

  return ret;
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_INLINEFUNCTIONS_MARCEL_TESTANDSET_H__ **/
