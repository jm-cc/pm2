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


#ifndef __LINUX_BITOPS_H__
#define __LINUX_BITOPS_H__


#include "sys/marcel_flags.h"
#include "marcel_compiler.h"
#include "asm/linux_bitops.h"
#include "tbx_compiler.h"
#include "linux_types.h"


/** Public functions **/
unsigned long marcel_ffs(unsigned long x);
unsigned long marcel_fls(unsigned long x);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal functions **/
static __tbx_inline__ unsigned long ma_generic_ffs(unsigned long x);
static __tbx_inline__ unsigned long ma_generic_fls(unsigned long x);
static __tbx_inline__ unsigned int ma_generic_hweight32(unsigned int w);
static __tbx_inline__ unsigned int ma_generic_hweight16(unsigned int w);
static __tbx_inline__ unsigned int ma_generic_hweight8(unsigned int w);
static __tbx_inline__ unsigned long ma_generic_hweight64(unsigned long long w);
static __tbx_inline__ unsigned long ma_hweight_long(unsigned long w);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_BITOPS_H__ **/
