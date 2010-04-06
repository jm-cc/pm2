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


#ifndef __MARCEL_DESCR_H__
#define __MARCEL_DESCR_H__


#include "sys/marcel_flags.h"
#include "asm/marcel_ctx.h"
#include "tbx_fast_list.h"
#include "marcel_config.h"
#include "marcel_topology.h"
#include "marcel_compiler.h"
#include "marcel_types.h"
#include "marcel_signal.h"
#include "marcel_threads.h"
#include "marcel_sem.h"
#include "linux_timer.h"
#ifdef MARCEL_MIGRATION_ENABLED
#include "marcel_exception.h"
#endif
#ifdef MA__LIBPTHREAD
#define __need_res_state
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#undef __need_res_state
#endif


/** Public macros **/
/* To be preferred when accessing a field of a marcel_t (inherited from glibc) */
#define THREAD_GETMEM(thread_desc, field)   ((thread_desc)->field)
#define THREAD_SETMEM(thread_desc, field, value)   ((thread_desc)->field)=(value)

/* To be preferred when accessing a field of the current thread (TODO: optimize into TLS when it is available */
#define SELF_GETMEM(field)	THREAD_GETMEM(MARCEL_SELF, field)
#define SELF_SETMEM(field, value) THREAD_SETMEM(MARCEL_SELF, field, value)

/* To be preferred for accessing the current thread's structure */
#define MARCEL_SELF (marcel_self())


/** Public data structures **/
/** \brief Context info for read write locks.

   The marcel_rwlock_info structure
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

enum ma_stack_kind_t {
	MA_DYNAMIC_STACK,
	MA_STATIC_STACK,
	MA_NO_STACK,
};

/** \brief Common class of task entities. A task may for instance be a thread or a seed. */
struct marcel_task {
	/** \brief Preemption depth counter.
	 *
	 * - Value of \p 0 means preemption allowed
	 * - Value \p <0 is not allowed and indicates a bug */
	int preempt_count;

	/** \brief Timer preemption depth counter.
	 *
	 * - Value of \p 0 means preemption allowed
	 * - Value \p <0 is not allowed and indicates a bug */
	int not_preemptible;

	/* Scheduler related data */

	/** \brief LWP onto which the thread is running. */
	struct marcel_lwp *lwp;

	/** \brief Set of VP bits indicating on which VP(s) the thread is currently allowed to be scheduled. */
	marcel_vpset_t vpset;

	/** \brief Thread state code. */
	unsigned int state;

	/** \brief Entity virtual class. */
	struct ma_entity as_entity;
#ifdef MA__BUBBLES
	/** \brief Bubble where children of this thread are automatically put by default. 
	 *
	 * Programmer may override this default on a per-created thread basis using either
	 * the marcel_sched_attr#natural_holder attribute to fix the natural
	 * holder of the created thread explicitely or the
	 * marcel_sched_attr#inheritholder attribute to fix the natural holder
	 * of the create thread implicitely to the natural holder of the
	 * creating thread */
	marcel_bubble_t default_children_bubble;
#endif
	/** \brief Dedicated space for saving thread current context on context switches. */
	marcel_ctx_t ctx_yield, ctx_restart;

	/** \brief Thread state set of flags. 
	 *
	 * TODO: indicate valid flags */
	volatile unsigned long flags;

	/** \brief Pointer to created child during thread creation. */
	marcel_t child;

	/** \brief Pointer to created child during thread creation. */
	marcel_t father;

	/** \brief User function to call as the main thread job. */
	marcel_func_t f_to_call;

	/** \brief User arg to pass the the user function marcel_task::f_to_call. */
	any_t arg;

	void (*f_pre_switch)(void *arg);
	void (*f_post_switch)(void *arg);

	//marcel_attr_t *shared_attr;
	
	/** \brief Pointer to the seed that a seed runner thread is currently running. */
	marcel_t cur_thread_seed;

	/** \brief Pointer to the seed runner thread that currently runs the seed. */
	marcel_t cur_thread_seed_runner;

	/* Gestion de la terminaison */
	/** \brief Flag indicating whether the thread is currently detached (set) or joignable (unset). */
	int detached;
	
	/** \brief Semaphore for synchronizing a thread currently attempting to join this thread. */
	marcel_sem_t client;

	/** \brief The user return value of the completed thread. */
	any_t ret_val; /* exit/join/cancel */

	/** \brief Timer for use in ma_schedule_timeout. */
	struct ma_timer_list schedule_timeout_timer;

	/** \brief Base of the thread stack. */
	any_t stack_base;
	
	/** \brief Code indicating how the thread stack memory block was allocated. */
	enum ma_stack_kind_t stack_kind;

	/** \brief Initial stack pointer.
	 * used only during thread creation in the different cases (main
	 * thread, normal thread, initial thread of an LWP) */
	long initial_sp;

	/** \brief ID number of the thread, chosen by the user through marcel_attr_setid */
	int id;

	/** \brief ID number of the thread, uniquely computed by marcel */
	int number;

