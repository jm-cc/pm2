
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

#ifndef ARCHDEP_EST_DEF
#define ARCHDEP_EST_DEF

#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"

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
#include <sched.h>

#if 1
static __inline__ void SCHED_YIELD(void)
{
  sched_yield();
}

#else // 1
/* The Linux 'sched_yield' syscall does not relinquish the processor
   immediately, so we use nanosleep(0) instead... */
static __inline__ void SCHED_YIELD(void)
{
  struct timespec t = { 0, 0 };

  nice(20); nanosleep(&t, NULL); nice(0);
}
#endif // 1

#else // ni SOLARIS ni LINUX

#include <sched.h>
static __inline__ void SCHED_YIELD(void)
{
  sched_yield();
}
#endif

#else // MA__LWP

#define SCHED_YIELD()  do {} while(0)

#endif // MA__LWP


/* Solaris sparc */
#if defined(SOLARIS_SYS) && defined(SPARC_ARCH)
#  define STACK_INFO
#  include <sys/stack.h>
#  define TOP_STACK_FREE_AREA     (WINDOWSIZE+128)
#  define SP_FIELD(buf)           ((buf)[1])
#endif

/* Alpha Linux */
#if defined(ALPHA_ARCH) && defined(LINUX_SYS)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     128
#  define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_SP])
#endif

/* Alpha Tru64 */
#if defined(ALPHA_ARCH) && defined(OSF_SYS)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     128
#  define JB_REGS         4               /* registers */
#  define JB_SP           (JB_REGS+30)    /* stack pointer */
#  define SP_FIELD(buf)           ((buf)[JB_SP])
#endif

/* Cray T3E */
#if defined(UNICOS_SYS) && defined(ALPHA_ARCH)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA	256
#  define SP_FIELD(buf)           ((buf)[2])
#  define FP_FIELD(buf)           ((buf)[3])
#endif

/* Any Intel x86 system */
#if defined(X86_ARCH)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     64
#  define SP_FIELD(buf)           ((buf)[MARCEL_JB_SP])
#endif

/* Itanium */
#if defined(IA64_ARCH)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     64
#  define JB_SP                   12
#  define SP_FIELD(buf)           ((buf).uc_mcontext.sc_gr[JB_SP])
#  define BSP_FIELD(buf)          ((buf).uc_mcontext.sc_ar_bsp)
#endif

/* IBM SP2 */
#if defined(AIX_SYS) && defined(RS6K_ARCH)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     256
#  define SP_FIELD(buf)           ((buf)[3])
#endif

/* Linux PPC */
#if defined(LINUX_SYS) && defined(PPC_ARCH)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     256
#  define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_GPR1])
#endif

/* SGI */
#if defined(IRIX_SYS) && defined(MIPS_ARCH)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     128
#  define SP_FIELD(buf)           ((buf)[JB_SP])
#endif

/* Darwin PPC (Mac OS X) */
#if defined(DARWIN_SYS) && defined(PPC_ARCH)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     256
#  define SP_FIELD(buf)           ((buf)[0])
#endif


/* ******************* Sparc spécific ******************* */
#if defined(SPARC_ARCH)
extern void call_ST_FLUSH_WINDOWS(void);
#else
#  define call_ST_FLUSH_WINDOWS()  (void)0
#endif

/* ******************* Sparc ******************* */

#if defined(SPARC_ARCH)
#  define SET_GET_SP_DEFINED
static __inline__ long get_sp()
{
  register long sp;

  __asm__ __volatile__("mov %%sp, %0" : "=r" (sp));
  return sp;
}
#  define set_sp(val) \
    __asm__ __volatile__("mov %0, %%sp\n\t" \
                         : : "r" (val) : "memory")
#endif


/* ******************* Intel ******************* */

#if defined(X86_ARCH)
#  define SET_GET_SP_DEFINED

#  ifdef MARCEL_SELF_IN_REG
//register marcel_t __marcel_self_in_reg asm ("%%gs");
#    define SET_MARCEL_SELF_FROM_SP(sp) \
       __asm__ __volatile__("movl %0, %%gs" \
                         : : "m" ( \
       ((((sp) & ~(SLOT_SIZE-1)) + SLOT_SIZE) - MAL(sizeof(task_desc)))\
        ) : "memory" )
#  else
#    define SET_MARCEL_SELF_FROM_SP(val) (void)(0)
#  endif

static __inline__ long get_gs(void)
{
  register long gs;

    __asm__ __volatile__("movl %%gs, %0" : "=r" (gs));
    return gs;
}

