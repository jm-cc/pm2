
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

#ifndef PRIVATEDEFS_EST_DEF
#define PRIVATEDEFS_EST_DEF

#include "sys/marcel_flags.h"
#include "sys/isomalloc_archdep.h"
#include "sys/marcel_kthread.h"
#include "pm2_list.h"
#include "marcel_exception.h"

#define MARCEL_ALIGN    64L
#define MAL(X)          (((X)+(MARCEL_ALIGN-1)) & ~(MARCEL_ALIGN-1))
#define MAL_BOT(X)      ((X) & ~(MARCEL_ALIGN-1))

_PRIVATE_ typedef enum {
  RUNNING,
  WAITING,
  BLOCKED,
  FROZEN,
  GHOST
} task_state;

#define IS_RUNNING(pid)     ((pid)->state == RUNNING)
#define IS_SLEEPING(pid)    ((pid)->state == WAITING)
#define IS_BLOCKED(pid)     ((pid)->state == BLOCKED)
#define IS_FROZEN(pid)      ((pid)->state == FROZEN)
#define IS_GHOST(pid)       ((pid)->state == GHOST)

#define SET_RUNNING(pid)    (pid)->state = RUNNING
#define SET_SLEEPING(pid)   (pid)->state = WAITING
#define SET_BLOCKED(pid)    (pid)->state = BLOCKED
#define SET_FROZEN(pid)     (pid)->state = FROZEN
#define SET_GHOST(pid)      (pid)->state = GHOST

_PRIVATE_ typedef struct _struct_exception_block {
  jmp_buf jb;
  struct _struct_exception_block *old_blk;
} _exception_block;

_PRIVATE_ struct __lwp_struct;

#ifdef MA__ACTIVATION
#include <asm/act.h>
#endif

#ifdef MA__MULTIPLE_RUNNING
/* A value of 0 is needed for MARCEL_READY in act_resume (upcall.c /
   upcall_unblock()) */
_PRIVATE_ typedef enum {
  MARCEL_READY=0, 
  MARCEL_RUNNING=1,
} marcel_state_t;
#endif

// VP mask: useful for selecting the set of "forbiden" LWP for a given thread
typedef unsigned long marcel_vpmask_t;

_PRIVATE_ typedef struct task_desc_struct {
#ifdef MA__MULTIPLE_RUNNING
  volatile marcel_state_t ext_state;
#endif
  struct list_head task_list;
  struct __lwp_struct *lwp;
  marcel_lock_t state_lock; // Prot�ge les changements d'�tats
                            // impliquant des changements de files.
  marcel_vpmask_t vpmask; // Contraintes sur le placement sur les LWP
  int special_flags; // utilis� pour marquer les task idle, upcall, idle, ...
  task_state state;
  jmp_buf jbuf;
#if defined(MA__LWPS)
  struct __lwp_struct *previous_lwp;
#endif
  int sched_policy;
  jmp_buf migr_jb;
  marcel_t child, father;
  _exception_block *cur_excep_blk;
  marcel_sem_t client, thread;
  cleanup_func_t cleanup_funcs[MAX_CLEANUP_FUNCS];
  handler_func_t deviation_func;
  marcel_func_t f_to_call;
  any_t ret_val, arg, deviation_arg,
    cleanup_args[MAX_CLEANUP_FUNCS],
    key[MAX_KEY_SPECIFIC],
    private_val, stack_base;
  long initial_sp, depl;
  unsigned long number, time_to_wake;
  marcel_exception_t cur_exception;
  char *exfile, *user_space_ptr;
  unsigned exline, not_migratable, 
    not_deviatable, next_cleanup_func;
  boolean detached, static_stack;
#ifdef MA__WORK
  volatile int has_work;
#endif
#ifdef ENABLE_STACK_JUMPING
  void *dummy; // Doit rester le _dernier_ champ
#endif
} task_desc;

_PRIVATE_ typedef struct {
  boolean executed;
  marcel_mutex_t mutex;
} marcel_once_t;

#ifdef MA__REMOVE_RUNNING_TASK
typedef struct list_head head_running_list_t;
#else
typedef marcel_t head_running_list_t;
#endif

_PRIVATE_ typedef struct __sched_struct {
  /*  volatile marcel_t new_task; */       /* Used by marcel_create */
  volatile head_running_list_t first;      /* Scheduler queue */
  volatile unsigned running_tasks;         /* Nb of user running tasks */
  marcel_lock_t sched_queue_lock;          /* Lock for scheduler queue */
  volatile head_running_list_t rt_first;   /* Real time threads queue */
  volatile boolean has_new_tasks;          /* Somebody gave us some work */
} __sched_t;

