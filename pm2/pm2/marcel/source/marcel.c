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

#include <marcel.h>
#include <mar_timing.h>
#include <safe_malloc.h>
#include <marcel_alloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <signal.h>

#ifdef UNICOS_SYS
#include <sys/mman.h>
#endif

#ifdef SOLARIS_SYS
#include <sys/stack.h>
#endif

#ifdef RS6K_ARCH
int _jmg(int r)
{
  if(r!=0) 
    unlock_task();
  return r;
}

#undef longjmp
void LONGJMP(jmp_buf buf, int val)
{
  static jmp_buf _buf;
  static int _val;

  lock_task();
  memcpy(_buf, buf, sizeof(jmp_buf));
  _val = val;
  set_sp(SP_FIELD(buf)-256);
  longjmp(_buf, _val);
}
#define longjmp(buf, v)	LONGJMP(buf, v)
#endif


#ifdef STACK_OVERFLOW_DETECT
#error "STACK_OVERFLOW_DETECT IS CURRENTLY NOT IMPLEMENTED"
#endif

static long page_size;

#define PA(X) ((((long)X)+(page_size-1)) & ~(page_size-1))

volatile boolean always_false = FALSE;

exception
   TASKING_ERROR = "TASKING_ERROR : A non-handled exception occurred in a task",
   DEADLOCK_ERROR = "DEADLOCK_ERROR : Global Blocking Situation Detected",
   STORAGE_ERROR = "STORAGE_ERROR : No space left on the heap",
   CONSTRAINT_ERROR = "CONSTRAINT_ERROR",
   PROGRAM_ERROR = "PROGRAM_ERROR",
   STACK_ERROR = "STACK_ERROR : Stack Overflow",
   TIME_OUT = "TIME OUT while being blocked on a semaphor",
   NOT_IMPLEMENTED = "NOT_IMPLEMENTED (sorry)",
   USE_ERROR = "USE_ERROR : Marcel was not compiled to enable this functionality",
   LOCK_TASK_ERROR = "LOCK_TASK_ERROR : All tasks blocked after bad use of lock_task()";

volatile unsigned long threads_created_in_cache = 0;

#ifdef MINIMAL_PREEMPTION

static marcel_t special_thread = NULL,
                yielding_thread = NULL;

#endif

/* C'EST ICI QU'IL EST PRATIQUE DE METTRE UN POINT D'ARRET
   LORSQUE L'ON VEUT EXECUTER PAS A PAS... */
void breakpoint()
{
  /* Lieu ideal ;-) pour un point d'arret */
}

/* =========== specifs =========== */
int marcel_cancel(marcel_t pid);

#ifdef STACK_OVERFLOW_DETECT

void stack_protect(marcel_t t)
{
  int r;

   r = mprotect((char *)PA(t->stack_base), page_size, PROT_NONE);
   if(r == -1)
      RAISE(CONSTRAINT_ERROR);
}

void stack_unprotect(marcel_t t)
{
  int r;

   r = mprotect((char *)PA(t->stack_base), page_size, PROT_READ | PROT_WRITE);
   if(r == -1)
      RAISE(CONSTRAINT_ERROR);
}

#endif

#ifdef DEBUG
void _showsigmask(char *msg)
{
  sigset_t set;

  sigprocmask(0, NULL, &set);
  mdebug("%s : SIG_ALRM %s\n",
	 msg, (sigismember(&set, SIGALRM) ? "masqued" : "unmasqued"));
}
#endif

static __inline__ void marcel_terminate(marcel_t t)
{
  int i;

  for(i=((marcel_t)t)->next_cleanup_func-1; i>=0; i--)
    (*((marcel_t)t)->cleanup_funcs[i])(((marcel_t)t)->cleanup_args[i]);
}

#ifdef DEBUG

static void print_thread(marcel_t pid)
{
  long sp;

   if(pid == marcel_self()) {
#if defined(ALPHA_ARCH)
      get_sp(&sp);
#else
      sp = (long)get_sp();
#endif
   } else {
      sp = (long)SP_FIELD(pid->jb);
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
	 (long)pid + MAL(sizeof(task_desc)),
	 pid->user_space_ptr,
	 pid->initial_sp,
	 sp
	 );
}

