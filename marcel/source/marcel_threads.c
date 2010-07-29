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


#include "marcel.h"
#include "marcel_sched_generic.h"
#include "tbx_compiler.h"
#include <sys/resource.h>
#include <sched.h>
#include <errno.h>


#if defined(MA__PROVIDE_TLS)
extern void *_dl_allocate_tls_init(void *) ma_libc_internal_function;
#endif

/** thread identity **/
TBX_NOINST marcel_t marcel_self()
{
	return ma_self() ;
}

/** thread stackbase **/
char *marcel_stackbase(marcel_t t)
{
	return (char *)t->stack_base;
}

/** preempt functions **/
void marcel_some_thread_preemption_enable(marcel_t t)
{
	ma_some_thread_preemption_enable(t);
}

void marcel_some_thread_preemption_disable(marcel_t t)
{
	ma_some_thread_preemption_disable(t);
}

int marcel_some_thread_is_preemption_disabled(marcel_t t)
{
	return ma_some_thread_is_preemption_disabled(t);
}


/****************************************************************/
/****************************************************************
 *                Création des threads
 */
/****************************************************************/

/****************************************************************
 *                Initialisation des structures
 */
static __inline__ void init_marcel_thread(marcel_t t, 
					  __const marcel_attr_t * __restrict attr)
{
	MARCEL_LOG_IN();
	PROF_THREAD_BIRTH(t);
	if (attr->seed) {
		t->as_entity = (struct ma_entity)MA_SCHED_ENTITY_INITIALIZER((t)->as_entity, MA_THREAD_SEED_ENTITY, MA_DEF_PRIO, "");
	} else {
		t->as_entity = (struct ma_entity)MA_SCHED_ENTITY_INITIALIZER((t)->as_entity, MA_THREAD_ENTITY, MA_DEF_PRIO, "");
	}

	/* we need to set the thread number early for tracing tools to early know
	 * whether it is an interesting thread or not */
	t->flags = attr->flags;

	/* Free within schedule_tail */
	t->preempt_count = MA_PREEMPT_OFFSET | MA_SOFTIRQ_OFFSET;
	t->not_preemptible = attr->not_preemptible;
	marcel_sched_init_marcel_thread(t, attr);
	//t->ctx_yield

	//t->flags above
	//t->child
	//t->father
	//t->f_to_call
	//t->arg

	t->f_pre_switch = attr->f_pre_switch;
	t->f_post_switch = attr->f_post_switch;

	t->cur_thread_seed = NULL;

	t->detached = !!(attr->__flags & MA_ATTR_FLAG_DETACHSTATE);
	if (!t->detached)
		marcel_sem_init(&t->client, 0);
	//t->ret_val

	ma_init_timer(&t->schedule_timeout_timer);
	t->schedule_timeout_timer.data = (unsigned long) t;
	t->schedule_timeout_timer.function = ma_process_timeout;

	//t->stack_base
	//t->stack_kind
	//t->initial_sp

	strncpy((t->as_entity).name, attr->name, MARCEL_MAXNAMESIZE);
	t->id = attr->id;
	PROF_EVENT2(set_thread_id, t, t->id);
	//t->number below

	//t->softirq_pending_in_hardirq = 0;

#ifdef MARCEL_MIGRATION_ENABLED
	//t->ctx_migr
	t->not_migratable = attr->not_migratable;
	//t->remaining_sleep_time
#endif /* MARCEL_MIGRATION_ENABLED */

#ifdef MARCEL_USERSPACE_ENABLED
	//t->real_f_to_call
	//t->sem_marcel_run
	if (attr->user_space)
		t->user_space_ptr = (char *) t - MAL(attr->user_space);
	else
		t->user_space_ptr = NULL;
#endif /* MARCEL_USERSPACE_ENABLED */

#ifdef MARCEL_POSTEXIT_ENABLED
	t->postexit_func = NULL;
	//t->postexit_arg
#endif /* MARCEL_POSTEXIT_ENABLED */

#ifdef MARCEL_ATEXIT_ENABLED
	//t->atexit_funcs
	//t->atexit_args
	t->next_atexit_func = 0;
#endif /* MARCEL_ATEXIT_ENABLED */

#ifdef MARCEL_CLEANUP_ENABLED
	t->last_cleanup = NULL;
#endif /* MARCEL_CLEANUP_ENABLED */

#ifdef MARCEL_EXCEPTIONS_ENABLED
	//t->cur_exception
	t->cur_excep_blk = NULL;
	//t->exfile
	//t->exline
#endif /* MARCEL_EXCEPTIONS_ENABLED */

	//t->key

#ifdef MARCEL_SUSPEND_ENABLED
	marcel_sem_init(&t->suspend_sem, 0);
#endif /* MARCEL_SUSPEND_ENABLED */

	ma_atomic_init(&t->top_utime, 0);

	//t->__errno=0;
	//t->__h_errno=0;
#ifdef MA__LIBPTHREAD
	//t->__res_state
#endif
	//t->all_threads

#ifdef MARCEL_DEBUG_SPINLOCK
	t->preempt_backtrace_size = 0;
	t->spinlock_backtrace = 0;
#endif

#ifdef MARCEL_DEVIATION_ENABLED
	t->not_deviatable = attr->not_deviatable;
	t->work = MARCEL_WORK_INIT;
#endif /* MARCEL_DEVIATION_ENABLED */

#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_lock_init(&t->siglock);
	marcel_sigemptyset(&t->sigpending);

	if (t == __main_thread) {
		marcel_sigemptyset(&t->curmask);
#ifdef __GLIBC__
		sigemptyset(&t->kcurmask);
#endif
	} else {
		t->curmask = MARCEL_SELF->curmask;
#ifdef __GLIBC__
		t->kcurmask = MARCEL_SELF->kcurmask;
#endif
	}

	//t->interrupted
	t->delivering_sig = 0;
	t->restart_deliver_sig = 0;
	marcel_sigemptyset(&t->waitset);
	t->waitsig = NULL;
	t->waitinfo = NULL;
#endif

#ifdef MARCEL_DEVIATION_ENABLED
#ifdef MARCEL_POSIX
	ma_spin_lock_init(&t->cancellock);
	t->cancelstate = MARCEL_CANCEL_ENABLE;
	t->canceltype = MARCEL_CANCEL_DEFERRED;
	t->canceled = MARCEL_NOT_CANCELED;
#endif
#endif /* MARCEL_DEVIATION_ENABLED */

	//t->tls

#if defined(ENABLE_STACK_JUMPING) && !defined(MA__SELF_VAR)
	*((marcel_t *) (ma_task_slot_top(t) - sizeof(void *))) = t;
#endif

	if (t == __main_thread) {
		t->number = 0;
	} else if (!(MA_TASK_NOT_COUNTED_IN_RUNNING(t))) {
		marcel_one_more_task(t);
	} else {
		/* TODO: per_lwp ? */
		static ma_atomic_t norun_pid = MA_ATOMIC_INIT(0);
		t->number = ma_atomic_dec_return(&norun_pid);
	}
	PROF_EVENT2_ALWAYS(set_thread_number, t, t->number);

	MARCEL_LOG_OUT();
}

/****************************************************************
 *                Démarrage retardé
 */