static __inline__ long get_sp(void)
{
  register long sp;

  __asm__ __volatile__("movl %%esp, %0" : "=r" (sp));
  return sp;
}
#  define set_sp(val) \
  do { \
    typeof(val) value=(val); \
    SET_MARCEL_SELF_FROM_SP(value); \
    __asm__ __volatile__("movl %0, %%esp" \
                       : : "m" (value) : "memory" ); \
  } while (0)
#endif


/* ******************* Intel IA64 ******************* */

#if defined(IA64_ARCH)
#  define SET_GET_SP_DEFINED

#  define SET_MARCEL_SELF_FROM_SP(val) (void)(0)
static __inline__ long get_sp(void)
{
  register long sp;

  __asm__ __volatile__(
		  ";; \n\t" \
		  "mov %0 = sp ;; \n\t"
		  ";; \n\t" \
		  : "=r" (sp));

  return sp;
}
static __inline__ long get_bsp(void)
{
  register long bsp;

  __asm__ __volatile__(
		  ";; \n\t" \
		  "flushrs \n\t" \
		  "mov %0 = ar.bsp ;; \n\t"
		  ";; \n\t" \
		  : "=r" (bsp));

  return bsp;
}
#  define set_sp(val) \
  do { \
    SET_MARCEL_SELF_FROM_SP(val); \
    __asm__ __volatile__( \
		    ";; \n\t" \
		    "mov sp = %0 \n\t" \
		    ";; \n\t" \
                       : : "r" (val) : "memory", "sp" ); \
  } while (0)
#define rTMP "%0"
#define rRSC "%1"
#  define set_sp_bsp(val, bsp) \
  do { \
    SET_MARCEL_SELF_FROM_SP(val); \
    __asm__ __volatile__( \
		    ";; \n\t" \
		    "flushrs \n\t" \
		    ";; \n\t" \
		    "mov.m "rRSC" = ar.rsc \n\t" \
		    ";; \n\t" \
		    "dep "rTMP" = 0, "rRSC", 16, 14 \n\t" \
		    ";; \n\t" \
		    "and "rTMP" = ~0x3, "rTMP" \n\t" \
		    ";; \n\t" \
		    "mov.m ar.rsc = "rTMP" \n\t" \
		    ";; \n\t" \
		    "loadrs \n\t" \
		    ";; \n\t" \
		    "mov.m ar.bspstore = %3 \n\t" \
		    ";; \n\t" \
		    "mov.m ar.rsc = "rRSC" \n\t" \
		    ";; \n\t" \
		    "mov sp = %2 \n\t" \
		    ";; \n\t" \
                       : : "r" (0), "r"(0), "r" (val), "r" (bsp) : "memory", "sp" ); \
  } while (0)
#endif


/* ******************* Mips ******************* */

#if defined(MIPS_ARCH)
#  define SET_GET_SP_DEFINED

static __inline__ long get_sp()
{
  register long sp;

  __asm__ __volatile__("move %0, $sp" : "=r" (sp));
  return sp;
}
#  define set_sp(val) \
  __asm__ __volatile__("move $sp, %0" \
                       : : "r" (val) : "memory" )
#endif


/* ******************* Alpha ******************* */

#if defined(ALPHA_ARCH)
#  define SET_GET_SP_DEFINED

static __inline__ long get_sp(void)
{
  register long sp;

  __asm__ __volatile__("addq $sp, $31, %0" : "=r" (sp));
  return sp;
}
#  define set_sp(val) \
  __asm__ __volatile__("addq %0, $31, $sp" \
                       : : "r" (val) : "memory" )
#endif


/* *********** PowerPC, RS6000 *********** */

#if defined(PPC_ARCH) || defined(RS6K_ARCH)
#  define SET_GET_SP_DEFINED

extern void set_sp(long);
extern long get_sp(void);
#endif

/* If MEMORY_BARRIER isn't defined in pt-machine.h, assume the
   architecture doesn't need a memory barrier instruction (e.g. Intel
   x86).  Still we need the compiler to respect the barrier and emit
   all outstanding operations which modify memory.  Some architectures
   distinguish between full, read and write barriers.  */

#  ifndef MEMORY_BARRIER
#    define MEMORY_BARRIER() asm ("" : : : "memory")
#  endif
#  ifndef READ_MEMORY_BARRIER
#    define READ_MEMORY_BARRIER() MEMORY_BARRIER()
#  endif
#  ifndef WRITE_MEMORY_BARRIER
#    define WRITE_MEMORY_BARRIER() MEMORY_BARRIER()
#  endif

#ifndef SET_GET_SP_DEFINED
#  error set_sp and get_sp not defined for the current ARCH
#else
#  undef SET_GET_SP_DEFINED
#endif

#ifndef STACK_INFO
#  error TOP_STACK_FREE_AREA and SP_FIELD not defined for the current ARCH
#else
#  undef STACK_INFO
#endif

#endif