void print_jmp_buf(char *name, jmp_buf buf)
{
  int i;

  for(i=0; i<sizeof(jmp_buf)/sizeof(char *); i++) {
    mdebug("%s[%d] = %p %s\n",
	   name,
	   i,
	   ((char **)buf)[i],
	   (((char **)buf + i) == (char **)&SP_FIELD(buf) ? "(sp)" : "")
	   );
  }
}

#endif

static __inline__ void init_task_desc(marcel_t t)
{
  t->cur_excep_blk = NULL;
  t->deviation_func = NULL;
  t->previous_lwp = NULL;
  t->next_cleanup_func = 0;
  t->in_sighandler = FALSE;
}

int marcel_create(marcel_t *pid, marcel_attr_t *attr, marcel_func_t func, any_t arg)
{
  marcel_t cur = marcel_self(), new_task;

  TIMING_EVENT("marcel_create");

  if(!attr)
    attr = &marcel_attr_default;

  lock_task();
  if(setjmp(cur->jb) == NORMAL_RETURN) {
#ifdef DEBUG
    breakpoint();
#endif
    MTRACE("Yield", cur);
    if(cur->child)
      marcel_insert_task(cur->child);
    unlock_task();
    return 0;
  } else {
    if(attr->stack_base) {
      register unsigned long top = MAL_BOT((long)attr->stack_base +
					   attr->stack_size);
#ifdef DEBUG
      if(top & (SLOT_SIZE-1)) { /* Not slot-aligned */
	unlock_task();
	RAISE(CONSTRAINT_ERROR);
      }
#endif
      new_task = (marcel_t)(top - MAL(sizeof(task_desc)));
      init_task_desc(new_task);
#ifdef STACK_CHECKING_ALLOWED
      memset(attr->stack_base, 0, attr->stack_size);
#endif
      new_task->stack_base = attr->stack_base;

#ifdef STACK_OVERFLOW_DETECT
      stack_protect(new_task);
#endif
      new_task->static_stack = TRUE;
    } else {
      char *bottom;
#ifdef DEBUG
      if(attr->stack_size > SLOT_SIZE)
	RAISE(NOT_IMPLEMENTED);
#endif
      bottom = marcel_slot_alloc();
      new_task = (marcel_t)(MAL_BOT((long)bottom + SLOT_SIZE) - MAL(sizeof(task_desc)));
      init_task_desc(new_task);
      new_task->stack_base = bottom;
      new_task->static_stack = FALSE;
    }

    mdebug("new_task stack top : %p\n", (char *)new_task + MAL(sizeof(task_desc)));

#ifdef USE_PRIORITIES
    new_task->prio = new_task->quantums = attr->priority;
#endif
    new_task->sched_policy = attr->sched_policy;
    new_task->not_migratable = attr->not_migratable;
    new_task->not_deviatable = attr->not_deviatable;
    new_task->detached = attr->detached;

    if(!attr->detached) {
      marcel_sem_init(&new_task->client, 0);
      marcel_sem_init(&new_task->thread, 0);
    }

    if(attr->user_space)
      new_task->user_space_ptr = (char *)new_task - MAL(attr->user_space);
    else
      new_task->user_space_ptr = NULL;

    new_task->father = cur;
    new_task->lwp = cur->lwp; /* In case timer_interrupt is called before
				 insert_task ! */
    new_task->f_to_call = func;
    new_task->arg = arg;
    new_task->initial_sp = (long)new_task - MAL(attr->user_space) - WINDOWSIZE;

    if(pid)
      *pid = new_task;

    marcel_one_more_task(new_task);
    MTRACE("Created", new_task);

#ifndef MINIMAL_PREEMPTION
    if((locked() > 1) /* lock_task has been called */
       || (new_task->user_space_ptr)
#ifdef SMP
       || (new_task->sched_policy != MARCEL_SCHED_OTHER)
#endif
       ) {
#endif
      new_task->father->child = (new_task->user_space_ptr ? NULL : new_task);

      call_ST_FLUSH_WINDOWS();
      set_sp(new_task->initial_sp);

      MTRACE("Yield", marcel_self());

      if(setjmp(marcel_self()->jb) == FIRST_RETURN) {
	call_ST_FLUSH_WINDOWS();
	longjmp(marcel_self()->father->jb, NORMAL_RETURN);
      }
#ifdef DEBUG
      breakpoint();
#endif
      MTRACE("Yield", marcel_self());
      unlock_task();
#ifndef MINIMAL_PREEMPTION
    } else {
      new_task->father->child = NULL;

      marcel_insert_task(new_task);

      call_ST_FLUSH_WINDOWS();
      set_sp(new_task->initial_sp);

      MTRACE("Yield", marcel_self());

      unlock_task();
    }
#endif

    marcel_exit((*marcel_self()->f_to_call)(marcel_self()->arg));
  }

  return 0;
}

