
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

#section common
/*
 * Similar to:
 * include/linux/interrupt.h
 */

#depend "sys/marcel_lwp.h[marcel_macros]"
#depend "asm-generic/linux_perlwp.h[marcel_macros]"
#depend "marcel_descr.h[types]"
extern int nr_threads;
//extern int last_tid;
//MA_DECLARE_PER_LWP(unsigned long, process_counts);
MA_DECLARE_PER_LWP(marcel_task_t *, current_thread);
//MA_DEFINE_PER_LWP(marcel_task_t *, idle_thread)=NULL;

#section marcel_functions
//extern int ma_nr_threads(void);
extern unsigned long ma_nr_running(void);
//extern unsigned long ma_nr_uninterruptible(void);
//extern unsigned long nr_iowait(void);

#section marcel_macros
#define MA_TASK_RUNNING		0
#define MA_TASK_INTERRUPTIBLE	1
#define MA_TASK_UNINTERRUPTIBLE	2
#define MA_TASK_STOPPED		4
#define MA_TASK_ZOMBIE		8
#define MA_TASK_DEAD		16
#define MA_TASK_GHOST		32
#define MA_TASK_MOVING		64

#define __ma_set_task_state(tsk, state_value)		\
	do { (tsk)->sched.state = (state_value); } while (0)
#define ma_set_task_state(tsk, state_value)		\
	ma_set_mb((tsk)->sched.state, (state_value))

#define __ma_set_current_state(state_value)			\
	do { MARCEL_SELF->sched.state = (state_value); } while (0)
#define ma_set_current_state(state_value)		\
	ma_set_mb(MARCEL_SELF->sched.state, (state_value))

#define MA_TASK_IS_RUNNING(tsk) ((tsk)->sched.internal.cur_rq && !(tsk)->sched.internal.array)
#define MA_TASK_IS_BLOCKED(tsk) ((tsk)->sched.internal.cur_rq &&  (tsk)->sched.internal.array)
#define MA_TASK_IS_SLEEPING(tsk) (!(tsk)->sched.internal.cur_rq)

/*
 * Scheduling policies
 */
//#define SCHED_NORMAL		0
//#define SCHED_FIFO		1
//#define SCHED_RR		2

//struct sched_param {
//	int sched_priority;
//};

#section marcel_variables
/*
 * This serializes "schedule()" and also protects
 * the run-queue from deletions/modifications (but
 * _adding_ to the beginning of the run-queue has
 * a separate lock).
 */
extern ma_rwlock_t tasklist_lock;

//void io_schedule(void);
//long io_schedule_timeout(long timeout);

//extern void update_process_times(int user);
//extern void update_one_process(marcel_task_t *p, unsigned long user,
//			       unsigned long system, int cpu);
extern void ma_scheduler_tick(int user_tick, int system);
//extern unsigned long cache_decay_ticks;

#section marcel_macros
#define	MA_MAX_SCHEDULE_TIMEOUT	LONG_MAX

#section functions
int marcel_yield_to(marcel_t next);

#section marcel_functions
extern signed long FASTCALL(ma_schedule_timeout(signed long timeout));
asmlinkage void ma_schedule(void);
asmlinkage void ma_schedule_tail(marcel_task_t *prev);

//struct sighand_struct {
//	atomic_t		count;
//	struct k_sigaction	action[_NSIG];
//	spinlock_t		siglock;
//};

/*
 * NOTE! "signal_struct" does not have it's own
 * locking, because a shared signal_struct always
 * implies a shared sighand_struct, so locking
 * sighand_struct is always a proper superset of
 * the locking of signal_struct.
 */
//struct signal_struct {
//	atomic_t		count;

	/* current thread group signal load-balancing target: */
//	task_t			*curr_target;

	/* shared signal handling: */
//	struct sigpending	shared_pending;

	/* thread group exit support */
//	int			group_exit;
//	int			group_exit_code;
	/* overloaded:
	 * - notify group_exit_task when ->count is equal to notify_count
	 * - everyone except group_exit_task is stopped during signal delivery
	 *   of fatal signals, group_exit_task processes the signal.
	 */
//	struct task_struct	*group_exit_task;
//	int			notify_count;

	/* thread group stop support, overloads group_exit_code too */
//	int			group_stop_count;
//};

