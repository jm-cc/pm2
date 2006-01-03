
#include "sys/marcel_flags.h" //VD: 
#ifdef MA__LIBPTHREAD

/* POSIX à revoir :
 *
 * create
 * exit
 * detach
 * setspecific
 * getspecific
 * join
 * cancel

*/

extern int printf (__const char *__restrict __format, ...) ;

static int infile=0;

#define ND(function) \
  int function(void) { \
    if (!infile) { \
      infile=1; \
      printf("libpthread(marcel): %s not yet implemented\n", __func__); \
      infile=0; \
    } \
    return 0; \
  }

#define ND2(function)

#define ND3(function) \
  int function(void) { \
        return 0; \
  }

/*ND(_IO_flockfile) ND(_IO_ftrylockfile) ND(_IO_funlockfile)*/ 
/*ND(__close) ND(__connect) ND(__fcntl) ND(__lseek) ND(__open) ND(__read) ND(__send) ND(__wait)*/ 
/*ND(__errno_location) ND(__h_errno_location)*/ 
/*ND(__pthread_atfork) ND(__pthread_initialize) ND(__pthread_getspecific)*/ 
/*ND(__pthread_key_create) ND(__pthread_mutex_destroy) ND(__pthread_mutex_init)*/ 
/*ND(__pthread_mutex_lock) ND(__pthread_mutex_trylock) ND(__pthread_mutex_unlock)*/ 
/*ND(__pthread_mutexattr_destroy) ND(__pthread_mutexattr_init)*/ 
/*ND(__pthread_mutexattr_settype) ND(__pthread_once) ND(__pthread_setspecific)*/ 
/*ND(__sigaction)*/ 
/*ND(__write)*/ 
/*ND(_pthread_cleanup_pop) ND(_pthread_cleanup_pop_restore) ND(_pthread_cleanup_push)*/ 
/*ND(_pthread_cleanup_push_defer) */
     /*ND(accept) ND(close) ND(connect) ND(fcntl) ND(fork) ND(fsync)*/ ND(longjmp) /*ND(lseek) ND(msync)*/ 
/*ND(flockfile) ND(funlockfile) ND(ftrylockfile) 
ND(nanosleep) ND(open) ND(pause)*/ ND2(raise) /*ND(read) ND(recv) ND(recvfrom)*/ ND(recvmsg) /*ND(send) 
ND(pthread_atfork)*/ ND(pthread_attr_destroy) /*ND(pthread_attr_getdetachstate) */
ND(pthread_attr_getinheritsched) ND(pthread_attr_getschedparam) 
ND(pthread_attr_getschedpolicy) ND(pthread_attr_getscope) /*ND(pthread_attr_init)*/ 
/*ND(pthread_attr_setdetachstate)*/ ND(pthread_attr_setinheritsched) 
ND(pthread_attr_setschedparam) ND(pthread_attr_setschedpolicy) 
ND(pthread_attr_setscope) /*ND(pthread_cancel) ND(pthread_cond_broadcast) 
ND(pthread_cond_destroy) ND(pthread_cond_init) ND(pthread_cond_signal) 
ND(pthread_cond_timedwait) ND(pthread_cond_wait) ND(pthread_condattr_destroy) 
ND(pthread_condattr_init) ND(pthread_create) ND(pthread_detach) ND(pthread_equal)*/ 
/*ND(pthread_exit)*/ /*ND(pthread_getschedparam)*/ /*ND(pthread_getspecific) ND(pthread_join)*/ 
/*ND(pthread_key_create) ND(pthread_key_delete)*/ ND(pthread_kill) 
ND(pthread_kill_other_threads_np) /*ND(pthread_mutexattr_getkind_np)*/ 
/*ND(pthread_mutex_destroy) ND(pthread_mutex_init) ND(pthread_mutex_lock)*/ 
/*ND(pthread_mutex_trylock) ND(pthread_mutex_unlock) ND(pthread_mutexattr_destroy) 
ND(pthread_mutexattr_init) ND(pthread_once) ND(pthread_self)*/ ND(pthread_setcancelstate) 
/*ND(pthread_mutexattr_setkind_np)*/ 
ND(pthread_setcanceltype) ND(pthread_setschedparam) /*ND(pthread_setspecific)*/ 
ND3(pthread_sigmask) ND(pthread_testcancel) 
/* ND(sem_destroy) ND(sem_getvalue) ND(sem_init) ND(sem_post) ND(sem_trywait) ND(sem_wait)*/ 
/*ND(sendmsg) ND(sendto)*/ ND2(sigaction) ND(siglongjmp) /*ND(system) ND(tcdrain) ND(wait)*/ 
ND(sigwait) 
/*ND(vfork) ND(__fork)*/ 
/*ND(waitpid) ND(write)*/ 