int marcel_join(marcel_t pid, any_t *status)
{
#ifdef DEBUG
  if(pid->detached)
    RAISE(PROGRAM_ERROR);
#endif

   marcel_sem_P(&pid->client);
   if(status)
      *status = pid->ret_val;
   marcel_sem_V(&pid->thread);
   return 0;
}

int marcel_exit(any_t val)
{
  marcel_t cur = marcel_self();
#ifdef SMP
  register __lwp_t *cur_lwp = cur->lwp;

  /* Durant cette fonction, il ne faut pas que le thread soit
     "déplacé" intempestivement (e.g. après avoir acquis stack_mutex)
     sur un autre LWP. */
  cur->sched_policy = MARCEL_SCHED_FIXED(cur_lwp->number);
  /* Ne pas oublier de mettre "ancien LWP a NULL", sinon la tache sera
     replacee dessus dans insert_task(). Cette optimisation est source
     de confusion et il vaudrait peut-etre mieux la supprimer... */
  cur->previous_lwp = NULL;
#endif

  cur->ret_val = val;
  if(!cur->detached) {
    marcel_sem_V(&cur->client);
    marcel_sem_P(&cur->thread);
  }

  if(cur->static_stack) { /* Si la pile a ete allouee
			     a l'exterieur de Marcel... */

    marcel_mutex_lock(&cur_lwp->stack_mutex); /* D'abord le mutex ! */

    lock_task();

    *cur_lwp->sec_desc = *cur; /* On fabrique un "faux" thread dans
				  la pile de secours et ce thread 
				  devient le thread courant */

    marcel_insert_task(cur_lwp->sec_desc); /* On insere le nouveau thread
					    *avant* de retirer l'ancien 
					    (pour eviter le deadlock) */
    SET_GHOST(cur);
    marcel_unchain_task(cur);

    cur_lwp->sec_desc->private_val = (any_t)cur; /* On recupere le contenu
						    de la variable statique
						    avant unlock_task */

#ifdef STACK_OVERFLOW_DETECT
    stack_unprotect(cur);
#endif

    call_ST_FLUSH_WINDOWS(); /* On bascule sur la pile de secours... */
    set_sp(SECUR_STACK_TOP(cur_lwp));
#ifdef SMP
    cur_lwp = marcel_self()->lwp;
#endif

    unlock_task();

    /* Ouf, le noyau n'est pas verrouille.. */
    marcel_terminate(cur_lwp->sec_desc->private_val);

    lock_task();

    marcel_mutex_unlock(&cur_lwp->stack_mutex); /* On libere le mutex, mais pas encore
						   le noyau : ce sera chose faite lors
						   du retour du setjmp... */

    /* On averti le main qu'une tache de plus va disparaitre,
       ce qui va provoquer insert_task(main_task) si
       le thread courant est le dernier de l'application... */
    marcel_one_task_less(cur_lwp->sec_desc);

    SET_GHOST(cur_lwp->sec_desc);
    cur = marcel_unchain_task(cur_lwp->sec_desc); /* Il est maintenant temps
						     de retirer le "faux"
						     thread ! */

    MTRACE("Exited", cur_lwp->sec_desc);

    call_ST_FLUSH_WINDOWS();
    longjmp(cur->jb, NORMAL_RETURN);

  } else { /* Ici, la pile a ete allouee par le noyau Marcel */

#ifdef PM2
    RAISE(PROGRAM_ERROR);
#endif

    marcel_terminate(cur);

    marcel_mutex_lock(&cur_lwp->stack_mutex); /* Acquisition du mutex *avant* 
						 lock_task ! */
    lock_task();

    marcel_mutex_unlock(&cur_lwp->stack_mutex); /* idem ci-dessus */

    marcel_one_task_less(cur); /* Voir quelque lignes ci-dessus... */

    SET_GHOST(cur);
    cur_lwp->sec_desc->child = marcel_unchain_task(cur);
    /* child designe maintenant la tache a
       laquelle on donnera la "main" avant de
       mourir. */

    MTRACE("Exited", cur);

    cur_lwp->sec_desc->father = cur;
    cur_lwp->sec_desc->lwp = cur_lwp;

    call_ST_FLUSH_WINDOWS(); /* On bascule donc sur une pile annexe */
    set_sp(SECUR_STACK_TOP(cur_lwp));

#ifdef SMP
    cur_lwp = marcel_self()->lwp;
#endif

#ifdef STACK_OVERFLOW_DETECT
    stack_unprotect(cur_lwp->sec_desc->father);
#endif

    marcel_slot_free(marcel_stackbase(cur_lwp->sec_desc->father));

    call_ST_FLUSH_WINDOWS();
    longjmp(cur_lwp->sec_desc->child->jb, NORMAL_RETURN);
  }

  return 0; /* Silly, isn't it ? */
}

