
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
 * thread_info.h: common low-level thread information accessors
 *
 * Copyright (C) 2002  David Howells (dhowells@redhat.com)
 * - Incorporating suggestions made by Linus Torvalds
 */

#section marcel_functions
/*
 * flag set/clear/test wrappers
 * - pass TIF_xxxx constants to these functions
 */

static inline void ma_set_thread_flag(int flag);
static inline void ma_clear_thread_flag(int flag);
static inline int ma_test_and_set_thread_flag(int flag);
static inline int ma_test_and_clear_thread_flag(int flag);
static inline int ma_test_thread_flag(int flag);
static inline void ma_set_ti_thread_flag(marcel_task_t *ti, int flag);
static inline void ma_clear_ti_thread_flag(marcel_task_t *ti, int flag);
static inline int ma_test_and_set_ti_thread_flag(marcel_task_t *ti, int flag);
static inline int ma_test_and_clear_ti_thread_flag(marcel_task_t *ti, int flag);
static inline int ma_test_ti_thread_flag(marcel_task_t *ti, int flag);
static inline void ma_set_need_resched(void);
static inline void ma_clear_need_resched(void);

#section marcel_inline
static inline void ma_set_thread_flag(int flag)
{
	ma_set_bit(flag,&MARCEL_SELF->flags);
}

static inline void ma_clear_thread_flag(int flag)
{
	ma_clear_bit(flag,&MARCEL_SELF->flags);
}

static inline int ma_test_and_set_thread_flag(int flag)
{
	return ma_test_and_set_bit(flag,&MARCEL_SELF->flags);
}

static inline int ma_test_and_clear_thread_flag(int flag)
{
	return ma_test_and_clear_bit(flag,&MARCEL_SELF->flags);
}

static inline int ma_test_thread_flag(int flag)
{
	return ma_test_bit(flag,&MARCEL_SELF->flags);
}

static inline void ma_set_ti_thread_flag(marcel_task_t *ti, int flag)
{
        ma_set_bit(flag,&ti->flags);
}

static inline void ma_clear_ti_thread_flag(marcel_task_t *ti, int flag)
{
        ma_clear_bit(flag,&ti->flags);
}

static inline int ma_test_and_set_ti_thread_flag(marcel_task_t *ti, int flag)
{
        return ma_test_and_set_bit(flag,&ti->flags);
}

static inline int ma_test_and_clear_ti_thread_flag(marcel_task_t *ti, int flag
)
{
        return ma_test_and_clear_bit(flag,&ti->flags);
}

static inline int ma_test_ti_thread_flag(marcel_task_t *ti, int flag)
{
        return ma_test_bit(flag,&ti->flags);
}

static inline void ma_set_need_resched(void)
{
        ma_set_thread_flag(TIF_NEED_RESCHED);
}

static inline void ma_clear_need_resched(void)
{
        ma_clear_thread_flag(TIF_NEED_RESCHED);
}