#ifdef MARCEL_USERSPACE_ENABLED
void marcel_getuserspace(marcel_t __restrict pid,
			 void * __restrict * __restrict user_space)
{
	*user_space = pid->user_space_ptr;
}

void marcel_run(marcel_t __restrict pid, any_t __restrict arg)
{
	pid->arg = arg;
	marcel_sem_V(&pid->sem_marcel_run);
}

static void* wait_marcel_run(void* arg TBX_UNUSED)
{
	marcel_t cur = ma_self();
	marcel_sem_P(&cur->sem_marcel_run);
	return ((cur->real_f_to_call)(cur->arg));
}
#endif /* MARCEL_USERSPACE_ENABLED */

/****************************************************************
 *                Création d'un thread
 */

/* special_mode est à 0 normalement. Pour les cas spéciaux (threads
 * internes, threads idle, ...), il est à 2.
 *
 * Il est calculé directement à la compilation par les deux fonctions
 * suivantes qui ne sont qu'un 'inline' de celle-ci
 */
#ifdef MA__LIBPTHREAD
int __pthread_multiple_threads __attribute__((visibility("hidden")));
#endif	

static __tbx_inline__ int 
marcel_create_internal(marcel_t * __restrict pid,
		       __const marcel_attr_t * __restrict attr, 
		       marcel_func_t func, any_t __restrict arg, 
		       __const int special_mode, 
		       __const unsigned long base_stack)
{
	marcel_t cur = ma_self(), new_task;
#ifdef MA__IFACE_PMARCEL
	marcel_attr_t myattr;
#endif
	void *stack_base;
	enum ma_stack_kind_t stack_kind;
	size_t stack_size;
	int err TBX_UNUSED;

	MARCEL_LOG_IN();

	MTRACE("marcel_create", cur);

#ifdef MA__LIBPTHREAD
	__pthread_multiple_threads = 1;
#endif

	if (!attr) {
#ifdef MA__IFACE_PMARCEL
		marcel_attr_init(&myattr);
		attr = &myattr;
#else
		attr = &marcel_attr_default;
#endif
	}

	/* TODO: we could even go further by having the programmer promise that
	 * the thread won't block, in which case we don't even need to use a
	 * stack, idle can run it! */
	if (attr->seed) {
		/* Seeds are allowed to choose a priority, which is then inherited by
		   their seed runner in `marcel_sched_seed_runner ()'.  */

	        new_task = ma_obj_alloc(marcel_thread_seed_allocator, attr->schedrq ? attr->schedrq->topolevel : NULL);
		PROF_EVENT1(thread_seed_birth, MA_PROFILE_TID(new_task));
		//new_task->shared_attr = attr;

#ifdef MA__DEBUG
		/* NOTE: this is quite costly, only do it to get gdb's
		 * marcel-threads & marcel_top working */
		marcel_one_more_task(new_task);
#endif

		/* Seeds are never scheduled directly but instead have support from a
		   "seed runner" thread.  Therefore, seeds are never actually
		   `MA_TASK_RUNNING'.  */
		new_task->state = MA_TASK_BORNING;

		/* Eventually, NEW_TASK will be assigned a seed runner.  */
		new_task->cur_thread_seed_runner = NULL;

		new_task->cur_thread_seed = NULL;
		new_task->f_to_call = func;
		new_task->arg = arg;
		new_task->stack_kind = MA_NO_STACK;
		new_task->detached = !!(attr->__flags & MA_ATTR_FLAG_DETACHSTATE);
		if (!new_task->detached)
			marcel_sem_init(&new_task->client, 0);
		marcel_sched_init_thread_seed(new_task, attr);
		marcel_wake_up_created_thread(new_task);
		if (pid)
			*pid = new_task;
		MARCEL_LOG_RETURN(0);
	}

	if (attr->__stackaddr) {
		register unsigned long top = ((unsigned long) attr->__stackaddr)
#ifndef MA__SELF_VAR
			& ~(THREAD_SLOT_SIZE - 1)
#endif
			;
		MARCEL_LOG("top=%lx, stack_top=%p\n", top, attr->__stackaddr);
		new_task = ma_slot_top_task(top);
		if ((unsigned long) new_task <=
		    (unsigned long) attr->__stackaddr - attr->__stacksize) {
			MARCEL_LOG("size = %lu, not big enough\n",
				   (unsigned long) attr->__stacksize);
			MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);	/* Not big enough */
		}
		/* check already done: stacksize <= THREAD_SLOT_SIZE */
		stack_base = (void *)((char *)attr->__stackaddr - attr->__stacksize);
		stack_size = attr->__stacksize;
		stack_kind = MA_STATIC_STACK;
		VALGRIND_STACK_REGISTER(stack_base, attr->__stackaddr);
		marcel_tls_attach(new_task);
	} else {		/* (!attr->stack_base) */
		char *bottom;
#ifdef MA__DEBUG
		if (attr->__stacksize + sizeof(marcel_task_t) > THREAD_SLOT_SIZE) {
			MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
		}
#endif
		bottom = marcel_tls_slot_alloc(attr->schedrq ? attr->schedrq->topolevel : NULL);
		PROF_EVENT(thread_stack_allocated);
		new_task = ma_slot_task(bottom);
		stack_base = bottom;
		stack_size = THREAD_SLOT_SIZE;
		stack_kind = MA_DYNAMIC_STACK;
	}			/* fin (attr->stack_base) */

	new_task->f_to_call = func;

	init_marcel_thread(new_task, attr);

#ifdef MARCEL_USERSPACE_ENABLED
	if (new_task->user_space_ptr && !attr->immediate_activation) {
		/* Le thread devra attendre marcel_run */
		new_task->f_to_call = &wait_marcel_run;
		new_task->real_f_to_call = func;
		marcel_sem_init(&new_task->sem_marcel_run, 0);
	}
#endif /* MARCEL_USERSPACE_ENABLED */

#if defined(MA__PROVIDE_TLS)
	_dl_allocate_tls_init(marcel_tcb(new_task));
#endif
	new_task->stack_base = stack_base;
	new_task->stack_kind = stack_kind;
	new_task->stack_size = stack_size;

	new_task->father = cur;

	new_task->arg = arg;

#ifdef MARCEL_USERSPACE_ENABLED
	new_task->initial_sp =
		(unsigned long) new_task - MAL(attr->user_space) -
		TOP_STACK_FREE_AREA;
#else /* MARCEL_USERSPACE_ENABLED */
	new_task->initial_sp =
		(unsigned long) new_task - TOP_STACK_FREE_AREA;
#endif /* MARCEL_USERSPACE_ENABLED */

	if (pid)
		*pid = new_task;

	MTRACE("Creation", new_task);

	//PROF_IN_EXT(newborn_thread);

	/* Le nouveau thread est démarré par le scheduler choisi */
	/* Seul le père revient... */
	err = marcel_sched_create(cur, new_task, attr, special_mode, base_stack);
	MA_BUG_ON(err != 0);

	if (cur->child)
		/* pour les processus normaux, on réveille le fils nous-même */
		marcel_wake_up_created_thread(cur->child);

	MARCEL_LOG_RETURN(0);
}

