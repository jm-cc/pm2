
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

#include "sys/marcel_sig.h"
#include "marcel.h"

#define MIN_TIME_SLICE		10000
#define DEFAULT_TIME_SLICE	MIN_TIME_SLICE

// Fréquence des messages de debug dans timer_interrupt
#ifndef TICK_RATE
#define TICK_RATE 1
#endif

// l'horloge de marcel
static volatile unsigned long __milliseconds = 0;

static boolean __marcel_sig_initialized = FALSE;

static volatile unsigned long time_slice = DEFAULT_TIME_SLICE;
static sigset_t sigalrmset, sigeptset;

// Fonction appelée à chaque fois que SIGALRM est délivré au LWP
// courant
static void timer_interrupt(int sig)
{
  marcel_t cur = marcel_self();
#ifdef MARCEL_DEBUG
  static unsigned long tick = 0;
#endif

#ifdef MARCEL_DEBUG
  if (++tick == TICK_RATE) {
    try_mdebug("\t\t\t<<tick>>\n");
    tick = 0;
  }
#endif

  marcel_update_time(cur);

#ifdef PM2DEBUG
  if(GET_LWP(cur) == NULL) {
    fprintf(stderr, "WARNING!!! GET_LWP(%p) == NULL!\n", cur);
  }
#endif

  if(!locked() && preemption_enabled()) {

    LOG_IN();

    MTRACE_TIMER("TimerSig", cur);

    lock_task();
    marcel_check_sleeping();
    marcel_check_polling(MARCEL_POLL_AT_TIMER_SIG);

#ifdef MA__SMP
    marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
    sigrelse(MARCEL_TIMER_SIGNAL);
#else
    sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif

    ma__marcel_yield();

    unlock_task();

    LOG_OUT();
  }
}

#ifdef MA_PROTECT_LOCK_TASK_FROM_SIG
#if defined(LINUX_SYS) && ( defined(X86_ARCH) || defined(ALPHA_ARCH) )
#include <asm/sigcontext.h>
static void timer_interrupt_protect(int sig, struct sigcontext context)
//#elif defined(SOLARIS_SYS) || defined(IRIX_SYS)
//static void timer_interrupt_protect(int sig, siginfo_t *siginfo , void *p)
//#elif defined(AIX_SYS) && defined(RS6K_ARCH)
//static void timer_interrupt_protect(int sig, int Code, struct sigcontext *SCP)
#else
#error MA_PROTECT_LOCK_TASK_FROM_SIG is not yet implemented on that system! \
  Sorry.
#endif
{
  unsigned long pc;

#if defined(LINUX_SYS) && defined(X86_ARCH)
  pc = context.eip;
#elif defined(LINUX_SYS) && defined(ALPHA_ARCH)
  pc = context.sc_pc;
#endif

  if ((pc < (unsigned long)ma_sched_protect_start) || 
      ((unsigned long)ma_sched_protect_end < pc)) {
    timer_interrupt(sig);
  }
}
#endif

// Trappeur de SIGSEGV, de manière à obtenir des indications sur le
// thread fautif (ça aide pour le debug !)
static void fault_catcher(int sig)
{
  marcel_t cur = marcel_self();

  fprintf(stderr, "OOPS!!! Signal %d catched on thread %p (%ld)\n",
	  sig, cur, cur->number);
  if(GET_LWP(cur) != NULL)
    fprintf(stderr, "OOPS!!! current lwp is %d\n", GET_LWP(cur)->number);

#if defined(LINUX_SYS) && defined(MA__LWPS)
  fprintf(stderr,
	  "OOPS!!! Entering endless loop "
	  "so you can try to attach process to gdb (pid = %d)\n",
	  getpid());
  for(;;) ;
#endif

  abort();
}

void marcel_sig_init(void)
{
  sigemptyset(&sigeptset);
  sigemptyset(&sigalrmset);
  sigaddset(&sigalrmset, MARCEL_TIMER_SIGNAL);

  __marcel_sig_initialized = TRUE;
}

int vince_dummy = 0;

void marcel_sig_exit(void)
{
#ifdef MA__TIMER
  struct sigaction sa;

  vince_dummy = 1;

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_DFL;
  sa.sa_flags = 0;
  sigaction(SIGSEGV, &sa, (struct sigaction *)NULL);

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_DFL;
  sa.sa_flags = 0;

  sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);
#endif
}

void marcel_sig_pause(void)
{
#ifndef USE_VIRTUAL_TIMER
  sigsuspend(&sigeptset);
#else
  SCHED_YIELD();
#endif
}

void marcel_sig_reset_timer(void)
{
#ifdef MA__TIMER
  struct itimerval value;

  LOG_IN();

  marcel_sig_enable_interrupts();

  if(__marcel_sig_initialized) {
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = time_slice;
    value.it_value = value.it_interval;
    setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
  }

  LOG_OUT();
#endif
}

void marcel_sig_start_timer(void)
{
#ifdef MA__TIMER
  static struct sigaction sa;

  LOG_IN();

  // On va essayer de rattraper SIGSEGV, etc.
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = fault_catcher;
  sa.sa_flags = 0;
  sigaction(SIGSEGV, &sa, (struct sigaction *)NULL);

  sigemptyset(&sa.sa_mask);
#ifdef MA_PROTECT_LOCK_TASK_FROM_SIG
  sa.sa_handler = (void *)timer_interrupt_protect;
#else
  sa.sa_handler = timer_interrupt;
#endif
#if !defined(WIN_SYS)
  sa.sa_flags = SA_RESTART;
#else
  sa.sa_flags = 0;
#endif

  sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);

  marcel_sig_reset_timer();

  LOG_OUT();
#endif
}

void marcel_sig_enable_interrupts(void)
{
#ifdef MA__TIMER
#ifdef MA__SMP
  marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
  sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif
}

void marcel_sig_disable_interrupts(void)
{
#ifdef MA__TIMER
#ifdef MA__SMP
  marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#else
  sigprocmask(SIG_BLOCK, &sigalrmset, NULL);
#endif
#endif
}

void marcel_sig_stop_timer(void)
{
#ifdef MA__TIMER
  LOG_IN();

  time_slice = 0;

  marcel_sig_reset_timer();

  marcel_sig_disable_interrupts();

  LOG_OUT();
#endif
}

void marcel_update_time(marcel_t cur)
{
  if(GET_LWP(cur)->number == 0)
    __milliseconds += time_slice/1000;
}

void marcel_settimeslice(unsigned long microsecs)
{
  LOG_IN();

  lock_task();

  if(microsecs == 0) {
    disable_preemption();
    if(time_slice != DEFAULT_TIME_SLICE) {
      time_slice = DEFAULT_TIME_SLICE;
      marcel_sig_reset_timer();
    }
  } else {
    enable_preemption();
    if(microsecs < MIN_TIME_SLICE) {
      time_slice = MIN_TIME_SLICE;
      marcel_sig_reset_timer();
    } else if(microsecs != time_slice) {
      time_slice = microsecs;
      marcel_sig_reset_timer();
    }
  }
  unlock_task();

  LOG_OUT();
}

unsigned long marcel_clock(void)
{
   return __milliseconds;
}
