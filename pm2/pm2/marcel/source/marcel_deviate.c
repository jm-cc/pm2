
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

#define MAX_RECORDS    1024

static deviate_record_t deviate_records[MAX_RECORDS];

static deviate_record_t *next_free = NULL;
static unsigned next_unalloc = 0;

static marcel_lock_t deviate_lock = MARCEL_LOCK_INIT;

// Attention: cette fonction n'est pas directement thread-safe
static deviate_record_t *deviate_record_alloc(void)
{
  deviate_record_t *res;

  if(next_free != NULL) {
    res = next_free;
    next_free = next_free->next;
  } else {
    if(next_unalloc == MAX_RECORDS)
      RAISE(CONSTRAINT_ERROR);
    res = &deviate_records[next_unalloc];
    next_unalloc++;
  }

  return res;
}

// Attention: cette fonction n'est pas directement thread-safe
static void deviate_record_free(deviate_record_t *rec)
{
  rec->next = next_free;
  next_free = rec;
}

// locked() == 1 et marcel_lock_locked(deviate_lock) == 1
static void marcel_deviate_record(marcel_t pid, handler_func_t h, any_t arg)
{
  deviate_record_t *ptr = deviate_record_alloc();

  ptr->func = h;
  ptr->arg = arg;

  ptr->next = pid->work.deviate_work;
  pid->work.deviate_work = ptr;

#ifdef MA__WORK
  SET_DEVIATE_WORK(pid);
#endif
}

// locked() == 1 et marcel_lock_locked(deviate_lock) == 1
static void do_execute_deviate_work(void)
{
  marcel_t cur = marcel_self();
  deviate_record_t *ptr;

  while((ptr = cur->work.deviate_work) != NULL) {
    handler_func_t h = ptr->func;
    any_t arg = ptr->arg;

    cur->work.deviate_work = ptr->next;
    deviate_record_free(ptr);

    marcel_lock_release(&deviate_lock);
    unlock_task();

    (*h)(arg);

    lock_task();
    marcel_lock_acquire(&deviate_lock);
  }

#ifdef MA__WORK
  CLR_DEVIATE_WORK(cur);
#endif
}

// locked() == 1 lorsque l'on exécute cette fonction
void marcel_execute_deviate_work(void)
{
  if(!marcel_self()->not_deviatable) {
    marcel_lock_acquire(&deviate_lock);

    do_execute_deviate_work();

    marcel_lock_release(&deviate_lock);
  }
}

#ifdef MA__ACTIVATION
#warning activations non gérées
#endif
static void insertion_relai(handler_func_t f, void *arg)
{ 
  marcel_ctx_t back;
  marcel_t cur = marcel_self();

  memcpy(back, cur->ctx_yield, sizeof(marcel_ctx_t));

  if(MA_THR_SETJMP(cur) == FIRST_RETURN) {
    marcel_ctx_longjmp(cur->father->ctx_yield, NORMAL_RETURN);
  } else {
    MA_THR_RESTARTED(cur, "Deviation");
    MA_BUG_ON(!ma_in_atomic());
    unlock_task();

    (*f)(arg);

    lock_task();
    marcel_ctx_longjmp(back, NORMAL_RETURN);
  }
}

/* VERY INELEGANT: to avoid inlining of insertion_relai function... */
typedef void (*relai_func_t)(handler_func_t f, void *arg);
static volatile relai_func_t relai_func = insertion_relai;

void marcel_do_deviate(marcel_t pid, handler_func_t h, any_t arg)
{
  static volatile handler_func_t f_to_call;
  static void * volatile argument;
  static volatile long initial_sp;

  if(marcel_ctx_setjmp(marcel_self()->ctx_yield) == FIRST_RETURN) {
    f_to_call = h;
    argument = arg;

    pid->father = marcel_self();

    initial_sp = MAL_BOT((unsigned long)marcel_ctx_get_sp(pid->ctx_yield)) -
      TOP_STACK_FREE_AREA - 256;

    marcel_ctx_switch_stack(marcel_self(), pid, initial_sp);

    (*relai_func)(f_to_call, argument);

    RAISE(PROGRAM_ERROR); // on ne doit jamais arriver ici !
  }
}

void marcel_deviate(marcel_t pid, handler_func_t h, any_t arg)
{ 
  LOG_IN();

  lock_task();
  if (pid == marcel_self()) {
    if (pid->not_deviatable) {
      marcel_lock_acquire(&deviate_lock);
      marcel_deviate_record(pid, h, arg);
      marcel_lock_release(&deviate_lock);
      unlock_task();
    } else {
      unlock_task();
      (*h)(arg);
    }
    LOG_OUT();
    return;
  }
  // On prend ce verrou très tôt pour s'assurer que la tâche cible ne
  // progresse pas au-delà d'un 'disable_deviation' pendant qu'on
  // l'inspecte...
  marcel_lock_acquire(&deviate_lock);
  if (pid->not_deviatable) {
    // Le thread n'est pas "déviable" en ce moment...

    marcel_deviate_record(pid, h, arg);

    marcel_lock_release(&deviate_lock);
    unlock_task();

    LOG_OUT();
    return;
  }
#if 0
  // Idée abandonnée
  if (marcel_sched_try_deviate(pid, h, arg)) {
    marcel_lock_release(&deviate_lock);
    unlock_task();
    LOG_OUT();
    return;
  }
#endif

  // En premier lieu, il faut empêcher la tâche 'cible' de changer
  // d'état
  //state_lock(pid);

#ifdef MA__WORK
  marcel_deviate_record(pid, h, arg);
  
  marcel_lock_release(&deviate_lock);
  unlock_task();

  ma_wake_up_state(pid,MA_TASK_STOPPED|MA_TASK_INTERRUPTIBLE|MA_TASK_FROZEN);
#ifdef MA__SMP
  // Heuristique: on va essayer d'accélérer les choses...
#warning TODO: deroute() dérègle l horloge
  // TODO: chercher plutôt un lwp qui atteint la runqueue...
  ma_lwp_t target_lwp = GET_LWP(pid);
  if (target_lwp!=LWP_SELF)
    marcel_kthread_kill(target_lwp->pid, MARCEL_TIMER_SIGNAL);
#endif

  LOG_OUT();
  return;
#else
	// Tant pis !
  RAISE(NOT_IMPLEMENTED);
#endif
    // La tâche n'est pas dans la file des threads prêts. On va donc
    // l'y insérer. ATTENTION: il faut éviter qu'un autre LWP 'vole'
    // la tâche avant que l'on ait pu la dévier. Il suffit pour cela
    // de la dévier _avant_ sa réinsertion...
    //marcel_do_deviate(pid, h, arg);

    //marcel_wake_up_thread(pid);

  LOG_OUT();
}

void marcel_enable_deviation(void)
{
  marcel_t cur = marcel_self();

  lock_task();

  marcel_lock_acquire(&deviate_lock);

  if(--cur->not_deviatable == 0)
    do_execute_deviate_work();

  marcel_lock_release(&deviate_lock);

  unlock_task();
}

void marcel_disable_deviation(void)
{
  lock_task();

  marcel_lock_acquire(&deviate_lock);

  ++marcel_self()->not_deviatable;

  marcel_lock_release(&deviate_lock);

  unlock_task();
}
