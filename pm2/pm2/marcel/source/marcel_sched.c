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

/* #define DEBUG */

#define BIND_LWP_ON_PROCESSORS

/* #define DO_PAUSE_INSTEAD_OF_SCHED_YIELD */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <marcel.h>
#include <safe_malloc.h>

#ifdef SMP
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#ifdef SOLARIS_SYS
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#endif
#endif

#ifdef __ACT__
#include <sys/upcalls.h>
#endif


#ifdef DEBUG
extern void breakpoint();
#endif

static int next_schedpolicy_available = __MARCEL_SCHED_AVAILABLE;
static marcel_schedpolicy_func_t user_policy[MARCEL_MAX_USER_SCHED];

static boolean marcel_initialized = FALSE;

static volatile unsigned __active_threads = 0,
  __sleeping_threads = 0,
  __blocked_threads = 0,
  __frozen_threads = 0;

__lwp_t __main_lwp;

#ifdef STANDARD_MAIN
task_desc __main_thread_struct;
#define __main_thread  (&__main_thread_struct)
#else
extern marcel_t __main_thread;
#endif

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS
static marcel_t __waiting_tasks = NULL;
#endif
static marcel_t __delayed_tasks = NULL;
static volatile unsigned long task_number = 1;

static unsigned __nb_lwp = 0;

/* These two locks must be acquired before accessing the corresponding
   global queue.  They should only encapsulate *non-blocking* code
   sections. */
static marcel_lock_t __delayed_lock = MARCEL_LOCK_INIT;
static marcel_lock_t __blocking_lock = MARCEL_LOCK_INIT;
static marcel_lock_t __polling_lock = MARCEL_LOCK_INIT;

#ifdef SMP
static unsigned  NB_LWPS;
#endif

#define MIN_TIME_SLICE		10000

#define DEFAULT_TIME_SLICE	20000

static volatile boolean time_slice_enabled = TRUE;
static volatile unsigned long time_slice = DEFAULT_TIME_SLICE;
static volatile unsigned long __milliseconds = 0;

static poll_struct_t poll_structs[MAX_POLL_IDS];
static unsigned nb_poll_structs = 0;

static poll_struct_t *__polling_tasks = NULL;

static struct {
       unsigned nb_tasks;
       boolean main_is_waiting, blocked;
} _main_struct = { 0, FALSE, FALSE };

/* TODO: Use atomic_inc and atomic_dec when accessing running_tasks! */
#define INC_STATS(t, lwp) \
  (((t) != (lwp)->sched_task) ? (lwp)->running_tasks++ : 0)
#define DEC_STATS(t, lwp) \
  (((t) != (lwp)->sched_task) ? (lwp)->running_tasks-- : 0)

#define one_more_active_task(t, lwp) \
  __active_threads++, \
  INC_STATS((t), (lwp)), \
  MTRACE("Activation", (t))

#define one_active_task_less(t, lwp) \
  __active_threads--, \
  DEC_STATS((t), (lwp))

#define one_more_sleeping_task(t) \
  __sleeping_threads++, MTRACE("Sleeping", (t))

#define one_sleeping_task_less(t) \
  __sleeping_threads--

#define one_more_blocked_task(t) \
  __blocked_threads++, MTRACE("Blocking", (t))

#define one_blocked_task_less(t) \
  __blocked_threads--

#define one_more_frozen_task(t) \
  __frozen_threads++, MTRACE("Freezing", (t))

#define one_frozen_task_less(t) \
  __frozen_threads--


void marcel_schedpolicy_create(int *policy,
			       marcel_schedpolicy_func_t func)
{
  if(next_schedpolicy_available <=
     __MARCEL_SCHED_AVAILABLE - MARCEL_MAX_USER_SCHED)
    RAISE(CONSTRAINT_ERROR);

  user_policy[__MARCEL_SCHED_AVAILABLE -
	      next_schedpolicy_available] = func;
  *policy = next_schedpolicy_available--;
}

unsigned marcel_nbvps(void)
{
#ifdef SMP
  return NB_LWPS;
#else
  return 1;
#endif
}

#ifdef DEBUG
static __inline__ void display_sched_queue(marcel_t pid)
{
  marcel_t t = pid;

  do {
    fprintf(stderr, "\t\tThread %p (LWP == %d)\n", t, t->lwp->number);
    t = t->next;
  } while(t != pid);
}
#endif

/* Returns the LWP of number 'vp % NB_LWPS' */
static __inline__ __lwp_t *find_lwp(unsigned vp)
{
  register __lwp_t *lwp = &__main_lwp;

  while(vp--)
    lwp = lwp->next;

  return lwp;
}

/* Returns an underloaded LWP if any, otherwise the suggested one */
static __inline__ __lwp_t *find_good_lwp(__lwp_t *suggested)
{
  __lwp_t *lwp = suggested;

  for(;;) {
    if(lwp->running_tasks < THREAD_THRESHOLD_LOW)
      return lwp;
    lwp = lwp->next;
    if(lwp == suggested)
      return lwp; /* No underloaded LWP, so return the suggested one */
  }
}

/* Returns the least underloaded LWP */
static __inline__ __lwp_t *find_best_lwp(void)
{
  __lwp_t *lwp = &__main_lwp;
  __lwp_t *best = lwp;
  unsigned nb = lwp->running_tasks;

  for(;;) {
    lwp = lwp->next;
    if(lwp == &__main_lwp)
      return best;
    if(lwp->running_tasks < nb) {
      nb = lwp->running_tasks;
      best = lwp;
    }
  }
}

unsigned long marcel_createdthreads(void)
{
   return task_number -1;    /* -1 pour le main */
}

unsigned marcel_nbthreads(void)
{
   return _main_struct.nb_tasks + 1;   /* + 1 pour le main */
}

unsigned marcel_activethreads(void)
{
   return __active_threads;
}