int marcel_cancel(marcel_t pid)
{
  if(pid == marcel_self()) {
    marcel_exit(NULL);
  } else {
    pid->ret_val = NULL;
    if(!pid->detached) {
      marcel_deviate(pid, (handler_func_t)marcel_exit, NULL);
      return 0;
    }

    lock_task();

    if(IS_BLOCKED(pid))
      RAISE(NOT_IMPLEMENTED);

    if(IS_SLEEPING(pid) || IS_FROZEN(pid))
      marcel_wake_task(pid, NULL);

    SET_GHOST(pid);
    marcel_unchain_task(pid);

    marcel_one_task_less(pid);

    MTRACE("Cancelled", pid);

    if(pid->static_stack) {
#ifdef STACK_OVERFLOW_DETECT
      stack_unprotect(pid);
#endif

      unlock_task();

      marcel_terminate(pid);

    } else {
#ifdef PM2
      RAISE(PROGRAM_ERROR);
#endif
#ifdef STACK_OVERFLOW_DETECT
      stack_unprotect(pid);
#endif

      unlock_task();

      marcel_terminate(pid);

      lock_task();
      marcel_slot_free(pid->stack_base);
      unlock_task();
    }
  }
  return 0;
}

int marcel_detach(marcel_t pid)
{
   pid->detached = TRUE;
   return 0;
}

void marcel_getuserspace(marcel_t pid, void **user_space)
{
   *user_space = pid->user_space_ptr;
}

void marcel_run(marcel_t pid, any_t arg)
{
  lock_task();
  pid->arg = arg;
  marcel_insert_task(pid);
  unlock_task();
}

int marcel_cleanup_push(cleanup_func_t func, any_t arg)
{
  marcel_t cur = marcel_self();

   if(cur->next_cleanup_func == MAX_CLEANUP_FUNCS)
      RAISE(CONSTRAINT_ERROR);

   cur->cleanup_args[cur->next_cleanup_func] = arg;
   cur->cleanup_funcs[cur->next_cleanup_func++] = func;
   return 0;
}

int marcel_cleanup_pop(boolean execute)
{
  int i;
  marcel_t cur = marcel_self();

   if(!cur->next_cleanup_func)
      RAISE(CONSTRAINT_ERROR);

   i = --cur->next_cleanup_func;
   if(execute)
      (*cur->cleanup_funcs[i])(cur->cleanup_args[i]);
   return 0;
}

int marcel_once(marcel_once_t *once, void (*f)(void))
{
   marcel_mutex_lock(&once->mutex);
   if(!once->executed) {
      once->executed = TRUE;
      marcel_mutex_unlock(&once->mutex);
      (*f)();
   } else
      marcel_mutex_unlock(&once->mutex);
   return 0;
}

static void __inline__ freeze(marcel_t t)
{
   if(t != marcel_self()) {
      lock_task();
      if(IS_BLOCKED(t) || IS_FROZEN(t)) {
         unlock_task();
         RAISE(PROGRAM_ERROR);
      } else if(IS_SLEEPING(t)) {
         marcel_wake_task(t, NULL);
      }
      SET_FROZEN(t);
      marcel_unchain_task(t);
      unlock_task();
   }
}

static void __inline__ unfreeze(marcel_t t)
{
   if(IS_FROZEN(t)) {
      lock_task();
      marcel_wake_task(t, NULL);
      unlock_task();
   }
}

void marcel_freeze(marcel_t *pids, int nb)
{
  int i;

   for(i=0; i<nb; i++)
      freeze(pids[i]);
}

void marcel_unfreeze(marcel_t *pids, int nb)
{
  int i;

   for(i=0; i<nb; i++)
      unfreeze(pids[i]);
}

