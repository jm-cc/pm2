#ifndef _UPCALLS_H
#define _UPCALLS_H

#ifdef ACT_TIMER
#ifdef CONFIG_ACT_TIMER
#undef CONFIG_ACT_TIMER
#endif
#define CONFIG_ACT_TIMER
#endif

#include <asm/act.h>
#include <asm/atomic.h>

#ifdef ACT_VERBOSE
#define ACTDEBUG(todo) (todo)
#else
#define ACTDEBUG(todo)
#endif

#include "marcel.h"

void act_lock(marcel_t self);
void act_unlock(marcel_t self);

void restart_thread(task_desc *desc);

void init_upcalls(int nb_act);
#define launch_upcalls(__nb_act_wanted)

#endif /* _UPCALLS_H */
