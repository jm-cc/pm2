
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
#include "marcel_sched_generic___sched_marcel_inline.h"
#include "tbx_compiler.h"

#ifdef __MINGW32__
#define SCHED_RR (-1)
#else
#include <sched.h>
#endif
#include <errno.h>

/****************************************************************/
/****************************************************************
 *                Cr�ation des threads
 */
/****************************************************************/

/****************************************************************
 *                Initialisation des structures
 */
static __inline__ void init_marcel_thread(marcel_t __restrict t, 
					  __const marcel_attr_t * __restrict attr,
					  int special_mode)
{
	PROF_THREAD_BIRTH(t);

	/* we need to set the thread number early for tracing tools to early know
	 * whether it is an interesting thread or not */
	t->flags = attr->flags;

	if (t == __main_thread) {
		t->number = 0;
	} else if (!(special_mode == 2 && MA_TASK_NOT_COUNTED_IN_RUNNING(t))) {
		marcel_one_more_task(t);
	} else {
		/* TODO: per_lwp ? */
		static ma_atomic_t norun_pid = MA_ATOMIC_INIT(0);
		t->number = ma_atomic_dec_return(&norun_pid);
	}
	PROF_EVENT2_ALWAYS(set_thread_number, t, t->number);

	/* Free within schedule_tail */
	t->preempt_count = MA_PREEMPT_OFFSET | MA_SOFTIRQ_OFFSET;
	marcel_sched_init_marcel_thread(t, attr);
	t->not_preemptible = attr->not_preemptible;
	//t->ctx_yield
	t->work = MARCEL_WORK_INIT;
	t->softirq_pending_in_hardirq = 0;
	//t->ctx_migr
	//t->child
	//t->father
	//t->f_to_call
	//t->real_f_to_call
	//t->arg
	//t->sem_marcel_run
	if (attr->user_space)
		t->user_space_ptr = (char *) t - MAL(attr->user_space);
	else
		t->user_space_ptr = NULL;

	t->detached = (attr->__detachstate == MARCEL_CREATE_DETACHED);

	if (!attr->__detachstate) {
		marcel_sem_init(&t->client, 0);
		marcel_sem_init(&t->thread, 0);
	}
	//t->ret_val

	t->postexit_func = NULL;
	//t->postexit_arg
	//t->atexit_funcs
	//t->atexit_args
	t->next_atexit_func = 0;
	t->last_cleanup = NULL;

	//t->cur_exception
	t->cur_excep_blk = NULL;
	//t->exfile
	//t->exline

	//t->key
	ma_init_timer(&t->schedule_timeout_timer);
	t->schedule_timeout_timer.data = (unsigned long) t;
	t->schedule_timeout_timer.function = ma_process_timeout;

	//t->stack_base
	//t->static_stack
	//t->initial_sp
	strncpy(t->name, attr->name, MARCEL_MAXNAMESIZE);
	t->id = attr->id;
	PROF_EVENT2(set_thread_id, t, t->id);
	//t->number
	t->not_migratable = attr->not_migratable;
	t->not_deviatable = attr->not_deviatable;
	marcel_sem_init(&t->suspend_sem, 0);
	ma_atomic_init(&t->top_utime, 0);

	//t->__errno=0;
	//t->__h_errno=0;
#ifdef MA__LIBPTHREAD
	//t->__res_state
#endif
	t->p_readlock_list = NULL;
	t->p_readlock_free = NULL;
	t->p_untracked_readlock_count = 0;
	t->p_nextwaiting = NULL;
	marcel_sem_init(&t->pthread_sync, 0);

#ifdef MARCEL_DEBUG_SPINLOCK
	t->preempt_backtrace_size = 0;
	t->spinlock_backtrace = 0;
#endif

	ma_spin_lock_init(&t->siglock);
	marcel_sigemptyset(&t->sigpending);
	t->delivering_sig = 0;
	t->restart_deliver_sig = 0;
	marcel_sigemptyset(&t->waitset);
	t->waitsig = NULL;
	t->waitinfo = NULL;
	t->curmask = MARCEL_SELF->curmask;
#ifdef MA__LIBPTHREAD
	ma_spin_lock_init(&t->cancellock);
	t->cancelstate = MARCEL_CANCEL_ENABLE;
	t->canceltype = MARCEL_CANCEL_DEFERRED;
	t->canceled = MARCEL_NOT_CANCELED;
#endif

#ifdef ENABLE_STACK_JUMPING
	*((marcel_t *) (ma_task_slot_top(t) - sizeof(void *))) = t;
#endif

}

void marcel_create_init_marcel_thread(marcel_t __restrict t, 
				      __const marcel_attr_t * __restrict attr)
{
	return init_marcel_thread(t, attr, 0);
}

/****************************************************************
 *                D�marrage retard�
 */
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

static void* wait_marcel_run(void* arg)
{
	marcel_t cur = marcel_self();
	marcel_sem_P(&cur->sem_marcel_run);
	return ((cur->real_f_to_call)(cur->arg));
}