#section marcel_structures
#if 0
/* POSIX.1b interval timer structure. */
struct ma_k_itimer {
	struct list_head list;		 /* free/ allocate list */
	spinlock_t it_lock;
	clockid_t it_clock;		/* which timer type */
	timer_t it_id;			/* timer id */
	int it_overrun;			/* overrun on pending signal  */
	int it_overrun_last;		 /* overrun on last delivered signal */
	int it_requeue_pending;          /* waiting to requeue this timer */
	int it_sigev_notify;		 /* notify word of sigevent struct */
	int it_sigev_signo;		 /* signo word of sigevent struct */
	sigval_t it_sigev_value;	 /* value word of sigevent struct */
	unsigned long it_incr;		/* interval specified in jiffies */
	marcel_task_t *it_process;	/* process to send signal to */
	struct timer_list it_timer;
	struct sigqueue *sigq;		/* signal queue entry. */
};
#endif

#if 0
struct task_struct {
	volatile long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	struct thread_info *thread_info;
	atomic_t usage;
	unsigned long flags;	/* per process flags, defined below */
	unsigned long ptrace;

	int lock_depth;		/* Lock depth */

	int prio, static_prio;
	struct list_head run_list;
	prio_array_t *array;

	unsigned long sleep_avg;
	long interactive_credit;
	unsigned long long timestamp;
	int activated;

	unsigned long policy;
	cpumask_t cpus_allowed;
	unsigned int time_slice, first_time_slice;

	struct list_head tasks;
	struct list_head ptrace_children;
	struct list_head ptrace_list;

	struct mm_struct *mm, *active_mm;

/* task state */
	struct linux_binfmt *binfmt;
	int exit_code, exit_signal;
	int pdeath_signal;  /*  The signal sent when the parent dies  */
	/* ??? */
	unsigned long personality;
	int did_exec:1;
	pid_t pid;
	pid_t __pgrp;		/* Accessed via process_group() */
	pid_t tty_old_pgrp;
	pid_t session;
	pid_t tgid;
	/* boolean value for session group leader */
	int leader;
	/* 
	 * pointers to (original) parent process, youngest child, younger sibling,
	 * older sibling, respectively.  (p->father can be replaced with 
	 * p->parent->pid)
	 */
	marcel_task_t *real_parent; /* real parent process (when being debugged) */
	marcel_task_t *parent;	/* parent process */
	struct list_head children;	/* list of my children */
	struct list_head sibling;	/* linkage in my parent's children list */
	marcel_task_t *group_leader;	/* threadgroup leader */

	/* PID/PID hash table linkage. */
	struct pid_link pids[PIDTYPE_MAX];

	wait_queue_head_t wait_chldexit;	/* for wait4() */
	struct completion *vfork_done;		/* for vfork() */
	int __user *set_child_tid;		/* CLONE_CHILD_SETTID */
	int __user *clear_child_tid;		/* CLONE_CHILD_CLEARTID */

	unsigned long rt_priority;
	unsigned long it_real_value, it_prof_value, it_virt_value;
	unsigned long it_real_incr, it_prof_incr, it_virt_incr;
	struct timer_list real_timer;
	struct list_head posix_timers; /* POSIX.1b Interval Timers */
	unsigned long utime, stime, cutime, cstime;
	unsigned long nvcsw, nivcsw, cnvcsw, cnivcsw; /* context switch counts */
	u64 start_time;
/* mm fault and swap info: this can arguably be seen as either mm-specific or thread-specific */
	unsigned long min_flt, maj_flt, nswap, cmin_flt, cmaj_flt, cnswap;
/* process credentials */
	uid_t uid,euid,suid,fsuid;
	gid_t gid,egid,sgid,fsgid;
	int ngroups;
	gid_t	groups[NGROUPS];
	kernel_cap_t   cap_effective, cap_inheritable, cap_permitted;
	int keep_capabilities:1;
	struct user_struct *user;
/* limits */
	struct rlimit rlim[RLIM_NLIMITS];
	unsigned short used_math;
	char comm[16];
/* file system info */
	int link_count, total_link_count;
	struct tty_struct *tty; /* NULL if no tty */
/* ipc stuff */
	struct sysv_sem sysvsem;
/* CPU-specific state of this task */
	struct thread_struct thread;
/* filesystem information */
	struct fs_struct *fs;
/* open file information */
	struct files_struct *files;
/* namespace */
	struct namespace *namespace;
/* signal handlers */
	struct signal_struct *signal;
	struct sighand_struct *sighand;

