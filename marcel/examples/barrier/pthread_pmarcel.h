#include <marcel.h>

/* marcel_glue_pthread.c */

#define pthread_t pmarcel_t

#define pthread_create(thread,attr,f,arg) pmarcel_create(thread,attr,f,arg)
#define pthread_self() pmarcel_self()
#define pthread_equal(threadd1,thread2) pmarcel_equal(thread1,thread2)
#define pthread_exit(ret) pmarcel_exit(ret)
//#define pthread_atfork(prepare,parent,child) pmarcel_atfork(prepare,parent,child) 

/* marcel_thread.c */

#define pthread_join(pid,status) pmarcel_join(pid,status)
#define pthread_cancel(pid) pmarcel_cancel(pid)
#define pthread_detach(pid) pmarcel_detach(pid)
#define pthread_cleanup_push(buffer,func,arg) pmarcel_cleanup_push(buffer,func,arg)
#define pthread_cleanup_pop(buffer,execute) pmarcel_cleanup_pop(buffer,execute)
#define pthread_setconcurrency(newlevel) pmarcel_setconcurrency(newlevel) 
#define pthread_getconcurrency(buf) pmarcel_getconcurrency(buf)
#define pthread_setcancelstate(state,oldstate) pmarcel_setcancelstate(state,oldstate) 
#define pthread_setcanceltype(type,oldtype) pmarcel_setcanceltype(type,oldtype)
#define pthread_testcancel() pmarcel_testcancel()
#define pthread_setschedparam(thread,policy,param) pmarcel_setschedparam(thread,policy,param)
#define pthread_getschedparam(thread,policy,param) pmarcel_getschedparam(thread,policy,param)
#define pthread_getcpuclockid(thread_id,clock_id) pmarcel_getcpuclockid(thread_id,clock_id)
//#define pthread_setschedprio(thread,prio) pmarcel_setschedprio(thread,prio)

/* marcel_attr.c */

#define pthread_attr_t pmarcel_attr_t

#define PTHREAD_ATTR_INITIALIZER PMARCEL_ATTR_INITIALIZER
#define PTHREAD_ATTR_DESTROYER PMARCEL_ATTR_DESTROYER

#define pthread_attr_init(attr) pmarcel_attr_init(attr) 
#define pthread_attr_destroy(attr) pmarcel_attr_destroy(attr)
#define pthread_attr_setstacksize(attr,size) pmarcel_attr_setstacksize(attr,size)
#define pthread_attr_getstacksize(attr,size) pmarcel_attr_getstacksize(attr,size)
#define pthread_attr_setstackaddr(attr,addr) pmarcel_attr_setstackaddr(attr,addr)
#define pthread_attr_getstackaddr(attr,addr) pmarcel_attr_getstackaddr(attr,addr)
#define pthread_attr_setstack(attr,addr,size) pmarcel_attr_setstack(attr,addr,size)
#define pthread_attr_getstack(attr,addr,size) pmarcel_attr_getstack(attr,addr,size)
#define pthread_attr_setdetachstate(attr,state) pmarcel_attr_setdetachstate(attr,state)
#define pthread_attr_getdetachstate(attr,state) pmarcel_attr_getdetachstate(attr,state)
#define pthread_attr_setinheritsched(attr,inherit) pmarcel_attr_setinheritsched(attr,inherit)
#define pthread_attr_getinheritsched(attr,inherit) pmarcel_attr_getinheritsched(attr,inherit)
#define pthread_attr_setscope(attr,scope) pmarcel_attr_setscope(attr,scope)
#define pthread_attr_getscope(attr,scope) pmarcel_attr_getscope(attr,scope)
#define pthread_attr_setschedpolicy(attr,policy) pmarcel_attr_setschedpolicy(attr,policy)
#define pthread_attr_getschedpolicy(attr,policy) pmarcel_attr_getschedpolicy(attr,policy)
#define pthread_attr_setschedparam(attr,param) pmarcel_attr_setschedparam(attr,param)
#define pthread_attr_getschedparam(attr,param) pmarcel_attr_getschedparam(attr,param)
//#define pthread_attr_setguardsize(attr, guardsize) pmarcel_attr_setguardsize(attr, guardsize) */
//#define pthread_attr_getguardsize(attr, guardsize) pmarcel_attr_getguardsize(attr, guardsize) */