/****************************************************************
 *                Cr�ation d'un thread
 */

/* special_mode est � 0 normalement. Pour les cas sp�ciaux (threads
 * internes, threads idle, ...), il est � 2.
 *
 * Il est calcul� directement � la compilation par les deux fonctions
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
	marcel_t cur = marcel_self(), new_task;
#ifdef MA__IFACE_PMARCEL
	marcel_attr_t myattr;
#endif
	void *stack_base;
	int static_stack;

	LOG_IN();

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

	if (attr->__stackaddr_set) {
		register unsigned long top = ((unsigned long) attr->__stackaddr)
		    & ~(THREAD_SLOT_SIZE - 1);
		mdebug("top=%lx, stack_top=%p\n", top, attr->__stackaddr);
		new_task = ma_slot_top_task(top);
		if ((unsigned long) new_task <=
		    (unsigned long) attr->__stackaddr - attr->__stacksize) {
			mdebug("size = %lu, not big enough\n",
			    (unsigned long) attr->__stacksize);
			MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);	/* Not big enough */
		}
		stack_base = attr->__stackaddr - attr->__stacksize;
		static_stack = tbx_true;
		/* TODO: Initialize TLS */
	} else {		/* (!attr->stack_base) */
		char *bottom;
#ifdef MA__DEBUG
		if (attr->__stacksize > THREAD_SLOT_SIZE) {
			MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
		}
#endif
		bottom = marcel_tls_slot_alloc();
		PROF_EVENT(thread_stack_allocated);
		new_task = ma_slot_task(bottom);
		stack_base = bottom;
		static_stack = tbx_false;
	}			/* fin (attr->stack_base) */

	init_marcel_thread(new_task, attr, special_mode);
	new_task->stack_base = stack_base;
	new_task->static_stack = static_stack;

	new_task->father = cur;
	if (new_task->user_space_ptr && !attr->immediate_activation) {
		/* Le thread devra attendre marcel_run */
		new_task->f_to_call = &wait_marcel_run;
		new_task->real_f_to_call = func;
		marcel_sem_init(&new_task->sem_marcel_run, 0);
	} else
		new_task->f_to_call = func;

	new_task->arg = arg;

	new_task->initial_sp =
	    (unsigned long) new_task - MAL(attr->user_space) -
	    TOP_STACK_FREE_AREA;

	if (pid)
		*pid = new_task;

	MTRACE("Creation", new_task);

	//PROF_IN_EXT(newborn_thread);

	/* Le nouveau thread est d�marr� par le scheduler choisi */
	/* Seul le p�re revient... */
	marcel_sched_create(cur, new_task, attr, special_mode, base_stack);

	if (cur->child)
		/* pour les processus normaux, on r�veille le fils nous-m�me */
		marcel_wake_up_created_thread(cur->child);

	LOG_RETURN(0);
}

