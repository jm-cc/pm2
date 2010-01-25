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


#ifndef __INLINEFUNCTIONS_LINUX_JIFFIES_H__
#define __INLINEFUNCTIONS_LINUX_JIFFIES_H__


#include "tbx_compiler.h"
#include "linux_jiffies.h"


/** Public global variables **/
/*
 * The 64-bit value is not volatile - you MUST NOT read it
 * without sampling the sequence number in xtime_lock.
 * get_jiffies_64() will do this for you as appropriate.
 */
#if (MA_BITS_PER_LONG >= 64)
static __tbx_inline__ ma_u64 ma_get_jiffies_64(void)
{
	return (ma_u64)ma_jiffies;
}
#endif


#endif /** __INLINEFUNCTIONS_LINUX_JIFFIES_H__ **/