/* nptl_mutex.c */

#define PTHREAD_MUTEX_DEFAULT PMARCEL_MUTEX_DEFAULT
#define PTHREAD_MUTEX_ERRORCHECK PMARCEL_MUTEX_ERRORCHECK
#define PTHREAD_MUTEX_INITIALIZER PMARCEL_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_NORMAL PMARCEL_MUTEX_NORMAL
#define PTHREAD_MUTEX_RECURSIVE PMARCEL_MUTEX_RECURSIVE

#define pthread_mutex_init(mutex,attr) pmarcel_mutex_init(mutex,attr)
#define pthread_mutex_destroy(mutex) pmarcel_mutex_destroy(mutex)
#define pthread_mutex_lock(mutex) pmarcel_mutex_lock(mutex)
#define pthread_mutex_trylock(mutex) pmarcel_mutex_trylock(mutex)
#define pthread_mutex_timedlock(mutex,time) pmarcel_mutex_timedlock(mutex,time)
#define pthread_mutex_unlock(mutex) pmarcel_mutex_unlock(mutex)
//#define pthread_mutex_getprioceiling(mutex,pceiling,old) pmarcel_mutex_getprioceiling(mutex,pceiling,old)
//#define pthread_mutex_setprioceiling(mutex,pceiling,old) pmarcel_mutex_setprioceiling(mutex,pceiling,old)

#define pthread_mutexattr_init(attr) pmarcel_mutexattr_init(attr)
#define pthread_mutexattr_destroy(attr) pmarcel_mutexattr_destroy(attr)
#define pthread_mutexattr_settype(attr,type) pmarcel_mutexattr_settype(attr,type)
#define pthread_mutexattr_gettype(attr,type) pmarcel_mutexattr_gettype(attr,type)
#define pthread_mutexattr_setpshared(attr,pshared) pmarcel_mutexattr_setpshared(attr,pshared)
#define pthread_mutexattr_getpshared(attr,pshared) pmarcel_mutexattr_getpshared(attr,pshared)
//#define pthread_mutexattr_getprioceiling(attr,pceiling) pmarcel_mutexattr_getprioceiling(attr,pceiling)
//#define pthread_mutexattr_setprioceiling(attr,pceiling) pmarcel_mutexattr_setprioceiling(attr,pceiling)
//#define pthread_mutexattr_getprotocol(attr,protocol) pmarcel_mutexattr_getprotocol(attr,protocol)
//#define pthread_mutexattr_setprotocol(attr,protocol) pmarcel_mutexattr_setprotocol(attr,protocol)

#define PTHREAD_ONCE_INIT PMARCEL_ONCE_INIT
#define pthread_once(once,f) pmarcel_once(once,f)

/* marcel_sem.c, ce n'est pas pthread_ */

#define SEM_INITIALIZER PMARCEL_SEM_INITIALIZER

#define sem_init(sem,pshared,initial) pmarcel_sem_init(sem,pshared,initial)
#define sem_destroy(sem) pmarcel_sem_destroy(sem)
#define sem_open(name,flags,...) pmarcel_sem_open(name,flags,...)
#define sem_close(sem) pmarcel_sem_close(sem)
#define sem_unlink(name) pmarcel_sem_unlink(name)
#define sem_post(sem) pmarcel_sem_post(sem)
#define sem_wait(sem) pmarcel_sem_wait(sem)
#define sem_trywait(sem) pmarcel_sem_trywait(sem)
#define sem_timedwait(sem,time) pmarcel_sem_timedwait(sem,time)
#define sem_getvalue(sem,sval) pmarcel_sem_getvalue(sem,sval) 

/* marcel_spin.c */