DEF_MARCEL_POSIX(int,create,(marcel_t * __restrict pid,
		  __const marcel_attr_t * __restrict attr,
									  marcel_func_t func, any_t __restrict arg),(pid,attr,func,arg),
{
        LOG_IN();
	LOG_RETURN(marcel_create_internal(pid, attr, func, arg,
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

static void postexit_thread_atexit_func(any_t arg) {

	struct marcel_topo_level *vp TBX_UNUSED = arg;

	MTRACE("postexit thread killed\n", marcel_self());
	MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
}

static void* postexit_thread_func(any_t arg)
{
	struct marcel_topo_level *vp = arg;

	LOG_IN();
	MTRACE("Start Postexit", marcel_self());
	marcel_atexit(postexit_thread_atexit_func, vp);
	for(;;) {
		marcel_sem_P(&ma_topo_vpdata(vp,postexit_thread));
		MTRACE("Postexit", marcel_self());
		if (!ma_topo_vpdata(vp,postexit_func)) {
			mdebug("postexit with NULL function!\n"
				"Who will desalocate the stack!!!\n");
		} else {
			(*ma_topo_vpdata(vp,postexit_func))
				(ma_topo_vpdata(vp,postexit_arg));
			ma_topo_vpdata(vp,postexit_func)=NULL;
		}
		marcel_sem_V(&ma_topo_vpdata(vp,postexit_space));
	}
	LOG_OUT();
	abort(); /* For security */
}

void marcel_threads_postexit_init(marcel_lwp_t *lwp)
{
}

void marcel_threads_postexit_start(marcel_lwp_t *lwp)
{
	marcel_attr_t attr;
	marcel_t postexit;
	char name[MARCEL_MAXNAMESIZE];

	LOG_IN();
	/* D�marrage du thread responsable des terminaisons */
	if (LWP_NUMBER(lwp) == -1)  {
		LOG_OUT();
		return;
	}
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"postexit/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_NORUN);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);
		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif
	marcel_create_special(&postexit, &attr, postexit_thread_func, lwp->vp_level);
	marcel_wake_up_created_thread(postexit);
	LOG_OUT();
}

MA_DEFINE_LWP_NOTIFIER_START(postexit, "Postexit thread",
			     marcel_threads_postexit_init, "'postexit' data",
			     marcel_threads_postexit_start, "Create and launch 'postexit' thread");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(postexit, MA_INIT_THREADS_DATA);
MA_LWP_NOTIFIER_CALL_ONLINE(postexit, MA_INIT_THREADS_THREAD);

/****************************************************************
 *                Enregistrement des fonctions de terminaison
 */
void marcel_postexit(marcel_postexit_func_t func, any_t arg)
{
	marcel_t cur;
	LOG_IN();
	cur = marcel_self();
	if (cur->postexit_func) {
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}
	cur->postexit_func=func;
	cur->postexit_arg=arg;
	LOG_OUT();
}

void marcel_atexit(marcel_atexit_func_t func, any_t arg)
{
	marcel_t cur = marcel_self();
	
	LOG_IN();

	if(cur->next_atexit_func == MAX_ATEXIT_FUNCS)
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	
	cur->atexit_args[cur->next_atexit_func] = arg;
	cur->atexit_funcs[cur->next_atexit_func++] = func;

	LOG_OUT();
}

/****************************************************************
 *                Terminaison
 */
/* Les fonctions de fin */
static __inline__ void marcel_atexit_exec(marcel_t t)
{
  int i;

  for(i=((marcel_t)t)->next_atexit_func-1; i>=0; i--)
    (*((marcel_t)t)->atexit_funcs[i])(((marcel_t)t)->atexit_args[i]);
}

static void detach_func(any_t arg) {
	marcel_t t=(marcel_t) arg;
	marcel_sem_V(&t->client);
	//	marcel_sem_P(&cur->thread);
}

static void TBX_NORETURN marcel_exit_internal(any_t val, int special_mode)
{
	marcel_t cur = marcel_self();
	struct marcel_topo_level *vp;
#ifdef MA__LWPS
	marcel_vpmask_t mask;
#endif
	
	LOG_IN();

	// On appelle marcel_atexit et marcel_cleanup sur la pile
	// courante. Les fonctions atexit ne doivent pas d�truire la
	// pile. Les fonctions postexit peuvent le faire.
	while (cur->last_cleanup) {
		_marcel_cleanup_pop(cur->last_cleanup, 1);
	}
	marcel_atexit_exec(cur);

	// gestion des thread_keys
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
			mdebug("  max iteration in key destructor for thread %i\n",cur->number);
#endif
	}
	
	cur->ret_val = val;
	//if(!cur->detached) {
	     //marcel_sem_V(&cur->client);
	     //marcel_sem_P(&cur->thread);
	//}

	ma_preempt_disable();
	vp = GET_LWP(cur)->vp_level;

#ifdef MA__LWPS
	/* Durant cette fonction, il ne faut pas que le thread soit
	   "d�plac�" intempestivement (e.g. apr�s avoir acquis
	   stack_mutex) sur un autre LWP. */
	mask = MARCEL_VPMASK_ALL_BUT_VP(vp->number);
	marcel_change_vpmask(&mask);
#endif
	ma_preempt_enable();

	// attendre que l'ordonnanceur soit pr�t
	marcel_sched_exit(MARCEL_SELF);

	// Ici, la pile a �t� allou�e par le noyau Marcel

	// Il faut acqu�rir le s�maphore pour postexit avant
	// de d�sactiver la pr�emption
	marcel_sem_P(&ma_topo_vpdata(vp,postexit_space));
	if (!cur->detached) {
		ma_topo_vpdata(vp,postexit_func)=detach_func;
		ma_topo_vpdata(vp,postexit_arg)=cur;
	} else {
		ma_topo_vpdata(vp,postexit_func)=cur->postexit_func;
		ma_topo_vpdata(vp,postexit_arg)=cur->postexit_arg;
	}

	ma_preempt_disable();

	// Il peut para�tre stupide de d�marrer le thread ex�cutant la
	// fonction postexit si t�t. Premi�rement, ce n'est pas grave
	// car d�sactiver la pr�emption garantit qu'aucun autre thread
	// (sur le LWP) ne sera ordonnanc�. Deuxi�mement, il _faut_
	// rel�cher ce verrou tr�s t�t (avant d'ex�cuter unchain_task)
	// car sinon la t�che idle risquerait d'�tre r�veill�e (par
	// unchain_task) alors que sem_V r�veillerait par ailleurs une
	// autre t�che du programme !
	marcel_sem_V(&ma_topo_vpdata(vp,postexit_thread)); /* idem ci-dessus */

	// M�me remarque que pr�c�demment : main_thread peut �tre
	// r�veill� � cet endroit, donc il ne faut appeler
	// unchain_task qu'apr�s.
	if (!(special_mode && MA_TASK_NOT_COUNTED_IN_RUNNING(cur))) {
		marcel_one_task_less(cur);
	}

	/* Changement d'�tat et appel au scheduler (d'o� on ne revient pas) */
	ma_set_current_state(MA_TASK_DEAD);
	ma_schedule();

	abort(); // For security
}
	
DEF_MARCEL_POSIX(void TBX_NORETURN, exit, (any_t val), (val),
{
        LOG_IN();
	if (marcel_self() == __main_thread) {
		marcel_end();
#ifdef STANDARD_MAIN
					 LOG_OUT();		
		exit(0);
#else
		__marcel_main_ret = 0;
		marcel_ctx_longjmp(__ma_initial_main_ctx, 1);
#endif
	} else
		marcel_exit_internal(val, 0);
	             LOG_OUT();
})
DEF_PTHREAD(void TBX_NORETURN, exit, (void *val), (val))

void TBX_NORETURN marcel_exit_special(any_t val)
{
	marcel_exit_internal(val, 1);
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

	if (pid == marcel_self()) {
		sp = (unsigned long) get_sp();
	} else {
		sp = (unsigned long) marcel_ctx_get_sp(pid->ctx_yield);
	}

	mdebug("thread %p :\n"
	    "\tlower bound : %p\n"
	    "\tupper bound : %lx\n"
	    "\tuser space : %p\n"
	    "\tinitial sp : %lx\n"
	    "\tcurrent sp : %lx\n",
	    pid,
	    pid->stack_base,
	    ma_task_slot_top(pid), pid->user_space_ptr, pid->initial_sp, sp);
}

void marcel_print_jmp_buf(char *name, jmp_buf buf)
{
	int i;

#ifdef IA64_ARCH
#  define SHOW_SP ""
#else
#  define SHOW_SP (((char **)buf + i) == (char **)&SP_FIELD(buf) ? "(sp)":"")
#endif
	for (i = 0; i < sizeof(jmp_buf) / sizeof(char *); i++) {
		mdebug("%s[%d] = %p %s\n",
		    name, i, ((char **) buf)[i], SHOW_SP);
	}
}

#endif

//PROF_NAME(thread_stack_allocated)
//PROF_NAME(newborn_thread)
//PROF_NAME(on_security_stack)

/************************join****************************/
static int TBX_UNUSED check_join(marcel_t tid, any_t *status)
{
	/* conflit entre ESRCH et EINVAL selon l'ordre */
	/* en effet ESRCH regarde aussi le champ detached */
	if (!MARCEL_THREAD_ISALIVE(tid)) {
		mdebug("check_join : thread %p dead\n", tid);
		return ESRCH;
	}

	if (MARCEL_THREAD_ISALIVE(tid)
	    && (tid->detached == MARCEL_CREATE_DETACHED)) {
		mdebug("check_join : thread %p detached\n", tid);
		return EINVAL;
	}
	return 0;
}

DEF_MARCEL(int, join, (marcel_t pid, any_t *status), (pid, status),
{
	LOG_IN();
#ifdef MA__DEBUG
	int err = check_join(pid, status);
	if (err)
		LOG_RETURN(err);;
#endif

	MA_BUG_ON(pid->detached);

	marcel_sem_P(&pid->client);
	if (status)
		*status = pid->ret_val;

	pid->detached = 1;

	/* Ex�cution de la fonction post_mortem */
	if (pid->postexit_func)
		(*pid->postexit_func) (pid->postexit_arg);

	/* Et lib�ration de la pile */
	if (!pid->static_stack)
		marcel_tls_slot_free(marcel_stackbase(pid));

	LOG_RETURN(0);
})

DEF_POSIX(int, join, (pmarcel_t ptid, any_t *status), (ptid, status),
{
	LOG_IN();
	marcel_t tid = (marcel_t) ptid;

	/* codes d'erreur */
	int err = check_join(tid, status);
	if (err)
		LOG_RETURN(err);

	LOG_RETURN(marcel_join(tid, status));
})
DEF_PTHREAD(int, join, (pthread_t tid, void **status), (tid, status))
DEF___PTHREAD(int, join, (pthread_t tid, void **status), (tid, status))

/*************pthread_cancel**********************************/

DEF_MARCEL(int, cancel, (marcel_t pid), (pid),
{
	LOG_IN();
	if (pid == marcel_self()) {
		LOG_OUT();
		marcel_exit(NULL);
	} else {
		pid->ret_val = NULL;
		mdebug("marcel %i kill %i\n", marcel_self()->number,
		    pid->number);
		marcel_deviate(pid, (handler_func_t) marcel_exit, NULL);
	}
	LOG_RETURN(0);
})

DEF_POSIX(int, cancel, (pmarcel_t ptid), (ptid),
{
	LOG_IN();
	marcel_t tid = (marcel_t) ptid;

	ma_spin_lock(&tid->cancellock);
	tid->canceled = MARCEL_IS_CANCELED;
	if ((tid->cancelstate == MARCEL_CANCEL_ENABLE)
	    && (tid->canceltype == MARCEL_CANCEL_ASYNCHRONOUS)) {

		if (tid == marcel_self()) {
			ma_spin_unlock(&tid->cancellock);
			LOG_OUT();
			marcel_exit(NULL);
		} else {
			tid->ret_val = NULL;
			mdebug("marcel %i kill %i\n", marcel_self()->number,
			    tid->number);
			marcel_deviate(tid, (handler_func_t) marcel_exit, NULL);
			ma_spin_unlock(&tid->cancellock);
		}
	} else
		ma_spin_unlock(&tid->cancellock);

	LOG_RETURN(0);
})

DEF_PTHREAD(int, cancel, (pthread_t pid), (pid))

/************************detach*********************/

static int TBX_UNUSED check_detach(marcel_t tid)
{
	/* conflit entre ESRCH et EINVAL selon l'ordre */
	/* en effet ESRCH regarde aussi le champ detached */
	if (!MARCEL_THREAD_ISALIVE(tid)) {
		mdebug("check_detach : thread %p dead\n", tid);
		return ESRCH;
	}

	if (MARCEL_THREAD_ISALIVE(tid)
	    && (tid->detached == MARCEL_CREATE_DETACHED)) {
		mdebug("check_detach : thread %p already detached\n", tid);
		return EINVAL;
	}
	return 0;
}

DEF_MARCEL(int, detach, (marcel_t pid), (pid),
{
	LOG_IN();
#ifdef MA__DEBUG
	int err = check_detach(pid);
	if (err)
		LOG_RETURN(err);
#endif

	pid->detached = MARCEL_CREATE_DETACHED;
	LOG_RETURN(0);
})

DEF_POSIX(int, detach, (pmarcel_t ptid), (ptid),
{
	marcel_t tid = (marcel_t) ptid;

	/* codes d'erreur */
	int err = check_detach(tid);
	if (err) {
		LOG_OUT();
		return err;
	}
	/* fin codes d'erreur */

	tid->detached = PMARCEL_CREATE_DETACHED;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, detach, (pthread_t pid), (pid))

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

#define __NO_WEAK_PTHREAD_ALIASES
#ifdef MA__LIBPTHREAD
#include <bits/libc-lock.h>
#endif
#undef NAME_PREFIX
#define NAME_PREFIX _
DEF_MARCEL_POSIX(void, cleanup_push,(struct _marcel_cleanup_buffer * __restrict __buffer,
				     cleanup_func_t func, any_t __restrict arg),
		(__buffer, func, arg),
{
	LOG_IN();
	marcel_t cur = marcel_self();
	__buffer->__routine = func;
	__buffer->__arg = arg;
	__buffer->__prev = cur->last_cleanup;
	cur->last_cleanup = __buffer;
	LOG_OUT();
})
DEF_PTHREAD(void, cleanup_push,(struct _pthread_cleanup_buffer *__buffer,
				     void (*__routine)(void *), void * __arg),
		(__buffer, __routine, __arg))

DEF_MARCEL_POSIX(void, cleanup_pop,(struct _marcel_cleanup_buffer *__buffer,
				tbx_bool_t execute), (__buffer, execute),
{
	LOG_IN();
	marcel_t cur = marcel_self();

	if (cur->last_cleanup != __buffer)
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);

	cur->last_cleanup = __buffer->__prev;
	if (execute)
		(*__buffer->__routine) (__buffer->__arg);
	LOG_OUT();
})
DEF_PTHREAD(void, cleanup_pop,(struct _pthread_cleanup_buffer *__buffer,
				     int __execute), (__buffer, __execute))
#undef NAME_PREFIX
#define NAME_PREFIX

void marcel_freeze(marcel_t * pids, int nb)
{
	int i;

	for (i = 0; i < nb; i++)
		if (pids[i] != marcel_self())
			ma_freeze_thread(pids[i]);
}

void marcel_unfreeze(marcel_t * pids, int nb)
{
	int i;

	for (i = 0; i < nb; i++)
		if (pids[i] != marcel_self())
			ma_unfreeze_thread(pids[i]);
}

/* WARNING!!! MUST BE LESS CONSTRAINED THAN MARCEL_ALIGN (64) */
#define ALIGNED_32(addr)(((unsigned long)(addr) + 31) & ~(31L))

// TODO : V�rifier le code avec les activations
void marcel_begin_hibernation(marcel_t __restrict t, transfert_func_t transf,
    void *__restrict arg, tbx_bool_t fork)
{
	unsigned long depl, blk;
	unsigned long bottom, top;
	marcel_t cur = marcel_self();

#ifdef MA__ACTIVATION
	MARCEL_EXCEPTION_RAISE("Not implemented");
#endif

	if (t == cur) {
		ma_preempt_disable();
		if (marcel_ctx_setjmp(cur->ctx_migr) == FIRST_RETURN) {

			call_ST_FLUSH_WINDOWS();
			top = ma_task_slot_top(cur);
			bottom =
			    ALIGNED_32((unsigned long) marcel_ctx_get_sp(cur->
				ctx_migr)) - ALIGNED_32(1);
			blk = top - bottom;
			depl = bottom - (unsigned long) cur->stack_base;

			mdebug("hibernation of thread %p", cur);
			mdebug("sp = %lu\n", get_sp());
			mdebug("sp_field = %lu\n",
			    (unsigned long) marcel_ctx_get_sp(cur->ctx_migr));
			mdebug("bottom = %lu\n", bottom);
			mdebug("top = %lu\n", top);
			mdebug("blk = %lu\n", blk);

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

		mdebug("hibernation of thread %p", t);
		mdebug("sp_field = %lu\n",
		    (unsigned long) marcel_ctx_get_sp(cur->ctx_migr));
		mdebug("bottom = %lu\n", bottom);
		mdebug("top = %lu\n", top);
		mdebug("blk = %lu\n", blk);

		(*transf) (t, depl, blk, arg);

		if (!fork) {
			marcel_cancel(t);
		}
	}
}

// TODO : V�rifier le code avec les activations
void marcel_end_hibernation(marcel_t __restrict t, post_migration_func_t f,
    void *__restrict arg)
{
#ifdef MA__ACTIVATION
	MARCEL_EXCEPTION_RAISE("Not implemented");
#endif

	memcpy(t->ctx_yield, t->ctx_migr, sizeof(marcel_ctx_t));

	mdebug("end of hibernation for thread %p", t);

	ma_preempt_disable();

	marcel_sched_init_marcel_thread(t, &marcel_attr_default);
	t->preempt_count = (2 * MA_PREEMPT_OFFSET) | MA_SOFTIRQ_OFFSET;
	marcel_one_more_task(t);
	ma_set_task_state(t, MA_TASK_INTERRUPTIBLE);
	if (t->remaining_sleep_time) {
		ma_mod_timer(&t->schedule_timeout_timer,
		    ma_jiffies + t->remaining_sleep_time);
	} else
		ma_wake_up_thread(t);

	if (f != NULL)
		marcel_deviate(t, f, arg);

	ma_preempt_enable();
}
static void __marcel_init main_thread_init(void)
{
	marcel_attr_t attr;
	char *name="main";

	LOG_IN();
	if (!__main_thread) {
		fprintf(stderr,"Couldn't find main thread's marcel_t, i.e. Marcel's main was not called yet. Please either load the stackalign module early, or use the STANDARD_MAIN option (but this will decrease performance)");
		abort();
	}
	memset(__main_thread, 0, sizeof(marcel_task_t));
	
#ifdef ENABLE_STACK_JUMPING
	*((marcel_t *)(ma_task_slot_top(__main_thread) - sizeof(void *))) = __main_thread;
#endif

	marcel_attr_init(&attr);
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, MARCEL_CREATE_JOINABLE);
	marcel_attr_setmigrationstate(&attr, tbx_false);
	marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_SHARED);
#ifdef MA__BUBBLES
	PROF_EVENT1_ALWAYS(bubble_sched_new,&marcel_root_bubble);
	marcel_attr_setinitbubble(&attr, &marcel_root_bubble);
#else
	marcel_attr_setinitrq(&attr, &ma_main_runqueue);
#endif

	ma_set_task_lwp(__main_thread,&__main_lwp);
	marcel_create_init_marcel_thread(__main_thread, &attr);
	__main_thread->initial_sp = get_sp();
#if defined(STANDARD_MAIN)
	/* the stack is not in a slot */
	__main_thread->stack_base = NULL;
#else
	__main_thread->stack_base = (any_t)(get_sp() & ~(THREAD_SLOT_SIZE-1));
#endif
	
	ma_preempt_count()=0;
	ma_irq_enter();
	ma_set_current_state(MA_TASK_BORNING);

#ifdef PM2DEBUG
	pm2debug_printf_state(PM2DEBUG_MARCEL_PRINTF_ALLOWED);
#endif
	PROF_SET_THREAD_NAME(__main_thread);
	LOG_OUT();
}

__ma_initfunc(main_thread_init, MA_INIT_THREADS_MAIN, "Initialise the main thread structure");

#ifdef STANDARD_MAIN
marcel_task_t __main_thread_struct;
#else
marcel_t __main_thread;
#endif


/*************************set/getconcurrency**********************/
/*The implementation shall use this as a hint, not a requirement.*/
static int concurrency = 0;
DEF_MARCEL_POSIX(int, setconcurrency,(int new_level),(new_level),
{
	LOG_IN();
	if (new_level < 0) {
#ifdef MA__DEBUG
		mdebug("(p)marcel_setconcurrency : invalid newlevel (%d)",
		    new_level);
#endif
		LOG_RETURN(EINVAL);
	}

	if (!new_level) {
		LOG_RETURN(0);
	}
	concurrency = new_level;
	LOG_RETURN(0);
})

DEF_PTHREAD(int,setconcurrency,(int new_level),(new_level))
DEF___PTHREAD(int,setconcurrency,(int new_level),(new_level))

DEF_MARCEL_POSIX(int, getconcurrency,(void),(),
{
	LOG_IN();
	LOG_OUT();
	return concurrency;
})

DEF_PTHREAD(int,getconcurrency,(void),())
DEF___PTHREAD(int,getconcurrency,(void),())

/***************************setcancelstate************************/
static int TBX_UNUSED check_setcancelstate(int state, int *oldstate)
{
	if ((state != MARCEL_CANCEL_ENABLE) && (state != MARCEL_CANCEL_DISABLE)) {
		mdebug
		    ("pmarcel_setcancelstate : valeur state (%d) invalide !\n",
		    state);
		return EINVAL;
	}
	return 0;
}

DEF_POSIX(int, setcancelstate,(int state, int *oldstate),(state, oldstate),
{
	LOG_IN();
	marcel_t cthread = marcel_self();

	/* error checking */
	int ret = check_setcancelstate(state, oldstate);
	if (ret)
		LOG_RETURN(ret);
	/* error checking end */

	ma_spin_lock(&cthread->cancellock);
	if (oldstate)
		*oldstate = cthread->cancelstate;
	cthread->cancelstate = state;
	ma_spin_unlock(&cthread->cancellock);

	LOG_RETURN(0);
})

DEF_PTHREAD(int, setcancelstate, (int state,int *oldstate), (state,oldstate))
DEF___PTHREAD(int, setcancelstate, (int state,int *oldstate), (state,oldstate))

/****************************setcanceltype************************/
static int TBX_UNUSED check_setcanceltype(int type, int *oldtype)
{
	if ((type != MARCEL_CANCEL_ASYNCHRONOUS)
	    && (type != MARCEL_CANCEL_DEFERRED)) {
		mdebug("pmarcel_setcanceltype : valeur type invalide : %d !\n",
		    type);
		return EINVAL;
	}
	return 0;
}

DEF_POSIX(int, setcanceltype,(int type, int *oldtype),(type, oldtype),
{
	LOG_IN();
	marcel_t cthread;
	cthread = marcel_self();

	/* error checking */
	int ret = check_setcanceltype(type, oldtype);
	if (ret)
		LOG_RETURN(ret);
	/* error checking end */

	ma_spin_lock(&cthread->cancellock);
	if (oldtype)
		*oldtype = cthread->canceltype;
	cthread->canceltype = type;
	ma_spin_unlock(&cthread->cancellock);

	LOG_RETURN(0);
})

DEF_PTHREAD(int, setcanceltype, (int type,int *oldtype), (type,oldtype))
DEF___PTHREAD(int, setcanceltype, (int type,int *oldtype), (type,oldtype))

/*****************************testcancel***************************/
//TODO : test_cancel shall create a cancellation point in the calling
//       thread. The pthread_testcancel() function shall have no effect 
//       if cancelability is disabled.

DEF_POSIX(void, testcancel,(void),(),
{
	LOG_IN();
	marcel_t cthread = marcel_self();
	ma_spin_lock(&cthread->cancellock);
	if ((cthread->cancelstate == MARCEL_CANCEL_ENABLE)
	    && (cthread->canceled == MARCEL_IS_CANCELED)) {
		ma_spin_unlock(&cthread->cancellock);
		cthread->ret_val = NULL;
		mdebug("thread %i s'annule et exit\n", cthread->number);
		LOG_OUT();
		marcel_exit(NULL);
	} else
		ma_spin_unlock(&cthread->cancellock);

	LOG_OUT();
})
 
DEF_PTHREAD(void, testcancel,(void),())
DEF___PTHREAD(void, testcancel,(void),())


#ifdef MA__IFACE_PMARCEL
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
strong_alias(__pmarcel_enable_asynccancel, __pthread_enable_asynccancel);
strong_alias(__pmarcel_disable_asynccancel, __pthread_disable_asynccancel);
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
			mdebug("setprio_posix2marcel : priority %d invalide\n",
			    prio);
			LOG_RETURN(EINVAL);
		}
	} else if (policy == SCHED_OTHER) {
		if (prio == 0)
			ma_sched_change_prio(thread, MA_DEF_PRIO);
		else {
			mdebug("setprio_posix2marcel : priority %d invalide\n",
			    prio);
			LOG_RETURN(EINVAL);
		}
	} else
		mdebug("setprio_posix2marcel : policy %d invalide\n", policy);
	/* on est pass� de prio posix a prio marcel */

	LOG_RETURN(0);
}