DEF_MARCEL_PMARCEL(int,create,(marcel_t * __restrict pid,
			       __const marcel_attr_t * __restrict attr,
			       marcel_func_t func, any_t __restrict arg),(pid,attr,func,arg),
{
	MARCEL_LOG_IN();
	MARCEL_LOG_RETURN(marcel_create_internal(pid, attr, func, arg,
						 0, (unsigned long)&arg));
})

int marcel_create_dontsched(marcel_t * __restrict pid,
			    __const marcel_attr_t * __restrict attr,
			    marcel_func_t func, any_t __restrict arg)
{
	return marcel_create_internal(pid, attr, func, arg,
				      1, (unsigned long)&arg);
}

int marcel_create_special(marcel_t * __restrict pid,
			  __const marcel_attr_t * __restrict attr,
			  marcel_func_t func, any_t __restrict arg)
{
	return marcel_create_internal(pid, attr, func, arg, 
				      2, (unsigned long)&arg);
}

/****************************************************************/
/****************************************************************
 *                Terminaison des threads
 */
/****************************************************************/

/****************************************************************
 *                Thread helper
 */

#ifdef MARCEL_POSTEXIT_ENABLED
static void postexit_thread_atexit_func(any_t arg) {

	struct marcel_topo_level *vp TBX_UNUSED = arg;

	MTRACE("postexit thread killed\n", ma_self());
	MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
}

static void* postexit_thread_func(any_t arg)
{
	struct marcel_topo_level *vp = arg;

	MTRACE("Start Postexit", ma_self());
	marcel_atexit(postexit_thread_atexit_func, vp);
	while(1) {
		marcel_sem_P(&ma_topo_vpdata_l(vp,postexit_thread));
		MTRACE("Postexit", ma_self());
		if (!ma_topo_vpdata_l(vp,postexit_func)) {
			MARCEL_LOG("postexit with NULL function!\n"
				   "Who will desalocate the stack!!!\n");
		} else {
			(*ma_topo_vpdata_l(vp,postexit_func))
				(ma_topo_vpdata_l(vp,postexit_arg));
			ma_topo_vpdata_l(vp,postexit_func)=NULL;
		}
		marcel_sem_V(&ma_topo_vpdata_l(vp,postexit_space));
	}

	abort();
}

static void marcel_threads_postexit_init(marcel_lwp_t *lwp TBX_UNUSED)
{
}

static void marcel_threads_postexit_start(marcel_lwp_t *lwp)
{
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];

	MARCEL_LOG_IN();
	/* Démarrage du thread responsable des terminaisons */
	if (ma_vpnum(lwp) == -1)  {
		MARCEL_LOG_OUT();
		return;
	}
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"postexit/%d",ma_vpnum(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(ma_vpnum(lwp)));
	marcel_attr_setflags(&attr, MA_SF_NORUN);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
#ifdef PM2
	{
	        char *stack = malloc(2*THREAD_SLOT_SIZE) ;
		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif
	marcel_create_special(&lwp->postexit_task, &attr, postexit_thread_func, lwp->vp_level);
	marcel_wake_up_created_thread(lwp->postexit_task);
	MARCEL_LOG_OUT();
}

MA_DEFINE_LWP_NOTIFIER_START(postexit, "Postexit thread",
			     marcel_threads_postexit_init, "'postexit' data",
			     marcel_threads_postexit_start, "Create and launch 'postexit' thread");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(postexit, MA_INIT_THREADS_DATA);
MA_LWP_NOTIFIER_CALL_ONLINE(postexit, MA_INIT_THREADS_THREAD);
#endif /* MARCEL_POSTEXIT_ENABLED */

/****************************************************************
 *                Enregistrement des fonctions de terminaison
 */
#ifdef MARCEL_POSTEXIT_ENABLED
void marcel_postexit(marcel_postexit_func_t func, any_t arg)
{
	marcel_t cur;
	MARCEL_LOG_IN();
	cur = ma_self();
	if (cur->postexit_func) {
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}
	cur->postexit_func=func;
	cur->postexit_arg=arg;
	MARCEL_LOG_OUT();
}
#endif /* MARCEL_POSTEXIT_ENABLED */

#ifdef MARCEL_ATEXIT_ENABLED
void marcel_atexit(marcel_atexit_func_t func, any_t arg)
{
	marcel_t cur = ma_self();
	
	MARCEL_LOG_IN();

	if(cur->next_atexit_func == MAX_ATEXIT_FUNCS)
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	
	cur->atexit_args[cur->next_atexit_func] = arg;
	cur->atexit_funcs[cur->next_atexit_func++] = func;

	MARCEL_LOG_OUT();
}
#endif /* MARCEL_ATEXIT_ENABLED */

/****************************************************************
 *                Terminaison
 */
#ifdef MARCEL_ATEXIT_ENABLED
/* Les fonctions de fin */
static __inline__ void marcel_atexit_exec(marcel_t t)
{
	int i;

	for(i=((marcel_t)t)->next_atexit_func-1; i>=0; i--)
		(*((marcel_t)t)->atexit_funcs[i])(((marcel_t)t)->atexit_args[i]);
}
#endif /* MARCEL_ATEXIT_ENABLED */

/* This is cleanup common to threads and thread seeds */
static void common_cleanup(marcel_t t)
{
	if (!(MA_TASK_NOT_COUNTED_IN_RUNNING(t)))
		marcel_one_task_less(t);
}

