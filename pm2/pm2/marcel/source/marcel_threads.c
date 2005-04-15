
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

inline static 
void marcel_postexit_internal(marcel_t cur,
			      marcel_postexit_func_t func, any_t arg);

/****************************************************************/
/****************************************************************
 *                Création des threads
 */
/****************************************************************/

/****************************************************************
 *                Initialisation des structures
 */
static __inline__ void init_marcel_thread(marcel_t t, 
					  __const marcel_attr_t *attr)
{
	/* Free within schedule_tail */
	t->preempt_count=MA_PREEMPT_OFFSET|MA_SOFTIRQ_OFFSET;
	marcel_sched_init_marcel_thread(t, attr);
	t->not_preemptible=attr->not_preemptible;
	//t->ctx_yield
	t->work = MARCEL_WORK_INIT;
	t->softirq_pending_in_hardirq=0;
	//t->ctx_migr
	t->flags = attr->flags;
	//t->child
	//t->father
	//t->f_to_call
	//t->real_f_to_call
	//t->arg
	//t->sem_marcel_run
	if(attr->user_space)
		t->user_space_ptr = 
			(char *)t - MAL(attr->user_space);
	else
		t->user_space_ptr = NULL;

	t->detached = (attr->__detachstate == MARCEL_CREATE_DETACHED);
	if(!attr->__detachstate) {
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

	//t->stack_base
	//t->static_stack
	//t->initial_sp
	//t->depl
	marcel_attr_getname(attr,t->name,MARCEL_MAXNAMESIZE);
	//t->number
	t->timer = NULL;
	t->not_migratable = attr->not_migratable;
	t->not_deviatable = attr->not_deviatable;
	marcel_sem_init(&t->suspend_sem, 0);

	t->__errno=0;
	t->__h_errno=0;
	//t->__res_state
	t->p_readlock_list=NULL;
	t->p_readlock_free=NULL;
	t->p_untracked_readlock_count=0;
	t->p_nextwaiting=NULL;
	marcel_sem_init(&t->pthread_sync, 0);  

	
#ifdef ENABLE_STACK_JUMPING
	*((marcel_t *)((char *)t + MAL(sizeof(marcel_task_t))
		       - sizeof(void *))) = t;
#endif
	
}

void marcel_create_init_marcel_thread(marcel_t t, 
				      __const marcel_attr_t *attr)
{
	return init_marcel_thread(t, attr);
}

/****************************************************************
 *                Démarrage retardé
 */
void marcel_getuserspace(marcel_t pid, void **user_space)
{
	*user_space = pid->user_space_ptr;
}

void marcel_run(marcel_t pid, any_t arg)
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
 *                Création d'un thread
 */

/* special_mode est à 0 normalement. Pour les cas spéciaux (threads
 * internes, threads idle, ...), il est à 1.
 *
 * Il est calculé directement à la compilation par les deux fonctions
 * suivantes qui ne sont qu'un 'inline' de celle-ci
 */
inline static int 
marcel_create_internal(marcel_t *pid, __const marcel_attr_t *attr, 
		       marcel_func_t func, any_t arg, 
		       __const int special_mode, 
		       __const unsigned long base_stack)
{
	marcel_t cur = marcel_self(), new_task;
#ifdef MA__POSIX_BEHAVIOUR
	marcel_attr_t myattr;
#endif
	void* stack_base;
	int static_stack;
	
	LOG_IN();

	TIMING_EVENT("marcel_create");
	
	if(!attr) {
#ifdef MA__POSIX_BEHAVIOUR
		marcel_attr_init(&myattr);
		attr=&myattr;
#else
		attr = &marcel_attr_default;
#endif
	}

	if(attr->__stackaddr_set) {
		register unsigned long top = MAL_BOT((unsigned long)attr->__stackaddr +
						     attr->__stacksize);
#ifdef MA__DEBUG
		mdebug("top=%lx, stack_base=%p\n", top, attr->__stackaddr);
		if(top & (THREAD_SLOT_SIZE-1)) { /* Not slot-aligned */
			RAISE(CONSTRAINT_ERROR);
		}
#endif
		new_task = (marcel_t)(top - MAL(sizeof(marcel_task_t)));
#ifdef STACK_CHECKING_ALLOWED
		memset(attr->__stackaddr, 0, attr->__stacksize);
#endif
		stack_base = attr->__stackaddr;
		
		static_stack = TRUE;
	} else { /* (!attr->stack_base) */
		char *bottom;

#ifdef MA__DEBUG
		if(attr->__stacksize > THREAD_SLOT_SIZE)
			RAISE(NOT_IMPLEMENTED);
#endif
		bottom = marcel_slot_alloc();
		
		PROF_EVENT(thread_stack_allocated);
		
		new_task= (marcel_t)(MAL_BOT((unsigned long)bottom + THREAD_SLOT_SIZE) -
				      MAL(sizeof(marcel_task_t)));

		stack_base = bottom;
		static_stack = FALSE;
	} /* fin (attr->stack_base) */

	init_marcel_thread(new_task, attr);
	new_task->stack_base = stack_base;
	new_task->static_stack = static_stack;
	
	if (!attr->__stackaddr_set) {
		marcel_postexit_internal(new_task, marcel_slot_free, 
					 marcel_stackbase(new_task));
	}
	
	new_task->father = cur;
	if (new_task->user_space_ptr && !attr->immediate_activation) {
		/* Le thread devra attendre marcel_run */
		new_task->f_to_call = &wait_marcel_run;
		new_task->real_f_to_call = func;
		marcel_sem_init(&new_task->sem_marcel_run, 0);
	} else {
		new_task->f_to_call = func;
	}
	new_task->arg = arg;
	
	new_task->initial_sp = (unsigned long)new_task - MAL(attr->user_space) -
		TOP_STACK_FREE_AREA;
	
	if(pid)
		*pid = new_task;
	
	if (!(special_mode && MA_TASK_NOT_COUNTED_IN_RUNNING(new_task))) {
		marcel_one_more_task(new_task);
	} else {
		static int norun_pid=0;
		int norun_pid_new;
		do {
			norun_pid_new=norun_pid;
		} while ( norun_pid_new != ma_cmpxchg(&norun_pid, norun_pid_new, norun_pid_new-1));
		new_task->number= norun_pid_new-1;
	}
	MTRACE("Creation", new_task);
	
	PROF_THREAD_BIRTH(new_task);
	
	PROF_IN_EXT(newborn_thread);

	/* Le nouveau thread est démarré par le scheduler choisi */
	/* Seul le père revient... */
	marcel_sched_create(cur, new_task, attr, special_mode, base_stack);

	if(cur->child)
		/* pour les processus normaux, on réveille le fils nous-même */
		ma_wake_up_created_thread(cur->child);

	LOG_OUT();
	return 0;
	
}

int marcel_create(marcel_t *pid, __const marcel_attr_t *attr,
		  marcel_func_t func, any_t arg)
{
	return marcel_create_internal(pid, attr, func, arg, 
				      0, (unsigned long)&arg);
}

int marcel_create_special(marcel_t *pid, __const marcel_attr_t *attr,
			  marcel_func_t func, any_t arg)
{
	return marcel_create_internal(pid, attr, func, arg, 
				      1, (unsigned long)&arg);
}

/****************************************************************/
/****************************************************************
 *                Terminaison des threads
 */
/****************************************************************/

/****************************************************************
 *                Thread helper
 */

MA_DEFINE_PER_LWP(marcel_postexit_func_t, postexit_func, NULL);
MA_DEFINE_PER_LWP(any_t, postexit_arg, NULL);
MA_DEFINE_PER_LWP(marcel_sem_t, postexit_thread, {0});
MA_DEFINE_PER_LWP(marcel_sem_t, postexit_space, {0});

static void postexit_thread_atexit_func(any_t arg) {

	marcel_lwp_t *lwp TBX_UNUSED =(marcel_lwp_t*)arg;

	TRACE("postexit thread killed on lwp %i\n", LWP_NUMBER(LWP_SELF));
	RAISE(PROGRAM_ERROR);
}

static void* postexit_thread_func(any_t arg)
{
	marcel_lwp_t *lwp=(marcel_lwp_t*)arg;

	LOG_IN();
	MTRACE("Start Postexit", marcel_self());
	marcel_atexit(postexit_thread_atexit_func, lwp);
	for(;;) {
		marcel_sem_P(&ma_per_lwp(postexit_thread, lwp));
		MTRACE("Postexit", marcel_self());
		if (!ma_per_lwp(postexit_func, lwp)) {
			mdebug("postexit with NULL function!\n"
				"Who will desalocate the stack!!!\n");
		} else {
			(*ma_per_lwp(postexit_func,lwp))
				(ma_per_lwp(postexit_arg,lwp));
			ma_per_lwp(postexit_func,lwp)=NULL;
		}
		marcel_sem_V(&ma_per_lwp(postexit_space,lwp));
	}
	LOG_OUT();
	abort(); /* For security */
}

void marcel_threads_postexit_init(marcel_lwp_t *lwp)
{
	marcel_sem_init(&ma_per_lwp(postexit_thread, lwp), 0);
	marcel_sem_init(&ma_per_lwp(postexit_space, lwp), 1);
}

void marcel_threads_postexit_start(marcel_lwp_t *lwp)
{
	marcel_attr_t attr;
	marcel_t postexit;
	char name[MARCEL_MAXNAMESIZE];

	LOG_IN();
	/* Démarrage du thread responsable des terminaisons */
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"postexit/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, TRUE);
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_NORUN);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);
		
		unsigned long stsize = (((unsigned long)(stack + 2*THREAD_SLOT_SIZE) & 
					 ~(THREAD_SLOT_SIZE-1)) - (unsigned long)stack);
		
		marcel_attr_setstackaddr(&attr, stack);
		marcel_attr_setstacksize(&attr, stsize);
	}
