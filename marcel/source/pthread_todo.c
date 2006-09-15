#include "errno.h"
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
    return ENOSYS; \
  }

#define NDC(function) \
  int function(void) { \
    if (!infile) { \
      infile=1; \
      printf("libpthread(marcel): %s not yet implemented\n", __func__); \
      infile=0; \
    } \
    errno = ENOSYS; \
    return -1; \
  }


#define ND2(function)

#define ND3(function) \
  int function(void) { \
        return 0; \
  }

ND(_pthread_cleanup_pop_restore)
ND(_pthread_cleanup_push_defer)
NDC(recvmsg)
ND(pthread_kill_other_threads_np)

ND(pthread_mutex_timedlock)
ND(pthread_rwlock_timedrdlock) ND(pthread_rwlock_timedwrlock)

ND(pthread_mutexattr_getrobust_np) ND(pthread_mutexattr_setrobust_np)
ND(pthread_mutex_consistent_np)

ND(pthread_mutexattr_getprotocol) ND(pthread_mutexattr_setprotocol)
ND(pthread_mutexattr_getprioceiling) ND(pthread_mutexattr_setprioceiling)
ND(pthread_mutex_getprioceiling) ND(pthread_mutex_setprioceiling)

ND(__pthread_unwind)

NDC(__wait) NDC(system) NDC(tcdrain) NDC(wait) NDC(msync) NDC(sendmsg) NDC(sendto)
#endif
