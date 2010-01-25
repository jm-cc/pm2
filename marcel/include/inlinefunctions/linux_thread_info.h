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


#ifndef __INLINEFUNCTIONS_LINUX_THREAD_INFO_H__
#define __INLINEFUNCTIONS_LINUX_THREAD_INFO_H__


/*
 * Copyright (C) 2002  David Howells (dhowells@redhat.com)
 * - Incorporating suggestions made by Linus Torvalds
 */


#include "linux_thread_info.h"
#ifdef __MARCEL_KERNEL__
#include "tbx_compiler.h"
#include "asm/linux_bitops.h"
#include "marcel_descr.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
static __tbx_inline__ void ma_set_thread_flag(int flag)
{
	ma_set_bit(flag,&SELF_GETMEM(flags));
}

static __tbx_inline__ void ma_clear_thread_flag(int flag)
{
	ma_clear_bit(flag,&SELF_GETMEM(flags));
}

static __tbx_inline__ int ma_test_and_set_thread_flag(int flag)
{
	return ma_test_and_set_bit(flag,&SELF_GETMEM(flags));
}

static __tbx_inline__ int ma_test_and_clear_thread_flag(int flag)
{
	return ma_test_and_clear_bit(flag,&SELF_GETMEM(flags));
}

static __tbx_inline__ int ma_test_thread_flag(int flag)
{
	return ma_test_bit(flag,&SELF_GETMEM(flags));
}

static __tbx_inline__ void ma_set_ti_thread_flag(marcel_task_t *ti, int flag)
{
        ma_set_bit(flag,&ti->flags);
}

static __tbx_inline__ void ma_clear_ti_thread_flag(marcel_task_t *ti, int flag)
{
        ma_clear_bit(flag,&ti->flags);
}

static __tbx_inline__ int ma_test_and_set_ti_thread_flag(marcel_task_t *ti, int flag)
{
        return ma_test_and_set_bit(flag,&ti->flags);
}

static __tbx_inline__ int ma_test_and_clear_ti_thread_flag(marcel_task_t *ti, int flag
)
{
        return ma_test_and_clear_bit(flag,&ti->flags);
}

static __tbx_inline__ int ma_test_ti_thread_flag(marcel_task_t *ti, int flag)
{
        return ma_test_bit(flag,&ti->flags);
}

/* set thread flags in other task's structures
 * - see asm/thread_info.h for TIF_xxxx flags available
 */
static __tbx_inline__ void ma_set_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	ma_set_ti_thread_flag(tsk,flag);
}

static __tbx_inline__ void ma_clear_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	ma_clear_ti_thread_flag(tsk,flag);
}

static __tbx_inline__ int ma_test_and_set_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	return ma_test_and_set_ti_thread_flag(tsk,flag);
}

static __tbx_inline__ int ma_test_and_clear_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	return ma_test_and_clear_ti_thread_flag(tsk,flag);
}

static __tbx_inline__ int ma_test_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	return ma_test_ti_thread_flag(tsk,flag);
}

static __tbx_inline__ void ma_set_tsk_need_togo(marcel_task_t *tsk)
{
	ma_set_tsk_thread_flag(tsk,TIF_NEED_TOGO);
}

static __tbx_inline__ void ma_clear_tsk_need_togo(marcel_task_t *tsk)
{
	ma_clear_tsk_thread_flag(tsk,TIF_NEED_TOGO);
}

static __tbx_inline__ int ma_signal_pending(marcel_task_t *p)
{
	return tbx_unlikely(ma_test_tsk_thread_flag(p,TIF_WORKPENDING));
}
  
static __tbx_inline__ int ma_need_togo(void)
{
	return tbx_unlikely(ma_test_thread_flag(TIF_NEED_TOGO));
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_LINUX_THREAD_INFO_H__ **/