#endif
	marcel_create_special(&postexit, &attr, postexit_thread_func, lwp);
	ma_wake_up_created_thread(postexit);
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
inline static 
void marcel_postexit_internal(marcel_t cur,
			      marcel_postexit_func_t func, any_t arg)
{
	LOG_IN();
	if (cur->postexit_func) {
		RAISE(CONSTRAINT_ERROR);
	}
	cur->postexit_func=func;
	cur->postexit_arg=arg;
	LOG_OUT();
}

void marcel_postexit(marcel_postexit_func_t func, any_t arg)
{
	return marcel_postexit_internal(marcel_self(), func, arg);
}

void marcel_atexit(marcel_atexit_func_t func, any_t arg)
{
	marcel_t cur = marcel_self();
	
	LOG_IN();

	if(cur->next_atexit_func == MAX_ATEXIT_FUNCS)
		RAISE(CONSTRAINT_ERROR);
	
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
	DEFINE_CUR_LWP(register, , );
	
	LOG_IN();

	// On appelle marcel_atexit et marcel_cleanup sur la pile
	// courante. Les fonctions atexit ne doivent pas détruire la
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
			mdebug("  max iteration in key destructor for thread %li\n",cur->number);
#endif
	}
	
	cur->ret_val = val;
	//if(!cur->detached) {
	//	marcel_sem_V(&cur->client);
	//	marcel_sem_P(&cur->thread);
	//}

	ma_preempt_disable();
	SET_CUR_LWP(GET_LWP(cur));