static void marcel_exit_internal(any_t val)
{
	marcel_t cur = ma_self();
#ifdef MARCEL_POSTEXIT_ENABLED
	struct marcel_topo_level *vp = NULL;
#  ifdef MA__LWPS
	marcel_vpset_t vpset;
#  endif
#endif /* MARCEL_POSTEXIT_ENABLED */
	
	MARCEL_LOG_IN();

	if (cur->cur_thread_seed) {
		cur->cur_thread_seed->ret_val = val;

		/* make sure waiter doesn't start before we're off. */
		ma_preempt_disable();

		int detached = cur->cur_thread_seed->detached;
#ifdef MA__DEBUG
		/* NOTE: this is quite costly, only do it to get gdb's
		 * marcel-threads & marcel_top working */
		marcel_one_task_less(cur->cur_thread_seed);
#endif

		marcel_funerals(cur->cur_thread_seed);
		if (!detached)
			marcel_sem_V(&cur->cur_thread_seed->client);

		/* try to die */
		ma_set_current_state(MA_TASK_DEAD);
		ma_schedule();

		return;
	}

	/* We may have been canceled during a sleep.  */
	ma_del_timer_sync(&SELF_GETMEM(schedule_timeout_timer));

	/* atexit and cleanup functions are called on the thread's stack.
	 * postexit functions are called outside the thread (either postexit or
	 * joiner), without stack when it is dynamic */
#ifdef MARCEL_CLEANUP_ENABLED
	while (cur->last_cleanup) {
		_marcel_cleanup_pop(cur->last_cleanup, tbx_true);
	}
#endif /* MARCEL_CLEANUP_ENABLED */

#ifdef MARCEL_ATEXIT_ENABLED
	marcel_atexit_exec(cur);
#endif /* MARCEL_ATEXIT_ENABLED */

#ifdef MARCEL_KEYS_ENABLED
	/* key handling */
	{
		int nb_keys=marcel_nb_keys;
		int key;
		int nb_bcl=0;
#define NB_MAX_BCL 1
		while (nb_keys && (nb_bcl++<NB_MAX_BCL)) {
			nb_keys=0;
			for(key=1; key<MAX_KEY_SPECIFIC; key++) {
				if (marcel_key_destructor[key] && cur->key[key]) {
					(*(marcel_key_destructor[key]))(cur->key[key]);
					nb_keys++;
				}
			}
		}
#ifdef MA__DEBUG
		if((NB_MAX_BCL>1) && (nb_bcl==NB_MAX_BCL))
			MARCEL_LOG("  max iteration in key destructor for thread %i\n",cur->number);
#endif
	}
#endif /* MARCEL_KEYS_ENABLED */

	ma_sigexit();

	/** update the return value **/
	cur->ret_val = val;

	/* wait for scheduler stuff */
	marcel_sched_exit(MARCEL_SELF);

#ifdef MARCEL_POSTEXIT_ENABLED
	if (cur->postexit_func) {
		ma_preempt_disable();
		vp = ma_get_task_lwp(cur)->vp_level;

#  ifdef MA__LWPS
		/* This thread mustn't be moved any more */
		vpset = MARCEL_VPSET_VP(vp->number);
		marcel_apply_vpset(&vpset);
#  endif
		ma_preempt_enable();

		/* we need to acquire postexit semaphore before disabling preemption */
		marcel_sem_P(&ma_topo_vpdata_l(vp,postexit_space));
		ma_topo_vpdata_l(vp,postexit_func)=cur->postexit_func;
		ma_topo_vpdata_l(vp,postexit_arg)=cur->postexit_arg;
	}
#endif /* MARCEL_POSTEXIT_ENABLED */

	/* make sure postexit or main doesn't start before we're off. */
	ma_preempt_disable();

#ifdef MARCEL_POSTEXIT_ENABLED
	if (cur->postexit_func)
		/* we can't wake joiner ourself, since it may get scheduled
		 * somewhere else on the machine */
		marcel_sem_V(&ma_topo_vpdata_l(vp,postexit_thread));
#endif /* MARCEL_POSTEXIT_ENABLED */

	common_cleanup(cur);

#ifndef MA__LWPS
	/* See marcel_funerals for the sem_V in the smp case.
	 * In mono, deadlock could occur if the only remaining thread is
	 * blocked on the corresponding join, as marcel_funerals is
	 * called _after_ the thread switch which will never happen
	 * since the only other thread is blocked on the sem_P.
	 *
	 * Fortunately, in mono it is safe to release the semaphore here, since
	 * preemption is already disabled, hence the joiner thread will have to
	 * wait for us to really die.
	 */
	if (!cur->detached)
		marcel_sem_V(&cur->client);
#else
	/** unlock thread which are joining main thread **/ 
	if (cur == __main_thread)
		marcel_sem_V(&cur->client);
#endif

	if (tbx_unlikely(ma_self() == __main_thread || ma_fork_generation)) {
	        ma_preempt_enable();

		/* If we have already forked, we have already cleared the
		 * scheduler in the child. Do not do anything else, and just
		 * exit */
		if (!ma_fork_generation)
		        marcel_end();
		
#ifdef STANDARD_MAIN
		exit(0);
#else
		__marcel_main_ret = 0;
		marcel_ctx_longjmp(__ma_initial_main_ctx, 1);
#endif
		abort(); // For security
	}

	/* go off */
	ma_set_current_state(MA_TASK_DEAD);
	
	ma_schedule();
}

DEF_MARCEL_PMARCEL(void TBX_NORETURN, exit, (any_t val), (val),
{
	marcel_exit_internal(val);
	abort(); // For security
})
DEF_PTHREAD(void TBX_NORETURN, exit, (void *val), (val))

void TBX_NORETURN marcel_exit_special(any_t val)
{
	marcel_exit_internal(val);
	abort(); // For security
}

void marcel_exit_canreturn(any_t val)
{
	marcel_exit_internal(val);
}

/* Called by scheduler after thread switch */
void marcel_funerals(marcel_t t) {
	if (ma_entity_task(t)->type == MA_THREAD_ENTITY && t->cur_thread_seed)
		common_cleanup(t);

	if (t->detached)
		ma_free_task_stack(t);
#ifdef MA__LWPS
	/* see marcel_exit_internal for the mono case */
	else
		marcel_sem_V(&t->client);
#endif
}

/****************************************************************/
/****************************************************************
 *                Divers
 */
/****************************************************************/


#ifdef MA__DEBUG

void marcel_print_thread(marcel_t pid)
{
	long sp;

	if (pid == ma_self()) {
		sp = (unsigned long) get_sp();
	} else {
		sp = (unsigned long) marcel_ctx_get_sp(pid->ctx_yield);
	}

	MARCEL_LOG("thread %p :\n"
		   "\tlower bound : %p\n"
		   "\tupper bound : %lx\n"
#ifdef MARCEL_USERSPACE_ENABLED
		   "\tuser space : %p\n"
#endif
		   "\tinitial sp : %lx\n"
		   "\tcurrent sp : %lx\n",
		   pid,
		   pid->stack_base,
		   ma_task_slot_top(pid),
#ifdef MARCEL_USERSPACE_ENABLED
		   pid->user_space_ptr,
#endif
		   pid->initial_sp, sp);
}

void marcel_print_jmp_buf(char *name, jmp_buf buf)
{
        unsigned int i;

#ifdef IA64_ARCH
#  define SHOW_SP ""
#else
#  define SHOW_SP (((char **)buf + i) == (char **)&SP_FIELD(buf) ? "(sp)":"")
#endif
	for (i = 0; i < sizeof(jmp_buf) / sizeof(char *); i++) {
		MARCEL_LOG("%s[%u] = %p %s\n",
			   name, i, ((char **) buf)[i], SHOW_SP);
	}
}

#endif

//PROF_NAME(thread_stack_allocated)
//PROF_NAME(newborn_thread)
//PROF_NAME(on_security_stack)

/************************join****************************/
static int check_join(marcel_t tid)
{
        if (tid == marcel_self()) {
		MARCEL_LOG("check_join : thread %p is self\n", tid);
		return EDEADLK;
	}


	/* conflit entre ESRCH et EINVAL selon l'ordre */
	/* en effet ESRCH regarde aussi le champ detached */
	if (!MARCEL_THREAD_ISALIVE(tid)) {
		MARCEL_LOG("check_join : thread %p dead\n", tid);
		return ESRCH;
	}

	if (MARCEL_THREAD_ISALIVE(tid) && (tid->detached == MARCEL_CREATE_DETACHED)) {
		MARCEL_LOG("check_join : thread %p detached\n", tid);
		return EINVAL;
	}
	return 0;
}

