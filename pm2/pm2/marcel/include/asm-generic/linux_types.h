
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

#section marcel_types

#include <limits.h>
#include <stdint.h>

typedef int8_t __ma_s8, ma_s8;
typedef uint8_t __ma_u8, ma_u8;

typedef int16_t __ma_s16, ma_s16;
typedef uint16_t __ma_u16, ma_u16;

typedef int32_t __ma_s32, ma_s32;
typedef uint32_t __ma_u32, ma_u32;

typedef int64_t __ma_s64, ma_s64;
typedef uint64_t __ma_u64, ma_u64;

#if ULONG_MAX == UINT8_MAX
#define MA_BITS_PER_LONG 8
#elif ULONG_MAX == UINT16_MAX
#define MA_BITS_PER_LONG 16
#elif ULONG_MAX == UINT32_MAX
#define MA_BITS_PER_LONG 32
#elif ULONG_MAX == UINT64_MAX
#define MA_BITS_PER_LONG 64
#else
#error "unsigned long too long !"
#endif
