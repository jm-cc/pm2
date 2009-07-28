
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
#include "tbx_compiler.h"
/*
 * similar to:
 * include/linux/timer.h
 */

#section marcel_macros
/*
 * per-LWP timer vector definitions:
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

#section marcel_types
typedef struct ma_tvec_t_base_s ma_tvec_base_t;

#section structures
struct ma_timer_list {
	struct tbx_fast_list_head entry;
	unsigned long expires;

	ma_spinlock_t lock;
	unsigned long magic;

	void (*function)(unsigned long);
	unsigned long data;

	struct ma_tvec_t_base_s *base;
};

#section marcel_structures
typedef struct ma_tvec_s {
	struct tbx_fast_list_head vec[TVN_SIZE];
} ma_tvec_t;

typedef struct ma_tvec_root_s {
	struct tbx_fast_list_head vec[TVR_SIZE];
} ma_tvec_root_t;

struct ma_tvec_t_base_s {
	ma_spinlock_t lock;
	unsigned long timer_jiffies;
	struct ma_timer_list *running_timer;
	ma_tvec_root_t tv1;
	ma_tvec_t tv2;
	ma_tvec_t tv3;
	ma_tvec_t tv4;
	ma_tvec_t tv5;
};/*  ____cacheline_aligned_in_smp; */

#section marcel_macros
#define MA_TIMER_MAGIC	0x4b87ad6e

#define MA_TIMER_INITIALIZER(_function, _expires, _data) {	\
		.function = (_function),			\
		.expires = (_expires),				\
		.data = (_data),				\
		.base = NULL,					\
		.magic = MA_TIMER_MAGIC,			\
		.lock = MA_SPIN_LOCK_UNLOCKED,			\
	}

#section marcel_inline
/***
 * ma_init_timer - initialize a timer.
 * @timer: the timer to be initialized
 *
 * init_timer() must be done to a timer prior calling *any* of the
 * other timer functions.
 */
static __tbx_inline__ void ma_init_timer(struct ma_timer_list * timer)
{
	timer->base = NULL;
	timer->magic = MA_TIMER_MAGIC;
	ma_spin_lock_init(&timer->lock);
}

/***
 * ma_timer_pending - is a timer pending?
 * @timer: the timer in question
 *
 * ma_timer_pending will tell whether a given timer is currently pending,
 * or not. Callers must ensure serialization wrt. other operations done
 * to this timer, eg. interrupt contexts, or other CPUs on SMP.
 *
 * return value: 1 if the timer is pending, 0 if not.
 */
static __tbx_inline__ int ma_timer_pending(const struct ma_timer_list * timer)
{
	return timer->base != NULL;
}

#section marcel_functions
extern void ma_add_timer_on(struct ma_timer_list *timer, ma_lwp_t lwp);
extern TBX_EXTERN int ma_del_timer(struct ma_timer_list * timer);
extern TBX_EXTERN int __ma_mod_timer(struct ma_timer_list *timer, unsigned long expires);
extern TBX_EXTERN int ma_mod_timer(struct ma_timer_list *timer, unsigned long expires);

#section marcel_inline
/***
 * ma_add_timer - start a timer
 * @timer: the timer to be added
 *
 * The kernel will do a ->function(->data) callback from the
 * timer interrupt at the ->expired point in the future. The
 * current time is 'jiffies'.
 *
 * The timer's ->expired, ->function (and if the handler uses it, ->data)
 * fields must be set prior calling this function.
 *
 * Timers with an ->expired field in the past will be executed in the next
 * timer tick.
 */
static __tbx_inline__ void ma_add_timer(struct ma_timer_list * timer)
{
	__ma_mod_timer(timer, timer->expires);
}

#section marcel_functions
#ifdef MA__LWPS
  extern TBX_EXTERN int ma_del_timer_sync(struct ma_timer_list * timer);
#else
# define ma_del_timer_sync(t) ma_del_timer(t)
#endif

extern void ma_init_timers(void);
extern void ma_update_process_times(int user_tick);