_PRIVATE_ typedef struct __lwp_struct {
  unsigned number;                         /* Serial number */
#ifdef MA__MULTIPLE_RUNNING
  marcel_t prev_running;                   /* Previous task after yielding */
#endif
#ifdef MA__ACTIVATION
  marcel_t upcall_new_task;                /* Task used in upcall_new */
#endif
  volatile atomic_t _locked;               /* Lock for (un)lock_task() */
  volatile boolean has_to_stop;            /* To force pthread_exit() */
  struct list_head lwp_list;
  char __security_stack[2 * SLOT_SIZE];    /* Used when own stack destruction is required */
  marcel_mutex_t stack_mutex;              /* To protect security_stack */
  volatile marcel_t sec_desc;              /* Task descriptor for security stack */
#ifdef MA__SMP
  jmp_buf home_jb;
  marcel_kthread_t pid;
#endif
#ifndef MA__ONE_QUEUE
  __sched_t __sched_data;
#endif
  marcel_t idle_task;                     /* "Idle" task */
} __lwp_t;

inline static __lwp_t* next_lwp(__lwp_t* lwp)
{
  return list_entry(lwp->lwp_list.next, __lwp_t, lwp_list);
}

inline static __lwp_t* prev_lwp(__lwp_t* lwp)
{
  return list_entry(lwp->lwp_list.prev, __lwp_t, lwp_list);
}

inline static marcel_t next_task(marcel_t task)
{
  return list_entry(task->task_list.next, task_desc, task_list);
}

inline static marcel_t prev_task(marcel_t task)
{
  return list_entry(task->task_list.prev, task_desc, task_list);
}

#ifdef MA__LWPS
#ifdef MA__ACTIVATION
#define MA__MAX_LWPS ACT_NB_MAX_CPU
#else
#define MA__MAX_LWPS 32
#endif
extern __lwp_t* addr_lwp[MA__MAX_LWPS];
#else
#define MA__MAX_LWPS 1
#endif


#ifdef MA__ONE_QUEUE
_PRIVATE_ extern __sched_t __sched_data;
#endif

_PRIVATE_ extern __lwp_t __main_lwp;
_PRIVATE_ extern task_desc __main_thread_struct;
_PRIVATE_ extern char __security_stack[];

static __inline__ marcel_t __marcel_self()
{
  register unsigned long sp = get_sp();

#ifdef STANDARD_MAIN
  if(IS_ON_MAIN_STACK(sp))
    return &__main_thread_struct;
  else
#endif
#ifdef ENABLE_STACK_JUMPING
    return *((marcel_t *)(((sp & ~(SLOT_SIZE-1)) + SLOT_SIZE - sizeof(void *))));
#else
    return (marcel_t)(((sp & ~(SLOT_SIZE-1)) + SLOT_SIZE) -
		      MAL(sizeof(task_desc)));
#endif
}

#define SECUR_TASK_DESC(lwp) \
   ((marcel_t)((((unsigned long)(lwp)->__security_stack + 2 * SLOT_SIZE) \
	        & ~(SLOT_SIZE-1)) - MAL(sizeof(task_desc))))
#define SECUR_STACK_TOP(lwp) \
   ((unsigned long)SECUR_TASK_DESC(lwp) - MAL(1) - TOP_STACK_FREE_AREA)


_PRIVATE_ enum {
  FIRST_RETURN,
  NORMAL_RETURN
};

#ifdef MA__ACTIVATION
_PRIVATE_ extern int nb_idle_sleeping; //TODO
#endif

#ifdef ENABLE_STACK_JUMPING
static __inline__ void marcel_prepare_stack_jump(void *stack)
{
  *(marcel_t *)(stack + SLOT_SIZE - sizeof(void *)) = __marcel_self();
}

static __inline__ void marcel_set_stack_jump(marcel_t m)
{
  register unsigned long sp = get_sp();

  *(marcel_t *)((sp & ~(SLOT_SIZE-1)) + SLOT_SIZE - sizeof(void *)) = m;
}
#endif

#ifdef DSM_SHARED_STACK
extern volatile marcel_t __next_thread;

static __inline__ marcel_t marcel_get_cur_thread() {
  return __next_thread;
}
#endif
 

/*********************************************************************
 * thread_key 
 * */

typedef void (*marcel_key_destructor_t)(any_t);
extern unsigned marcel_nb_keys;
extern marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC];
extern int marcel_key_present[MAX_KEY_SPECIFIC];
extern marcel_lock_t marcel_key_lock;

#endif
