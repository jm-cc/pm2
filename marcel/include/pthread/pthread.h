/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006, 2008, 2009 "the PM2 team" (see AUTHORS file)
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

#ifndef _MARCEL_PTHREAD_H
#define _MARCEL_PTHREAD_H

#ifdef __MARCEL_H__
#error "When this file is in the include path, marcel.h mustn't be #included directly. Always just #include <pthread.h>."
#endif
#include <marcel.h>
#include <stdlib.h>


#ifdef __cplusplus

/* C++ replacements for the new/delete operator family.  */

#include <new>
#include <cstddef>

/* The code below needs to access libc's `malloc ()' and `free ()'.  */
#undef malloc
#undef free

extern "C++" {

	inline void *operator   new(size_t _size) throw(std::bad_alloc) {
		void *mem;

		 mem = malloc(_size);
		if (tbx_unlikely(mem == NULL))
			throw std::bad_alloc();

		 return mem;
	}
	inline void *operator   new(size_t _size, const std::nothrow_t & _nt) throw() {
		return malloc(_size);
	}
	inline void *operator   new[] (size_t _size) throw(std::bad_alloc) {
		void *mem;

		mem = malloc(_size);
		if (tbx_unlikely(mem == NULL))
			throw std::bad_alloc();

		return mem;
	}
	inline void *operator   new[] (size_t _size, const std::nothrow_t & _nt) throw() {
		return malloc(_size);
	}

	inline void
	operator   delete(void *mem) throw() {
		free(mem);
	}
	inline void
	operator   delete(void *mem, const std::nothrow_t & _nt) throw() {
		free(mem);
	}
	inline void
	operator   delete[] (void *mem) throw() {
		free(mem);
	}
	inline void
	operator   delete[] (void *mem, const std::nothrow_t & _nt) throw() {
		free(mem);
	}
}

#endif				/* __cplusplus */


#ifndef MA__IFACE_PMARCEL
#warning "This file can't be used without the pmarcel option in the flavor"
#endif

/* typage */

#undef pthread_t
#define pthread_t pmarcel_t

#undef pthread_attr_t
#define pthread_attr_t pmarcel_attr_t

#undef pthread_barrier_t
#define pthread_barrier_t pmarcel_barrier_t

#undef pthread_barrierattr_t
#define pthread_barrierattr_t pmarcel_barrierattr_t

#undef pthread_mutex_t
#define pthread_mutex_t pmarcel_mutex_t

#undef pthread_mutexattr_t
#define pthread_mutexattr_t pmarcel_mutexattr_t

#undef pthread_spinlock_t
#define pthread_spinlock_t pmarcel_spinlock_t

#undef pthread_rwlock_t
#define pthread_rwlock_t pmarcel_rwlock_t

#undef pthread_rwlockattr_t
#define pthread_rwlockattr_t pmarcel_rwlockattr_t

#undef pthread_cond_t
#define pthread_cond_t pmarcel_cond_t

#undef pthread_condattr_t
#define pthread_condattr_t pmarcel_condattr_t

#undef pthread_key_t
#define pthread_key_t pmarcel_key_t

#undef pthread_once_t
#define pthread_once_t pmarcel_once_t

#undef sigaction
#define sigaction marcel_sigaction
#undef sa_handler
#define sa_handler marcel_sa_handler
#undef sa_mask
#define sa_mask marcel_sa_mask
#undef sa_flags
#define sa_flags marcel_sa_flags
#undef sa_sigaction
#define sa_sigaction marcel_sa_sigaction

#undef sigset_t
#define sigset_t pmarcel_sigset_t

#undef sem_t
#define sem_t pmarcel_sem_t

#undef SEM_FAILED
#define SEM_FAILED PMARCEL_SEM_FAILED

#undef cpu_set_t
#define cpu_set_t pmarcel_cpu_set_t

/* macros */

#undef PTHREAD_ATTR_INITIALIZER
#define PTHREAD_ATTR_INITIALIZER PMARCEL_ATTR_INITIALIZER
#undef PTHREAD_ATTR_DESTROYER
#define PTHREAD_ATTR_DESTROYER PMARCEL_ATTR_DESTROYER

