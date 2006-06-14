
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

static ma_allocator_t * deviate_records;
static marcel_lock_t deviate_lock = MARCEL_LOCK_INIT;

void __marcel_init ma_deviate_init(void) {
  deviate_records = ma_new_obj_allocator(0,
		      ma_obj_allocator_malloc, (void*) sizeof(ma_allocator_t),
		      ma_obj_allocator_free, NULL,
		      POLICY_HIERARCHICAL, 0);
}

// Attention: cette fonction n'est pas directement thread-safe
#define deviate_record_alloc() ma_obj_alloc(deviate_records)
#define deviate_record_free(rec) ma_obj_free(deviate_records, rec)

// pr�emption d�sactiv�e et marcel_lock_locked(deviate_lock) == 1
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

// pr�emption d�sactiv�e et marcel_lock_locked(deviate_lock) == 1
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
    ma_preempt_enable();

    (*h)(arg);

    ma_preempt_disable();
    marcel_lock_acquire(&deviate_lock);
  }

#ifdef MA__WORK
  CLR_DEVIATE_WORK(cur);
#endif
}

// pr�emption d�sactiv�e lorsque l'on ex�cute cette fonction
void marcel_execute_deviate_work(void)
{
  if(!SELF_GETMEM(not_deviatable)) {
    marcel_lock_acquire(&deviate_lock);

    do_execute_deviate_work();

    marcel_lock_release(&deviate_lock);
  }
}

#ifdef MA__ACTIVATION
#ifdef PM2_DEV
#warning activations non g�r�es
#endif
#endif
static void insertion_relai(handler_func_t f, void *arg)
{ 
  marcel_ctx_t back;
  marcel_t cur = marcel_self();

  memcpy(back, cur->ctx_yield, sizeof(marcel_ctx_t));

  if(MA_THR_SETJMP(cur) == FIRST_RETURN) {
    marcel_ctx_longjmp(cur->father->ctx_yield, NORMAL_RETURN);
  } else {
    MA_THR_DESTROYJMP(cur);
    MA_THR_RESTARTED(cur, "Deviation");
    MA_BUG_ON(!ma_in_atomic());
    ma_preempt_enable();

    (*f)(arg);

    ma_preempt_disable();
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

  if(marcel_ctx_setjmp(SELF_GETMEM(ctx_yield)) == FIRST_RETURN) {
    f_to_call = h;
    argument = arg;

    pid->father = marcel_self();

    initial_sp = MAL_BOT((unsigned long)marcel_ctx_get_sp(pid->ctx_yield)) -
      TOP_STACK_FREE_AREA - 256;

    marcel_ctx_switch_stack(marcel_self(), pid, initial_sp, &arg);

    (*relai_func)(f_to_call, argument);

    MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR); // on ne doit jamais arriver ici !
  }
  marcel_ctx_destroyjmp(SELF_GETMEM(ctx_yield));
}

void marcel_deviate(marcel_t pid, handler_func_t h, any_t arg)
{ 
  LOG_IN();

  ma_preempt_disable();
  if (pid == marcel_self()) {
    if (pid->not_deviatable) {
      marcel_lock_acquire(&deviate_lock);
      marcel_deviate_record(pid, h, arg);
      marcel_lock_release(&deviate_lock);
      ma_preempt_enable();
    } else {
      ma_preempt_enable();
      (*h)(arg);
    }
    LOG_OUT();
    return;
  }
  // On prend ce verrou tr�s t�t pour s'assurer que la t�che cible ne
  // progresse pas au-del� d'un 'disable_deviation' pendant qu'on
  // l'inspecte...
  marcel_lock_acquire(&deviate_lock);
  if (pid->not_deviatable) {
    // Le thread n'est pas "d�viable" en ce moment...

    marcel_deviate_record(pid, h, arg);

    marcel_lock_release(&deviate_lock);
    ma_preempt_enable();

    LOG_OUT();
    return;
  }

  // En premier lieu, il faut emp�cher la t�che 'cible' de changer
  // d'�tat
  //state_lock(pid);

  // TODO: quand le thread n'est pas actif, utiliser marcel_do_deviate()
  // (plus efficace)
#ifdef MA__WORK
  marcel_deviate_record(pid, h, arg);
  
  marcel_lock_release(&deviate_lock);
  ma_preempt_enable();

  ma_wake_up_state(pid,MA_TASK_INTERRUPTIBLE|MA_TASK_FROZEN);

  LOG_OUT();
  return;
#else
	// Tant pis !
  MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
#endif

  LOG_OUT();
}

void marcel_enable_deviation(void)
{
  marcel_t cur = marcel_self();

  ma_preempt_disable();
  marcel_lock_acquire(&deviate_lock);

  if(--cur->not_deviatable == 0)
    do_execute_deviate_work();

  marcel_lock_release(&deviate_lock);
  ma_preempt_enable();
}

void marcel_disable_deviation(void)
{
  ma_preempt_disable();
  marcel_lock_acquire(&deviate_lock);

  ++SELF_GETMEM(not_deviatable);

  marcel_lock_release(&deviate_lock);
  ma_preempt_enable();
}
