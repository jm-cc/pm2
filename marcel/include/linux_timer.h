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


#ifndef __LINUX_TIMER_H__
#define __LINUX_TIMER_H__


#include <limits.h>
#include "tbx_compiler.h"
#include "tbx_fast_list.h"
#include "sys/marcel_flags.h"
#ifdef __MARCEL_KERNEL__
#include "linux_spinlock.h"
#include "marcel_types.h"
#endif


/** Public data structures **/
struct ma_timer_list {
	struct tbx_fast_list_head entry;
	unsigned long expires;

#ifdef PM2_DEBUG
	unsigned long magic;
#endif

	void (*function)(unsigned long);
	unsigned long data;

	struct ma_timer_base_s *base;
};


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/*
 * per-LWP timer vector definitions:
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

#define MA_TIMER_MAGIC	0x4b87ad6e

#ifdef PM2_DEBUG

#define MA_TIMER_INITIALIZER_MAGIC .magic = MA_TIMER_MAGIC,
#else
#define MA_TIMER_INITIALIZER_MAGIC
#endif
#define MA_TIMER_INITIALIZER(_function, _expires, _data) {	\
		.function = (_function),			\
		.expires = (_expires),				\
		.data = (_data),				\
		.base = &__ma_init_timer_base,			\
		MA_TIMER_INITIALIZER_MAGIC			\
	}


/** Internal data types **/
typedef struct ma_tvec_t_base_s ma_tvec_base_t;


/** Internal data structures **/
typedef struct ma_tvec_s {
	struct tbx_fast_list_head vec[TVN_SIZE];
} ma_tvec_t;

typedef struct ma_tvec_root_s {
	struct tbx_fast_list_head vec[TVR_SIZE];
} ma_tvec_root_t;

struct ma_timer_base_s {
	ma_spinlock_t lock;
	struct ma_timer_list *running_timer;
};

struct ma_tvec_t_base_s {
	struct ma_timer_base_s t_base;
	unsigned long timer_jiffies;
	ma_tvec_root_t tv1;
	ma_tvec_t tv2;
	ma_tvec_t tv3;
	ma_tvec_t tv4;
	ma_tvec_t tv5;
};/*  ____cacheline_aligned_in_smp; */


/** Internal global variables **/
extern struct ma_timer_base_s __ma_init_timer_base;


/** Internal functions **/
extern void ma_add_timer_on(struct ma_timer_list *timer, ma_lwp_t lwp);
extern TBX_EXTERN int ma_del_timer(struct ma_timer_list * timer);
extern TBX_EXTERN int __ma_mod_timer(struct ma_timer_list *timer, unsigned long expires);
extern TBX_EXTERN int ma_mod_timer(struct ma_timer_list *timer, unsigned long expires);

#ifdef MA__LWPS
  extern TBX_EXTERN int ma_del_timer_sync(struct ma_timer_list * timer);
#else
# define ma_del_timer_sync(t) ma_del_timer(t)
#endif

extern void ma_init_timers(void);
extern void ma_update_process_times(int user_tick);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_TIMER_H__ **/
