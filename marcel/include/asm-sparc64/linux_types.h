
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#section common
/*
 * similar to:
 * include/asm-sparc/types.h
 */

#section marcel_types

/*
 * _xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space.
 */

/*
 * This file is never included by application software unless
 * explicitly requested (e.g., via linux/types.h) in which case the
 * application is Linux specific so (user-) name space pollution is
 * not a major issue.  However, for interoperability, libraries still
 * need to be careful to avoid a name clashes.
 */

//#ifndef __ASSEMBLY__

//typedef unsigned short umode_t;

typedef __signed__ char __ma_s8;
typedef unsigned char __ma_u8;

typedef __signed__ short __ma_s16;
typedef unsigned short __ma_u16;

typedef __signed__ int __ma_s32;
typedef unsigned int __ma_u32;

typedef __signed__ long __ma_s64;
typedef unsigned long __ma_u64;

//#endif /* __ASSEMBLY__ */

//#ifdef __KERNEL__

#section macros
#define MA_BITS_PER_LONG 64
#section marcel_types

//#ifndef __ASSEMBLY__

typedef __signed__ char ma_s8;
typedef unsigned char ma_u8;

typedef __signed__ short ma_s16;
typedef unsigned short ma_u16;

typedef __signed__ int ma_s32;
typedef unsigned int ma_u32;

typedef __signed__ long ma_s64;
typedef unsigned long ma_u64;

//typedef u32 dma_addr_t;
//typedef u64 dma64_addr_t;

//#endif /* __ASSEMBLY__ */

//#endif /* __KERNEL__ */

