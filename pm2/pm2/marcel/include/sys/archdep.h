
#ifndef ARCHDEP_EST_DEF
#define ARCHDEP_EST_DEF

#include "sys/marcel_flags.h"
#include "sys/marcel_for_win.h"

#ifdef MA__LWPS

#if defined(SOLARIS_SYS)
#include <thread.h>
static __inline__ void SCHED_YIELD(void)
{
  thr_yield();
}
#elif defined(LINUX_SYS)
#include <time.h>
#include <unistd.h>

#ifdef MA__ACTIVATIONS
static __inline__ void SCHED_YIELD(void)
{
  sched_yield();
}

#else
/* The Linux 'sched_yield' syscall does not relinquish the processor
   immediately, so we user nanosleep(0) instead... */
static __inline__ void SCHED_YIELD(void)
{
  struct timespec t = { 0, 0 };

  nice(20); nanosleep(&t, NULL); nice(0);
}
#endif
#else
#include <sched.h>
static __inline__ void SCHED_YIELD(void)
{
  sched_yield();
}
#endif
#endif


/* Solaris sparc */
#if defined(SOLARIS_SYS) && defined(SPARC_ARCH)
#define TOP_STACK_FREE_AREA     (WINDOWSIZE+128)
#define SP_FIELD(buf)           ((buf)[1])
#endif

/* DEC Alpha farm */
#if defined(OSF_SYS) && defined(ALPHA_ARCH)
#define TOP_STACK_FREE_AREA     128
#define SP_FIELD(buf)           ((buf)[27])
#endif

/* Cray T3E */
#if defined(UNICOS_SYS) && defined(ALPHA_ARCH)
#define TOP_STACK_FREE_AREA	256
#define SP_FIELD(buf)           ((buf)[2])
#define FP_FIELD(buf)           ((buf)[3])
#endif

/* Any Intel x86 system */
#if defined(X86_ARCH)
#define TOP_STACK_FREE_AREA     64
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_SP])
#endif

/* IBM SP2 */
#if defined(AIX_SYS) && defined(RS6K_ARCH)
#define TOP_STACK_FREE_AREA     256
#define SP_FIELD(buf)           ((buf)[3])
#endif

/* Linux PPC */
#if defined(LINUX_SYS) && defined(PPC_ARCH)
#define TOP_STACK_FREE_AREA     256
#define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_GPR1])
#endif

/* SGI */
#if defined(IRIX_SYS) && defined(MIPS_ARCH)
#define TOP_STACK_FREE_AREA     128
#define SP_FIELD(buf)           ((buf)[JB_SP])
#endif


/* ******************* Sparc ******************* */

#if defined(SPARC_ARCH)
static __inline__ long get_sp()
{
  register long sp;

  __asm__ __volatile__("mov %%sp, %0" : "=r" (sp));
  return sp;
}
#define set_sp(val) \
    __asm__ __volatile__("mov %0, %%sp\n\t" \
                         : : "r" (val) : "memory")
extern void call_ST_FLUSH_WINDOWS(void);
#else
#define call_ST_FLUSH_WINDOWS()  (void)0
#endif


/* ******************* Intel ******************* */

#if defined(X86_ARCH)
static __inline__ long get_sp()
{
  register long sp;

  __asm__ __volatile__("movl %%esp, %0" : "=r" (sp));
  return sp;
}
#define set_sp(val) \
  __asm__ __volatile__("movl %0, %%esp" \
                       : : "m" (val) : "memory" )
#endif


/* ******************* Mips ******************* */

#if defined(MIPS_ARCH)
static __inline__ long get_sp()
{
  register long sp;

  __asm__ __volatile__("move %0, $sp" : "=r" (sp));
  return sp;
}
#define set_sp(val) \
  __asm__ __volatile__("move $sp, %0" \
                       : : "r" (val) : "memory" )
#endif


/* ******************* Alpha ******************* */

#if defined(ALPHA_ARCH)
extern void set_sp(long);
extern void get_sp(long *);
#endif


/* *********** PowerPC, RS6000 *********** */

#if defined(PPC_ARCH) || defined(RS6K_ARCH)
extern void set_sp(long);
extern long get_sp(void);
#endif


#endif