unsigned marcel_sleepingthreads(void)
{
  return __sleeping_threads;
}

unsigned marcel_blockedthreads(void)
{
  return __blocked_threads;
}

unsigned marcel_frozenthreads(void)
{
  return __frozen_threads;
}

static marcel_lock_t __wait_lock = MARCEL_LOCK_INIT;

void marcel_one_more_task(marcel_t pid)
{
  marcel_lock_acquire(&__wait_lock);

  pid->number = task_number++;
  _main_struct.nb_tasks++;

  marcel_lock_release(&__wait_lock);
}

void marcel_one_task_less(marcel_t pid)
{
  marcel_lock_acquire(&__wait_lock);

  if(--_main_struct.nb_tasks == 0 && _main_struct.main_is_waiting) {
    marcel_wake_task(__main_thread, &_main_struct.blocked);
  }

  marcel_lock_release(&__wait_lock);
}

static void wait_all_tasks_end(void)
{
  lock_task();

  marcel_lock_acquire(&__wait_lock);

  if(_main_struct.nb_tasks) {
    _main_struct.main_is_waiting = TRUE;
    _main_struct.blocked = TRUE;
    marcel_give_hand(&_main_struct.blocked, &__wait_lock);
  } else {
    marcel_lock_release(&__wait_lock);
    unlock_task();
  }
}

marcel_t marcel_radical_next_task(void)
{
  marcel_t cur = marcel_self(), t;
#ifdef SMP
  __lwp_t *cur_lwp = cur->lwp;
#endif

  sched_lock(cur_lwp);

  if(cur->quantums-- == 1) {
    cur->quantums = cur->prio;
    t = cur->next;
  } else {
    if(cur->next->quantums <= cur->quantums) {
      if(cur_lwp->__first[0] == cur)
	t = cur->next;
      else
	t = cur_lwp->__first[0];
    } else
      t = cur->next;
  }

#ifdef __ACT__
  ACTDEBUG(printf("Warning : marcel_radical_next_task used\n"));
  while (t->state_ext==RUNNING) {
    cur=t;
    if(cur->quantums == 0) {
      cur->quantums = cur->prio;
      t = cur->next;
    } else {
      if(cur->next->quantums <= cur->quantums) {
	if(cur_lwp->__first[0] == cur)
	  t = cur->next;
	else
	  t = cur_lwp->__first[0];
      } else
	t = cur->next;
    }
  }
#endif

  sched_unlock(cur_lwp);

  return t;
}

void marcel_insert_task(marcel_t t)
{
#ifdef USE_PRIORITIES
  unsigned i;
#endif
  register marcel_t p;
#ifdef SMP
  __lwp_t *self_lwp = marcel_self()->lwp, *cur_lwp;

  switch(t->sched_policy) {
  case MARCEL_SCHED_OTHER : {
    cur_lwp = self_lwp;
    break;
  }
  case MARCEL_SCHED_AFFINITY : {
    cur_lwp = find_good_lwp(t->previous_lwp ? : self_lwp);
    break;
  }
  case MARCEL_SCHED_BALANCE : {
    cur_lwp = find_best_lwp();
    break;
  }
  default: {
    if(t->sched_policy >= 0)
      /* MARCEL_SCHED_FIXED(vp) */
      cur_lwp = (t->previous_lwp ? : find_lwp(t->sched_policy));
    else
      /* MARCEL_SCHED_USER_... */
      cur_lwp = (*user_policy[__MARCEL_SCHED_AVAILABLE - 
			      (t->sched_policy)])(t, self_lwp);
  }
  }

  if(t != self_lwp->sched_task)
    sched_lock(cur_lwp);

  mdebug("\t\t\t<Insertion of task %p on LWP %d>\n", t, cur_lwp->number);
#endif

#ifdef USE_PRIORITIES
  for(i=t->prio; cur_lwp->__first[i] == NULL; i--) ;
  p = cur_lwp->__first[i];
#else
  p = cur_lwp->__first[0];
#endif
  t->prev = p->prev;
  t->next = p;
  p->prev = t;
  t->prev->next = t;
#ifdef USE_PRIORITIES
  cur_lwp->__first[t->prio] = t;
  if(i != 0 && p == cur_lwp->__first[0])
    cur_lwp->__first[0] = t;
#else
  cur_lwp->__first[0] = t;
#endif

  SET_RUNNING(t);
  one_more_active_task(t, cur_lwp);

  t->lwp = cur_lwp;

#ifdef SMP
  if(t != self_lwp->sched_task) {
    cur_lwp->has_new_tasks = TRUE;
    sched_unlock(cur_lwp);
  } else
    self_lwp->has_new_tasks = FALSE;
#endif
}

/* TODO: With marcel_deviate, a task can be waked although its state
   is "RUNNING". Should fix the problem here! */
void marcel_wake_task(marcel_t t, boolean *blocked)
{
  register marcel_t p;

  if(IS_SLEEPING(t)) {

    /* No need to acquire "__delayed_lock" : this is done in
       check_delayed_tasks. */

    if(t == __delayed_tasks)
      __delayed_tasks = t->next;
    if((p = t->next) != NULL)
      p->prev = t->prev;
    if((p = t->prev) != NULL)
      p->next = t->next;

    one_sleeping_task_less(t);

  } else if(IS_BLOCKED(t)) {

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

    marcel_lock_acquire(&__blocking_lock);

    if((p = t->prev) == t)
      __waiting_tasks = NULL;
    else {
      if(t == __waiting_tasks)
	__waiting_tasks = p;
      p->next = t->next;
      p->next->prev = p;
    }

    marcel_lock_release(&__blocking_lock);

#endif

    one_blocked_task_less(t);

  } else if(IS_FROZEN(t)) {

    one_frozen_task_less(t);

  }

  marcel_insert_task(t);

  if(blocked != NULL)
    *blocked = FALSE;
}

