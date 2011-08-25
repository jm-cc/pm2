/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* The "atfork" stuff */


#include "marcel.h"
#include "marcel_pmarcel.h"


/* Child process increments this after fork.  */
unsigned long int ma_fork_generation = 0;

/* atfork handlers: provided by atfork call */
typedef struct {
	void (*prepare)(void);
	void (*parent)(void);
	void (*child)(void);
} atfork_handlers;
static ma_rwlock_t atfork_list_lock = MA_RW_LOCK_UNLOCKED;
p_tbx_slist_t atfork_list = NULL;

DEF_MARCEL_PMARCEL(int, atfork, (void (*prepare) (void), void (*parent) (void), void (*child) (void)),
	    (prepare, parent, child), 
{
	atfork_handlers *new_handlers;

	/** register new handler **/
	new_handlers = (atfork_handlers *)marcel_malloc(sizeof(*new_handlers));
	if (new_handlers) {
		new_handlers->prepare = prepare;
		new_handlers->parent = parent;
		new_handlers->child = child;

		ma_write_lock(&atfork_list_lock);
		tbx_slist_add_at_head(atfork_list, new_handlers);
		ma_write_unlock(&atfork_list_lock);

		return 0;
	}

	return ENOMEM;
})
DEF_PTHREAD(int, atfork, (void (*prepare) (void), void (*parent) (void), void (*child) (void)), (prepare, parent, child))
DEF___PTHREAD(int, atfork, (void (*prepare) (void), void (*parent) (void), void (*child) (void)), (prepare, parent, child))


/* Remove THREAD, a dangling thread descriptor in the child process, from the
 * global thread list and from the runqueues.  */
static void remove_thread(marcel_t thread)
{
	if (ma_fork_generation) {
		int state TBX_UNUSED;
		ma_holder_t *holder;

		/* XXX: Should we call key destructors as well?  */

		/* Internal threads, such as "idle" and the other threads part of
		   `struct marcel_lwp', are not chained in the `all_threads' list, so
		   we can't use `marcel_one_task_less()' for them.  */
		if (! (MA_TASK_NOT_COUNTED_IN_RUNNING(thread)))
			marcel_one_task_less(thread);

		/* We don't care about the state of THREAD since it no longer exists.  */
		holder = ma_task_holder_rawlock(thread);
		state = ma_get_entity(ma_entity_task(thread));
		ma_holder_rawunlock(holder);

		ma_set_task_state(thread, MA_TASK_DEAD);
	
		/* FIXME: THREAD might still be referenced by a bubble so we don't free it
		 * for now. */
		/* ma_free_task_stack(thread); */
	} else {
		marcel_detach(thread);
		ma_exit_internal(thread, NULL);
	}
}


#if !HAVE_DECL_PTHREAD_KILL_OTHER_THREADS_NP
void pthread_kill_other_threads_np(void);
#endif
DEF_MARCEL_PMARCEL(void, kill_other_threads_np, (void), (),
{
	struct marcel_topo_level *vp;
	marcel_t thread;
	marcel_t next_thread;

	for_all_vp(vp) {
		tbx_fast_list_for_each_entry_safe(thread, next_thread,
						  &ma_topo_vpdata_l(vp, all_threads),
						  all_threads) {
			MA_BUG_ON(tbx_fast_list_empty(&thread->all_threads));
			if (thread != ma_self()
#ifdef MA__LWPS
			    && thread != __ma_get_lwp_var(idle_task)
#endif
				)
				remove_thread(thread);
		}
	}
})
DEF_PTHREAD(void, kill_other_threads_np, (void), ())


#ifdef MARCEL_FORK_ENABLED

#ifdef MA__LWPS
/* Number of LWPs waiting for fork(2) completion */
static ma_atomic_t nr_waiting_fork_completion = MA_ATOMIC_INIT(0);
/* Disallow concurent fork calls */
static ma_spinlock_t forklock = MA_SPIN_LOCK_UNLOCKED;
/* Synchronisation between forkd threads and thread which calls fork */
static tbx_bool_t  ma_in_fork = tbx_false;

/* Wake-up forkd thread on the lwp */
static inline void ma_wakeup_lwp_forkd(ma_lwp_t lwp);
#endif