#define pthread_spin_init(spinlock,pshared) pmarcel_spin_init(spinlock,pshared)
#define pthread_spin_destroy(spinlock) pmarcel_spin_destroy(spinlock)
#define pthread_spin_lock(spinlock) pmarcel_spin_lock(spinlock)
#define pthread_spin_trylock(spinlock) pmarcel_spin_trylock(spinlock)
#define pthread_spin_unlock(spinlock) pmarcel_spin_unlock(spinlock)

/* marcel_signal.c, tout n'est pas pthread_ */

#include <signal.h>
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

//#define SIG_BLOCK PMARCEL_SIG_BLOCK
//#define SIG_UNBLOCK PMARCEL_SIG_UNBLOCK
//#define SIG_SETMASK PMARCEL_SIG_SETMASK
//#define NSIG PMARCEL_NSIG

#define pause pmarcel_pause
#define raise pmarcel_raise
#define pthread_kill(thread,sig) pmarcel_kill(thread,sig)
#define pthread_sigmask(how,set,oset) pmarcel_sigmask(how,set,oset)
#define sigpending(set) pmarcel_sigpending(set)
#define sigwait(set,sig) pmarcel_sigwait(set,sig)
#define sigsuspend(set) pmarcel_sigsuspend(set)
//#define sigsetjmp(env,savemask) pmarcel_sigsetjmp(env,savemask)
#define siglongjmp(env,val) pmarcel_siglongjmp(env,val)
#define signal(sig,handler) pmarcel_signal(sig,handler)
//#define sigaction(sig,act,oact) pmarcel_sigaction(sig,act,oact)

#define sigset_t pmarcel_sigset_t

#define sigemptyset     pmarcel_sigemptyset
#define sigfillset      pmarcel_sigfillset
#define sigismember     pmarcel_sigismember
#define sigaddset       pmarcel_sigaddset
#define sigdelset       pmarcel_sigdelset
#define sigisemptyset   pmarcel_sigisemptyset

/* nptl_cond.c */

#define PTHREAD_COND_INITIALIZER PMARCEL_COND_INITIALIZER

#define pthread_cond_init(cond,attr) pmarcel_cond_init(cond,attr)
#define pthread_cond_destroy(cond) pmarcel_cond_destroy(cond)
#define pthread_cond_signal(cond) pmarcel_cond_signal(cond)
#define pthread_cond_broadcast(cond) pmarcel_cond_broadcast(cond)
#define pthread_cond_wait(cond,mutex) pmarcel_cond_wait(cond,mutex)
#define pthread_cond_timedwait(cond,mutex,time) pmarcel_cond_timedwait(cond,mutex,time)

#define pthread_condattr_init(attr) pmarcel_condattr_init(attr)
#define pthread_condattr_destroy(attr) pmarcel_condattr_destroy(attr)
#define pthread_condattr_getpshared(attr,pshared) pmarcel_condattr_getpshared(attr,pshared)
#define pthread_condattr_setpshared(attr,pshared) pmarcel_condattr_setpshared(attr,pshared)
#define pthread_condattr_getclock(attr,clock) pmarcel_condattr_getclock(attr,clock)
#define pthread_condattr_setclock(attr,clock) pmarcel_condattr_setclock(attr,clock)

/* marcel_keys.c */

#define PTHREAD_MAX_KEY_SPECIFIC MAX_KEY_SPECIFIC

#define pthread_key_create(key,func) pmarcel_key_create(key,func)
#define pthread_key_delete(key) pmarcel_key_delete(key)
#define pthread_setspecific(key,value) pmarcel_setspecific(key,value)
#define pthread_getspecific(key) pmarcel_getspecific(key)

/* pthread_rwlock.c */

#define PTHREAD_RWLOCK_INITIALIZER PMARCEL_RWLOCK_INITIALIZER