	sigset_t blocked, real_blocked;
	struct sigpending pending;

	unsigned long sas_ss_sp;
	size_t sas_ss_size;
	int (*notifier)(void *priv);
	void *notifier_data;
	sigset_t *notifier_mask;
	
	void *security;

/* Thread group tracking */
   	u32 parent_exec_id;
   	u32 self_exec_id;
/* Protection of (de-)allocation: mm, files, fs, tty */
	spinlock_t alloc_lock;
/* Protection of proc_dentry: nesting proc_lock, dcache_lock, write_lock_irq(&tasklist_lock); */
	spinlock_t proc_lock;
/* context-switch lock */
	spinlock_t switch_lock;

/* journalling filesystem info */
	void *journal_info;

/* VM state */
	struct reclaim_state *reclaim_state;

	struct dentry *proc_dentry;
	struct backing_dev_info *backing_dev_info;

	struct io_context *io_context;

	unsigned long ptrace_message;
	siginfo_t *last_siginfo; /* For ptrace use.  */
};
#endif

//#section marcel_functions
//extern void __ma_put_task_struct(marcel_task_t *tsk);
//#section marcel_macros
//#define ma_get_task_struct(tsk) do { ma_atomic_inc(&(tsk)->usage); } while(0)
#if 0
#define ma_put_task_struct(tsk) \
do { if (ma_atomic_dec_and_test(&(tsk)->usage)) __ma_put_task_struct(tsk); } while(0)
#endif

/*
 * Per process flags
 */
//#define PF_STARTING	0x00000002	/* being created */
//#define PF_EXITING	0x00000004	/* getting shut down */
//#define PF_DEAD		0x00000008	/* Dead */

//#define PF_FREEZE	0x00004000	/* this task should be frozen for suspend */
//#define PF_IOTHREAD	0x00008000	/* this thread is needed for doing I/O to swap */
//#define PF_FROZEN	0x00010000	/* frozen for system suspend */

#section marcel_functions
#if 0
#ifdef MA__LWPS
extern int ma_set_lwps_allowed(marcel_task_t *p, ma_lwpmask_t new_mask);
#else
static inline int ma_set_lwps_allowed(marcel_task_t *p, ma_lwpmask_t new_mask)
{
	return 0;
}
#endif
#endif

//extern unsigned long long ma_sched_clock(void);

#if 0
#ifdef CONFIG_NUMA
extern void sched_balance_exec(void);
extern void node_nr_running_init(void);
#else
#define sched_balance_exec()   {}
#define node_nr_running_init() {}
#endif
#endif

/* Move tasks off this (offline) CPU onto another. */
//extern void migrate_all_tasks(void);
//extern void set_user_nice(task_t *p, long nice);
//extern int task_prio(task_t *p);
//extern int task_nice(task_t *p);
//extern int task_curr(task_t *p);
//extern int idle_cpu(int cpu);

void ma_yield(void);

//extern marcel_task_t *find_task_by_pid(int pid);

//extern unsigned long itimer_ticks;
//extern unsigned long itimer_next;
//extern void do_timer(struct pt_regs *);

extern int FASTCALL(ma_wake_up_state(marcel_task_t * tsk, unsigned int state));
extern int FASTCALL(ma_wake_up_thread(marcel_task_t * tsk));
#ifdef MA__LWPS
 extern void ma_kick_process(marcel_task_t * tsk);
#else
 static inline void ma_kick_process(marcel_task_t *tsk) { }
#endif
extern void FASTCALL(ma_wake_up_created_thread(marcel_task_t * tsk));

int ma_sched_change_prio(marcel_t t, int prio);

//extern void FASTCALL(ma_sched_fork(marcel_task_t * p));
//extern void FASTCALL(ma_sched_exit(marcel_task_t * p));

#if 0
extern void ma_proc_caches_init(void);
extern void ma_flush_signals(marcel_task_t *);
extern void ma_flush_signal_handlers(marcel_task_t *, int force_default);
extern int ma_dequeue_signal(marcel_task_t *tsk, sigset_t *mask, siginfo_t *info);

