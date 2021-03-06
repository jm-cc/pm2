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


#ifndef __SYS_LINUX_JIFFIES_H__
#define __SYS_LINUX_JIFFIES_H__


#include "tbx_compiler.h"
#include "marcel_config.h"
#include "sys/linux_types.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
/*
 *	These inlines deal with timer wrapping correctly. You are 
 *	strongly encouraged to use them
 *	1. Because people otherwise forget
 *	2. Because if the timer wrap changes in future you won't have to
 *	   alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define ma_time_after(a,b)		\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)(b) - (long)(a) < 0))
#define ma_time_before(a,b)	ma_time_after(b,a)
#define ma_time_after_eq(a,b)	\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)(a) - (long)(b) >= 0))
#define ma_time_before_eq(a,b)	ma_time_after_eq(b,a)

/*
 * The 64-bit value is not volatile - you MUST NOT read it
 * without sampling the sequence number in xtime_lock.
 * get_jiffies_64() will do this for you as appropriate.
 */
extern unsigned long long ma_jiffies_64;
extern unsigned long volatile ma_jiffies;

#if (MA_BITS_PER_LONG < 64)
unsigned long long ma_get_jiffies_64(void);
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_LINUX_JIFFIES_H__ **/
