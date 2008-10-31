#include<sys/types.h>
#include<unistd.h>
#include "errno.h"
#include "sys/marcel_flags.h"
#include "tbx_compiler.h"

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

static int TBX_UNUSED infile=0;

#define ND(function) \
  int function(void) { \
    if (!infile) { \
      infile=1; \
      printf("libpthread(marcel): %s not yet implemented\n", __func__); \
      infile=0; \
    } \
    return ENOSYS; \
  }

#ifdef MA__LIBPTHREAD
#define NDP(prefix,function) \
        ND(prefix##pthread_##function) \
        int prefix##pmarcel_##function(void); \
        ND(prefix##pmarcel_##function)
#elif defined(MA__IFACE_PMARCEL)
#define NDP(prefix,function) \
        int prefix##pmarcel_##function(void); \
        ND(prefix##pmarcel_##function)
#else
#define NDP(prefix,function)
#endif

#define NDSP(function) NDP(_,function) 
#define NDSSP(function) NDP(__,function) 

#ifdef MA__LIBPTHREAD
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
#else
#define NDC(function)
#endif

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

NDC(tcdrain) NDC(msync) NDC(sendmsg) 
//NDC(sendto)