/*************************setschedprio************************/
DEF_MARCEL_POSIX(int, setschedprio,(marcel_t thread,int prio),(thread,prio),
{
	LOG_IN();
	int policy;

	/* d�termination de la police POSIX d'apr�s la priorit� Marcel et la pr�emption */
	int mprio = thread->sched.internal.entity.prio;
	if (mprio >= MA_DEF_PRIO)
		policy = SCHED_OTHER;
	else if (mprio <= MA_RT_PRIO) {
		if (marcel_some_thread_is_preemption_disabled(thread))
			policy = SCHED_FIFO;
		else
			policy = SCHED_RR;
	}
	/* policy d�termin�e */

	int ret = setprio_posix2marcel(thread, prio, policy);
	if (ret)
		LOG_RETURN(ret);

	LOG_RETURN(0);
})

DEF_PTHREAD(int, setschedprio, (pthread_t thread, int prio),(thread, prio))
DEF___PTHREAD(int, setschedprio, (pthread_t thread, int prio),(thread, prio))

/*************************setschedparam****************************/
DEF_MARCEL(int, setschedparam,(marcel_t thread, int policy, 
                 const struct marcel_sched_param *__restrict param), (thread,policy,param),
{
	LOG_IN();

	if (param == NULL) {
		mdebug("marcel_setschedparam : valeur attr ou param NULL\n");
		LOG_RETURN(EINVAL);
	}
	if ((policy != SCHED_FIFO) && (policy != SCHED_RR)
	    && (policy != SCHED_OTHER)) {
		mdebug("marcel_setschedparam : valeur policy(%d) invalide\n",
		    policy);
		LOG_RETURN(EINVAL);
	}

	marcel_sched_setscheduler(thread, policy, param);

	LOG_RETURN(0);
})

