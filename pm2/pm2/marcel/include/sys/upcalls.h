#ifndef _UPCALLS_H
#define _UPCALLS_H

#include <asm/act.h>
#include <asm/atomic.h>

#define ACTDEBUG(todo) (todo)
//#define ACTDEBUG(todo)

#include <marcel.h>

#define RETURN_FROM_UPCALL 1

typedef enum {
  ACT_UNUSED,
  ACT_RUNNING,
} act_state_t;

typedef struct {
  marcel_t current_thread;
  act_state_t state;
  marcel_t start_thread;
} act_info_t;

extern volatile marcel_t thread_to_schedule;
extern volatile int schedule_new_thread;

void act_lock(marcel_t self);
void act_unlock(marcel_t self);
marcel_t act_update(marcel_t new);

void restart_thread(task_desc *desc);
void act_new(act_id_t new);
void act_preempt(act_id_t cur_aid, act_id_t stopped_aid, int fast_restart);
void act_block(act_id_t cur_aid, act_id_t stopped_aid);
void act_unblock(act_id_t cur_aid, act_id_t stopped_aid, int fast_restart);
void act_restart(act_id_t cur_aid);

void init_upcalls(int nb_act);
void launch_upcalls(int __nb_act_wanted);

#endif /* _UPCALLS_H */
