/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#ifndef PRIVATEDEFS_EST_DEF
#define PRIVATEDEFS_EST_DEF

#include <sys/isomalloc_archdep.h>

#ifdef SMP
#include <pthread.h>
#endif

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

_PRIVATE_ typedef struct task_desc_struct {
  jmp_buf jb;
  struct task_desc_struct *next,*prev;
  struct __lwp_struct *lwp, *previous_lwp;
  int sched_policy;
  task_state state;
  jmp_buf migr_jb;
  marcel_t child, father;
  _exception_block *cur_excep_blk;
  marcel_sem_t client, thread;
  unsigned prio, quantums;
  cleanup_func_t cleanup_funcs[MAX_CLEANUP_FUNCS];
  handler_func_t deviation_func;
  marcel_func_t f_to_call;
  any_t ret_val, arg, deviation_arg,
    cleanup_args[MAX_CLEANUP_FUNCS],
    key[MAX_KEY_SPECIFIC],
    private_val, stack_base;
  long initial_sp, depl;
  unsigned long number, time_to_wake;
  char *cur_exception, *exfile, *user_space_ptr;
  unsigned exline, not_migratable, 
    not_deviatable, next_cleanup_func;
  boolean in_sighandler, detached, static_stack;
} task_desc;

_PRIVATE_ typedef struct {
  boolean executed;
  marcel_mutex_t mutex;
} marcel_once_t;

_PRIVATE_ typedef struct __lwp_struct {
  unsigned number;                         /* Serial number */
  volatile marcel_t __first[MAX_PRIO+1];   /* Scheduler queue */
  volatile unsigned running_tasks;         /* Nb of user running tasks */
  marcel_lock_t sched_queue_lock;          /* Lock for scheduler queue */
#ifdef X86_ARCH
  volatile atomic_t _locked;               /* Lock for (un)lock_task() */
#else
  volatile unsigned _locked;               /* Lock for (un)lock_task() */
#endif
  marcel_t sched_task;                     /* "Idle" task */
  volatile boolean has_to_stop;            /* To force pthread_exit() */
  volatile boolean has_new_tasks;          /* Somebody gave us some work */
  volatile marcel_t new_task;              /* Used by marcel_create */
  struct __lwp_struct *prev, *next;        /* Double linking */
  char __security_stack[2 * SLOT_SIZE];    /* Used when own stack destruction is required */
  marcel_mutex_t stack_mutex;              /* To protect security_stack */
  volatile marcel_t sec_desc;              /* Task descriptor for security stack */
#ifdef SMP
  jmp_buf home_jb;
  pthread_t pid;
#endif
} __lwp_t;

_PRIVATE_ extern __lwp_t __main_lwp;
_PRIVATE_ extern task_desc __main_thread_struct;
_PRIVATE_ extern char __security_stack[];

#ifndef SMP
#define cur_lwp   (&__main_lwp)
#endif

static __inline__ marcel_t __marcel_self()
{
  register unsigned long sp = get_sp();

#ifdef STANDARD_MAIN
  if(IS_ON_MAIN_STACK(sp))
    return &__main_thread_struct;
  else
#endif
    return (marcel_t)(((sp & ~(SLOT_SIZE-1)) + SLOT_SIZE) -
		      MAL(sizeof(task_desc)));
}

#define SECUR_TASK_DESC(lwp) \
   ((marcel_t)((((unsigned long)(lwp)->__security_stack + 2 * SLOT_SIZE) \
	        & ~(SLOT_SIZE-1)) - MAL(sizeof(task_desc))))
#define SECUR_STACK_TOP(lwp) \
   ((unsigned long)SECUR_TASK_DESC(lwp) - MAL(1) - 2*WINDOWSIZE)


_PRIVATE_ enum {
  FIRST_RETURN,
  NORMAL_RETURN
};

#endif