#define pthread_rwlock_init(rwlock,attr) pmarcel_rwlock_init(rwlock,attr)
#define pthread_rwlock_destroy(rwlock) pmarcel_rwlock_destroy(rwlock)
#define pthread_rwlock_rdlock(rwlock) pmarcel_rwlock_rdlock(rwlock)
#define pthread_rwlock_timedrdlock(rwlock,time) pmarcel_rwlock_timedrdlock(rwlock,time)
#define pthread_rwlock_tryrdlock(rwlock) pmarcel_rwlock_tryrdlock(rwlock)
#define pthread_rwlock_wrlock(rwlock) pmarcel_rwlock_wrlock(rwlock)
#define pthread_rwlock_timedwrlock(rwlock,time) pmarcel_rwlock_timedwrlock(rwlock,time)
#define pthread_rwlock_trywrlock(rwlock) pmarcel_rwlock_trywrlock(rwlock)
#define pthread_rwlock_unlock(rwlock) pmarcel_rwlock_unlock(rwlock)

#define pthread_rwlockattr_init(attr) pmarcel_rwlockattr_init(attr)
#define pthread_rwlockattr_destroy(attr) pmarcel_rwlockattr_destroy(attr)
#define pthread_rwlockattr_setpshared(attr,pshared) pmarcel_rwlockattr_setpshared(attr,pshared)
#define pthread_rwlockattr_getpshared(attr,pshared) pmarcel_rwlockattr_getpshared(attr,pshared)

/* marcel_barrier.c */

#define PTHREAD_BARRIER_SERIAL_THREAD PMARCEL_BARRIER_SERIAL_THREAD

#define pthread_barrier_init(bar,attr,num) pmarcel_barrier_init(bar,attr,num)
#define pthread_barrier_destroy(bar) pmarcel_barrier_destroy(bar)
#define pthread_barrier_wait(bar) pmarcel_barrier_wait(bar)

#define pthread_barrierattr_init(attr) pmarcel_barrierattr_init(attr)
#define pthread_barrierattr_destroy(attr) pmarcel_barrierattr_destroy(attr)
#define pthread_barrierattr_setpshared(attr,pshared) pmarcel_barrierattr_setpshared(attr,pshared)
#define pthread_barrierattr_getpshared(attr,pshared) pmarcel_barrierattr_getpshared(attr,pshared)

/* autres macros de pthread.h */

#define PTHREAD_EXPLICIT_SCHED PMARCEL_EXPLICIT_SCHED
#define PTHREAD_INHERIT_SCHED PMARCEL_INHERIT_SCHED

#define PTHREAD_SCOPE_SYSTEM PMARCEL_SCOPE_SYSTEM
#define PTHREAD_SCOPE_PROCESS PMARCEL_SCOPE_PROCESS

#define PTHREAD_PROCESS_PRIVATE PMARCEL_PROCESS_PRIVATE
#define PTHREAD_PROCESS_SHARED PMARCEL_PROCESS_SHARED

#define PTHREAD_CREATE_DETACHED PMARCEL_CREATE_DETACHED 
#define PTHREAD_CREATE_JOINABLE PMARCEL_CREATE_JOINABLE

#define PTHREAD_CANCELED PMARCEL_CANCELED
#define PTHREAD_CANCEL_ENABLE PMARCEL_CANCEL_ENABLE
#define PTHREAD_CANCEL_DISABLE PMARCEL_CANCEL_DISABLE
#define PTHREAD_CANCEL_DEFERRED PMARCEL_CANCEL_DEFERRED
#define PTHREAD_CANCEL_ASYNCHRONOUS PMARCEL_CANCEL_ASYNCHRONOUS

#define PTHREAD_STACK_MIN PMARCEL_STACK_MIN
#define PTHREAD_THREADS_MAX PMARCEL_THREADS_MAX
#define PTHREAD_DESTRUCTOR_ITERATIONS PMARCEL_DESTRUCTOR_ITERATIONS

//#define SCHED_FIFO PMARCEL_SCHED_FIFO
//#define SCHED_RR PMARCEL_SCHED_RR
//#define SCHED_OTHER PMARCEL_SCHED_OTHER
//#define SCHED_OTHER_SPORADIC PMARCEL_SCHED_SPORADIC

#define PTHREAD_PRIO_INHERIT PMARCEL_PRIO_INHERIT
#define PTHREAD_PRIO_NONE PMARCEL_PRIO_NONE
#define PTHREAD_PRIO_PROTECT PMARCEL_PRIO_PROTECT