#ifdef MA__LWPS
	/* Durant cette fonction, il ne faut pas que le thread soit
	   "déplacé" intempestivement (e.g. après avoir acquis
	   stack_mutex) sur un autre LWP. */
	marcel_change_vpmask(MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(cur_lwp)));
#endif
	ma_preempt_enable();

	// Ici, la pile a été allouée par le noyau Marcel

	// Il faut acquérir le sémaphore pour postexit avant
	// d'exécuter lock_task.
	marcel_sem_P(&ma_per_lwp(postexit_space,cur_lwp));
	if (!cur->detached) {
		ma_per_lwp(postexit_func,cur_lwp)=detach_func;
		ma_per_lwp(postexit_arg,cur_lwp)=cur;
	} else {
		ma_per_lwp(postexit_func,cur_lwp)=cur->postexit_func;
		ma_per_lwp(postexit_arg,cur_lwp)=cur->postexit_arg;
	}

	ma_preempt_disable();

	// Il peut paraître stupide de démarrer le thread exécutant la
	// fonction postexit si tôt. Premièrement, ce n'est pas grave
	// car lock_task garanti qu'aucun autre thread (sur le LWP) ne
	// sera ordonnancé. Deuxièmement, il _faut_ relâcher ce verrou
	// très tôt (avant d'exécuter unchain_task) car sinon la tâche
	// idle risquerait d'être réveillée (par unchain_task) alors
	// que sem_V réveillerait par ailleurs une autre tâche du
	// programme !
	marcel_sem_V(&ma_per_lwp(postexit_thread, cur_lwp)); /* idem ci-dessus */

	// Même remarque que précédemment : main_thread peut être
	// réveillé à cet endroit, donc il ne faut appeler
	// unchain_task qu'après.
	if (!(special_mode && MA_TASK_NOT_COUNTED_IN_RUNNING(cur))) {
		marcel_one_task_less(cur);
	}

	/* Changement d'état et appel au scheduler (d'où on ne revient pas) */
	ma_set_current_state(MA_TASK_DEAD);
	ma_schedule();

	abort(); // For security
}
	