static inline int ma_dequeue_signal_lock(marcel_task_t *tsk, sigset_t *mask, siginfo_t *info)
{
	//unsigned long flags;
	int ret;

	ma_spin_lock_softirq(&tsk->sighand->siglock);
	ret = ma_dequeue_signal(tsk, mask, info);
	ma_spin_unlock_softirq(&tsk->sighand->siglock);

	return ret;
}	

extern void block_all_signals(int (*notifier)(void *priv), void *priv,
			      sigset_t *mask);
extern void unblock_all_signals(void);
extern void release_task(marcel_task_t * p);
extern int send_sig_info(int, struct siginfo *, marcel_task_t *);
extern int send_group_sig_info(int, struct siginfo *, marcel_task_t *);
extern int force_sig_info(int, struct siginfo *, marcel_task_t *);
extern int __kill_pg_info(int sig, struct siginfo *info, pid_t pgrp);
extern int kill_pg_info(int, struct siginfo *, pid_t);
extern int kill_sl_info(int, struct siginfo *, pid_t);
extern int kill_proc_info(int, struct siginfo *, pid_t);
extern void notify_parent(marcel_task_t *, int);
extern void do_notify_parent(marcel_task_t *, int);
extern void force_sig(int, marcel_task_t *);
extern void force_sig_specific(int, marcel_task_t *);
extern int send_sig(int, marcel_task_t *, int);
extern void zap_other_threads(marcel_task_t *p);
extern int kill_pg(pid_t, int, int);
extern int kill_sl(pid_t, int, int);
extern int kill_proc(pid_t, int, int);
extern struct sigqueue *sigqueue_alloc(void);
extern void sigqueue_free(struct sigqueue *);
extern int send_sigqueue(int, struct sigqueue *,  marcel_task_t *);
extern int send_group_sigqueue(int, struct sigqueue *,  marcel_task_t *);
extern int do_sigaction(int, const struct k_sigaction *, struct k_sigaction *);
extern int do_sigaltstack(const stack_t __user *, stack_t __user *, unsigned long);

/* These can be the second arg to send_sig_info/send_group_sig_info.  */
#define SEND_SIG_NOINFO ((struct siginfo *) 0)
#define SEND_SIG_PRIV	((struct siginfo *) 1)
#define SEND_SIG_FORCED	((struct siginfo *) 2)

/* True if we are on the alternate signal stack.  */

static inline int on_sig_stack(unsigned long sp)
{
	return (sp - current->sas_ss_sp < current->sas_ss_size);
}

static inline int sas_ss_flags(unsigned long sp)
{
	return (current->sas_ss_size == 0 ? SS_DISABLE
		: on_sig_stack(sp) ? SS_ONSTACK : 0);
}


extern int  copy_thread(int, unsigned long, unsigned long, unsigned long, marcel_task_t *, struct pt_regs *);
extern void flush_thread(void);
extern void exit_thread(void);

extern void exit_mm(marcel_task_t *);
extern void exit_files(marcel_task_t *);
extern void exit_signal(marcel_task_t *);
extern void __exit_signal(marcel_task_t *);
extern void exit_sighand(marcel_task_t *);
extern void __exit_sighand(marcel_task_t *);
extern void exit_itimers(marcel_task_t *);

extern NORET_TYPE void do_group_exit(int);

extern void reparent_to_init(void);
extern void daemonize(const char *, ...);
extern int allow_signal(int);
extern int disallow_signal(int);
extern task_t *child_reaper;

extern int do_execve(char *, char __user * __user *, char __user * __user *, struct pt_regs *);
extern long do_fork(unsigned long, unsigned long, struct pt_regs *, unsigned long, int __user *, int __user *);
extern marcel_task_t * copy_process(unsigned long, unsigned long, struct pt_regs *, unsigned long, int __user *, int __user *);
#endif

#section marcel_functions
#ifdef MA__LWPS
extern void ma_wait_task_inactive(marcel_task_t * p);
#else
#define ma_wait_task_inactive(p)	do { } while (0)
#endif

#if 0
#define REMOVE_LINKS(p) do {					\
	if (thread_group_leader(p))				\
		list_del_init(&(p)->tasks);			\
	remove_parent(p);					\
	} while (0)

#define SET_LINKS(p) do {					\
	if (thread_group_leader(p))				\
		list_add_tail(&(p)->tasks,&init_task.tasks);	\
	add_parent(p, (p)->parent);				\
	} while (0)