/* WARNING!!! MUST BE LESS CONSTRAINED THAN MARCEL_ALIGN (64) */
#define ALIGNED_32(addr)(((unsigned long)(addr) + 31) & ~(31L))

void marcel_begin_hibernation(marcel_t t, transfert_func_t transf, 
			      void *arg, boolean fork)
{
  unsigned long depl, blk;
  unsigned long bottom, top;
  marcel_t cur = marcel_self();

  if(t == cur) {
    lock_task();
    if(setjmp(cur->migr_jb) == FIRST_RETURN) {

      call_ST_FLUSH_WINDOWS();
      top = (long)cur + ALIGNED_32(sizeof(task_desc));
      bottom = ALIGNED_32((long)SP_FIELD(cur->migr_jb)) - ALIGNED_32(1);
      blk = top - bottom;
      depl = bottom - (long)cur->stack_base;
      unlock_task();

      mdebug("hibernation of thread %p", cur);
      mdebug("sp = %lu\n", get_sp());
      mdebug("sp_field = %lu\n", SP_FIELD(cur->migr_jb));
      mdebug("bottom = %lu\n", bottom);
      mdebug("top = %lu\n", top);
      mdebug("blk = %lu\n", blk);

      (*transf)(cur, depl, blk, arg);

      if(!fork)
	marcel_exit(NULL);
    } else {
#ifdef DEBUG
      breakpoint();
#endif
      unlock_task();
    }
  } else {
    memcpy(t->migr_jb, t->jb, sizeof(jmp_buf));

    top = (long)t + ALIGNED_32(sizeof(task_desc));
    bottom = ALIGNED_32((long)SP_FIELD(t->jb)) - ALIGNED_32(1);
    blk = top - bottom;
    depl = bottom - (long)t->stack_base;

    mdebug("hibernation of thread %p", t);
    mdebug("sp_field = %lu\n", SP_FIELD(t->migr_jb));
    mdebug("bottom = %lu\n", bottom);
    mdebug("top = %lu\n", top);
    mdebug("blk = %lu\n", blk);

    (*transf)(t, depl, blk, arg);

    if(!fork) {
      marcel_cancel(t);
    }
  }
}

marcel_t marcel_alloc_stack(unsigned size)
{
  marcel_t t;
  char *st;

  lock_task();

#ifdef PM2
  RAISE(PROGRAM_ERROR);
#endif

  st = marcel_slot_alloc();

  t = (marcel_t)(MAL_BOT((long)st + size) - MAL(sizeof(task_desc)));

  init_task_desc(t);
  t->stack_base = st;

#ifdef STACK_OVERFLOW_DETECT
  stack_protect(t);
#endif
#ifdef STACK_CHECKING_ALLOWED
  memset(t->stack_base, 0, size);
#endif

  unlock_task();

  return t;
}

void marcel_end_hibernation(marcel_t t, post_migration_func_t f, void *arg)
{
  memcpy(t->jb, t->migr_jb, sizeof(jmp_buf));

  mdebug("end of hibernation for thread %p", t);

  lock_task();

  marcel_insert_task(t);
  marcel_one_more_task(t);

  if(f != NULL)
    marcel_deviate(t, f, arg);

  unlock_task();
}

void marcel_enabledeviation(void)
{
  volatile handler_func_t func;
  any_t arg;
  marcel_t cur = marcel_self();

  lock_task();
  if(--cur->not_deviatable == 0 && cur->deviation_func != NULL) {
    func = cur->deviation_func;
    arg = cur->deviation_arg;
    cur->deviation_func = NULL;
    unlock_task();
    (*func)(arg);
  } else
    unlock_task();
}

void marcel_disabledeviation(void)
{
  ++marcel_self()->not_deviatable;
}

static void insertion_relai(handler_func_t f, void *arg)
{ 
  jmp_buf back;
  marcel_t cur = marcel_self();

  memcpy(back, cur->jb, sizeof(jmp_buf));
  marcel_disablemigration(cur);
  if(setjmp(cur->jb) == FIRST_RETURN) {
    call_ST_FLUSH_WINDOWS();
    longjmp(cur->father->jb, NORMAL_RETURN);
  } else {
#ifdef DEBUG
    breakpoint();
#endif
    unlock_task();
    (*f)(arg);
    lock_task();
    marcel_enablemigration(cur);
    memcpy(cur->jb, back, sizeof(jmp_buf));
    call_ST_FLUSH_WINDOWS(); /* surement inutile... */
    longjmp(cur->jb, NORMAL_RETURN);
  }
}