DEF_MARCEL(int, join, (marcel_t pid, any_t *status), (pid, status),
{
	int err;

	MARCEL_LOG_IN();
	err = check_join(pid);
	if (err)
		MARCEL_LOG_RETURN(err);

	MA_BUG_ON(pid->detached);

	marcel_sem_P(&pid->client);
	if (status)
		*status = pid->ret_val;

	pid->detached = 1;

#ifdef MARCEL_POSTEXIT_ENABLED
	if (pid->postexit_func)
		(*pid->postexit_func) (pid->postexit_arg);
#endif /* MARCEL_POSTEXIT_ENABLED */

	/** __main_thread is still running: marcel_end() **/
	if (pid != __main_thread)
		ma_free_task_stack(pid);

	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, join, (pmarcel_t ptid, any_t *status), (ptid, status),
{
	int err;

	MARCEL_LOG_IN();
	marcel_t tid = (marcel_t) ptid;

	err = check_join(tid);
	if (err)
		MARCEL_LOG_RETURN(err);

	MARCEL_LOG_RETURN(marcel_join(tid, status));
})
DEF_PTHREAD(int, join, (pthread_t tid, void **status), (tid, status))
DEF___PTHREAD(int, join, (pthread_t tid, void **status), (tid, status))

/*************pthread_cancel**********************************/
#ifdef MARCEL_DEVIATION_ENABLED
static int check_cancel(marcel_t tid)
{
	if (!MARCEL_THREAD_ISALIVE(tid)) {
		MARCEL_LOG("marcel_cancel : thread %p dead\n", tid);
		return ESRCH;
	}

	return 0;
}

DEF_MARCEL(int, cancel, (marcel_t pid), (pid),
{
	int err;

	MARCEL_LOG_IN();
	err = check_cancel(pid);
	if (err)
		MARCEL_LOG_RETURN(err);

	pid->ret_val = MARCEL_CANCELED;
	if (pid == ma_self()) {
		MARCEL_LOG_OUT();
		marcel_exit(MARCEL_CANCELED);
	} else {
		MARCEL_LOG("marcel %i kill %i\n", ma_self()->number,
			   pid->number);
		marcel_deviate(pid, (marcel_handler_func_t) marcel_exit, MARCEL_CANCELED);
	}
	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, cancel, (pmarcel_t ptid), (ptid),
{
	int err;
	marcel_t tid = (marcel_t) ptid;

	MARCEL_LOG_IN();
	err = check_cancel(tid);
	if (err)
		MARCEL_LOG_RETURN(err);

	ma_spin_lock(&tid->cancellock);
	tid->canceled = MARCEL_IS_CANCELED;
	tid->ret_val  = PMARCEL_CANCELED;
	if ((tid->cancelstate == MARCEL_CANCEL_ENABLE) && (tid->canceltype == MARCEL_CANCEL_ASYNCHRONOUS)) {
		if (tid == ma_self()) {
			ma_spin_unlock(&tid->cancellock);
			MARCEL_LOG_OUT();
			marcel_exit(PMARCEL_CANCELED);
		} else {
			MARCEL_LOG("marcel %i kill %i\n", ma_self()->number, tid->number);
			marcel_deviate(tid, (marcel_handler_func_t) marcel_exit, PMARCEL_CANCELED);
			ma_spin_unlock(&tid->cancellock);
		}
	} else {
		ma_wake_up_thread(tid);
		ma_spin_unlock(&tid->cancellock);
	}

	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int, cancel, (pthread_t pid), (pid))
#endif /* MARCEL_DEVIATION_ENABLED */

/************************detach*********************/

static int check_detach(marcel_t tid)
{
	/* conflit entre ESRCH et EINVAL selon l'ordre */
	/* en effet ESRCH regarde aussi le champ detached */
	if (!MARCEL_THREAD_ISALIVE(tid)) {
		MARCEL_LOG("check_detach : thread %p dead\n", tid);
		return ESRCH;
	}

	if (MARCEL_THREAD_ISALIVE(tid)
	    && (tid->detached == MARCEL_CREATE_DETACHED)) {
		MARCEL_LOG("check_detach : thread %p already detached\n", tid);
		return EINVAL;
	}
	return 0;
}

DEF_MARCEL(int, detach, (marcel_t pid), (pid),
{
	int err;

	MARCEL_LOG_IN();
	err = check_detach(pid);
	if (err)
		MARCEL_LOG_RETURN(err);

	pid->detached = MARCEL_CREATE_DETACHED;
	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, detach, (pmarcel_t ptid), (ptid),
{
	int err;
	marcel_t tid = (marcel_t) ptid;

	err = check_detach(tid);
	if (err)
		MARCEL_LOG_RETURN(err);

	tid->detached = PMARCEL_CREATE_DETACHED;
	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int, detach, (pthread_t pid), (pid))

#ifdef MARCEL_SUSPEND_ENABLED
static void suspend_handler(any_t arg)
{
	if ((unsigned long) arg) {
		// Suspend
		marcel_sem_P(&(SELF_GETMEM(suspend_sem)));
	} else {
		// Resume
		marcel_sem_V(&(SELF_GETMEM(suspend_sem)));
	}
}

void marcel_suspend(marcel_t pid)
{
	marcel_deviate(pid, suspend_handler, (any_t) 1);
}

void marcel_resume(marcel_t pid)
{
	marcel_deviate(pid, suspend_handler, (any_t) 0);
}
#endif /* MARCEL_SUSPEND_ENABLED */

#ifdef MARCEL_CLEANUP_ENABLED
#define __NO_WEAK_PTHREAD_ALIASES

#ifdef MA__LIBPTHREAD

#ifdef HAVE_BITS_LIBC_LOCK_H
#include <bits/libc-lock.h>
#else
# define HAVE_DECL__PTHREAD_CLEANUP_POP  0
# define HAVE_DECL__PTHREAD_CLEANUP_PUSH 0
#endif

#if !HAVE_DECL__PTHREAD_CLEANUP_POP
extern void _pthread_cleanup_pop (struct _pthread_cleanup_buffer *buffer,
				  int execute);
#endif

#if !HAVE_DECL__PTHREAD_CLEANUP_PUSH
extern void _pthread_cleanup_push (struct _pthread_cleanup_buffer *buffer,
				   void (*routine) (void *), void *arg);
#endif

#endif /* MA__LIBPTHREAD */

#undef NAME_PREFIX
#define NAME_PREFIX _
DEF_MARCEL_PMARCEL(void, cleanup_push,(struct _marcel_cleanup_buffer * __restrict __buffer,
				       cleanup_func_t func, any_t __restrict arg),
		   (__buffer, func, arg),
{
	MARCEL_LOG_IN();
	marcel_t cur = ma_self();
	__buffer->__routine = func;
	__buffer->__arg = arg;
	__buffer->__prev = cur->last_cleanup;
	cur->last_cleanup = __buffer;
	MARCEL_LOG_OUT();
})
DEF_PTHREAD(void, cleanup_push,(struct _pthread_cleanup_buffer *__buffer,
				void (*__routine)(void *), void * __arg),
	    (__buffer, __routine, __arg))
#ifdef PM2_DEV
#warning TODO: _pthread_cleanup_push_defer,pop_restore
/* _defer and _restore version have to handle cancellation. See NPTL's
 * cleanup_defer_compat.c */
#endif
#ifdef MA__LIBPTHREAD
DEF_STRONG_ALIAS(_pthread_cleanup_push, _pthread_cleanup_push_defer)
#endif

DEF_MARCEL_PMARCEL(void, cleanup_pop,(struct _marcel_cleanup_buffer *__buffer,
				      tbx_bool_t execute), (__buffer, execute),
{
	MARCEL_LOG_IN();
	marcel_t cur = ma_self();

	if (cur->last_cleanup != __buffer)
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);

	cur->last_cleanup = __buffer->__prev;
	if (execute)
		(*__buffer->__routine) (__buffer->__arg);
	MARCEL_LOG_OUT();
})
DEF_PTHREAD(void, cleanup_pop,(struct _pthread_cleanup_buffer *__buffer,
			       int __execute), (__buffer, __execute))
#ifdef MA__LIBPTHREAD
DEF_STRONG_ALIAS(_pthread_cleanup_pop, _pthread_cleanup_pop_restore)
#endif

#undef NAME_PREFIX
#define NAME_PREFIX
#endif /* MARCEL_CLEANUP_ENABLED */

#ifdef MARCEL_MIGRATION_ENABLED
void marcel_freeze(marcel_t * pids, int nb)
{
	int i;

	for (i = 0; i < nb; i++)
		if (pids[i] != ma_self())
			ma_freeze_thread(pids[i]);
}

void marcel_unfreeze(marcel_t * pids, int nb)
{
	int i;

	for (i = 0; i < nb; i++)
		if (pids[i] != ma_self())
			ma_unfreeze_thread(pids[i]);
}

/* WARNING!!! MUST BE LESS CONSTRAINED THAN MARCEL_ALIGN (64) */
#define ALIGNED_32(addr)(((unsigned long)(addr) + 31) & ~(31L))

MARCEL_INLINE void marcel_disablemigration(marcel_t pid)
{
	pid->not_migratable++;
}

MARCEL_INLINE void marcel_enablemigration(marcel_t pid)
{
	pid->not_migratable--;
}

void marcel_begin_hibernation(marcel_t __restrict t, transfert_func_t transf,
			      void *__restrict arg, tbx_bool_t fork)
{
	unsigned long depl, blk;
	unsigned long bottom, top;
	marcel_t cur = ma_self();

	if (t == cur) {
		ma_preempt_disable();
		if (marcel_ctx_setjmp(cur->ctx_migr) == FIRST_RETURN) {

			ma_ST_FLUSH_WINDOWS();
			top = ma_task_slot_top(cur);
			bottom =
				ALIGNED_32((unsigned long) marcel_ctx_get_sp(cur->
									     ctx_migr)) - ALIGNED_32(1);
			blk = top - bottom;
			depl = bottom - (unsigned long) cur->stack_base;

			MARCEL_LOG("hibernation of thread %p", cur);
			MARCEL_LOG("sp = %lu\n", get_sp());
			MARCEL_LOG("sp_field = %lu\n",
				   (unsigned long) marcel_ctx_get_sp(cur->ctx_migr));
			MARCEL_LOG("bottom = %lu\n", bottom);
			MARCEL_LOG("top = %lu\n", top);
			MARCEL_LOG("blk = %lu\n", blk);

			(*transf) (cur, depl, blk, arg);

			if (!fork)
				marcel_exit(NULL);
		} else {
#ifdef MA__DEBUG
			marcel_breakpoint();
#endif
			MA_THR_RESTARTED(MARCEL_SELF, "End of hibernation");
			ma_schedule_tail(__ma_get_lwp_var(previous_thread));
			ma_preempt_enable();
		}
	} else {
		long timer_remaining;
		memcpy(t->ctx_migr, t->ctx_yield, sizeof(marcel_ctx_t));
		timer_remaining =
			t->schedule_timeout_timer.expires - ma_jiffies;
		if (timer_remaining > 0) {
			ma_del_timer_sync(&t->schedule_timeout_timer);
			t->remaining_sleep_time = timer_remaining;
		} else
			t->remaining_sleep_time = 0;

		top = ma_task_slot_top(t);
		bottom =
			ALIGNED_32((unsigned long) marcel_ctx_get_sp(t->
								     ctx_yield)) - ALIGNED_32(1);
		blk = top - bottom;
		depl = bottom - (unsigned long) t->stack_base;

		MARCEL_LOG("hibernation of thread %p", t);
		MARCEL_LOG("sp_field = %lu\n",
			   (unsigned long) marcel_ctx_get_sp(cur->ctx_migr));
		MARCEL_LOG("bottom = %lu\n", bottom);
		MARCEL_LOG("top = %lu\n", top);
		MARCEL_LOG("blk = %lu\n", blk);

		(*transf) (t, depl, blk, arg);

		if (!fork) {
#ifdef MARCEL_DEVIATION_ENABLED
			marcel_cancel(t);
#else /* MARCEL_DEVIATION_ENABLED */
			MA_BUG();
#endif /* MARCEL_DEVIATION_ENABLED */
		}
	}
}

void marcel_end_hibernation(marcel_t __restrict t, post_migration_func_t f,
			    void *__restrict arg)
{
	memcpy(t->ctx_yield, t->ctx_migr, sizeof(marcel_ctx_t));

	MARCEL_LOG("end of hibernation for thread %p", t);

	ma_preempt_disable();

	marcel_sched_init_marcel_thread(t, &marcel_attr_default);
	t->preempt_count = (2 * MA_PREEMPT_OFFSET) | MA_SOFTIRQ_OFFSET;
	marcel_one_more_task(t);
	ma_set_task_state(t, MA_TASK_INTERRUPTIBLE);
	if (t->remaining_sleep_time) {
		marcel_mod_timer(&t->schedule_timeout_timer,
				 ma_jiffies + t->remaining_sleep_time);
	} else
		ma_wake_up_thread(t);

#ifdef MARCEL_DEVIATION_ENABLED
	if (f != NULL)
		marcel_deviate(t, f, arg);
#else /* MARCEL_DEVIATION_ENABLED */
	MA_BUG_ON(f);
#endif /* MARCEL_DEVIATION_ENABLED */

	ma_preempt_enable();
}
#endif /* MARCEL_MIGRATION_ENABLED */

static void main_thread_init(void)
{
	marcel_attr_t attr;

	MARCEL_LOG_IN();
#ifndef STANDARD_MAIN
	if (__main_thread == NULL) {
		fprintf(stderr,"Couldn't find main thread's marcel_t, i.e. Marcel's main was not called yet. Please either load the stackalign module early, or use the STANDARD_MAIN option (but this will decrease performance)");
		abort();
	}
#endif
	memset(__main_thread, 0, sizeof(marcel_task_t));
	
#if defined(ENABLE_STACK_JUMPING) && !defined(MA__SELF_VAR)
	*((marcel_t *)(ma_task_slot_top(__main_thread) - sizeof(void *))) = __main_thread;
#endif

	marcel_attr_init(&attr);
	marcel_attr_setname(&attr,"main");
	marcel_attr_setdetachstate(&attr, MARCEL_CREATE_JOINABLE);
#ifdef MARCEL_MIGRATION_ENABLED
	marcel_attr_setmigrationstate(&attr, tbx_false);
#endif /* MARCEL_MIGRATION_ENABLED */
	marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_SHARED);
#ifdef MA__BUBBLES
	PROF_EVENT1_ALWAYS(bubble_sched_new,&marcel_root_bubble);
	marcel_attr_setnaturalbubble(&attr, &marcel_root_bubble);
#else
	marcel_attr_setschedrq(&attr, &ma_main_runqueue);
#endif

	ma_set_task_lwp(__main_thread,&__main_lwp);
	init_marcel_thread(__main_thread, &attr);
	__main_thread->initial_sp = get_sp();
#if defined(STANDARD_MAIN)
	/* the stack is not in a slot */
	__main_thread->stack_base = NULL;
	{
		struct rlimit rlp;
		getrlimit(RLIMIT_STACK, &rlp);		
		__main_thread->stack_size = rlp.rlim_cur;
	}
#else
	__main_thread->stack_base = (any_t)(get_sp() & ~(THREAD_SLOT_SIZE-1));
	__main_thread->stack_size = THREAD_SLOT_SIZE;
#endif
	
	ma_preempt_count()=0;
	ma_irq_enter();
	ma_set_current_state(MA_TASK_BORNING);

#ifdef PM2DEBUG
	marcel_debug_show_thread_info(tbx_true) ;
#endif
	PROF_SET_THREAD_NAME(__main_thread);
	MARCEL_LOG_OUT();
}

__ma_initfunc(main_thread_init, MA_INIT_THREADS_MAIN, "Initialise the main thread structure");

#ifdef STANDARD_MAIN
marcel_task_t __main_thread_struct = {.not_preemptible = 1};
#else
marcel_t __main_thread;
#endif


/*************************set/getconcurrency**********************/
/*The implementation shall use this as a hint, not a requirement.*/
/* TODO: change the current number of VP? Quit hard to augment it... */
static int concurrency = 0;
DEF_MARCEL_PMARCEL(int, setconcurrency,(int new_level),(new_level),
{
	MARCEL_LOG_IN();
	if (new_level < 0) {
#ifdef MA__DEBUG
		MARCEL_LOG("(p)marcel_setconcurrency : invalid newlevel (%d)",
			   new_level);
#endif
		MARCEL_LOG_RETURN(EINVAL);
	}

	if (!new_level) {
		MARCEL_LOG_RETURN(0);
	}
	concurrency = new_level;
	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int,setconcurrency,(int new_level),(new_level))
DEF___PTHREAD(int,setconcurrency,(int new_level),(new_level))

DEF_MARCEL_PMARCEL(int, getconcurrency,(void),(),
{
	MARCEL_LOG_IN();
	MARCEL_LOG_RETURN(concurrency);
})

DEF_PTHREAD(int,getconcurrency,(void),())
DEF___PTHREAD(int,getconcurrency,(void),())

/***************************setcancelstate************************/
#if defined(MARCEL_DEVIATION_ENABLED) && defined(MARCEL_POSIX)
static int check_setcancelstate(int state)
{
	if ((state != MARCEL_CANCEL_ENABLE) && (state != MARCEL_CANCEL_DISABLE)) {
		MARCEL_LOG("pmarcel_setcancelstate : valeur state (%d) invalide !\n", state);
		return EINVAL;
	}
	return 0;
}

DEF_PMARCEL(int, setcancelstate,(int state, int *oldstate),(state, oldstate),
{
	int err;
	marcel_t cthread = ma_self();

	MARCEL_LOG_IN();

	err = check_setcancelstate(state);
	if (err)
		MARCEL_LOG_RETURN(err);

	ma_spin_lock(&cthread->cancellock);
	if (oldstate)
		*oldstate = cthread->cancelstate;
	cthread->cancelstate = state;
	ma_spin_unlock(&cthread->cancellock);

	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int, setcancelstate, (int state,int *oldstate), (state,oldstate))
DEF___PTHREAD(int, setcancelstate, (int state,int *oldstate), (state,oldstate))
#endif

/****************************setcanceltype************************/
#if defined(MARCEL_DEVIATION_ENABLED) && defined(MARCEL_POSIX)
static int check_setcanceltype(int type)
{
	if ((type != MARCEL_CANCEL_ASYNCHRONOUS) && (type != MARCEL_CANCEL_DEFERRED)) {
		MARCEL_LOG("pmarcel_setcanceltype : valeur type invalide : %d !\n", type);
		return EINVAL;
	}
	return 0;
}

DEF_PMARCEL(int, setcanceltype,(int type, int *oldtype),(type, oldtype),
{
	int err;
	marcel_t cthread = ma_self();

	MARCEL_LOG_IN();

	err = check_setcanceltype(type);
	if (err)
		MARCEL_LOG_RETURN(err);

	ma_spin_lock(&cthread->cancellock);
	if (oldtype)
		*oldtype = cthread->canceltype;
	cthread->canceltype = type;
	ma_spin_unlock(&cthread->cancellock);

	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int, setcanceltype, (int type,int *oldtype), (type,oldtype))
DEF___PTHREAD(int, setcanceltype, (int type,int *oldtype), (type,oldtype))
#endif

/*****************************testcancel***************************/
//TODO : test_cancel shall create a cancellation point in the calling
//       thread. The pthread_testcancel() function shall have no effect 
//       if cancelability is disabled.

#if defined(MARCEL_DEVIATION_ENABLED) && defined(MARCEL_POSIX)
DEF_PMARCEL(void, testcancel,(void),(),
{
	MARCEL_LOG_IN();
	marcel_t cthread = ma_self();
	ma_spin_lock(&cthread->cancellock);
	if ((cthread->cancelstate == MARCEL_CANCEL_ENABLE)
	    && (cthread->canceled == MARCEL_IS_CANCELED)) {
		ma_spin_unlock(&cthread->cancellock);
		cthread->ret_val = PMARCEL_CANCELED;
		MARCEL_LOG("thread %i s'annule et exit\n", cthread->number);
		MARCEL_LOG_OUT();
		marcel_exit(PMARCEL_CANCELED);
	} else
		ma_spin_unlock(&cthread->cancellock);

	MARCEL_LOG_OUT();
})
 
DEF_PTHREAD(void, testcancel,(void),())
DEF___PTHREAD(void, testcancel,(void),())


#ifdef MA__IFACE_PMARCEL
/* XXX: NPTL now gets it directly compiled into GLIBC :/ */
int fastcall __pmarcel_enable_asynccancel (void) {
	int old;
	pmarcel_setcanceltype(PMARCEL_CANCEL_ASYNCHRONOUS, &old);
	return old;
}

void fastcall __pmarcel_disable_asynccancel(int old) {
	pmarcel_setcanceltype(old, NULL);
	pmarcel_testcancel();
}
#ifdef MA__LIBPTHREAD
DEF_STRONG_ALIAS(__pmarcel_enable_asynccancel, __pthread_enable_asynccancel)
DEF_STRONG_ALIAS(__pmarcel_disable_asynccancel, __pthread_disable_asynccancel)
#endif
#endif
#endif

/***********************setprio_posix2marcel******************/
static int setprio_posix2marcel(marcel_t thread,int prio,int policy)
{
	/* on passe de prio posix a prio marcel */
	if ((policy == SCHED_RR) || (policy == SCHED_FIFO)) {
		if ((prio >= 0) && (prio <= MA_RT_PRIO))
			ma_sched_change_prio(thread, MA_RT_PRIO - prio);
		else {
			MARCEL_LOG("setprio_posix2marcel : priority %d invalide\n",
				   prio);
			MARCEL_LOG_RETURN(EINVAL);
		}
	} else if (policy == SCHED_OTHER) {
		if (prio == 0)
			ma_sched_change_prio(thread, MA_DEF_PRIO);
		else {
			MARCEL_LOG("setprio_posix2marcel : priority %d invalide\n",
				   prio);
			MARCEL_LOG_RETURN(EINVAL);
		}
	} else
		MARCEL_LOG("setprio_posix2marcel : policy %d invalide\n", policy);
	/* on est passé de prio posix a prio marcel */

	MARCEL_LOG_RETURN(0);
}

/*************************setschedprio************************/
DEF_MARCEL_PMARCEL(int, setschedprio,(marcel_t thread,int prio),(thread,prio),
{
	MARCEL_LOG_IN();
	int policy;

	/* détermination de la police POSIX d'après la priorité Marcel et la préemption */
	int mprio = thread->as_entity.prio;
	if (mprio >= MA_DEF_PRIO)
		policy = SCHED_OTHER;
	else if (mprio <= MA_RT_PRIO) {
		if (ma_some_thread_is_preemption_disabled(thread))
			policy = SCHED_FIFO;
		else
			policy = SCHED_RR;
	}
	/* policy déterminée */

	int ret = setprio_posix2marcel(thread, prio, policy);
	if (ret)
		MARCEL_LOG_RETURN(ret);

	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int, setschedprio, (pthread_t thread, int prio),(thread, prio))
DEF___PTHREAD(int, setschedprio, (pthread_t thread, int prio),(thread, prio))

/*************************setschedparam****************************/
DEF_MARCEL(int, setschedparam,(marcel_t thread, int policy, 
			       const struct marcel_sched_param *__restrict param), (thread,policy,param),
{
	MARCEL_LOG_IN();

	if (param == NULL) {
		MARCEL_LOG("marcel_setschedparam : valeur attr ou param NULL\n");
		MARCEL_LOG_RETURN(EINVAL);
	}
	if ((policy != SCHED_FIFO) && (policy != SCHED_RR)
	    && (policy != SCHED_OTHER)) {
		MARCEL_LOG("marcel_setschedparam : valeur policy(%d) invalide\n",
			   policy);
		MARCEL_LOG_RETURN(EINVAL);
	}

	marcel_sched_setscheduler(thread, policy, param);

	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, setschedparam,(pmarcel_t pmthread, int policy, 
				const struct marcel_sched_param *__restrict param), (pmthread,policy,param),
{
	MARCEL_LOG_IN();
	marcel_t thread = (marcel_t) pmthread;

	if (param == NULL) {
		MARCEL_LOG("pmarcel_setschedparam : valeur attr ou param NULL\n");
		MARCEL_LOG_RETURN(EINVAL);
	}

	if ((policy != SCHED_FIFO) && (policy != SCHED_RR)
	    && (policy != SCHED_OTHER)) {
		MARCEL_LOG("pmarcel_setschedparam : valeur policy(%d) invalide\n",
			   policy);
		MARCEL_LOG_RETURN(EINVAL);
	}

	/* (dés)activation de la préemption en fonction de SCHED_FIFO */
	if (policy == SCHED_FIFO) {
		if (!ma_some_thread_is_preemption_disabled(thread))
			ma_some_thread_preemption_disable(thread);
	} else {
		if (ma_some_thread_is_preemption_disabled(thread))
			ma_some_thread_preemption_enable(thread);
	}

	int ret = setprio_posix2marcel(thread, param->__sched_priority, policy);
	if (ret)
		MARCEL_LOG_RETURN(ret);

	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int, setschedparam, (pthread_t thread, int policy, 
				 __const struct sched_param *__restrict param), (thread, policy, param))
DEF___PTHREAD(int, setschedparam, (pthread_t thread, int policy,
				   __const struct sched_param *__restrict param), (thread, policy, param))

/******************************getschedparam*************************/
DEF_MARCEL(int, getschedparam,(marcel_t thread, int *__restrict policy,
			       struct marcel_sched_param *__restrict param),(thread,policy,param),
{
	if ((param == NULL) || (policy == NULL)) {
		MARCEL_LOG("pmarcel_getschedparam : valeur policy ou param NULL\n");
		MARCEL_LOG_RETURN(EINVAL);
	}

	*policy = marcel_sched_getscheduler(thread);
	marcel_sched_getparam(thread, param);

	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, getschedparam,(pmarcel_t pmthread, int *__restrict policy,
				struct marcel_sched_param *__restrict param),(pmthread,policy,param),
{
	MARCEL_LOG_IN();
	marcel_t thread = (marcel_t) pmthread;

	if ((param == NULL) || (policy == NULL)) {
		MARCEL_LOG("pmarcel_getschedparam : valeur policy ou param NULL\n");
		MARCEL_LOG_RETURN(EINVAL);
	}

	/* détermination de la police et priority POSIX d'après la priorité Marcel et la préemption */
	int mprio = thread->as_entity.prio;

	if (mprio >= MA_DEF_PRIO) {
		param->__sched_priority = 0;
		*policy = SCHED_OTHER;
	} else if (mprio <= MA_RT_PRIO) {
		param->__sched_priority = MA_RT_PRIO - mprio;
		if (ma_some_thread_is_preemption_disabled(thread))
			*policy = SCHED_FIFO;
		else
			*policy = SCHED_RR;
	}
	/* policy et priority POSIX déterminées */

	MARCEL_LOG_RETURN(0);
})

DEF_PTHREAD(int, getschedparam, (pthread_t thread, int *__restrict policy, 
				 struct sched_param *__restrict param), (thread, policy, param))
DEF___PTHREAD(int, getschedparam, (pthread_t thread, int *__restrict policy, 
				   struct sched_param *__restrict param), (thread, policy, param))

#if !defined(MARCEL_DONT_USE_POSIX_THREADS) || defined(MA__LIBPTHREAD)
#ifdef _POSIX_CPUTIME
#  if _POSIX_CPUTIME >= 0
/**********************getcpuclockid****************************/
DEF_PMARCEL(int,getcpuclockid,(pmarcel_t thread_id TBX_UNUSED, clockid_t *clock_id),(thread_id,clock_id),
{
	MARCEL_LOG_IN();
	clock_getcpuclockid(0, clock_id);
	MARCEL_LOG_RETURN(0);
})
#  endif
#endif

DEF_PTHREAD(int,getcpuclockid,(pthread_t thread_id, clockid_t *clock_id),(thread_id,clock_id))
DEF___PTHREAD(int,getcpuclockid,(pthread_t thread_id, clockid_t *clock_id),(thread_id,clock_id))
#endif

#ifdef PROFILE
/** Write a HW counter sampling value associated to a given thread. */
void ma_thread_record_hw_sample(marcel_t t, unsigned long hw_id, unsigned long long hw_val) {
	PROF_EVENT4(fut_thread_hw_sample, MA_PROFILE_TID(t), hw_id,
		    (unsigned long)(((unsigned long long)hw_val) & 0xffffffff),
		    (unsigned long)((((unsigned long long)hw_val) >> 32) & 0xffffffff));
}
#endif

/* TODO : several functions may fail if: [ESRCH]
   No thread could be found corresponding to that specified by the given thread ID.pthread_detach, pthread_getschedparam, pthread_join, pthread_kill, pthread_cancel, pthread_setschedparam, pthread_setschedprio */
