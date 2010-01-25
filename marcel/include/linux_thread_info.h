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


#ifndef __LINUX_THREAD_INFO_H__
#define __LINUX_THREAD_INFO_H__


/*
 * Copyright (C) 2002  David Howells (dhowells@redhat.com)
 * - Incorporating suggestions made by Linus Torvalds
 */


#include "sys/marcel_flags.h"


#ifdef __MARCEL_KERNEL__
#include "tbx_compiler.h"
#include "marcel_types.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal functions **/
/*
 * flag set/clear/test wrappers
 * - pass TIF_xxxx constants to these functions
 */
static __tbx_inline__ void ma_set_thread_flag(int flag);
static __tbx_inline__ void ma_clear_thread_flag(int flag);
static __tbx_inline__ int ma_test_and_set_thread_flag(int flag);
static __tbx_inline__ int ma_test_and_clear_thread_flag(int flag);
static __tbx_inline__ int ma_test_thread_flag(int flag);
static __tbx_inline__ void ma_set_ti_thread_flag(marcel_task_t *ti, int flag);
static __tbx_inline__ void ma_clear_ti_thread_flag(marcel_task_t *ti, int flag);
static __tbx_inline__ int ma_test_and_set_ti_thread_flag(marcel_task_t *ti, int flag);
static __tbx_inline__ int ma_test_and_clear_ti_thread_flag(marcel_task_t *ti, int flag);
static __tbx_inline__ int ma_test_ti_thread_flag(marcel_task_t *ti, int flag);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_THREAD_INFO_H__ **/