/* Prepare to the fork(2) call in the parent process.  */
static void parent_prepare_fork(void)
{
#ifdef MA__LWPS
	/* First move to the main LWP since it makes a load of things 
	 * easier for handling the fork. */
	ma_runqueue_t *rq = ma_lwp_rq(&__main_lwp);
	ma_lwp_t lwp;

	SELF_GETMEM(fork_holder) = ma_bind_to_holder(!ma_is_first_lwp(MA_LWP_SELF), &rq->as_holder);
	MA_BUG_ON(! ma_is_first_lwp(MA_LWP_SELF));
	ma_spin_lock_softirq(&forklock);

	/* Wait lwps running forkd thread */
	ma_atomic_inc(&nr_waiting_fork_completion);
	ma_in_fork = tbx_true;
	ma_preempt_disable();
	ma_lwp_list_lock_read();
	ma_for_all_lwp(lwp) {
		if (lwp != MA_LWP_SELF)
			ma_wakeup_lwp_forkd(lwp);
	}
	ma_lwp_list_unlock_read();
	ma_preempt_enable();
	ma_smp_mb();
	while (marcel_nbvps() != (unsigned int)ma_atomic_read(&nr_waiting_fork_completion)) {
		ma_smp_mb();
		ma_cpu_relax();
	}
#endif
	/* Acquire the `extlib' mutex, disable preemption, bottom halves and the
	 * timer altogether so that the child can peacefully do its cleanup job
	 * once its started.  */
	marcel_extlib_protect();
	ma_preempt_disable();
	ma_local_bh_disable();
	marcel_sig_stop_itimer();


#if defined(LINUX_SYS) && defined(X86_64_ARCH) && defined(MA__PROVIDE_TLS)
	marcel_t self = ma_self();

	/* Work-around what seems like a kernel bug: fs doesn't properly get
	 * reloaded in child, so let's just make sure the base is in the MSR...
	 */
	if (self != __main_thread)
		syscall(SYS_arch_prctl, ARCH_SET_FS, marcel_tcb(self));
#endif
}

/* Restore the state of the parent process after fork(2).  */
static void cleanup_parent_after_fork(void)
{
#ifdef MA__LWPS
	/* Release lwps */
	ma_in_fork = tbx_false;
	ma_atomic_dec(&nr_waiting_fork_completion);
	ma_smp_mb();
	while (ma_atomic_read(&nr_waiting_fork_completion)) {
			ma_smp_mb();
			ma_cpu_relax();
	}
	ma_spin_unlock_softirq(&forklock);
#endif

	marcel_sig_reset_timer();
	ma_local_bh_enable_no_resched();
	ma_preempt_enable();
	marcel_extlib_unprotect();
#ifdef MA__LWPS
	/* Go back to original holder */
	ma_bind_to_holder(1, SELF_GETMEM(fork_holder));
#endif
}

#ifdef MA__LWPS
/* Remove LWP, a dangling LWP descriptor in the child process, from the
 * global LWP list. */
static void remove_lwp(marcel_lwp_t * lwp)
{
	MARCEL_LOG_IN();
	MARCEL_LOG("process %i: removing LWP %p\n", getpid(), lwp);

#if 0				/* XXX: LWP may not be on-line yet so `idle_task' et al. may not exist
				 * yet.  */
	remove_thread(lwp->idle_task);
#ifdef MARCEL_POSTEXIT_ENABLED
	remove_thread(lwp->postexit_task);
#endif
	remove_thread(lwp->ksoftirqd_task);
	remove_thread(lwp->forkd_task);
	remove_thread(lwp->run_task);
#endif

	tbx_fast_list_del(&lwp->lwp_list);

	/* FIXME: There are live `marcel_t' objects pointing to this LWP so don't
	 * free it for now.  */
	/* marcel_free_node(lwp, sizeof(marcel_lwp_t), ma_lwp_os_node(lwp)); */

	MARCEL_LOG_OUT();
}
#endif