#define next_task(p)	list_entry((p)->tasks.next, marcel_task_t, tasks)
#define prev_task(p)	list_entry((p)->tasks.prev, marcel_task_t, tasks)

#define for_each_process(p) \
	for (p = &init_task ; (p = next_task(p)) != &init_task ; )
#endif

#if 0
/*
 * Careful: do_each_thread/while_each_thread is a double loop so
 *          'break' will not work as expected - use goto instead.
 */
#define do_each_thread(g, t) \
	for (g = t = &init_task ; (g = t = next_task(g)) != &init_task ; ) do

#define while_each_thread(g, t) \
	while ((t = next_thread(t)) != g)

extern task_t * FASTCALL(next_thread(task_t *p));

#define thread_group_leader(p)	(p->pid == p->tgid)

static inline int thread_group_empty(task_t *p)
{
	struct pid *pid = p->pids[PIDTYPE_TGID].pidptr;

	return pid->task_list.next->next == &pid->task_list;
}

#define delay_group_leader(p) \
		(thread_group_leader(p) && !thread_group_empty(p))

extern void unhash_process(marcel_task_t *p);
#endif

#section marcel_inline
/* Protects ->fs, ->files, ->mm, and synchronises with wait4().
 * Nests both inside and outside of read_lock(&tasklist_lock).
 * It must not be nested with write_lock_irq(&tasklist_lock),
 * neither inside nor outside.
 */
#if 0
static inline void ma_task_lock(marcel_task_t *p)
{
	ma_spin_lock(&p->alloc_lock);
}

static inline void ma_task_unlock(marcel_task_t *p)
{
	ma_spin_unlock(&p->alloc_lock);
}
#endif 

/* set thread flags in other task's structures
 * - see asm/thread_info.h for TIF_xxxx flags available
 */
static inline void ma_set_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	ma_set_ti_thread_flag(tsk,flag);
}

static inline void ma_clear_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	ma_clear_ti_thread_flag(tsk,flag);
}

static inline int ma_test_and_set_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	return ma_test_and_set_ti_thread_flag(tsk,flag);
}

static inline int ma_test_and_clear_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	return ma_test_and_clear_ti_thread_flag(tsk,flag);
}

static inline int ma_test_tsk_thread_flag(marcel_task_t *tsk, int flag)
{
	return ma_test_ti_thread_flag(tsk,flag);
}

static inline void ma_set_tsk_need_resched(marcel_task_t *tsk)
{
	ma_set_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

static inline void ma_clear_tsk_need_resched(marcel_task_t *tsk)
{
	ma_clear_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

static inline int ma_signal_pending(marcel_task_t *p)
{
	return tbx_unlikely(ma_test_tsk_thread_flag(p,TIF_WORKPENDING));
}
  
static inline int ma_need_resched(void)
{
	return tbx_unlikely(ma_test_thread_flag(TIF_NEED_RESCHED));
}

extern void __ma_cond_resched(void);
static inline void ma_cond_resched(void)
{
	if (ma_need_resched())
		__ma_cond_resched();
}

/*
 * cond_resched_lock() - if a reschedule is pending, drop the given lock,
 * call schedule, and on return reacquire the lock.
 *
 * This works OK both with and without CONFIG_PREEMPT.  We do strange low-level
 * operations here to prevent schedule() from being called twice (once via
 * spin_unlock(), once by hand).
 */
static inline void ma_cond_resched_lock(ma_spinlock_t * lock)
{
	if (ma_need_resched()) {
		_ma_raw_spin_unlock(lock);
		ma_preempt_enable_no_resched();
		__ma_cond_resched();
		ma_spin_lock(lock);
	}
}

#if 0
/* Reevaluate whether the task has signals pending delivery.
   This is required every time the blocked sigset_t changes.
   callers must hold sighand->siglock.  */

extern FASTCALL(void recalc_sigpending_tsk(marcel_task_t *t));
extern void recalc_sigpending(void);

extern void signal_wake_up(marcel_task_t *t, int resume_stopped);
#endif

/*
 * Wrappers for p->thread_info->cpu access. No-op on UP.
 */
static inline ma_lwp_t ma_task_lwp(marcel_task_t *p)
{
	return GET_LWP(p); //p->thread_info->cpu;
}

static inline void ma_set_task_lwp(marcel_task_t *p, ma_lwp_t lwp)
{
	SET_LWP(p, lwp);
	//p->thread_info->cpu = cpu;
}

