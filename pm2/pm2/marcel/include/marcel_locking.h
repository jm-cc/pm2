
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


/****************************************************************
 * Pour la compatibilit� avec marcel_lock_*
 */

#section macros
#define MARCEL_LOCK_INIT MA_SPIN_LOCK_UNLOCKED

#section types
#depend "linux_spinlock.h[marcel_types]"
typedef ma_spinlock_t marcel_lock_t;

#section marcel_macros
#define marcel_lock_init(l)       _ma_raw_spin_init(l)
#define marcel_lock_acquire(l)    _ma_raw_spin_lock(l) 
#define marcel_lock_tryacquire(l) _ma_raw_spin_trylock(l)
#define marcel_lock_release(l)    _ma_raw_spin_unlock(l)
#define marcel_lock_locked(l)     _ma_raw_spin_is_locked(l)

/****************************************************************
 * Pour la compatibilit� avec lock_task et assimil�
 */

#section marcel_macros
#define lock_task() ma_preempt_disable()
#define unlock_task() ma_preempt_enable()
#define locked() ma_preempt_count()


#section marcel_functions
/*
#ifdef MA_PROTECT_LOCK_TASK_FROM_SIG
void ma_sched_protect_start(void);
void ma_lock_task(void);
void ma_unlock_task(void);
void ma_sched_protect_end(void);
#else
static __inline__ void ma_lock_task(void);
static __inline__ void ma_unlock_task(void);
#endif
*/

#section marcel_inline
/*
#ifndef MA_PROTECT_LOCK_TASK_FROM_SIG
static __inline__ void ma_lock_task(void)
{
  atomic_inc(&marcel_self()->_locked);
}

static __inline__ void ma_unlock_task(void)
{
  marcel_t cur __attribute__ ((unused)) = marcel_self();

  if(atomic_read(&cur->_locked) == 1) {

#ifdef MARCEL_RT
    if(__rt_task_exist && !MA_TASK_REAL_TIME(cur))
      ma__marcel_find_and_yield_to_rt_task();
#endif

#ifdef MA__WORK
    if(cur->has_work)
      do_work(cur);
#endif
  }

  atomic_dec(&cur->_locked);

#if defined(PM2DEBUG) && defined(MA__ACTIVATION)
  pm2debug_flush();
#endif
}
#endif
*/

#section marcel_macros
// Il faut �viter de placer les appels � '*lock_task_debug' au sein
// des fonctions elles-m�mes, car il y aurait un pb de r�cursivit�
// dans tbx_debug...
/*
#ifdef DEBUG_LOCK_TASK
#define lock_task()    \
  do { \
    lock_task_debug("\t=> lock %i++\n", locked()); \
    ma_lock_task(); \
  } while(0)
#define unlock_task() \
  do { \
    ma_unlock_task(); \
    lock_task_debug("\t=> unlock --%i\n", locked()); \
  } while(0)
#else
#define lock_task()    ma_lock_task()
#define unlock_task()  ma_unlock_task()
#endif
*/

#section marcel_functions
//static __inline__ void unlock_task_for_debug(void);
#section marcel_inline
/*
static __inline__ void unlock_task_for_debug(void)
{
  atomic_dec(&marcel_self()->_locked);
}
*/