#undef PTHREAD_MUTEX_DEFAULT
#define PTHREAD_MUTEX_DEFAULT PMARCEL_MUTEX_DEFAULT
#undef PTHREAD_MUTEX_ERRORCHECK
#define PTHREAD_MUTEX_ERRORCHECK PMARCEL_MUTEX_ERRORCHECK
#undef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER PMARCEL_MUTEX_INITIALIZER
#undef PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_NORMAL PMARCEL_MUTEX_NORMAL
#undef PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_RECURSIVE PMARCEL_MUTEX_RECURSIVE

#undef PTHREAD_ONCE_INIT
#define PTHREAD_ONCE_INIT PMARCEL_ONCE_INIT

#undef SEM_INITIALIZER
#define SEM_INITIALIZER PMARCEL_SEM_INITIALIZER

#undef PTHREAD_COND_INITIALIZER
#define PTHREAD_COND_INITIALIZER PMARCEL_COND_INITIALIZER

#undef PTHREAD_MAX_KEY_SPECIFIC
#define PTHREAD_MAX_KEY_SPECIFIC MAX_KEY_SPECIFIC

#undef PTHREAD_EXPLICIT_SCHED
#define PTHREAD_EXPLICIT_SCHED PMARCEL_EXPLICIT_SCHED
#undef PTHREAD_INHERIT_SCHED
#define PTHREAD_INHERIT_SCHED PMARCEL_INHERIT_SCHED

#undef PTHREAD_SCOPE_SYSTEM
#define PTHREAD_SCOPE_SYSTEM PMARCEL_SCOPE_SYSTEM
#undef PTHREAD_SCOPE_PROCESS
#define PTHREAD_SCOPE_PROCESS PMARCEL_SCOPE_PROCESS

#undef PTHREAD_PROCESS_PRIVATE
#define PTHREAD_PROCESS_PRIVATE PMARCEL_PROCESS_PRIVATE
#undef PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED PMARCEL_PROCESS_SHARED

#undef PTHREAD_CREATE_DETACHED
#define PTHREAD_CREATE_DETACHED PMARCEL_CREATE_DETACHED
#undef PTHREAD_CREATE_JOINABLE
#define PTHREAD_CREATE_JOINABLE PMARCEL_CREATE_JOINABLE

#undef PTHREAD_CANCELED
#define PTHREAD_CANCELED PMARCEL_CANCELED
#undef PTHREAD_CANCEL_ENABLE
#define PTHREAD_CANCEL_ENABLE PMARCEL_CANCEL_ENABLE
#undef PTHREAD_CANCEL_DISABLE
#define PTHREAD_CANCEL_DISABLE PMARCEL_CANCEL_DISABLE
#undef PTHREAD_CANCEL_DEFERRED
#define PTHREAD_CANCEL_DEFERRED PMARCEL_CANCEL_DEFERRED
#undef PTHREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCEL_ASYNCHRONOUS PMARCEL_CANCEL_ASYNCHRONOUS

#undef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN PMARCEL_STACK_MIN
#undef PTHREAD_THREADS_MAX
#define PTHREAD_THREADS_MAX PMARCEL_THREADS_MAX
#undef PTHREAD_DESTRUCTOR_ITERATIONS
#define PTHREAD_DESTRUCTOR_ITERATIONS PMARCEL_DESTRUCTOR_ITERATIONS

#undef PTHREAD_PRIO_INHERIT
#define PTHREAD_PRIO_INHERIT PMARCEL_PRIO_INHERIT
#undef PTHREAD_PRIO_NONE
#define PTHREAD_PRIO_NONE PMARCEL_PRIO_NONE
#undef PTHREAD_PRIO_PROTECT
#define PTHREAD_PRIO_PROTECT PMARCEL_PRIO_PROTECT

#undef PTHREAD_BARRIER_SERIAL_THREAD
#define PTHREAD_BARRIER_SERIAL_THREAD PMARCEL_BARRIER_SERIAL_THREAD

#undef PTHREAD_RWLOCK_INITIALIZER
#define PTHREAD_RWLOCK_INITIALIZER PMARCEL_RWLOCK_INITIALIZER

/* marcel_glue_pthread.c */

#undef pthread_create
#define pthread_create(thread,attr,f,arg) pmarcel_create(thread,attr,f,arg)
#undef pthread_self
#define pthread_self() pmarcel_self()
#undef pthread_equal
#define pthread_equal(thread1,thread2) pmarcel_equal(thread1,thread2)
#undef pthread_exit
#define pthread_exit(ret) pmarcel_exit(ret)
#undef pthread_atfork
#define pthread_atfork(prepare,parent,child) pmarcel_atfork(prepare,parent,child)