static void cleanup_child_after_fork(void)
{
	/* FIXME: If the child tries to use `marcel_end()', then bad things will
	 * happen since Marcel is left in a haphazard state.  */
#ifndef MA__LIBPTHREAD
	ma_fork_generation ++;
#endif

#ifdef MA__LWPS
	marcel_lwp_t *lwp, *next_lwp;

	/* XXX: We only handle fork(2) invocations from the main LWP.  */
	/* printf ("self = %p, lwp-self = %p\n", ma_self(), MA_LWP_SELF); */
	MA_BUG_ON(!ma_is_first_lwp(MA_LWP_SELF));

	/* Reset counter */
	ma_in_fork = tbx_false;
	ma_smp_mb();
	ma_atomic_set(&nr_waiting_fork_completion, 0);
	ma_spin_unlock_softirq(&forklock);
#endif

#ifdef MA__BUBBLES
	/* FIXME: At the end of this function, the child is left with a full
	 * topology but only has a single LWP (thus a single VP).  Consequently,
	 * the topology is not really usable.  To minimize the risk, we install a
	 * harmless bubble scheduler that won't try to traverse the topology and VP
	 * list.  */
	marcel_bubble_change_sched(&marcel_bubble_null_sched);
#endif

	/* The child and the father would share the output FD else, and the child
	 * does not have any thread to show any more anyway.  */
	marcel_exit_top();

	/* Remove and free all thread objects but the current one so that the child
	 * process has a single thread of execution, as with pthread.  */
	marcel_kill_other_threads_np();

#ifdef MA__LWPS
	/* From the kernel perspective, the child process has only one LWP (i.e.,
	 * one "kernel thread").  Remove and free all LWP objects but the current
	 * one.  */
	tbx_fast_list_for_each_entry_safe(lwp, next_lwp, &ma_list_lwp_head, lwp_list) {
		if (lwp != MA_LWP_SELF)
			remove_lwp(lwp);
	}

	__ma_get_lwp_var(pid) = marcel_kthread_self();
	__ma_get_lwp_var(vpnum) = 0;

	/* We're left with a single LWP, thus a single VP.  */
	ma_atomic_set(&ma__last_vp, 0);
	marcel_nb_vp = 1;

	/* Update thread timer pointer: thread can migrate on new lwp */
	ma_init_timer(&SELF_GETMEM(schedule_timeout_timer));
#endif

	/* XXX: Tasklets assigned to LWPs that we just removed will never be
	 * run (PIOMan might mind).  */

	/* XXX: At this point flockfile() may not work properly.  */

	/* Restart the timer and preemption.  This allows applications using
	 * Marcel's libpthread to work correctly.  */
	marcel_sig_create_timer(MA_LWP_SELF);
	marcel_sig_reset_timer();
	ma_local_bh_enable_no_resched();
	ma_preempt_enable();
	marcel_extlib_unprotect();
}


#ifdef MA__LWPS
static inline void ma_wakeup_lwp_forkd(ma_lwp_t lwp)
{
	/* Interrupts are disabled: no need to stop preemption */
	/* Avec marcel, seul la preemption est supprimée */
	marcel_task_t *tsk = lwp->forkd_task;

	if (tsk && tsk->state != MA_TASK_RUNNING)
		ma_wake_up_thread(tsk);
}

static int forkd(void *foo TBX_UNUSED)
{
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	ma_preempt_disable();

	while (1) {
		ma_smp_mb();
		if (tbx_false == ma_in_fork) {
			ma_preempt_enable();
			if (tbx_unlikely(ma_in_atomic())) {
				PM2_LOG("bad: scheduling while atomic (%06x)! Did you forget to unlock a spinlock?\n",
					ma_preempt_count());
				ma_show_preempt_backtrace();
				MA_BUG();
			}
			ma_schedule();
			ma_preempt_disable();
			continue;
		}

		__ma_set_current_state(MA_TASK_RUNNING);
		ma_atomic_inc(&nr_waiting_fork_completion);
		while (tbx_true == ma_in_fork)
			ma_cpu_relax();
		__ma_set_current_state(MA_TASK_INTERRUPTIBLE);
		ma_atomic_dec(&nr_waiting_fork_completion);
	}
}

inline static marcel_task_t *create_forkd_start(ma_lwp_t lwp, int vp, const char *fmt, ...)
{
	marcel_t t;
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
	int err TBX_UNUSED;
	va_list ap;

	MARCEL_LOG_IN();
	/* Start forkd thread with high priority */
	marcel_attr_init(&attr);
	va_start(ap, fmt);
	marcel_vsnprintf(name, MARCEL_MAXNAMESIZE, fmt, ap);
	va_end(ap);
	marcel_attr_setname(&attr, name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	if (lwp)
		marcel_attr_setschedrq(&attr, ma_lwp_rq(lwp));
	else
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(vp));
	marcel_attr_setflags(&attr, MA_SF_NORUN);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
#ifdef PM2
	{
		char *stack = malloc(2 * THREAD_SLOT_SIZE);
		marcel_attr_setstackaddr(&attr, (void *) ((unsigned long) 
							  (stack + THREAD_SLOT_SIZE) &  ~(THREAD_SLOT_SIZE - 1)));
	}
#endif
	err = marcel_create_special(&t, &attr, (void *(*)(void *)) forkd, NULL);
	MA_BUG_ON(err != 0);
	marcel_wake_up_created_thread(t);
	return t;
}