/* VERY INELEGANT: to avoid inlining of insertion_relai function... */
typedef void (*relai_func_t)(handler_func_t f, void *arg);
static relai_func_t relai_func = insertion_relai;

void marcel_deviate(marcel_t pid, handler_func_t h, any_t arg)
{ 
  static volatile handler_func_t f_to_call;
  static void * volatile argument;
  static volatile long initial_sp;

  lock_task();

#ifdef SMP
  if(marcel_self()->lwp != pid->lwp)
    RAISE(NOT_IMPLEMENTED);
#endif

  if(pid->not_deviatable) {
    if(pid->deviation_func != NULL) {
      unlock_task();
      RAISE(NOT_IMPLEMENTED);
    }
    pid->deviation_func = h;
    pid->deviation_arg = arg;
    unlock_task();
    return;
  }
  if(pid == marcel_self()) {
    unlock_task();
    (*h)(arg);
  } else {
    if(IS_BLOCKED(pid) || IS_SLEEPING(pid))
      marcel_wake_task(pid, NULL);
    else if(IS_FROZEN(pid))
      marcel_wake_task(pid, NULL);

    if(setjmp(marcel_self()->jb) == NORMAL_RETURN) {
      unlock_task();
    } else {
      f_to_call = h;
      argument = arg;

      pid->father = marcel_self();

      initial_sp = MAL_BOT((long)SP_FIELD(pid->jb)) - 2*WINDOWSIZE - 256;

      call_ST_FLUSH_WINDOWS();
      set_sp(initial_sp);

      (*relai_func)(f_to_call, argument);

      RAISE(PROGRAM_ERROR); /* on ne doit jamais arriver ici ! */
    }
  }
}

static unsigned __nb_lwp = 0;

static void marcel_parse_cmdline(int *argc, char **argv, boolean do_not_strip)
{
  int i, j;

  i = j = 1;

  while(i < *argc) {
#ifdef SMP
    if(!strcmp(argv[i], "-nvp")) {
      if(i == *argc-1) {
	fprintf(stderr,
		"Fatal error: -nvp option must be followed "
		"by <nb_of_virtual_processors>.\n");
	exit(1);
      }
      if(do_not_strip) {
	__nb_lwp = atoi(argv[i+1]);
	if(__nb_lwp < 1 || __nb_lwp > MAX_LWP) {
	  fprintf(stderr,
		  "Error: nb of VP should be between 1 and %d\n",
		  MAX_LWP);
	  exit(1);
	}
	argv[j++] = argv[i++];
	argv[j++] = argv[i++];
      } else
	i += 2;
      continue;
    } else
#endif
      argv[j++] = argv[i++];
  }
  *argc = j;
  argv[j] = NULL;

  mdebug("Suggested nb of Virtual Processors : %d\n", __nb_lwp);
}

void marcel_strip_cmdline(int *argc, char *argv[])
{
  marcel_parse_cmdline(argc, argv, FALSE);
}

void marcel_init(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  if(!already_called) {

    /* Analyse but do not strip */
    marcel_parse_cmdline(argc, argv, TRUE);

#ifndef PM2
    /* Strip without analyse */
    marcel_strip_cmdline(argc, argv);
#endif

#ifndef PM2
    timing_init();
#endif

#if !defined(PM2) && defined(USE_SAFE_MALLOC)
    safe_malloc_init();
#endif

#ifdef SOLARIS_SYS
    page_size = sysconf(_SC_PAGESIZE);
#else
#ifdef UNICOS_SYS
    page_size = 1024;
#else
    page_size = getpagesize();
#endif
#endif

    marcel_slot_init();
    marcel_sched_init(__nb_lwp);
    marcel_io_init();

    already_called = TRUE;
  }
}

void marcel_end(void)
{
  marcel_sched_shutdown();
  marcel_slot_exit();
  mdebug("threads created in cache : %ld\n", threads_created_in_cache);
}

unsigned long marcel_cachedthreads(void)
{
   return threads_created_in_cache;
}

unsigned long marcel_usablestack(void)
{
  long sp;

#ifdef ALPHA_ARCH
   get_sp(&sp);
#else
   sp = (long)get_sp();
#endif
   return sp - (long)marcel_self()->stack_base;
}