/* marcel_thread.c */

#undef pthread_join
#define pthread_join(pid,status) pmarcel_join(pid,status)
#undef pthread_cancel
#define pthread_cancel(pid) pmarcel_cancel(pid)
#undef pthread_detach
#define pthread_detach(pid) pmarcel_detach(pid)
#undef pthread_cleanup_push
#define pthread_cleanup_push(routine,arg) pmarcel_cleanup_push(routine,arg)
#undef pthread_cleanup_pop
#define pthread_cleanup_pop(execute) pmarcel_cleanup_pop(execute)
#undef pthread_setconcurrency
#define pthread_setconcurrency(newlevel) pmarcel_setconcurrency(newlevel)
#undef pthread_getconcurrency
#define pthread_getconcurrency() pmarcel_getconcurrency()
#undef pthread_setcancelstate
#define pthread_setcancelstate(state,oldstate) pmarcel_setcancelstate(state,oldstate)
#undef pthread_setcanceltype
#define pthread_setcanceltype(type,oldtype) pmarcel_setcanceltype(type,oldtype)
#undef pthread_testcancel
#define pthread_testcancel() pmarcel_testcancel()
#undef pthread_setschedprio
#define pthread_setschedprio(thread,priority) pmarcel_setschedprio(thread,priority)
#undef pthread_setschedparam
#define pthread_setschedparam(thread,policy,param) pmarcel_setschedparam(thread,policy,param)
#undef pthread_getschedparam
#define pthread_getschedparam(thread,policy,param) pmarcel_getschedparam(thread,policy,param)
#undef pthread_getcpuclockid
#define pthread_getcpuclockid(thread_id,clock_id) pmarcel_getcpuclockid(thread_id,clock_id)
#undef pthread_getaffinity_np
#define pthread_getaffinity_np(thread, size, attr) pmarcel_getaffinity_np(thread, size, attr)

/* marcel_attr.c */

#undef pthread_attr_init
#define pthread_attr_init(attr) pmarcel_attr_init(attr)
#undef pthread_attr_destroy
#define pthread_attr_destroy(attr) pmarcel_attr_destroy(attr)
#undef pthread_attr_setstacksize
#define pthread_attr_setstacksize(attr,size) pmarcel_attr_setstacksize(attr,size)
#undef pthread_attr_getstacksize
#define pthread_attr_getstacksize(attr,size) pmarcel_attr_getstacksize(attr,size)
#undef pthread_attr_setstackaddr
#define pthread_attr_setstackaddr(attr,addr) pmarcel_attr_setstackaddr(attr,addr)
#undef pthread_attr_getstackaddr
#define pthread_attr_getstackaddr(attr,addr) pmarcel_attr_getstackaddr(attr,addr)
#undef pthread_attr_setstack
#define pthread_attr_setstack(attr,addr,size) pmarcel_attr_setstack(attr,addr,size)
#undef pthread_attr_getstack
#define pthread_attr_getstack(attr,addr,size) pmarcel_attr_getstack(attr,addr,size)
#undef pthread_attr_setdetachstate
#define pthread_attr_setdetachstate(attr,state) pmarcel_attr_setdetachstate(attr,state)
#undef pthread_attr_getdetachstate
#define pthread_attr_getdetachstate(attr,state) pmarcel_attr_getdetachstate(attr,state)
#undef pthread_attr_setinheritsched
#define pthread_attr_setinheritsched(attr,inherit) pmarcel_attr_setinheritsched(attr,inherit)
#undef pthread_attr_getinheritsched
#define pthread_attr_getinheritsched(attr,inherit) pmarcel_attr_getinheritsched(attr,inherit)
#undef pthread_attr_setscope
#define pthread_attr_setscope(attr,scope) pmarcel_attr_setscope(attr,scope)
#undef pthread_attr_getscope
#define pthread_attr_getscope(attr,scope) pmarcel_attr_getscope(attr,scope)
#undef pthread_attr_setschedpolicy
#define pthread_attr_setschedpolicy(attr,policy) pmarcel_attr_setschedpolicy(attr,policy)
#undef pthread_attr_getschedpolicy
#define pthread_attr_getschedpolicy(attr,policy) pmarcel_attr_getschedpolicy(attr,policy)
#undef pthread_attr_setschedparam
#define pthread_attr_setschedparam(attr,param) pmarcel_attr_setschedparam(attr,param)
#undef pthread_attr_getschedparam
#define pthread_attr_getschedparam(attr,param) pmarcel_attr_getschedparam(attr,param)
#undef pthread_attr_setguardsize
#define pthread_attr_setguardsize(attr, guardsize) pmarcel_attr_setguardsize(attr, guardsize)
#undef pthread_attr_getguardsize
#define pthread_attr_getguardsize(attr, guardsize) pmarcel_attr_getguardsize(attr, guardsize)
#undef pthread_attr_getaffinity_np
#define pthread_attr_getaffinity_np(thread, size, attr) pmarcel_attr_getaffinity_np(thread, size, attr)
#undef pthread_getattr_np
#define pthread_getattr_np(thread,attr) pmarcel_getattr_np(thread,attr)