	/* Used when TIF_BLOCK_HARDIRQ is set (cf softirq.c) */
	//unsigned long softirq_pending_in_hardirq;

#ifdef MARCEL_MIGRATION_ENABLED
       /* Contexte de migration */
	marcel_ctx_t ctx_migr;
	unsigned not_migratable;
	unsigned long remaining_sleep_time;
#endif /* MARCEL_MIGRATION_ENABLED */

#ifdef MARCEL_USERSPACE_ENABLED
	/* démarrage retardé */
	marcel_func_t real_f_to_call;
	marcel_sem_t sem_marcel_run;
	char *user_space_ptr;
#endif /* MARCEL_USERSPACE_ENABLED */

#ifdef MARCEL_POSTEXIT_ENABLED
	/*         postexit stuff */
	marcel_postexit_func_t postexit_func;
	any_t postexit_arg;
#endif /* MARCEL_POSTEXIT_ENABLED */

#ifdef MARCEL_ATEXIT_ENABLED
	/*         atexit stuff */
	marcel_atexit_func_t atexit_funcs[MAX_ATEXIT_FUNCS];
	any_t atexit_args[MAX_ATEXIT_FUNCS];
	unsigned next_atexit_func;
#endif /* MARCEL_ATEXIT_ENABLED */

#ifdef MARCEL_CLEANUP_ENABLED
	/*         cleanup */
	struct _marcel_cleanup_buffer *last_cleanup;
#endif /* MARCEL_CLEANUP_ENABLED */

#ifdef MARCEL_EXCEPTIONS_ENABLED
	/* Gestion des exceptions */
	marcel_exception_t cur_exception;
	struct marcel_exception_block *cur_excep_blk;
	char *exfile;
	unsigned exline;
#endif /* MARCEL_EXCEPTIONS_ENABLED */

#ifdef MARCEL_KEYS_ENABLED
	/* Clés */
	any_t key[MAX_KEY_SPECIFIC];
#endif /* MARCEL_KEYS_ENABLED */

#ifdef MARCEL_SUSPEND_ENABLED
	/* suspend */
	marcel_sem_t suspend_sem;
#endif /* MARCEL_SUSPEND_ENABLED */

	/* marcel-top */
	ma_atomic_t top_utime/*, top_stime*/;

#ifdef MA__PROVIDE_TLS
#if defined(X86_ARCH) || defined(X86_64_ARCH)
	/* On x86/x86_64, LDT descriptor to handle the TLS segment of this thread */
	unsigned short tls_desc;
#endif
#else
	int __errno;
	int __h_errno;
#endif

#ifdef MA__LIBPTHREAD
	/* Pour le code provenant de la libpthread */
	struct __res_state __res_state;
#endif

	/* per-lwp list of all threads */
	struct tbx_fast_list_head all_threads;

#ifdef MARCEL_DEBUG_SPINLOCK
	/* backtrace pour débugguer la préemption */
	void *preempt_backtrace[TBX_BACKTRACE_DEPTH];
	size_t preempt_backtrace_size;
	int spinlock_backtrace;
#endif

#ifdef MARCEL_DEVIATION_ENABLED
	/* travaux en cours (deviate, signaux, ...) */
	unsigned not_deviatable;
	struct marcel_work work;
#endif /* MARCEL_DEVIATION_ENABLED */

#ifdef MARCEL_SIGNALS_ENABLED
/*********signaux***********/
	ma_spinlock_t siglock;
	marcel_sigset_t sigpending;
	siginfo_t siginfo[MARCEL_NSIG];      
	/* Thread's signal mask as seen by the application.  */
	marcel_sigset_t curmask;
	/* kernel version, synced with curmask, to optimize context-switches.  */
#ifdef __GLIBC__
	sigset_t kcurmask;
#endif

	int interrupted;
	int delivering_sig;
	int restart_deliver_sig;
	/********sigtimedwait**********/
	marcel_sigset_t waitset;
	int *waitsig;
	siginfo_t *waitinfo;
#endif

#ifdef MARCEL_DEVIATION_ENABLED
#ifdef MA__IFACE_PMARCEL
	/*********attributs*********/
	ma_spinlock_t cancellock;
	int cancelstate;
	int canceltype;
	int canceled;
#endif
#endif /* MARCEL_DEVIATION_ENABLED */

#ifdef MA__PROVIDE_TLS
	/* TLS Exec (non dynamique) */
	char tls[MA_TLS_AREA_SIZE];
#endif

	/* Used for fork() handling, to put back fork() parent to where it
	 * was. */
	ma_holder_t *fork_holder;

#if defined(ENABLE_STACK_JUMPING) && !defined(MA__SELF_VAR)
	void *dummy; /*  Doit rester le _dernier_ champ */
#endif
};


/** Public functions **/
/* ==== get current thread or LWP id ==== */
/* To be preferred for accessing the current thread's identifier */
MARCEL_INLINE TBX_NOINST marcel_t marcel_self(void);
static __tbx_inline__ char *marcel_stackbase(marcel_t pid);


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define MAL(X)          (((X)+(MARCEL_ALIGN-1)) & ~(MARCEL_ALIGN-1))
#define MAL_BOT(X)      ((X) & ~(MARCEL_ALIGN-1))


/* 0x000000FF : marcel_descr
 * 0x0000FF00 : marcel_sched
 * 0x00020000 : marcel_descr (debug)
 */
#define TIF_WORKPENDING         1       /* work pending */
#define TIF_POLLING_NRFLAG      2       /* true if idle() is polling vpdata->need_resched */
#define TIF_NEED_TOGO           3       /* rescheduling necessary, no affinity */
#define TIF_DEBUG_IN_PROGRESS   16      /* true if pm2debug is in execution */
#define TIF_BLOCK_HARDIRQ       17      /* true if softirq can be raised in hardirq */

#define _TIF_WORKPENDING        (1<<TIF_WORKPENDING)
#define _TIF_POLLING_NRFLAG     (1<<TIF_POLLING_NRFLAG)
#define _TIF_DEBUG_IN_PROGRESS  (1<<TIF_DEBUG_IN_PROGRESS)
#define _TIF_BLOCK_HARDIRQ      (1<<TIF_BLOCK_HARDIRQ)


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_DESCR_H__ **/