DEF_MARCEL_POSIX(void TBX_NORETURN, exit, (any_t val), (val))
{
	marcel_exit_internal(val, 0);
}
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

void print_thread(marcel_t pid)
{
  long sp;

   if(pid == marcel_self()) {
     sp = (unsigned long)get_sp();
   } else {
     sp = (unsigned long)marcel_ctx_get_sp(pid->ctx_yield);
   }

  mdebug("thread %p :\n"
	 "\tlower bound : %p\n"
	 "\tupper bound : %lx\n"
	 "\tuser space : %p\n"
	 "\tinitial sp : %lx\n"
	 "\tcurrent sp : %lx\n"
	 ,
	 pid,
	 pid->stack_base,
	 (unsigned long)pid + MAL(sizeof(marcel_task_t)),
	 pid->user_space_ptr,
	 pid->initial_sp,
	 sp
	 );
}

void print_jmp_buf(char *name, jmp_buf buf)
{
  int i;

#ifdef IA64_ARCH
#  define SHOW_SP ""
#else
#  define SHOW_SP (((char **)buf + i) == (char **)&SP_FIELD(buf) ? "(sp)":"")
#endif
  for(i=0; i<sizeof(jmp_buf)/sizeof(char *); i++) {
    mdebug("%s[%d] = %p %s\n",
	   name,
	   i,
	   ((char **)buf)[i],
	   SHOW_SP
	  );
  }
}

#endif

//PROF_NAME(thread_stack_allocated)
//PROF_NAME(newborn_thread)
//PROF_NAME(on_security_stack)

inline static 
void marcel_postexit_internal(marcel_t cur,
			      marcel_postexit_func_t func, any_t arg);

DEF_MARCEL_POSIX(int, join, (marcel_t pid, any_t *status), (pid, status))
{
	LOG_IN();

#ifdef MA__DEBUG
	if(pid->detached)
		RAISE(PROGRAM_ERROR);
#endif

	marcel_sem_P(&pid->client);
	if(status)
		*status = pid->ret_val;
	
	/* Exécution de la fonction post_mortem */
	(*pid->postexit_func)(pid->postexit_arg);
/* 	{  */
/* 		DEFINE_CUR_LWP(register, =, GET_LWP(pid)); */
/* 		marcel_sem_P(&cur_lwp->postexit_space); */
/* 		cur_lwp->postexit_func=pid->postexit_func; */
/* 		cur_lwp->postexit_arg=pid->postexit_arg; */
/* 		marcel_sem_V(&cur_lwp->postexit_thread); */
/* 	} */
	LOG_OUT();
	return 0;
}
DEF_PTHREAD(int, join, (pthread_t pid, void **status), (pid, status))