/* nptl_mutex.c */

#undef pthread_mutex_init
#define pthread_mutex_init(mutex,attr) pmarcel_mutex_init(mutex,attr)
#undef pthread_mutex_destroy
#define pthread_mutex_destroy(mutex) pmarcel_mutex_destroy(mutex)
#undef pthread_mutex_lock
#define pthread_mutex_lock(mutex) pmarcel_mutex_lock(mutex)
#undef pthread_mutex_trylock
#define pthread_mutex_trylock(mutex) pmarcel_mutex_trylock(mutex)
#undef pthread_mutex_timedlock
#define pthread_mutex_timedlock(mutex,time) pmarcel_mutex_timedlock(mutex,time)
#undef pthread_mutex_unlock
#define pthread_mutex_unlock(mutex) pmarcel_mutex_unlock(mutex)
#undef pthread_mutex_getprioceiling
#define pthread_mutex_getprioceiling(mutex,pceiling) pmarcel_mutex_getprioceiling(mutex,pceiling)
#undef pthread_mutex_setprioceiling
#define pthread_mutex_setprioceiling(mutex,pceiling,old) pmarcel_mutex_setprioceiling(mutex,pceiling,old)

#undef pthread_mutexattr_init
#define pthread_mutexattr_init(attr) pmarcel_mutexattr_init(attr)
#undef pthread_mutexattr_destroy
#define pthread_mutexattr_destroy(attr) pmarcel_mutexattr_destroy(attr)
#undef pthread_mutexattr_settype
#define pthread_mutexattr_settype(attr,type) pmarcel_mutexattr_settype(attr,type)
#undef pthread_mutexattr_gettype
#define pthread_mutexattr_gettype(attr,type) pmarcel_mutexattr_gettype(attr,type)
#undef pthread_mutexattr_setpshared
#define pthread_mutexattr_setpshared(attr,pshared) pmarcel_mutexattr_setpshared(attr,pshared)
#undef pthread_mutexattr_getpshared
#define pthread_mutexattr_getpshared(attr,pshared) pmarcel_mutexattr_getpshared(attr,pshared)
#undef pthread_mutexattr_getprioceiling
#define pthread_mutexattr_getprioceiling(attr,pceiling) pmarcel_mutexattr_getprioceiling(attr,pceiling)
#undef pthread_mutexattr_setprioceiling
#define pthread_mutexattr_setprioceiling(attr,pceiling) pmarcel_mutexattr_setprioceiling(attr,pceiling)
#undef pthread_mutexattr_getprotocol
#define pthread_mutexattr_getprotocol(attr,protocol) pmarcel_mutexattr_getprotocol(attr,protocol)
#undef pthread_mutexattr_setprotocol
#define pthread_mutexattr_setprotocol(attr,protocol) pmarcel_mutexattr_setprotocol(attr,protocol)

#undef pthread_once
#define pthread_once(once,f) pmarcel_once(once,f)

/* marcel_sem.c, ce n'est pas pthread_ */

