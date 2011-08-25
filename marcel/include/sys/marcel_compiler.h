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


#ifndef __SYS_MARCEL_COMPILER_H__
#define __SYS_MARCEL_COMPILER_H__


#include "marcel_config.h"
#include "tbx_compiler.h"


/** Public macros **/
#define ma_barrier() tbx_barrier()
#define ma_access_once(x) (* (volatile __typeof__(x) *)&(x))


#if defined(__MARCEL_KERNEL__) && (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 3))
#  error Your gcc is too old for Marcel, please upgrade to at least 3.3
#endif


/* to handle unused parameter warnings in some functions */
#ifdef MA__BUBBLES
#define BUBBLE_VAR_UNUSED
#else
#define BUBBLE_VAR_UNUSED TBX_UNUSED
#endif
#ifdef MA__LWPS
#define LWPS_VAR_UNUSED
#else
#define LWPS_VAR_UNUSED TBX_UNUSED
#endif
#ifdef MA__TIMER
#define TIMER_VAR_UNUSED
#else
#define TIMER_VAR_UNUSED TBX_UNUSED
#endif
#ifdef MARCEL_STATS_ENABLED
#define STATS_VAR_UNUSED
#else
#define STATS_VAR_UNUSED TBX_UNUSED
#endif


#endif /** __SYS_MARCEL_COMPILER_H__ **/
