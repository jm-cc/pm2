/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: archdep.h,v $
Revision 1.2  2000/01/31 15:56:45  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef ARCHDEP_EST_DEF
#define ARCHDEP_EST_DEF

#ifdef SMP

#if defined(SOLARIS_SYS)
#include <thread.h>
static __inline__ void SCHED_YIELD(void)
{
  thr_yield();
}
#elif defined(LINUX_SYS)
#include <time.h>
#include <unistd.h>
/* The Linux 'sched_yield' syscall does not relinquish the processor
   immediately, so we user nanosleep(0) instead... */
static __inline__ void SCHED_YIELD(void)
{
  struct timespec t = { 0, 0 };

  nice(20); nanosleep(&t, NULL); nice(0);
}
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
#define SP_FIELD(buf)           ((buf)[1])
#endif

/* DEC Alpha farm */
#if defined(OSF_SYS) && defined(ALPHA_ARCH)
#define WINDOWSIZE              128
#define SP_FIELD(buf)           ((buf)[27])
#endif

/* Cray T3E */
#if defined(UNICOS_SYS) && defined(ALPHA_ARCH)
#define WINDOWSIZE		256
#define SP_FIELD(buf)           ((buf)[2])
#define FP_FIELD(buf)           ((buf)[3])
#endif

/* Any Intel x86 system */
#if defined(X86_ARCH)
#define WINDOWSIZE              32
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_SP])
#endif

/* IBM SP2 */
#if defined(AIX_SYS) && defined(RS6K_ARCH)
#define WINDOWSIZE              256
#define SP_FIELD(buf)           ((buf)[3])
#endif

/* Linux PPC */
#if defined(LINUX_SYS) && defined(PPC_ARCH)
#define WINDOWSIZE              256
#define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_GPR1])
#endif

/* SGI */
#if defined(IRIX_SYS) && defined(MIPS_ARCH)
#define WINDOWSIZE              128
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
#define call_ST_FLUSH_WINDOWS() /* Only exists for sparc */
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