#undef sem_init
#define sem_init(sem,pshared,initial) pmarcel_sem_init(sem,pshared,initial)
#undef sem_destroy
#define sem_destroy(sem) pmarcel_sem_destroy(sem)
#undef sem_open
#define sem_open(name,flags,...) pmarcel_sem_open(name,flags,##__VA_ARGS__)
#undef sem_close
#define sem_close(sem) pmarcel_sem_close(sem)
#undef sem_unlink
#define sem_unlink(name) pmarcel_sem_unlink(name)
#undef sem_post
#define sem_post(sem) pmarcel_sem_post(sem)
#undef sem_wait
#define sem_wait(sem) pmarcel_sem_wait(sem)
#undef sem_trywait
#define sem_trywait(sem) pmarcel_sem_trywait(sem)
#undef sem_timedwait
#define sem_timedwait(sem,time) pmarcel_sem_timedwait(sem,time)
#undef sem_getvalue
#define sem_getvalue(sem,sval) pmarcel_sem_getvalue(sem,sval)

/* marcel_spin.c */

#undef pthread_spin_init
#define pthread_spin_init(spinlock,pshared) pmarcel_spin_init(spinlock,pshared)
#undef pthread_spin_destroy
#define pthread_spin_destroy(spinlock) pmarcel_spin_destroy(spinlock)
#undef pthread_spin_lock
#define pthread_spin_lock(spinlock) pmarcel_spin_lock(spinlock)
#undef pthread_spin_trylock
#define pthread_spin_trylock(spinlock) pmarcel_spin_trylock(spinlock)
#undef pthread_spin_unlock
#define pthread_spin_unlock(spinlock) pmarcel_spin_unlock(spinlock)

/* marcel_signal.c */

#undef pause
#define pause() pmarcel_pause()
#undef alarm
#define alarm(sec) pmarcel_alarm(sec)
#undef getitimer
#define getitimer(which,value) pmarcel_getitimer(which,value)
#undef setitimer
#define setitimer(which,value,ovalue) pmarcel_setitimer(which,value,ovalue)
#undef raise
#define raise(sig) pmarcel_raise(sig)
#undef pthread_kill
#define pthread_kill(thread,sig) pmarcel_kill(thread,sig)
#undef sigpending
#define sigpending(set) pmarcel_sigpending(set)
#undef sigtimedwait
#define sigtimedwait(set,info,timeout) pmarcel_sigtimedwait(set,info,timeout)
#undef sigwaitinfo
#define sigwaitinfo(set,info) pmarcel_sigwaitinfo(set,info)
#undef sigwait
#define sigwait(set,sig) pmarcel_sigwait(set,sig)
#undef sigsuspend
#define sigsuspend(set) pmarcel_sigsuspend(set)
#undef siglongjmp
#define siglongjmp(env,val) pmarcel_siglongjmp(env,val)
#undef signal
#define signal(sig,handler) pmarcel_signal(sig,handler)
#undef pthread_sigmask
#define pthread_sigmask(how,set,oset) pmarcel_sigmask(how,set,oset)
#undef sigprocmask
#define sigprocmask(how,set,oset) marcel_sigmask(how,set,oset)
#undef sigsetjmp
#define sigsetjmp(env,savemask) pmarcel_sigsetjmp(env,savemask)
/* #define sigaction(sig,act,oact) pmarcel_sigaction(sig,act,oact) déja fait pour la structure */
#undef sighold
#define sighold(sig) pmarcel_sighold(sig)
#undef sigrelse
#define sigrelse(sig) pmarcel_sigrelse(sig)
#undef sigignore
#define sigignore(sig) pmarcel_sigignore(sig)
#undef sigpause
#define sigpause(sig) pmarcel_sigpause(sig)

#undef sigemptyset
#define sigemptyset     pmarcel_sigemptyset
#undef sigfillset
#define sigfillset      pmarcel_sigfillset
#undef sigismember
#define sigismember     pmarcel_sigismember
#undef sigaddset
#define sigaddset       pmarcel_sigaddset
#undef sigdelset
#define sigdelset       pmarcel_sigdelset
#undef sigisemptyset
#define sigisemptyset   pmarcel_sigisemptyset

/* nptl_cond.c */

