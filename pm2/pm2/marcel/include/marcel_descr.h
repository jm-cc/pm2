
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


/****************************************************************/

#section types
typedef struct marcel_task marcel_task_t;
typedef marcel_task_t *p_marcel_task_t;
typedef p_marcel_task_t marcel_t;

#section marcel_macros

#define THREAD_GETMEM(thread_desc, field)   ((thread_desc)->field)
#define THREAD_SETMEM(thread_desc, field, value)   ((thread_desc)->field)=(value)
#define SELF_GETMEM(field)	THREAD_GETMEM(MARCEL_SELF, field)
#define SELF_SETMEM(field, value) THREAD_SETMEM(MARCEL_SELF, field, value)

#define MARCEL_SELF (__marcel_self())
#define marcel_current (__marcel_self())

#section marcel_structures
#depend "asm/linux_atomic.h[marcel_structures]"
#depend "sys/marcel_work.h[marcel_structures]"
#depend "marcel_sched_generic.h[marcel_structures]"
#depend "marcel_attr.h[macros]"
 /* Pour struct __res_state */
#define __need_res_state
#depend <resolv.h>
#undef __need_res_state

/* Context info for read write locks. The marcel_rwlock_info structure
   is information about a lock that has been read-locked by the thread
   in whose list this structure appears. The marcel_rwlock_context
   is embedded in the thread context and contains a pointer to the
   head of the list of lock info structures, as well as a count of
   read locks that are untracked, because no info structure could be
   allocated for them. */
struct _marcel_rwlock_t;
typedef struct _marcel_rwlock_info {
  struct _marcel_rwlock_info *pr_next;
  struct _marcel_rwlock_t *pr_lock;
  int pr_lock_count;
} marcel_readlock_info;

struct marcel_task {
	/* =0 : preemption allowed, <0 : BUG */
	int preempt_count;
	/* Idem, but just for timer preemption */
	int not_preemptible;
	/* Données relatives au scheduler */
	struct marcel_sched_task sched;
	/* Changements de contexte (sauvegarde état) */
	marcel_ctx_t ctx_yield;
	/* travaux en cours (deviate, signaux, ...) */
	struct marcel_work work;
	/* Used when TIF_BLOCK_HARDIRQ is set (cf softirq.c) */
	unsigned long softirq_pending_in_hardirq;
       /* Contexte de migration */
	marcel_ctx_t ctx_migr;
	/* utilisé pour marquer les task idle, upcall, idle, ... */
	unsigned long flags;
	/* Pour la création des threads */
	marcel_t child, father;
	marcel_func_t f_to_call;
	marcel_func_t real_f_to_call;
	any_t arg;
	marcel_sem_t sem_marcel_run;
	char *user_space_ptr;
	/* Gestion de la terminaison */
	boolean detached;
	marcel_sem_t client, thread;
	any_t ret_val; /* exit/join/cancel */
	/*         postexit stuff */
	marcel_postexit_func_t postexit_func;
	any_t postexit_arg;
	/*         atexit stuff */
	marcel_atexit_func_t atexit_funcs[MAX_ATEXIT_FUNCS];
	any_t atexit_args[MAX_ATEXIT_FUNCS];
	unsigned next_atexit_func;
	/*         cleanup */
	struct _marcel_cleanup_buffer *last_cleanup;
	/* Gestion des exceptions */
	marcel_exception_t cur_exception;
	struct marcel_exception_block *cur_excep_blk;
	char *exfile;
	unsigned exline;
	/* Clés */
	any_t key[MAX_KEY_SPECIFIC];



	any_t stack_base;
	int static_stack;
	long initial_sp, depl;
	char name[MARCEL_MAXNAMESIZE];
	unsigned long number, time_to_wake;
	unsigned not_migratable, not_deviatable;
	marcel_sem_t suspend_sem;
	
	/* Pour le code provenant de la libpthread */
	int __errno;
	int __h_errno;
	struct __res_state __res_state;
	/*         List of readlock info structs */
	marcel_readlock_info *p_readlock_list;
	/*         Free list of structs */
	marcel_readlock_info *p_readlock_free;
	/*         Readlocks not tracked by list */
	int p_untracked_readlock_count;
	/*         Next element in the queue holding the thr */
	marcel_t p_nextwaiting;
	marcel_sem_t pthread_sync;

#ifdef ENABLE_STACK_JUMPING
	void *dummy; // Doit rester le _dernier_ champ
#endif
};

#section inline
#depend "[marcel_inline]"
/* ==== get current thread or LWP id ==== */
extern MARCEL_INLINE marcel_t marcel_self(void) __THROW
{
  return __marcel_self();
}

#section marcel_macros
#define MARCEL_ALIGN    64L
#define MAL(X)          (((X)+(MARCEL_ALIGN-1)) & ~(MARCEL_ALIGN-1))
#define MAL_BOT(X)      ((X) & ~(MARCEL_ALIGN-1))

#section marcel_functions
static __inline__ marcel_t __marcel_self(void);
#section marcel_inline
#depend "sys/isomalloc_archdep.h"
#depend "asm/marcel_archdep.h"
static __inline__ marcel_t __marcel_self(void)
{
#ifdef MARCEL_SELF_IN_REG
  return (marcel_t)get_gs();
#else
  register unsigned long sp = get_sp();

#ifdef STANDARD_MAIN
  if(IS_ON_MAIN_STACK(sp))
    return &__main_thread_struct;
  else
#endif
#ifdef ENABLE_STACK_JUMPING
    return *((marcel_t *)(((sp & ~(THREAD_SLOT_SIZE-1)) + THREAD_SLOT_SIZE - sizeof(void *))));
#else
    return (marcel_t)(((sp & ~(THREAD_SLOT_SIZE-1)) + THREAD_SLOT_SIZE) -
		      MAL(sizeof(marcel_task_t)));
#endif
#endif
}

#section marcel_macros

/* 0x000000FF : marcel_descr
 * 0x0000FF00 : marcel_sched
 * 0x00020000 : marcel_descr (debug)
 */
#define TIF_NEED_RESCHED        0       /* rescheduling necessary */
#define TIF_WORKPENDING         1       /* work pending */
#define TIF_POLLING_NRFLAG      2       /* true if idle() is polling TIF_NEED_RESCHED */
#define TIF_DEBUG_IN_PROGRESS   16      /* true if pm2debug is in execution */
#define TIF_BLOCK_HARDIRQ       17      /* true if softirq can be raised in hardirq */

#define _TIF_NEED_RESCHED       (1<<TIF_NEED_RESCHED)
#define _TIF_WORKPENDING        (1<<TIF_WORKPENDING)
#define _TIF_POLLING_NRFLAG     (1<<TIF_POLLING_NRFLAG)
#define _TIF_DEBUG_IN_PROGRESS  (1<<TIF_DEBUG_IN_PROGRESS)
#define _TIF_BLOCK_HARDIRQ      (1<<TIF_BLOCK_HARDIRQ)