/* Remove from ready queue and insert into waiting queue
   (if it is BLOCKED) or delayed queue (if it is WAITING). */
marcel_t marcel_unchain_task(marcel_t t)
{
  marcel_t p, r;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

  ACTDEBUG(printf("marcel_unchain_task(%p->%p->%p->%p->%p\n", t,
		  t->next, t->next->next, t->next->next->next, 
		  t->next->next->next->next));

  sched_lock(cur_lwp);

  one_active_task_less(t, cur_lwp);
#ifdef SMP
  /* For affinity scheduling: */
  t->previous_lwp = cur_lwp;
#endif

#ifdef __ACT__
  r = t;
  do {
    r=r->next;
    if ((r->state_ext == MARCEL_RUNNING) && (r!=marcel_self()))
      ACTDEBUG(printf("marcel_unchain_task(%p) : %p yet running\n", t, r));
  } while ((r->state_ext == MARCEL_RUNNING) && (r!=t) && (r!=marcel_self()));
  /* Pas très sur de moi ici. Apparemment, il faut renvoyer la
     prochaine task à ordonnancer. Avec les activation, il ne faut pas
     en prendre une tournant sur une aute activation, mais on peut
     prendre nous-même. Mais est-ce que marcel_self() est valide
     chaque fois que l'on appelle cette fonction (on a pu faire des
     changements de pile...) */
#else
  r = t->next;
#endif
  ACTDEBUG(printf("marcel_unchain_task(%p) : next=%p\n", t, r));
  if(r == t) {
    r = cur_lwp->sched_task;
    marcel_wake_task(r, NULL);
  }
  r->prev = t->prev;
  r->prev->next = r;
#ifdef USE_PRIORITIES
  if(cur_lwp->__first[t->prio] == t) {
    cur_lwp->__first[t->prio] = ((r->prio == t->prio) ? r : NULL);
    if(cur_lwp->__first[0] == t)
      cur_lwp->__first[0] = r;
  }
#else
  cur_lwp->__first[0] = r;
#endif

  sched_unlock(cur_lwp);

  if(IS_SLEEPING(t)) {

    one_more_sleeping_task(t);

    marcel_lock_acquire(&__delayed_lock);

    /* insertion dans "__delayed_tasks" */
    {
      marcel_t cur;

      p = NULL;
      cur = __delayed_tasks;
      while(cur != NULL && cur->time_to_wake < t->time_to_wake) {
	p = cur;
	cur = cur->next;
      }
      t->next = cur;
      t->prev = p;
      if(p == NULL)
	__delayed_tasks = t;
      else
	p->next = t;
      if(cur != NULL)
	cur->prev = t;
    }

    marcel_lock_release(&__delayed_lock);

  } else if(IS_BLOCKED(t)) {

    one_more_blocked_task(t);

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

    marcel_lock_acquire(&__blocking_lock);

    if(__waiting_tasks == NULL) {
      __waiting_tasks = t;
      t->next = t->prev = t;
    } else {
      t->next = __waiting_tasks;
      t->prev = __waiting_tasks->prev;
      t->prev->next = __waiting_tasks->prev = t;
    }

    marcel_lock_release(&__blocking_lock);

#endif

  } else if(IS_FROZEN(t)) {

    one_more_frozen_task(t);

  }
  /* Do nothing particular for GHOST tasks... */
  return r;
}

static __inline__ marcel_t next_task(marcel_t t, __lwp_t *lwp)
{
  register marcel_t res;

  sched_lock(lwp);

#ifdef USE_PRIORITIES
#ifdef __ACT__
#error not yet implemented.
#endif
  if(t->quantums-- == 1) {
    t->quantums = t->prio;
    res = t->next;
  } else {
    if(t->next->quantums <= t->quantums)
      res = lwp->__first[0];
    else
      res = t->next;
  }
#else
#ifdef __ACT__
  res=t;
  do {
    res = res->next;
  } while ((res->state_ext == MARCEL_RUNNING) && (res != t));
  //ACTDEBUG(printf("next_task(%p) : next=%p\n", t, res));  
#else
  res = t->next;
#endif
#endif

  sched_unlock(lwp);

  return res;
}