unsigned long marcel_unusedstack(void)
{
#ifdef STACK_CHECKING_ALLOWED
   char *repere = (char *)marcel_self()->stack_base;
   unsigned *ptr = (unsigned *)repere;

   while(!(*ptr++));
   return ((char *)ptr - repere);

#else
   RAISE(USE_ERROR);
   return 0;
#endif
}

void *tmalloc(unsigned size)
{
  void *p;

   if(size) {
      lock_task();
      if((p = MALLOC(size)) == NULL)
         RAISE(STORAGE_ERROR);
      else {
         unlock_task();
         return p;
      }
   }
   return NULL;
}

void *trealloc(void *ptr, unsigned size)
{
  void *p;

   lock_task();
   if((p = REALLOC(ptr, size)) == NULL)
      RAISE(STORAGE_ERROR);
   unlock_task();
   return p;
}

void *tcalloc(unsigned nelem, unsigned elsize)
{
  void *p;

   if(nelem && elsize) {
      lock_task();
      if((p = CALLOC(nelem, elsize)) == NULL)
         RAISE(STORAGE_ERROR);
      else {
         unlock_task();
         return p;
      }
   }
   return NULL;
}

void tfree(void *ptr)
{
   if(ptr) {
      lock_task();
      FREE((char *)ptr);
      unlock_task();
   }
}

#ifdef SMP
static unsigned __io_lock = 0;
#endif

static __inline__ void io_lock()
{
  lock_task();
#ifdef SMP
  while(testandset(&__io_lock))
    SCHED_YIELD();
#endif
}

static __inline__ void io_unlock()
{
#ifdef SMP
  release(&__io_lock);
#endif
  unlock_task();
}

int tprintf(char *format, ...)
{
  static va_list args;
  int retour;

  io_lock();

  va_start(args, format);
  retour = vprintf(format, args);
  va_end(args);

  io_unlock();

  return retour;
}

int tfprintf(FILE *stream, char *format, ...)
{
  static va_list args;
  int retour;

  io_lock();

  va_start(args, format);
  retour = vfprintf(stream, format, args);
  va_end(args);

  io_unlock();
  return retour;
}

volatile unsigned _nb_keys = 1;
/* 
 * Hummm... Should be 0, but for obscure reasons,
 * 0 is a RESERVED value. DON'T CHANGE IT !!! 
*/

int marcel_key_create(marcel_key_t *key, void (*func)(any_t))
{ /* pour l'instant, le destructeur n'est pas utilise */

   lock_task();
   if(_nb_keys == MAX_KEY_SPECIFIC) {
      unlock_task();
      RAISE(CONSTRAINT_ERROR);
   }
   *key = _nb_keys++;
   unlock_task();
   return 0;
}

/* ================== Gestion des exceptions : ================== */

int _raise(exception ex)
{
  marcel_t cur = marcel_self();

   if(ex == NULL)
      ex = cur->cur_exception ;

   if(cur->cur_excep_blk == NULL) {
      lock_task();
      fprintf(stderr, "\nUnhandled exception %s in task %ld"
	      "\nFILE : %s, LINE : %d\n",
	      ex, cur->number, cur->exfile, cur->exline);
      *(int *)0L = -1; /* To generate a core file */
      exit(1);
   } else {
      cur->cur_exception = ex ;
      call_ST_FLUSH_WINDOWS();
      longjmp(cur->cur_excep_blk->jb, 1);
   }
   return 0;
}

/* =============== Gestion des E/S non bloquantes =============== */


int tselect(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds)
{
  int res = 0;
  struct timeval timeout;
  fd_set rfds, wfds, efds;

   do {
      FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&efds);
      if(readfds) rfds = *readfds;
      if(writefds) wfds = *writefds;
      if(exceptfds) efds = *exceptfds;

      timerclear(&timeout);
      res = select(width, &rfds, &wfds, &efds, &timeout);
      if(res <= 0) {
#ifdef MINIMAL_PREEMPTION
	marcel_givehandback();
#else
	marcel_trueyield();
#endif
      }
   } while(res <= 0);

   if(readfds) *readfds = rfds;
   if(writefds) *writefds = wfds;
   if(exceptfds) *exceptfds = efds;

   return res;
}


#ifndef STANDARD_MAIN

extern int marcel_main(int argc, char *argv[]);

marcel_t __main_thread;

static jmp_buf __initial_main_jb;
static volatile int __main_ret;

