#ifndef _UPCALLS_H
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