/*ND(__libc_allocate_rtsig)*/ 
/*ND(__libc_current_sigrtmin) ND(__libc_current_sigrtmax)*/ 
/*ND(pthread_attr_getguardsize) ND(pthread_attr_setguardsize) */
/*ND(pthread_attr_getstackaddr) ND(pthread_attr_setstackaddr) */
/*ND(pthread_attr_getstacksize) ND(pthread_attr_setstacksize)  */
     //déjà définis: ND(pthread_attr_init) ND(pthread_create) 
ND(pthread_getconcurrency) ND(pthread_setconcurrency) 
/*ND(pthread_mutexattr_gettype) ND(pthread_mutexattr_settype)*/ 
/*ND(pthread_rwlock_init) ND(pthread_rwlock_destroy) ND(pthread_rwlock_rdlock)*/ 
/*ND(pthread_rwlock_tryrdlock) ND(pthread_rwlock_wrlock) ND(pthread_rwlock_trywrlock)*/ 
/*ND(pthread_rwlock_unlock) ND(pthread_rwlockattr_init) ND(pthread_rwlockattr_destroy)*/ 
/*ND(pthread_rwlockattr_getkind_np) ND(pthread_rwlockattr_setkind_np)*/ 
/*ND(pthread_rwlockattr_getpshared) ND(pthread_rwlockattr_setpshared)*/ 
     //déjà définis: ND(sem_destroy) ND(sem_getvalue) ND(sem_init) ND(sem_post) ND(sem_trywait) ND(sem_wait) 

ND(sem_close) ND(sem_open) ND(sem_unlink) 

/*ND(__vfork)*/ 

/*ND(__pthread_rwlock_init) ND(__pthread_rwlock_destroy) ND(__pthread_rwlock_rdlock)*/ 
/*ND(__pthread_rwlock_tryrdlock) ND(__pthread_rwlock_wrlock)*/ 
/*ND(__pthread_rwlock_trywrlock) ND(__pthread_rwlock_unlock)*/ 
/*ND(__res_state)*/ 
/*ND(open64) ND(__open64) 
ND(pread) ND(__pread64) ND(pread64) ND(pwrite) ND(__pwrite64) ND(pwrite64) ND(lseek64)*/ 
ND(pthread_attr_getstack) ND(pthread_attr_setstack) 
ND(pthread_barrier_destroy) ND(pthread_barrier_init) ND(pthread_barrier_wait) 
ND(pthread_barrierattr_destroy) ND(pthread_barrierattr_init) 
ND(pthread_barrierattr_getpshared) ND(pthread_barrierattr_setpshared) 
/*ND(pthread_condattr_getpshared) ND(pthread_condattr_setpshared) */
ND(pthread_getcpuclockid) 
ND(pthread_mutex_timedlock) 
/*ND(pthread_mutexattr_getpshared) ND(pthread_mutexattr_setpshared)*/ 
ND(pthread_rwlock_timedrdlock) ND(pthread_rwlock_timedwrlock) 
ND(pthread_spin_destroy) ND(pthread_spin_init) ND(pthread_spin_lock) 
ND(pthread_spin_trylock) ND(pthread_spin_unlock) 
/*ND(pthread_yield)*/ 
ND(sem_timedwait) 

ND(pthread_getattr_np) 

/*ND(__nanosleep)*/ 
#if 0
ND(__libc_internal_tsd_address) ND(__libc_alloca_cutoff) 
ND(__libc_internal_tsd_get) ND(__libc_internal_tsd_set) 
ND(__pthread_kill_other_threads_np) 
#endif

#endif
