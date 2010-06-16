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
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

/* Child process increments this after fork.  */
unsigned long int ma_fork_generation = 0;

extern int __register_atfork(void (*prepare)(void),void (*parent)(void),void (*child)(void), void * dso);

DEF_POSIX(int,atfork,(void (*prepare)(void),void (*parent)(void),void (*child)(void)),(prepare,parent,child),
{
        return __register_atfork(prepare, parent, child, NULL); 
})

DEF_PTHREAD(int,atfork,(void (*prepare)(void),void (*parent)(void),void (*child)(void)),(prepare,parent,child));
DEF___PTHREAD(int,atfork,(void (*prepare)(void),void (*parent)(void),void (*child)(void)),(prepare,parent,child));

#ifdef MA__LIBPTHREAD
#include <sys/syscall.h>

extern pid_t __libc_fork(void);

/* We need to provide our own version of `fork()' since that's what Glibc's
 * libpthread does.  */
pid_t __fork(void)
{
	/* We need to call libc's fork to get its internal locks & such properly reset */
        return __libc_fork();
}

weak_alias (__fork, fork);
weak_alias (__fork, vfork);

pid_t wait(int *status)
{
	return syscall(SYS_wait4,-1,status,0,NULL);
}

extern int __libc_system(const char *line);

int system(const char *line)
{
	return __libc_system(line);
}

#endif /* MA__LIBPTHREAD */


/* fork(2) handling.  */

/* Prepare to the fork(2) call in the parent process.  */
static void parent_prepare_fork(void) {
	/* First move to the main LWP since it makes a load of things easier for handling the fork. */
	ma_runqueue_t *rq = ma_lwp_rq(&__main_lwp);
	marcel_t self = marcel_self();
#ifdef MA__LWPS
	SELF_GETMEM(fork_holder) = ma_bind_to_holder(!ma_is_first_lwp(MA_LWP_SELF), &rq->as_holder);
#endif
	/* Acquire the `extlib' mutex, disable preemption, bottom halves and the
	 * timer altogether so that the child can peacefully do its cleanup job
	 * once its started.  */
	marcel_extlib_protect();
	ma_preempt_disable();
	ma_local_bh_disable();
	marcel_sig_stop_itimer();

#if defined(MA__PROVIDE_TLS) && defined(LINUX_SYS) && defined(X86_64_ARCH)
	/* Work-around what seems like a kernel bug: fs doesn't properly get
	 * reloaded in child, so let's just make sure the base is in the MSR...
	 */
	if (self != __main_thread)
		syscall(SYS_arch_prctl, ARCH_SET_FS, marcel_tcb(self));
#endif
}

/* Restore the state of the parent process after fork(2).  */
static void cleanup_parent_after_fork(void) {
	marcel_sig_reset_timer();
	ma_local_bh_enable_no_resched();
	ma_preempt_enable();
	marcel_extlib_unprotect();
#ifdef MA__LWPS
	/* Go back to original holder */
	ma_bind_to_holder(1, SELF_GETMEM(fork_holder));
#endif
}

/* Remove THREAD, a dangling thread descriptor in the child process, from the
 * global thread list and from the runqueues.  */
static void remove_thread(marcel_t thread)
{
	int state TBX_UNUSED;
	ma_holder_t *holder;
	char name[MARCEL_MAXNAMESIZE];

	marcel_getname(thread, name, sizeof(name));
	mdebug("process %i: removing thread %p aka. `%s'\n", getpid(), thread, name);

	marcel_setname(thread, "dead thread from parent process");

	/* XXX: Should we call key destructors as well?  */

	/* Internal threads, such as "idle" and the other threads part of
	   `struct marcel_lwp', are not chained in the `all_threads' list, so
	   we can't use `marcel_one_task_less()' for them.  */
	if (!(MA_TASK_NOT_COUNTED_IN_RUNNING(thread)))
		marcel_one_task_less(thread);

	holder = ma_task_holder_rawlock(thread);

	/* We don't care about the state of THREAD since it no longer exists.  */
	state = ma_get_entity(ma_entity_task(thread));

	ma_holder_rawunlock(holder);

	ma_set_task_state(thread, MA_TASK_DEAD);

	/* FIXME: THREAD might still be referenced by a bubble so we don't free it
	 * for now. */
	/* ma_free_task_stack(thread); */
}