int main(int argc, char *argv[])
{
  static int __argc;
  static char **__argv;

  if(!setjmp(__initial_main_jb)) {

    __main_thread = (marcel_t)((((unsigned long)get_sp() - 128) & ~(SLOT_SIZE-1)) -
			       MAL(sizeof(task_desc)));

    mdebug("main_thread set to %p\n", __main_thread);
    mdebug("sp = %lx\n", get_sp());

    __argc = argc; __argv = argv;

    set_sp((unsigned long)__main_thread - 2 * WINDOWSIZE);

    mdebug("sp = %lx\n", get_sp());

    __main_ret = marcel_main(__argc, __argv);

    longjmp(__initial_main_jb, 1);
  }

  return __main_ret;
}

#endif


/* *** Christian Perez Stuff ;-) *** */

void marcel_special_init(marcel_t *liste)
{
   *liste = NULL;
}

/* ATTENTION : lock_task doit avoir ete appele au prealable ! */
void marcel_special_P(marcel_t *liste)
{
  register marcel_t cur = marcel_self(), p;
  marcel_t next;
#ifdef SMP
  __lwp_t *cur_lwp = cur->lwp;
#endif

   next = marcel_radical_next_task();

   p = cur->prev;
   p->next = cur->next;
   p->next->prev = p;
   if(cur_lwp->__first[cur->prio] == cur) {
      cur_lwp->__first[cur->prio] = ((cur->next->prio == cur->prio) ? cur->next : NULL);
      if(cur_lwp->__first[0] == cur)
	 cur_lwp->__first[0] = cur->next;
   }

   if(*liste == NULL) {
      *liste = cur;
      cur->next = cur->prev = cur;
   } else {
      cur->next = *liste;
      cur->prev = (*liste)->prev;
      cur->prev->next = cur->next->prev = cur;
   }

   cur->quantums = cur->prio;
   SET_BLOCKED(cur);
   if(setjmp(cur->jb) == NORMAL_RETURN) {
#ifdef DEBUG
      breakpoint();
#endif
      unlock_task();
   } else {
      call_ST_FLUSH_WINDOWS();
      longjmp(next->jb, NORMAL_RETURN);
   }
}

void marcel_special_V(marcel_t *pliste)
{
  register marcel_t liste = *pliste;
  register unsigned i;
  marcel_t p, queue = liste->prev;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

   lock_task();

   /* on regarde la priorité du premier   */
   /* (tous doivent etre de meme priorite */
   if(liste->prio == MAX_PRIO)
      i = 0;
   else
      for(i=liste->prio; cur_lwp->__first[i] == NULL; i--) ;
   p = cur_lwp->__first[i];
   liste->prev = p->prev;
   queue->next = p;
   p->prev = queue;
   liste->prev->next = liste;
   cur_lwp->__first[liste->prio] = liste;
   if(p == cur_lwp->__first[0])
      cur_lwp->__first[0] = liste;

   *pliste = 0;

   unlock_task();
}

void marcel_special_VP(marcel_sem_t *s, marcel_t *liste)
{
  cell *c;
  register marcel_t p, cur = marcel_self();
  marcel_t next;
#ifdef SMP
  __lwp_t *cur_lwp = cur->lwp;
#endif

   lock_task();

   if(++(s->value) <= 0) {
     c = s->first;
     s->first = c->next;
     marcel_wake_task(c->task, &c->blocked);
   }

   next = marcel_radical_next_task();

      p = cur->prev;
      p->next = cur->next;
      p->next->prev = p;
      if(cur_lwp->__first[cur->prio] == cur) {
         cur_lwp->__first[cur->prio] = ((cur->next->prio == cur->prio) ? cur->next : NULL);
         if(cur_lwp->__first[0] == cur)
	    cur_lwp->__first[0] = cur->next;
      }

      if(*liste == NULL) {
         *liste = cur;
         cur->next = cur->prev = cur;
      } else {
         cur->next = *liste;
         cur->prev = (*liste)->prev;
         cur->prev->next = (*liste)->prev = cur;
      }

      cur->quantums = cur->prio;
      SET_BLOCKED(cur);
      if(setjmp(cur->jb) == NORMAL_RETURN) {
#ifdef DEBUG
         breakpoint();
#endif
         unlock_task();
      } else {
         call_ST_FLUSH_WINDOWS();
         longjmp(next->jb, NORMAL_RETURN);
      }
}

/* End of Christian Perez Stuff */

