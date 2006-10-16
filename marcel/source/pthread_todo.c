#include<sys/syscall.h>
#include<sys/types.h>
#include<unistd.h>
#include "errno.h"
#include "sys/marcel_flags.h"
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

#define NDP(prefix,function) \
        ND(prefix##pthread_##function) \
        ND(prefix##pmarcel_##function)

#define NDSP(function) NDP(_,function) 
#define NDSSP(function) NDP(__,function) 

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

NDSP(cleanup_pop_restore)
NDSP(cleanup_push_defer)
NDC(recvmsg)
NDP(,kill_other_threads_np)

NDP(,mutexattr_getrobust_np) NDP(,mutexattr_setrobust_np)
NDP(,mutex_consistent_np)

NDP(,mutexattr_getprotocol) NDP(,mutexattr_setprotocol)
NDP(,mutexattr_getprioceiling) NDP(,mutexattr_setprioceiling)
NDP(,mutex_getprioceiling) NDP(,mutex_setprioceiling)

NDP(,condattr_setclock) NDP(,condattr_getclock)

NDSSP(unwind)

pid_t wait(int *status)
{
	return syscall(SYS_wait4,-1,status,0,NULL);
}

NDC(system) NDC(tcdrain) NDC(msync) NDC(sendmsg) NDC(sendto)
#endif