#undef pthread_cond_init
#define pthread_cond_init(cond,attr) pmarcel_cond_init(cond,attr)
#undef pthread_cond_destroy
#define pthread_cond_destroy(cond) pmarcel_cond_destroy(cond)
#undef pthread_cond_signal
#define pthread_cond_signal(cond) pmarcel_cond_signal(cond)
#undef pthread_cond_broadcast
#define pthread_cond_broadcast(cond) pmarcel_cond_broadcast(cond)
#undef pthread_cond_wait
#define pthread_cond_wait(cond,mutex) pmarcel_cond_wait(cond,mutex)
#undef pthread_cond_timedwait
#define pthread_cond_timedwait(cond,mutex,time) pmarcel_cond_timedwait(cond,mutex,time)
#undef pthread_condattr_init
#define pthread_condattr_init(attr) pmarcel_condattr_init(attr)
#undef pthread_condattr_destroy
#define pthread_condattr_destroy(attr) pmarcel_condattr_destroy(attr)
#undef pthread_condattr_getpshared
#define pthread_condattr_getpshared(attr,pshared) pmarcel_condattr_getpshared(attr,pshared)
#undef pthread_condattr_setpshared
#define pthread_condattr_setpshared(attr,pshared) pmarcel_condattr_setpshared(attr,pshared)
#undef pthread_condattr_getclock
#define pthread_condattr_getclock(attr,clock) pmarcel_condattr_getclock(attr,clock)
#undef pthread_condattr_setclock
#define pthread_condattr_setclock(attr,clock) pmarcel_condattr_setclock(attr,clock)

/* marcel_keys.c */

#undef pthread_key_create
#define pthread_key_create(key,func) pmarcel_key_create(key,func)
#undef pthread_key_delete
#define pthread_key_delete(key) pmarcel_key_delete(key)
#undef pthread_setspecific
#define pthread_setspecific(key,value) pmarcel_setspecific(key,value)
#undef pthread_getspecific
#define pthread_getspecific(key) pmarcel_getspecific(key)

/* pthread_rwlock.c */

#undef pthread_rwlock_init
#define pthread_rwlock_init(rwlock,attr) pmarcel_rwlock_init(rwlock,attr)
#undef pthread_rwlock_destroy
#define pthread_rwlock_destroy(rwlock) pmarcel_rwlock_destroy(rwlock)
#undef pthread_rwlock_rdlock
#define pthread_rwlock_rdlock(rwlock) pmarcel_rwlock_rdlock(rwlock)
#undef pthread_rwlock_timedrdlock
#define pthread_rwlock_timedrdlock(rwlock,time) pmarcel_rwlock_timedrdlock(rwlock,time)
#undef pthread_rwlock_tryrdlock
#define pthread_rwlock_tryrdlock(rwlock) pmarcel_rwlock_tryrdlock(rwlock)
#undef pthread_rwlock_wrlock
#define pthread_rwlock_wrlock(rwlock) pmarcel_rwlock_wrlock(rwlock)
#undef pthread_rwlock_timedwrlock
#define pthread_rwlock_timedwrlock(rwlock,time) pmarcel_rwlock_timedwrlock(rwlock,time)
#undef pthread_rwlock_trywrlock
#define pthread_rwlock_trywrlock(rwlock) pmarcel_rwlock_trywrlock(rwlock)
#undef pthread_rwlock_unlock
#define pthread_rwlock_unlock(rwlock) pmarcel_rwlock_unlock(rwlock)

#undef pthread_rwlockattr_init
#define pthread_rwlockattr_init(attr) pmarcel_rwlockattr_init(attr)
#undef pthread_rwlockattr_destroy
#define pthread_rwlockattr_destroy(attr) pmarcel_rwlockattr_destroy(attr)
#undef pthread_rwlockattr_setpshared
#define pthread_rwlockattr_setpshared(attr,pshared) pmarcel_rwlockattr_setpshared(attr,pshared)
#undef pthread_rwlockattr_getpshared
#define pthread_rwlockattr_getpshared(attr,pshared) pmarcel_rwlockattr_getpshared(attr,pshared)

/* marcel_barrier.c */

#undef pthread_barrier_init
#define pthread_barrier_init(bar,attr,num) pmarcel_barrier_init(bar,attr,num)
#undef pthread_barrier_destroy
#define pthread_barrier_destroy(bar) pmarcel_barrier_destroy(bar)
#undef pthread_barrier_wait
#define pthread_barrier_wait(bar) pmarcel_barrier_wait(bar)

