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


#ifndef __ASM_IA64_LINUX_TYPES_H__
#define __ASM_IA64_LINUX_TYPES_H__


/** Public macros **/
#define MA_BITS_PER_LONG 64


/** Public data types **/
/*
 #ifdef __ASSEMBLY__
 # define __IA64_UL(x)		(x)
 # define __IA64_UL_CONST(x)	x
 #else
 # define __IA64_UL(x)		((unsigned long)(x))
 # define __IA64_UL_CONST(x)	x##UL
 #endif

 #ifndef __ASSEMBLY__

// typedef unsigned int umode_t;
*/

typedef __signed__ char __ma_s8;
typedef unsigned char __ma_u8;

typedef __signed__ short __ma_s16;
typedef unsigned short __ma_u16;

typedef __signed__ int __ma_s32;
typedef unsigned int __ma_u32;

typedef __signed__ long __ma_s64;
typedef unsigned long __ma_u64;

typedef __ma_s8 ma_s8;
typedef __ma_u8 ma_u8;

typedef __ma_s16 ma_s16;
typedef __ma_u16 ma_u16;

typedef __ma_s32 ma_s32;
typedef __ma_u32 ma_u32;

typedef __ma_s64 ma_s64;
typedef __ma_u64 ma_u64;

/* DMA addresses are 64-bits wide, in general.  */
typedef ma_u64 ma_dma_addr_t;


#endif /** __ASM_IA64_LINUX_TYPES_H__ **/