DEF_MARCEL_POSIX(int, cancel, (marcel_t pid), (pid))
{
  if(pid == marcel_self()) {
    marcel_exit(NULL);
  } else {
    pid->ret_val = NULL;
    mdebug("marcel %li kill %li\n", marcel_self()->number, pid->number);
    marcel_deviate(pid, (handler_func_t)marcel_exit, NULL);
  }
  return 0;
}
DEF_PTHREAD(int, cancel, (pthread_t pid), (pid))

DEF_MARCEL_POSIX(int, detach, (marcel_t pid), (pid))
{
   pid->detached = TRUE;
   return 0;
}
DEF_PTHREAD(int, detach, (pthread_t pid), (pid))

static void suspend_handler(any_t arg)
{
  if((unsigned long)arg) {
    // Suspend
    marcel_sem_P(&(marcel_self()->suspend_sem));
  } else {
    // Resume
    marcel_sem_V(&(marcel_self()->suspend_sem));
  }
}

void marcel_suspend(marcel_t pid)
{
  marcel_deviate(pid, suspend_handler, (any_t)1);
}

void marcel_resume(marcel_t pid)
{
  marcel_deviate(pid, suspend_handler, (any_t)0);
}

#undef NAME_PREFIX
#define NAME_PREFIX _
DEF_MARCEL_POSIX(void, cleanup_push,(struct _marcel_cleanup_buffer *__buffer,
				     cleanup_func_t func, any_t arg),
		(__buffer, func, arg))
{
	marcel_t cur = marcel_self();
	__buffer->__routine=func;
	__buffer->__arg=arg;
	__buffer->__prev=cur->last_cleanup;
	cur->last_cleanup=__buffer;
}
DEF_PTHREAD(void, cleanup_push,(struct _pthread_cleanup_buffer *__buffer,
				     void (*__routine)(void *), void * __arg),
		(__buffer, __routine, __arg))

DEF_MARCEL_POSIX(void, cleanup_pop,(struct _marcel_cleanup_buffer *__buffer,
				    boolean execute), (__buffer, execute))
{
	marcel_t cur = marcel_self();
	
	if(cur->last_cleanup != __buffer)
		RAISE(PROGRAM_ERROR);

	cur->last_cleanup=__buffer->__prev;
	if(execute)
		(*__buffer->__routine)(__buffer->__arg);
}
DEF_PTHREAD(void, cleanup_pop,(struct _pthread_cleanup_buffer *__buffer,
				     int __execute), (__buffer, __execute))
#undef NAME_PREFIX
#define NAME_PREFIX

void marcel_freeze(marcel_t *pids, int nb)
{
  int i;

   for(i=0; i<nb; i++)
     if(pids[i] != marcel_self())
        ma_freeze_thread(pids[i]);
}

void marcel_unfreeze(marcel_t *pids, int nb)
{
  int i;

   for(i=0; i<nb; i++)
      if(pids[i] != marcel_self())
        ma_unfreeze_thread(pids[i]);
}

/* WARNING!!! MUST BE LESS CONSTRAINED THAN MARCEL_ALIGN (64) */
#define ALIGNED_32(addr)(((unsigned long)(addr) + 31) & ~(31L))

