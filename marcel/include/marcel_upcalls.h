
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

#section macros
#ifdef MA__ACTIVATION
#  ifdef ACT_TIMER
#    ifdef CONFIG_ACT_TIMER
#      undef CONFIG_ACT_TIMER
#    endif
#    define CONFIG_ACT_TIMER
#  endif
#  include "asm/act.h"
#endif

#section marcel_macros
#ifdef MA__DEBUG
#define MA_ACT_SET_THREAD_DEBUG(thread) \
  do { \
	MA_BUG_ON(!((marcel_task_t*) \
		    (ma_per_lwp(act_info, (lwp)).current_act_id)) \
                    ->preempt_count);\
        MA_BUG_ON(!thread->preempt_count); \
  } while (0)
#else
#define MA_ACT_SET_THREAD_DEBUG(thread)
#endif

//        MA_ACT_SET_THREAD_DEBUG(lwp, thread);

#ifdef MA__ACTIVATION
#define MA_ACT_SET_THREAD(thread) \
  do { \
        __ma_get_lwp_var(act_info).current_act_id=(unsigned long)(thread); \
        __ma_get_lwp_var(act_info).critical_section=&(thread)->preempt_count;\
	ma_smp_mb(); \
  } while (0)
#else
#define MA_ACT_SET_THREAD(thread) ((void)0)
#endif

#section marcel_functions
void marcel_upcalls_disallow(void);

#section marcel_types
enum {
	ACT_RESTART_FROM_SCHED,
	ACT_RESTART_FROM_IDLE,
	ACT_RESTART_FROM_UPCALL_NEW,
};