DEF_POSIX(int, setschedparam,(pmarcel_t pmthread, int policy, 
                 const struct marcel_sched_param *__restrict param), (pmthread,policy,param),
{
	LOG_IN();
	marcel_t thread = (marcel_t) pmthread;

	if (param == NULL) {
		mdebug("pmarcel_setschedparam : valeur attr ou param NULL\n");
		LOG_RETURN(EINVAL);
	}

	if ((policy != SCHED_FIFO) && (policy != SCHED_RR)
	    && (policy != SCHED_OTHER)) {
		mdebug("pmarcel_setschedparam : valeur policy(%d) invalide\n",
		    policy);
		LOG_RETURN(EINVAL);
	}

	/* (d�s)activation de la pr�emption en fonction de SCHED_FIFO */
	if (policy == SCHED_FIFO) {
		if (!marcel_some_thread_is_preemption_disabled(thread))
			marcel_some_thread_preemption_disable(thread);
	} else {
		if (marcel_some_thread_is_preemption_disabled(thread))
			marcel_some_thread_preemption_enable(thread);
	}

	int ret = setprio_posix2marcel(thread, param->__sched_priority, policy);
	if (ret)
		LOG_RETURN(ret);

	LOG_RETURN(0);
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
		mdebug("pmarcel_getschedparam : valeur policy ou param NULL\n");
		LOG_RETURN(EINVAL);
	}

	*policy = marcel_sched_getscheduler(thread);
	marcel_sched_getparam(thread, param);

	LOG_RETURN(0);
})

