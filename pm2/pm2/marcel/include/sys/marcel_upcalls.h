#ifndef _UPCALLS_H
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#define _UPCALLS_H

#ifdef MA__ACTIVATION

#ifdef ACT_TIMER
#ifdef CONFIG_ACT_TIMER
#undef CONFIG_ACT_TIMER
#endif
#define CONFIG_ACT_TIMER
#endif

#include <asm/act.h>

#include "marcel.h"

extern volatile int act_nb_unblocked;

void act_lock(marcel_t self);
void act_unlock(marcel_t self);

void act_goto_next_task(marcel_t pid, int from);
//void restart_thread(task_desc *desc);

void init_upcalls(int nb_act);
//#define launch_upcalls(__nb_act_wanted)

enum {
	ACT_RESTART_FROM_SCHED,
	ACT_RESTART_FROM_IDLE,
	ACT_RESTART_FROM_UPCALL_NEW,
};

#endif 

#endif /* _UPCALLS_H */