#undef pthread_barrierattr_init
#define pthread_barrierattr_init(attr) pmarcel_barrierattr_init(attr)
#undef pthread_barrierattr_destroy
#define pthread_barrierattr_destroy(attr) pmarcel_barrierattr_destroy(attr)
#undef pthread_barrierattr_setpshared
#define pthread_barrierattr_setpshared(attr,pshared) pmarcel_barrierattr_setpshared(attr,pshared)
#undef pthread_barrierattr_getpshared
#define pthread_barrierattr_getpshared(attr,pshared) pmarcel_barrierattr_getpshared(attr,pshared)

#undef CPU_SETSIZE
#define CPU_SETSIZE PMARCEL_CPU_SETSIZE
#undef CPU_SET
#define CPU_SET(cpu, cpusetp) PMARCEL_CPU_SET(cpu, cpusetp)
#undef CPU_CLR
#define CPU_CLR(cpu, cpusetp) PMARCEL_CPU_CLR(cpu, cpusetp)
#undef CPU_ISSET
#define CPU_ISSET(cpu, cpusetp) PMARCEL_CPU_ISSET(cpu, cpusetp)
#undef CPU_ZERO
#define CPU_ZERO(cpu) PMARCEL_CPU_ZERO(cpu)
#undef CPU_COUNT
#define CPU_COUNT(cpu) PMARCEL_CPU_COUNT(cpu)
#undef CPU_SET_S
#define CPU_SET_S(cpu, setsize, cpusetp) PMARCEL_CPU_SET_S(cpu, setsize, cpusetp)
#undef CPU_CLR_S
#define CPU_CLR_S(cpu, setsize, cpusetp) PMARCEL_CPU_CLR_S(cpu, setsize, cpusetp)
#undef CPU_ISSET_S
#define CPU_ISSET_S(cpu, setsize, cpusetp) PMARCEL_CPU_ISSET_S(cpu, setsize, cpusetp)
#undef CPU_ZERO_S
#define CPU_ZERO_S(setsize, cpusetp) PMARCEL_CPU_ZERO_S(setsize, cpusetp)
#undef CPU_COUNT_S
#define CPU_COUNT_S(setsize, cpu) PMARCEL_CPU_COUNT_S(setsize, cpu)

#if defined(__cplusplus)
#undef pthread_once
#define pthread_once pmarcel_once
#undef pthread_getspecific
#define pthread_getspecific pmarcel_getspecific
#undef pthread_setspecific
#define pthread_setspecific pmarcel_setspecific
#undef pthread_create
#define pthread_create pmarcel_create
#undef pthread_cancel
#define pthread_cancel pmarcel_cancel
#undef pthread_mutex_lock
#define pthread_mutex_lock pmarcel_mutex_lock
#undef pthread_mutex_trylock
#define pthread_mutex_trylock pmarcel_mutex_trylock
#undef pthread_mutex_unlock
#define pthread_mutex_unlock pmarcel_mutex_unlock
#undef pthread_mutex_init
#define pthread_mutex_init pmarcel_mutex_init
#undef pthread_key_create
#define pthread_key_create pmarcel_key_create
#undef pthread_key_delete
#define pthread_key_delete pmarcel_key_delete
#undef pthread_mutexattr_init
#define pthread_mutexattr_init pmarcel_mutexattr_init
#undef pthread_mutexattr_settype
#define pthread_mutexattr_settype pmarcel_mutexattr_settype
#undef pthread_mutexattr_destroy
#define pthread_mutexattr_destroy pmarcel_mutexattr_destroy
#undef pthread_cond_broadcast
#define pthread_cond_broadcast pmarcel_cond_broadcast
#undef pthread_cond_wait
#define pthread_cond_wait pmarcel_cond_wait
#endif				/* __cpluscplus */

#ifdef __GNUC__

/* Automatically initialize Marcel, so that applications that include
   this file don't have to explicitly call `marcel_init ()'.

	 XXX: This hack doesn't work on non-GCC or non-ELF platforms such as
	 Mac OS X.  */
static void __attribute__ ((__constructor__)) ma_initialize_pmarcel(void)
{
	int argc = 0;

	if (!marcel_test_activity()) {
		marcel_init(&argc, NULL);
		marcel_ensure_abi_compatibility(MARCEL_HEADER_HASH);
	}
}

/* Note: We don't have any detructor calling `marcel_end()' here.  That would
 * be incompatible with pthreads since it implies joining all currently
 * running threads, which doesn't happen with pthreads.  */

#endif


#endif	/*_MARCEL_PTHREAD_H*/