// TODO : Vérifier le code avec les activations
void marcel_begin_hibernation(marcel_t t, transfert_func_t transf, 
			      void *arg, boolean fork)
{
  unsigned long depl, blk;
  unsigned long bottom, top;
  marcel_t cur = marcel_self();

#ifdef MA__ACTIVATION
  RAISE("Not implemented");
#endif

  if(t == cur) {
    lock_task();
    if(marcel_ctx_setjmp(cur->ctx_migr) == FIRST_RETURN) {

      call_ST_FLUSH_WINDOWS();
      top = (unsigned long)cur + ALIGNED_32(sizeof(marcel_task_t));
      bottom = ALIGNED_32((unsigned long)marcel_ctx_get_sp(cur->ctx_migr)) - ALIGNED_32(1);
      blk = top - bottom;
      depl = bottom - (unsigned long)cur->stack_base;

      mdebug("hibernation of thread %p", cur);
      mdebug("sp = %lu\n", get_sp());
      mdebug("sp_field = %lu\n", (unsigned long)marcel_ctx_get_sp(cur->ctx_migr));
      mdebug("bottom = %lu\n", bottom);
      mdebug("top = %lu\n", top);
      mdebug("blk = %lu\n", blk);

      (*transf)(cur, depl, blk, arg);

      if(!fork)
	marcel_exit(NULL);
    } else {
#ifdef MA__DEBUG
      breakpoint();
#endif
      MA_THR_RESTARTED(MARCEL_SELF,"End of hibernation");
      ma_schedule_tail(MARCEL_SELF);
      unlock_task();
    }
  } else {
    memcpy(t->ctx_migr, t->ctx_yield, sizeof(marcel_ctx_t));
    if (t->timer) {
	    ma_del_timer_sync(t->timer);
	    t->remaining_sleep_time = t->timer->expires - ma_jiffies;
    }

    top = (unsigned long)t + ALIGNED_32(sizeof(marcel_task_t));
    bottom = ALIGNED_32((unsigned long)marcel_ctx_get_sp(t->ctx_yield)) - ALIGNED_32(1);
    blk = top - bottom;
    depl = bottom - (unsigned long)t->stack_base;

    mdebug("hibernation of thread %p", t);
    mdebug("sp_field = %lu\n", (unsigned long)marcel_ctx_get_sp(cur->ctx_migr));
    mdebug("bottom = %lu\n", bottom);
    mdebug("top = %lu\n", top);
    mdebug("blk = %lu\n", blk);

    (*transf)(t, depl, blk, arg);

    if(!fork) {
      marcel_cancel(t);
    }
  }
}

// TODO : Vérifier le code avec les activations
void marcel_end_hibernation(marcel_t t, post_migration_func_t f, void *arg)
{
#ifdef MA__ACTIVATION
  RAISE("Not implemented");
#endif

  memcpy(t->ctx_yield, t->ctx_migr, sizeof(marcel_ctx_t));

  mdebug("end of hibernation for thread %p", t);

  lock_task();

  marcel_sched_init_marcel_thread(t, &marcel_attr_default);
  t->preempt_count=(2*MA_PREEMPT_OFFSET)|MA_SOFTIRQ_OFFSET;
  marcel_one_more_task(t);
  if (t->timer) {
	t->timer->expires = ma_jiffies + t->remaining_sleep_time;
	/* TODO: on a perdu cet état. Ça ne devrait pas être écrasé par freeze */
	ma_set_task_state(t, MA_TASK_INTERRUPTIBLE);
	ma_add_timer(t->timer);
  } else
  	ma_wake_up_thread(t);

  if(f != NULL)
    marcel_deviate(t, f, arg);

  unlock_task();
}

void __marcel_init main_thread_init(void)
{
	marcel_attr_t attr;
	char *name="main";

	LOG_IN();
	memset(__main_thread, 0, sizeof(marcel_task_t));
	
	marcel_attr_init(&attr);
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, MARCEL_CREATE_JOINABLE);
	marcel_attr_setmigrationstate(&attr, FALSE);
#if 0
TODO: vieux code
#ifndef MA__ONE_QUEUE
	// Limitation (actuelle) du mode 'SMP multi-files' : le thread
	// 'main' doit toujours rester sur le LWP 0
	marcel_attr_setvpmask(MARCEL_VPMASK_ALL_BUT_VP(0));
#endif
#endif
	ma_set_task_lwp(__main_thread,&__main_lwp);
	marcel_create_init_marcel_thread(__main_thread, &attr);
	__main_thread->initial_sp = get_sp();
	
	ma_preempt_count()=0;
	ma_irq_enter();
	ma_set_current_state(MA_TASK_RUNNING);

#ifdef PM2DEBUG
	pm2debug_printf_state(PM2DEBUG_MARCEL_PRINTF_ALLOWED);
#endif
	PROF_SET_THREAD_NAME();
	LOG_OUT();
}

__ma_initfunc(main_thread_init, MA_INIT_THREADS_MAIN,
	      "Initialise the main thread structure");

#ifdef STANDARD_MAIN
marcel_task_t __main_thread_struct;
#else
marcel_t __main_thread;
#endif


