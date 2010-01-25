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


#ifndef __LINUX_TYPES_H__
#define __LINUX_TYPES_H__


#include "sys/marcel_flags.h"
#include "asm/linux_types.h"


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define MA_BITS_TO_LONGS(bits) \
	(((bits)+MA_BITS_PER_LONG-1)/MA_BITS_PER_LONG)
#define MA_DECLARE_BITMAP(name,bits) \
	unsigned long name[MA_BITS_TO_LONGS(bits)]
#define MA_CLEAR_BITMAP(name,bits) \
	memset(name, 0, MA_BITS_TO_LONGS(bits)*sizeof(unsigned long))


/** Internal data types **/
#ifndef __MA_BIT_TYPES_DEFINED__
#define __MA_BIT_TYPES_DEFINED__

typedef		__ma_u8		ma_u_int8_t;
typedef		__ma_s8		ma_int8_t;
typedef		__ma_u16	ma_u_int16_t;
typedef		__ma_s16	ma_int16_t;
typedef		__ma_u32	ma_u_int32_t;
typedef		__ma_s32	ma_int32_t;

#endif /* !(__MA_BIT_TYPES_DEFINED__) */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_TYPES_H__ **/