static void forkd_init(ma_lwp_t lwp TBX_UNUSED)
{
}

static void forkd_start(ma_lwp_t lwp)
{
	marcel_task_t *p;
	/* fork are always executed on main_lwp: no need to create forkd for this */
	if (ma_spare_lwp_ext(lwp) || ma_is_first_lwp(lwp))
		return;

	p = create_forkd_start(lwp, -1, "forkd/lwp%p", lwp);
	ma_per_lwp(forkd_task, lwp) = p;
}

MA_DEFINE_LWP_NOTIFIER_START(forkd, "forkd",
			     forkd_init, "Do nothing",
			     forkd_start, "Create and launch 'forkd'");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(forkd, MA_INIT_FORKD);
MA_LWP_NOTIFIER_CALL_ONLINE(forkd, MA_INIT_FORKD);
#endif

static void fork_handling_init(void)
{
	atfork_list = tbx_slist_nil();
}
__ma_initfunc(fork_handling_init, MA_INIT_MAIN_LWP, "Fork handling");


/* We need to provide our own version of `fork()' since that's what Glibc's
 * libpthread does.
 * We need to call libc's fork to get its internal locks & such properly reset */
DEF_MARCEL_PMARCEL(pid_t, fork, (void),(),
{
	pid_t child;
	atfork_handlers *h;
	unsigned int atfork_length;

	/* user-registered pre-fork handlers (LIFO order) */
	parent_prepare_fork();
	ma_read_lock(&atfork_list_lock);
	atfork_length = tbx_slist_get_length(atfork_list);
	if (atfork_length) {
		tbx_slist_ref_to_head(atfork_list);
		while ((atfork_length --)) {
			h = tbx_slist_ref_get(atfork_list);
		
			if (h->prepare)
				(h->prepare)();

			tbx_slist_ref_forward(atfork_list);
		}
	}
	ma_read_unlock(&atfork_list_lock);

	child = MA_REALSYM(fork)();

	if (child) {
		/* parent: user-registered pre-fork handlers (FIFO order) */
		cleanup_parent_after_fork();

		ma_read_lock(&atfork_list_lock);
		atfork_length = tbx_slist_get_length(atfork_list);
		if (atfork_length) {
			tbx_slist_ref_to_tail(atfork_list);
			do {
				h = tbx_slist_ref_get(atfork_list);
		
				if (h->parent)
					(h->parent)();

				tbx_slist_ref_backward(atfork_list);
			} while ((-- atfork_length));
		}
		ma_read_unlock(&atfork_list_lock);
	} else {
		/* child: user-registered pre-fork handlers (FIFO order) */
		cleanup_child_after_fork();

		ma_read_lock(&atfork_list_lock);
		atfork_length = tbx_slist_get_length(atfork_list);
		if (atfork_length) {
			tbx_slist_ref_to_tail(atfork_list);
			do {
				h = tbx_slist_ref_get(atfork_list);
		
				if (h->child)
					(h->child)();

				tbx_slist_ref_backward(atfork_list);
			} while ((-- atfork_length));
		}
		ma_read_unlock(&atfork_list_lock);
	}

	return child;
})

#else /* MARCEL_FORK_ENABLED */

/* Warns user if Marcel is not compiled with the fork management */
DEF_MARCEL_PMARCEL(pid_t, fork, (void), (),
{
	MA_WARN_USER("You must compiled this library with fork management\n");
	errno = ENOTSUP;
	return -1;
})

#endif /* MARCEL_FORK_ENABLED */


#ifdef MA__LIBPTHREAD
extern pid_t __fork(void);
pid_t __fork(void)
{
	return marcel_fork();
}
DEF_WEAK_ALIAS(__fork, fork)
DEF_WEAK_ALIAS(__fork, vfork)
#endif