void marcel_yield(void)
{
  register marcel_t cur = marcel_self();
#ifdef SMP
  __lwp_t *cur_lwp = cur->lwp;
#endif

  lock_task();

#ifdef __ACT__
  cur->sched_by = SCHED_BY_MARCEL;
  if(setjmp(cur->jb.migr_jb) == NORMAL_RETURN) {
#else
  if(setjmp(cur->jb) == NORMAL_RETURN) {
#endif
#ifdef DEBUG
    breakpoint();
#endif
    MTRACE("Preemption", cur);
#ifdef __ACT__
    cur->state_ext=MARCEL_RUNNING;
#endif
    unlock_task();
    return;
  }
  call_ST_FLUSH_WINDOWS();
#ifdef __ACT__
  cur->state_ext=MARCEL_READY;
  restart_thread(next_task(cur, cur_lwp));
#else
  longjmp(next_task(cur, cur_lwp)->jb, NORMAL_RETURN);
#endif
}


#ifndef __ACT__
void marcel_explicityield(marcel_t t)
{
  register marcel_t cur = marcel_self();

  lock_task();
  if(setjmp(cur->jb) == NORMAL_RETURN) {
#ifdef DEBUG
    breakpoint();
#endif
    MTRACE("Preemption", cur);
    unlock_task();
  } else {
    call_ST_FLUSH_WINDOWS();
    longjmp(t->jb, NORMAL_RETURN);
  }
}

void marcel_trueyield(void)
{
  marcel_t next;
  register marcel_t cur = marcel_self();

  lock_task();

  next = marcel_radical_next_task();

  if(setjmp(cur->jb) == NORMAL_RETURN) {
#ifdef DEBUG
    breakpoint();
#endif
    MTRACE("Preemption", cur);
    unlock_task();
  } else {
    call_ST_FLUSH_WINDOWS();
    longjmp(next->jb, NORMAL_RETURN);
  }
}
#endif /* __ACT__ */

void marcel_give_hand(boolean *blocked, marcel_lock_t *lock)
{
  marcel_t next;
  register marcel_t cur = marcel_self();
#ifdef SMP
  volatile boolean first_time = TRUE;
#endif

  if(locked() != 1)
    RAISE(LOCK_TASK_ERROR);
  do {
#ifdef __ACT__
    cur->sched_by = SCHED_BY_MARCEL;
    if(setjmp(cur->jb.migr_jb) == NORMAL_RETURN) {
#else
    if(setjmp(cur->jb) == NORMAL_RETURN) {
#endif
#ifdef DEBUG
      breakpoint();
#endif

#ifdef __ACT__
      cur->state_ext=MARCEL_RUNNING;
#endif

      MTRACE("Preemption", cur);
    } else {
      SET_BLOCKED(cur);
      next = marcel_unchain_task(cur);
      ACTDEBUG(printf("marcel_give_hand next=%p\n", next)); 

#ifdef SMP
      if(first_time) {
	first_time = FALSE;
	
	marcel_lock_release(lock);
      }
#endif

      call_ST_FLUSH_WINDOWS();
#ifdef __ACT__
      cur->state_ext=MARCEL_READY;
      restart_thread(next);
#else
      longjmp(next->jb, NORMAL_RETURN);
#endif
    }
  } while(*blocked);
  unlock_task();
}

#ifndef __ACT__
void marcel_tempo_give_hand(unsigned long timeout,
			    boolean *blocked, marcel_sem_t *s)
{
  marcel_t next, cur = marcel_self();
  unsigned long ttw = __milliseconds + timeout;

#ifdef SMP
  RAISE(NOT_IMPLEMENTED);
#endif

  if(locked() != 1)
    RAISE(LOCK_TASK_ERROR);
  marcel_disablemigration(cur);
  do {
    if(setjmp(cur->jb) == NORMAL_RETURN) {
#ifdef DEBUG
      breakpoint();
#endif
      MTRACE("Preemption", cur);

      if((*blocked) && __milliseconds >= ttw) {
	/* Expiration timer ou retour d'une deviation : */
	/* le thread n'a pas ete reveille par un sem_V ! */
	cell *pc = NULL,
	     *cc;

	marcel_lock_acquire(&s->lock);

	cc = s->first;
	while(cc->task != cur) {
	  pc = cc;
	  cc = cc->next;
	}
	(s->value)++;
	if(pc == NULL)
            s->first = cc->next;
         else if((pc->next = cc->next) == NULL)
            s->last = pc;

	marcel_lock_release(&s->lock);

	marcel_enablemigration(cur);
	unlock_task();
	RAISE(TIME_OUT);
      }
    } else {

      cur->time_to_wake = ttw;
      SET_SLEEPING(cur);
      next = marcel_unchain_task(cur);

      call_ST_FLUSH_WINDOWS();
      longjmp(next->jb, NORMAL_RETURN);
    }
  } while(*blocked);
  marcel_enablemigration(cur);
  unlock_task();
}
#endif /* __ACT__ */

void marcel_setspecialthread(marcel_t pid)
{
#ifdef MINIMAL_PREEMPTION
  special_thread = pid;
#else
  RAISE(USE_ERROR);
#endif
}

#ifndef __ACT__
void marcel_givehandback(void)
{
#ifdef MINIMAL_PREEMPTION
  marcel_t p = yielding_thread;

  if(p != NULL) {
    yielding_thread = NULL;
    marcel_explicityield(p);
  } else
    marcel_trueyield();
#else
  marcel_trueyield();
#endif
}
#endif /* __ACT__ */

void marcel_delay(unsigned long millisecs)
{
  marcel_t p, cur = marcel_self();
  unsigned long ttw = __milliseconds + millisecs;

  lock_task();

  mdebug("\t\t\t<Thread %p goes sleeping>\n", cur);

  do {
#ifdef __ACT__
    cur->sched_by = SCHED_BY_MARCEL;
    if(setjmp(cur->jb.migr_jb) == NORMAL_RETURN) {
#else
    if(setjmp(cur->jb) == NORMAL_RETURN) {
#endif
#ifdef DEBUG
      breakpoint();
#endif
#ifdef __ACT__
      cur->state_ext=MARCEL_RUNNING;
#endif      
      MTRACE("Preemption", cur);
    } else {

      cur->time_to_wake = ttw;
      SET_SLEEPING(cur);
      p = marcel_unchain_task(cur);

      call_ST_FLUSH_WINDOWS();
#ifdef __ACT__
      cur->state_ext=MARCEL_READY;
      restart_thread(p);
#else
      longjmp(p->jb, NORMAL_RETURN);
#endif
    }

  } while(__milliseconds < ttw);

  unlock_task();
}

int marcel_check_delayed_tasks(boolean from_sched)
{
  unsigned long present = __milliseconds;
  int waked_some_task = 0;
  register marcel_t next;
  poll_struct_t *ps, *p;

  lock_task();

  if(marcel_lock_tryacquire(&__delayed_lock)) {

    /* Delayed tasks */
    next = __delayed_tasks;
    while(next != NULL && next->time_to_wake <= present) {
      register marcel_t t = next;
      /* wake the "next" task */
      mdebug("\t\t\t<Waking thread %p (%ld)>\n", next, next->number);

      next = next->next;
      marcel_wake_task(t, NULL);
      waked_some_task = 1;
    }

    marcel_lock_release(&__delayed_lock);
  }

  if(marcel_lock_tryacquire(&__polling_lock)) {

    /* Polling tasks */
    ps = __polling_tasks;
    if(ps != NULL)
      do {
	if(++ps->count == ps->divisor) {
	  register poll_cell_t *cell;
#ifdef SMP
	  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

	  ps->count = 0;
	  cell = (poll_cell_t *)(*ps->func)(ps,
					    cur_lwp->running_tasks,
					    __sleeping_threads,
					    __blocked_threads);
	  if(cell != MARCEL_POLL_FAILED) {

	    waked_some_task = 1;
	    marcel_wake_task(cell->task, &cell->blocked);

	    /* Retrait de d'un élément de la liste */
	    if(cell->prev != NULL)
	      cell->prev->next = cell->next;
	    else
	      ps->first_cell = cell->next;
	    if(cell->next != NULL)
	      cell->next->prev = cell->prev;

	    if(ps->first_cell != NULL) {
	      /* S'il reste des éléments, alors il faut re-factoriser */
	      (*(ps->gfunc))((marcel_pollid_t)ps);
	    } else {
	      /* Sinon, il faut retirer la tache de __polling_task */
	      if((p = ps->prev) == ps) {
		__polling_tasks = NULL;
		break;
	      } else {
		if(ps == __polling_tasks)
		  __polling_tasks = p;
		p->next = ps->next;
		p->next->prev = p;
	      }
	    }
	  }
	}
	ps = ps->next;
      } while(ps != __polling_tasks);

    marcel_lock_release(&__polling_lock);
  }

  unlock_task();
  return waked_some_task;
}

int marcel_setprio(marcel_t pid, unsigned prio)
{
  unsigned old = pid->prio;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

   if(prio == 0 || prio > MAX_PRIO)
      RAISE(CONSTRAINT_ERROR);
   lock_task();
   if(!IS_RUNNING(pid))
      pid->prio = pid->quantums = prio;
   else if(pid->next == pid) { /* pid == seule tache active */
      cur_lwp->__first[old] = NULL;
      cur_lwp->__first[prio] = pid;
      pid->prio = pid->quantums = prio;
   } else {
     SET_FROZEN(pid);
     marcel_unchain_task(pid);
     pid->prio = pid->quantums = prio;
     marcel_wake_task(pid, NULL);
   }
   unlock_task();
   return 0;
}

int marcel_getprio(marcel_t pid, unsigned *prio)
{
   *prio = pid->prio;
   return 0;
}

#ifdef SMP
static int do_work_stealing(void)
{
  /* Currently not implemented ! */
  return 0;
}
#endif

static sigset_t sigalrmset, sigeptset;
static void start_timer(void);
static void set_timer(void);
void stop_timer(void);

#ifdef __ACT__
/* Each processor must be able to run something */
any_t wait_and_yield(any_t arg)
{
  int nb=0;

  for(;;) {
    if(!((nb++) % 1000000))
      ACTDEBUG(printf("wait_and_yield %p yielding\n", marcel_self()));
    marcel_yield();
  }
}
#endif

/* Except at the beginning, lock_task() is supposed to be permanently
   handled */
any_t sched_func(any_t arg)
{
  marcel_t next, cur = marcel_self();
#ifdef SMP
  __lwp_t *cur_lwp = cur->lwp;
#endif

#ifndef __ACT__
  start_timer(); /* May be redundant for main LWP */
#endif

  lock_task();

  ACTDEBUG(printf("sched_func starting with lock_task\n")); 

  do {

    mdebug("\t\t\t<Scheduler scheduled> (LWP = %d)\n", cur_lwp->number);
    ACTDEBUG(printf("\t\t\t<Scheduler scheduled> (LWP = %d)\n",
		    cur_lwp->number));

#ifdef SMP
    /* Note that the current implementation does not detect DEADLOCKS
       any more when SMP mode is on. This will possibly get fixed in
       future versions. */
    {
      int successful;

      do {

	/* Do I have to stop? */
	if(cur_lwp->has_to_stop) {
	  stop_timer();
	  if(cur_lwp == &__main_lwp)
	    RAISE(PROGRAM_ERROR);
	  else {
	    mdebug("\t\t\t<LWP %d exiting>\n", cur_lwp->number);
	    longjmp(cur_lwp->home_jb, 1);
	  }
	}

	/* Do I have new jobs? */
	if(cur_lwp->has_new_tasks) {
#ifdef DEBUG
	  sched_lock(cur_lwp);
	  mdebug("\tLWP %d has new tasks\n", cur_lwp->number);
	  display_sched_queue(marcel_self());
	  sched_unlock(cur_lwp);
#endif
	  break; /* Exit loop */
	}

	/* Check for delayed tasks and polling stuff to do... */
	successful = marcel_check_delayed_tasks(TRUE);

	/* If previous step unsuccessful, then try to steal threads to
           other LWPs */
	if(!successful)
	  successful = do_work_stealing();

	/* If nothing successful */
	if(!successful)
#ifdef DO_PAUSE_INSTEAD_OF_SCHED_YIELD
	  sigsuspend(&sigeptset);
#else
	  SCHED_YIELD();
#endif

      } while(!successful);
    }
#else
    /* Look if some delayed tasks can be waked */
    if(__delayed_tasks == NULL && __polling_tasks == NULL)
      RAISE(DEADLOCK_ERROR);

    if(__polling_tasks == NULL) {
      while(!marcel_check_delayed_tasks(TRUE))
	pause();
    } else {
      while(!marcel_check_delayed_tasks(TRUE))
	/* True polling ! */ ;
    }
#endif /* SMP */

    SET_FROZEN(cur);
    next = marcel_unchain_task(cur);

    mdebug("\t\t\t<Scheduler unscheduled> (LWP = %d)\n", cur_lwp->number);

#ifdef __ACT__
    cur->sched_by = SCHED_BY_MARCEL;
    if(setjmp(cur->jb.migr_jb) == FIRST_RETURN) {
#else
    if(setjmp(cur->jb) == FIRST_RETURN) {
#endif
      call_ST_FLUSH_WINDOWS();
#ifdef __ACT__
      cur->state_ext=MARCEL_READY;
      restart_thread(next);
#else
      longjmp(next->jb, NORMAL_RETURN);
#endif
    }

#ifdef __ACT__
    cur->state_ext=MARCEL_RUNNING;
#endif

    MTRACE("Preemption", cur);

  } while(1);
}

static void init_lwp(__lwp_t *lwp, marcel_t initial_task)
{
  int i;
  marcel_attr_t attr;

  lwp->number = __nb_lwp++;
  lwp->has_to_stop = FALSE;
  lwp->has_new_tasks = FALSE;
  lwp->sched_queue_lock = MARCEL_LOCK_INIT;
#ifdef X86_ARCH
#ifdef __ACT__
  initial_task->aid=1; //TODO : Hack because I know the first aid is 1 but...
#else
  atomic_set(&lwp->_locked, 0);
#endif
#else
  lwp->_locked = 0;
#endif
  lwp->sec_desc = SECUR_TASK_DESC(lwp);
  marcel_mutex_init(&lwp->stack_mutex, NULL);
  lwp->running_tasks = 0;

  for(i = 1; i <= MAX_PRIO; i++)
    lwp->__first[i] = NULL;

  if(initial_task) {
    lwp->__first[0] = lwp->__first[initial_task->prio] = initial_task;
    initial_task->lwp = lwp;
    initial_task->prev = initial_task->next = initial_task;
  }

  /* Création de la tâche __sched_task (i.e. "Idle") */
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
#ifdef PM2
  {
    char *stack = MALLOC(2*SLOT_SIZE);

    unsigned long stsize = (((unsigned long)(stack + 2*SLOT_SIZE) & 
			     ~(SLOT_SIZE-1)) - (unsigned long)stack);

    marcel_attr_setstackaddr(&attr, stack);
    marcel_attr_setstacksize(&attr, stsize);
  }
#endif
  lock_task();
  if(lwp->number == 0)
    sched_lock(lwp);
  marcel_create(&lwp->sched_task, &attr, sched_func, NULL);
  if(lwp->number == 0)
    sched_unlock(lwp);

  MTRACE("IdleTask", lwp->sched_task);
  SET_FROZEN(lwp->sched_task);
  marcel_unchain_task(lwp->sched_task);

  sched_lock(lwp);

  unlock_task();
}

#if defined(SMP) || defined(__ACT__)
static unsigned __nb_processors = 1;
#endif

#ifdef SMP

static void *lwp_startup_func(void *arg)
{
  __lwp_t *lwp = (__lwp_t *)arg;

  if(setjmp(lwp->home_jb)) {
    mdebug("sched %d Exiting\n", lwp->number);
    pthread_exit(0);
  }

  mdebug("\t\t\t<LWP %d started (pid == %lx)>\n",
	 lwp->number, pthread_self());

#if defined(BIND_LWP_ON_PROCESSORS) && defined(SOLARIS_SYS)
  if(processor_bind(P_LWPID, P_MYID,
		    (processorid_t)(lwp->number % __nb_processors),
		    NULL) != 0) {
    perror("processor_bind");
    exit(1);
  } else
    mdebug("LWP %d bound to processor %d\n",
	   lwp->number, (lwp->number % __nb_processors));
#endif

  lwp->__first[0] = lwp->__first[lwp->sched_task->prio] = lwp->sched_task;
  lwp->sched_task->lwp = lwp;
  lwp->sched_task->previous_lwp = NULL;
  lwp->sched_task->sched_policy = MARCEL_SCHED_FIXED(lwp->number);
  lwp->sched_task->prev = lwp->sched_task->next = lwp->sched_task;

  /* Can't use lock_task() because marcel_self() is not yet usable ! */
#ifdef X86_ARCH
  atomic_inc(&lwp->_locked);
#else
  lwp->_locked++;
#endif

  sched_unlock(lwp);

  SET_RUNNING(lwp->sched_task);
  call_ST_FLUSH_WINDOWS();
  longjmp(lwp->sched_task->jb, NORMAL_RETURN);
  
  return NULL;
}

static void create_new_lwp(void)
{
  __lwp_t *lwp = (__lwp_t *)MALLOC(sizeof(__lwp_t)),
          *cur_lwp = marcel_self()->lwp;

  init_lwp(lwp, NULL);

  /* Add to global linked list */
  lwp->next = cur_lwp;
  lwp->prev = cur_lwp->prev;
  cur_lwp->prev->next = lwp;
  cur_lwp->prev = lwp;
}

static void start_lwp(__lwp_t *lwp)
{
  pthread_attr_t attr;

  /* Start new Pthread */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_create(&lwp->pid, &attr, lwp_startup_func, (void *)lwp);
}
#endif

void marcel_sched_init(unsigned nb_lwp)
{
  marcel_initialized = TRUE;

  sigemptyset(&sigeptset);
  sigemptyset(&sigalrmset);
  sigaddset(&sigalrmset, MARCEL_TIMER_SIGNAL);

  ACTDEBUG(printf("marcel_sched_init(%i)\n", nb_lwp)); 
#if defined(SMP) || defined(__ACT__)

#ifdef SOLARIS_SYS
  __nb_processors = sysconf(_SC_NPROCESSORS_CONF);

#if defined(BIND_LWP_ON_PROCESSORS)
  if(processor_bind(P_LWPID, P_MYID, (processorid_t)0, NULL) != 0) {
    perror("processor_bind");
    exit(1);
  } else
    mdebug("LWP %d bound to processor %d\n", 0, 0);
#endif

#elif defined(LINUX_SYS)

  __nb_processors = WEXITSTATUS(system("exit `grep rocessor /proc/cpuinfo"
				       " | wc -l`"));

#elif defined(IRIX_SYS)

  __nb_processors = sysconf(_SC_NPROC_CONF);

#else
  __nb_processors = 1;
#endif

  mdebug("%d processors available\n", __nb_processors);

#ifdef __ACT__
  _main_struct.nb_tasks = -1-__nb_processors;
#else
  NB_LWPS = (nb_lwp ? nb_lwp : __nb_processors);
  _main_struct.nb_tasks = -NB_LWPS;
#endif
#else
  _main_struct.nb_tasks = -1;
#endif

  memset(__main_thread, 0, sizeof(task_desc));
  __main_thread->detached = FALSE;
  __main_thread->not_migratable = 1;
  __main_thread->prio = __main_thread->quantums = 1;
  /* Some Pthread packages do not support that a
     LWP != main_LWP execute on the main stack, so: */
  __main_thread->sched_policy = MARCEL_SCHED_FIXED(0);

  ACTDEBUG(printf("marcel_sched_init memset done\n")); 

  /* Initialization of "main LWP" (required even when SMP not set). */
  init_lwp(&__main_lwp, __main_thread);
  ACTDEBUG(printf("marcel_sched_init init_lwp done\n")); 

  __main_lwp.next = __main_lwp.prev = &__main_lwp;
  __main_lwp.sched_task->sched_policy = MARCEL_SCHED_FIXED(0);
  __main_lwp.sched_task->previous_lwp = NULL;

  SET_RUNNING(__main_thread);
  one_more_active_task(__main_thread, &__main_lwp);
  MTRACE("MainTask", __main_thread);

  ACTDEBUG(printf("marcel_sched_init one task done\n")); 

  sched_unlock(&__main_lwp);

  ACTDEBUG(printf("marcel_sched_init sched_unlock done\n")); 

#ifdef SMP

  /* Creation en deux phases en prevision du work stealing qui peut
     demarrer tres vite sur certains LWP... */
  {
    int i;
    __lwp_t *lwp;

    for(i=1; i<NB_LWPS; i++)
      create_new_lwp();

    for(lwp = __main_lwp.next; lwp != &__main_lwp; lwp = lwp->next)
      start_lwp(lwp);
  }
#endif

#ifdef __ACT__
  {
    int i;
    for(i=0; i<__nb_processors; i++) { /* TODO : autant que de processeurs */
      marcel_create(NULL, NULL, wait_and_yield, NULL);
    }
  }
#else
  /* Démarrage d'un timer Unix pour récupérer périodiquement 
     un signal (par ex. SIGALRM). */
  start_timer();
#endif
  ACTDEBUG(printf("marcel_sched_init done\n")); 
}

#ifdef SMP
static void stop_lwp(__lwp_t *lwp)
{
  int cc;

  lwp->has_to_stop = TRUE;
  /* Join corresponding Pthread */
  do {
    cc = pthread_join(lwp->pid, NULL);
  } while(cc == -1 && errno == EINTR);

  /* TODO: the stack of the lwp->sched_task is currently *NOT FREED*.
     This should be fixed! */
}
#endif

void marcel_sched_shutdown()
{
#ifdef SMP
  __lwp_t *lwp;
#endif

  wait_all_tasks_end();

  stop_timer();

#ifdef SMP
  if(marcel_self()->lwp != &__main_lwp)
    RAISE(PROGRAM_ERROR);
  lwp = __main_lwp.next;
  while(lwp != &__main_lwp) {
    stop_lwp(lwp);
    lwp = lwp->next;
    FREE(lwp->prev);
  }
#else
  /* Destroy master-sched's stack */
  marcel_cancel(__main_lwp.sched_task);
#ifdef PM2
  /* __sched_task is detached, so we can free its stack now */
  FREE(marcel_stackbase(__main_lwp.sched_task));
#endif
#endif  
}

/* TODO: Call lock_task() before re-enabling the signals to avoid stack
   overflow by recursive interrupts handlers. This needs a modified version
   of marcel_yield() that do not lock_task()... */
static void timer_interrupt(int sig)
{
  marcel_t cur = marcel_self();

  if(cur->lwp == &__main_lwp)
    __milliseconds += time_slice/1000;

  if(!locked() && time_slice_enabled) {

    MTRACE("TimerSig", cur);

    cur->in_sighandler = TRUE;

    marcel_check_delayed_tasks(FALSE);

#ifdef SMP
    pthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
    sigrelse(MARCEL_TIMER_SIGNAL);
#else
    sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif

#ifdef MINIMAL_PREEMPTION
    if(special_thread != NULL && special_thread != cur) {
      yielding_thread = cur;
      marcel_explicityield(special_thread);
    }
#else
    marcel_yield();
#endif
    cur->in_sighandler = FALSE;
  }
}

static void set_timer(void)
{
  struct itimerval value;

#ifdef SMP
    pthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif

  if(marcel_initialized) {
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = time_slice;
    value.it_value = value.it_interval;
    setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
  }
}

static void start_timer(void)
{
  static struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = timer_interrupt;
  sa.sa_flags = SA_RESTART;

  sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);

  set_timer();
}

void stop_timer(void)
{
  time_slice = 0;
  set_timer();
}

void marcel_settimeslice(unsigned long microsecs)
{
  lock_task();
  if(microsecs == 0) {
    time_slice_enabled = FALSE;
    if(time_slice != DEFAULT_TIME_SLICE) {
      time_slice = DEFAULT_TIME_SLICE;
      set_timer();
    }
  } else {
    time_slice_enabled = TRUE;
    if(microsecs < MIN_TIME_SLICE) {
      time_slice = MIN_TIME_SLICE;
      set_timer();
    } else if(microsecs != time_slice) {
      time_slice = microsecs;
      set_timer();
    }
  }
  unlock_task();
}

unsigned long marcel_clock(void)
{
   return __milliseconds;
}

void marcel_snapshot(snapshot_func_t f)
{
  marcel_t t;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

  lock_task();

  t = cur_lwp->__first[0];
  do {
    (*f)(t);
    t = t->next;
  } while(t != cur_lwp->__first[0]);

  if(__delayed_tasks != NULL) {
    t = __delayed_tasks;
    do {
      (*f)(t);
      t = t->next;
    } while(t != NULL);
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  if(__waiting_tasks != NULL) {
    t = __waiting_tasks;
    do {
      (*f)(t);
      t = t->next;
    } while(t != __waiting_tasks);
  }

#endif

  unlock_task();
}

void marcel_threadslist(int max, marcel_t *pids, int *nb, int which)
{
  marcel_t t;
  int nb_pids = 0;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

  if( ((which & MIGRATABLE_ONLY) && (which & NOT_MIGRATABLE_ONLY)) ||
      ((which & DETACHED_ONLY) && (which & NOT_DETACHED_ONLY)) ||
      ((which & BLOCKED_ONLY) && (which & NOT_BLOCKED_ONLY)) ||
      ((which & SLEEPING_ONLY) && (which & NOT_SLEEPING_ONLY)))
    RAISE(CONSTRAINT_ERROR);

  lock_task();

  t = cur_lwp->__first[0];
  do {

    if(t->detached) {
      if(which & NOT_DETACHED_ONLY)
	continue;
    } else if(which & DETACHED_ONLY)
      continue;

    if(t->not_migratable) {
      if(which & MIGRATABLE_ONLY)
	continue;
    } else if(which & NOT_MIGRATABLE_ONLY)
      continue;

    if(IS_BLOCKED(t)) {
      if(which & NOT_BLOCKED_ONLY)
	continue;
    } else if(which & BLOCKED_ONLY)
      continue;

    if(IS_SLEEPING(t)) {
      if(which & NOT_SLEEPING_ONLY)
	continue;
    } else if(which & SLEEPING_ONLY)
      continue;

    if(nb_pids < max)
      pids[nb_pids++] = t;
    else
      nb_pids++;

  } while(t = t->next, t != cur_lwp->__first[0]);

  if(__delayed_tasks != NULL) {
    t = __delayed_tasks;
    do {

    if(t->detached) {
      if(which & NOT_DETACHED_ONLY)
	continue;
    } else if(which & DETACHED_ONLY)
      continue;

    if(t->not_migratable) {
      if(which & MIGRATABLE_ONLY)
	continue;
    } else if(which & NOT_MIGRATABLE_ONLY)
      continue;

    if(IS_BLOCKED(t)) {
      if(which & NOT_BLOCKED_ONLY)
	continue;
    } else if(which & BLOCKED_ONLY)
      continue;

    if(IS_SLEEPING(t)) {
      if(which & NOT_SLEEPING_ONLY)
	continue;
    } else if(which & SLEEPING_ONLY)
      continue;

    if(nb_pids < max)
      pids[nb_pids++] = t;
    else
      nb_pids++;

    } while(t = t->next, t != NULL);
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  if(__waiting_tasks != NULL) {
    t = __waiting_tasks;
    do {
      if(t->detached) {
	if(which & NOT_DETACHED_ONLY)
	  continue;
      } else if(which & DETACHED_ONLY)
	continue;

      if(t->not_migratable) {
	if(which & MIGRATABLE_ONLY)
	  continue;
      } else if(which & NOT_MIGRATABLE_ONLY)
	continue;

      if(IS_BLOCKED(t)) {
	if(which & NOT_BLOCKED_ONLY)
	  continue;
      } else if(which & BLOCKED_ONLY)
	continue;

      if(IS_SLEEPING(t)) {
	if(which & NOT_SLEEPING_ONLY)
	  continue;
      } else if(which & SLEEPING_ONLY)
	continue;

      if(nb_pids < max)
	pids[nb_pids++] = t;
      else
	nb_pids++;

    } while(t = t->next, t != __waiting_tasks);
  }

#endif

  *nb = nb_pids;
  unlock_task();
}

marcel_pollid_t marcel_pollid_create(marcel_pollgroup_func_t g,
				     marcel_poll_func_t f,
				     int divisor)
{
  marcel_pollid_t id;

  lock_task();
  if(nb_poll_structs == MAX_POLL_IDS) {
    unlock_task();
    RAISE(CONSTRAINT_ERROR);
  }
  id = &poll_structs[nb_poll_structs++];
  unlock_task();

  id->first_cell = id->cur_cell = NULL;
  id->gfunc = g;
  id->func = f;
  id->divisor = divisor;
  id->count = 0;

  return id;
}

void marcel_poll(marcel_pollid_t id, any_t arg)
{
  poll_cell_t cell;

  lock_task();

  cell.task = marcel_self();
  cell.blocked = TRUE;
  cell.arg = arg;

  marcel_lock_acquire(&__polling_lock);

  /* Insertion dans la liste des taches sur le meme id */
  cell.prev = NULL;
  if((cell.next = id->first_cell) != NULL)
    cell.next->prev = &cell;
  id->first_cell = &cell;

  /* Factorisation "utilisateur" du polling */
  (*(id->gfunc))(id);

  /* Enregistrement éventuel dans la __polling_tasks */
  if(cell.next == NULL) {
    /* Il n'y avait pas encore un polling de ce type */
    if(__polling_tasks == NULL) {
      __polling_tasks = id;
      id->next = id->prev = id;
    } else {
      id->next = __polling_tasks;
      id->prev = __polling_tasks->prev;
      id->prev->next = __polling_tasks->prev = id;
    }
  }

  marcel_give_hand(&cell.blocked, &__polling_lock);
}

