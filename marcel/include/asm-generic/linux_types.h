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


#ifndef __ASM_GENERIC_LINUX_TYPES_H__
#define __ASM_GENERIC_LINUX_TYPES_H__


#include <limits.h>
#include "tbx_intdef.h"


#ifndef IRIX_SYS

#ifndef ULONG_MAX
#  error ULONG_MAX undefined
#endif
#ifndef UINT32_MAX
# error UINT32_MAX undefined
#endif

#if ULONG_MAX == UINT8_MAX
#define MA_BITS_PER_LONG 8
#elif ULONG_MAX == UINT16_MAX
#define MA_BITS_PER_LONG 16
#elif ULONG_MAX == UINT32_MAX
#define MA_BITS_PER_LONG 32
#elif ULONG_MAX == UINT64_MAX
#define MA_BITS_PER_LONG 64
#else
#error "unknown size for unsigned long."
#endif

#else				/* IRIX_SYS */

#ifdef MIPS_ARCH
#define MA_BITS_PER_LONG _MIPS_SZLONG
#else
#error "unknown arch for Irix"
#endif
#endif				/* IRIX_SYS */


#endif /** __ASM_GENERIC_LINUX_TYPES_H__ **/
