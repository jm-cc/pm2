
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
#ifndef MA__ACTIVATION
#  define marcel_upcall_enable(lwp, upcall) ((void)0)
#  define marcel_upcall_disable(lwp, upcall) ((void)0)
#  define marcel_upcall_init(nb_act) ((void)0)
#endif

#section marcel_variables
#ifdef MA__ACTIVATION
//extern volatile int act_nb_unblocked;
extern marcel_t marcel_next[ACT_NB_MAX_CPU];
#endif

//void act_goto_next_task(marcel_t pid, int from);
//void restart_thread(task_desc *desc);

#section marcel_functions
#ifdef MA__ACTIVATION
void init_upcalls(int nb_act);
//#define launch_upcalls(__nb_act_wanted)
#endif

#section marcel_types
enum {
	ACT_RESTART_FROM_SCHED,
	ACT_RESTART_FROM_IDLE,
	ACT_RESTART_FROM_UPCALL_NEW,
};