DEF_POSIX(int, getschedparam,(pmarcel_t pmthread, int *__restrict policy,
                                     struct marcel_sched_param *__restrict param),(pmthread,policy,param),
{
	LOG_IN();
	marcel_t thread = (marcel_t) pmthread;

	if ((param == NULL) || (policy == NULL)) {
		mdebug("pmarcel_getschedparam : valeur policy ou param NULL\n");
		LOG_RETURN(EINVAL);
	}

	/* d�termination de la police et priority POSIX d'apr�s la priorit� Marcel et la pr�emption */
	int mprio = thread->sched.internal.entity.prio;

	if (mprio >= MA_DEF_PRIO) {
		param->__sched_priority = 0;
		*policy = SCHED_OTHER;
	} else if (mprio <= MA_RT_PRIO) {
		param->__sched_priority = MA_RT_PRIO - mprio;
		if (marcel_some_thread_is_preemption_disabled(thread))
			*policy = SCHED_FIFO;
		else
			*policy = SCHED_RR;
	}
	/* policy et priority POSIX d�termin�es */

	LOG_RETURN(0);
})

DEF_PTHREAD(int, getschedparam, (pthread_t thread, int *__restrict policy, 
   struct sched_param *__restrict param), (thread, policy, param))
DEF___PTHREAD(int, getschedparam, (pthread_t thread, int *__restrict policy, 
   struct sched_param *__restrict param), (thread, policy, param))

/**********************getcpuclockid****************************/
DEF_POSIX(int,getcpuclockid,(pmarcel_t thread_id, clockid_t *clock_id),(thread_id,clock_id),
{
	LOG_IN();
	clock_getcpuclockid(0, clock_id);
	LOG_RETURN(0);
})

DEF_PTHREAD(int,getcpuclockid,(pthread_t thread_id, clockid_t *clock_id),(thread_id,clock_id));
DEF___PTHREAD(int,getcpuclockid,(pthread_t thread_id, clockid_t *clock_id),(thread_id,clock_id));

/* TODO : several functions may fail if: [ESRCH]
No thread could be found corresponding to that specified by the given thread ID.pthread_detach, pthread_getschedparam, pthread_join, pthread_kill, pthread_cancel, pthread_setschedparam, pthread_setschedprio */
