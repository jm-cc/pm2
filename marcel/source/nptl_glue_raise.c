
#include "marcel.h"
#ifdef MA__PTHREAD_FUNCTIONS

#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>

#undef _syscall2
#define _syscall2(type,name,type1,arg1,type2,arg2) \
        type name(type1 arg1,type2 arg2) \
{ \
        long __res; \
        __asm__ volatile ("push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx" \
           : "=a" (__res) \
           : "0" (__NR_##name),"r" ((long)(arg1)),"c" ((long)(arg2)) \
           ); \
        __syscall_return(type,__res); \
}

static inline _syscall0(pid_t,gettid)
static inline _syscall2(int, tkill, int, tid, int, sig)
//_syscall3(int, tgkill, int, tgid, int, tid, int, sig)

int raise (int sig)
{
	//pid_t pid=getpid();
	pid_t selftid=gettid();
	return tkill(selftid, sig);
}

#endif /* MA__PTHREAD_FUNCTIONS */