#ifdef MA__LWPS
/* Remove LWP, a dangling LWP descriptor in the child process, from the
 * global LWP list.  */
static void remove_lwp(marcel_lwp_t * lwp)
{
	LOG_IN();

	mdebug("process %i: removing LWP %p\n", getpid(), lwp);

#if 0 /* XXX: LWP may not be on-line yet so `idle_task' et al. may not exist
			 * yet.  */
#ifdef MA__LWPS
	remove_thread(lwp->idle_task);
#endif
#ifdef MARCEL_POSTEXIT_ENABLED
	remove_thread(lwp->postexit_task);
#endif
	remove_thread(lwp->ksoftirqd_task);
	remove_thread(lwp->run_task);
#endif

	tbx_fast_list_del(&lwp->lwp_list);

	/* FIXME: There are live `marcel_t' objects pointing to this LWP so don't
	 * free it for now.  */
	/* marcel_free_node(lwp, sizeof(marcel_lwp_t), ma_lwp_os_node(lwp)); */

	LOG_OUT();
}
#endif

static void cleanup_child_after_fork(void)
{
	struct marcel_topo_level *vp;
#ifdef MA__LWPS
	marcel_lwp_t *lwp, *next_lwp;

	/* XXX: We only handle fork(2) invocations from the main LWP.  */
	/* printf ("self = %p, lwp-self = %p\n", marcel_self(), MA_LWP_SELF); */
	MA_BUG_ON(!ma_is_first_lwp(MA_LWP_SELF));
#endif

	ma_fork_generation++;

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
	for_all_vp(vp) {
		marcel_t thread, next_thread;

		tbx_fast_list_for_each_entry_safe(thread, next_thread, &ma_topo_vpdata_l(vp, all_threads), all_threads) {
			MA_BUG_ON(tbx_fast_list_empty(&thread->all_threads));
			if (thread != marcel_self ()
#ifdef MA__LWPS
					&& thread != __ma_get_lwp_var(idle_task)
#endif
				)
				remove_thread(thread);
		}
	}

#ifdef MA__LWPS
	/* From the kernel perspective, the child process has only one LWP (i.e.,
	 * one "kernel thread").  Remove and free all LWP objects but the current
	 * one.  */
	tbx_fast_list_for_each_entry_safe(lwp, next_lwp, &ma_list_lwp_head, lwp_list) {
		if (lwp != MA_LWP_SELF)
			remove_lwp(lwp);
	}

	__ma_get_lwp_var(pid) = marcel_kthread_self();
	/* __ma_get_lwp_var(vpnum) = 0; */

	/* We're left with a single LWP, thus a single VP.  */
	/* ma__nb_vp = 1; */
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

	/* FIXME: If the child tries to use `marcel_end()', then bad things will
	 * happen since Marcel is left in a haphazard state.  */
}


/* "Best effort" fork(2) handling.  The intent is to allow at least
 * mono-threaded children to work correctly.  This is particularly important
 * for innoncent applications run over Marcel's libpthread.  */
static void __marcel_init fork_handling_init(void) {
#ifdef MA__LWPS
# define ma_atfork marcel_kthread_atfork
#else
# define ma_atfork(prepare, parent, child)							\
  __register_atfork((prepare), (parent), (child), NULL)
#endif

	ma_atfork(parent_prepare_fork, cleanup_parent_after_fork, cleanup_child_after_fork);

#undef ma_atfork
}

__ma_initfunc(fork_handling_init, MA_INIT_MAIN_LWP, "Fork handling");
