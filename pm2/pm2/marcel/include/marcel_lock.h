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
$Log: marcel_lock.h,v $
Revision 1.4  2000/03/01 16:45:22  oaumage
- suppression des warnings en compilation  -g

Revision 1.3  2000/01/31 15:56:23  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_LOCK_EST_DEF
#define MARCEL_LOCK_EST_DEF

#define MARCEL_SPIN_ITERATIONS 100

/* 
 * Architecture-dependant definitions.
 * Most are extracted from Linux kernel sources & LinuxThreads sources.
 */

#ifdef X86_ARCH

#ifdef SMP
#define LOCK_PREFIX "lock\n\t"
#else
#define LOCK_PREFIX ""
#endif

#define __atomic_fool_gcc(x) (*(volatile struct { int a[100]; } *)x)

#ifdef SMP
typedef struct { volatile int counter; } atomic_t;
#else
#ifndef __ACT__
typedef struct { int counter; } atomic_t;
#endif
#endif

#ifdef __ACT__
#include <asm/atomic.h>
#else

#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)
#define atomic_set(v,i)		(((v)->counter) = (i))

static __inline__ void atomic_inc(volatile atomic_t *v)
{
	__asm__ __volatile__(
		"incl %0"
		:"=m" (__atomic_fool_gcc(v))
		:"m" (0));
}

static __inline__ void atomic_dec(volatile atomic_t *v)
{
	__asm__ __volatile__(
		"decl %0"
		:"=m" (__atomic_fool_gcc(v))
		:"m" (0));
}

#endif  /* __ACT__ */

static __inline__ int testandset(int *spinlock) __attribute__ ((unused));
static __inline__ int testandset(int *spinlock)
{
  int ret;

  __asm__ __volatile__(
       "xchgl %0, %1"
       : "=r"(ret), "=m"(*spinlock)
       : "0"(1), "m"(*spinlock)
       : "memory");

  return ret;
}

#define release(spinlock) (*(spinlock) = 0)

#endif /* ix86 */


#ifdef SPARC_ARCH

static __inline__ int testandset(int *spinlock)
{
  char ret = 0;

  __asm__ __volatile__("ldstub [%0], %1"
	: "=r"(spinlock), "=r"(ret)
	: "0"(spinlock), "1" (ret) : "memory");

  return (int)ret;
}

#define release(spinlock) \
  __asm__ __volatile__("stbar\n\tstb %1,%0" : "=m"(*(spinlock)) : "r"(0));

#endif /* SPARC */

#ifdef ALPHA_ARCH

static __inline__ long int testandset(int *spinlock)
{
  long int ret, temp;

  __asm__ __volatile__(
	"/* Inline spinlock test & set */\n"
	"1:\t"
	"ldl_l %0,%3\n\t"
	"bne %0,2f\n\t"
	"or $31,1,%1\n\t"
	"stl_c %1,%2\n\t"
	"beq %1,1b\n"
	"2:\tmb\n"
	"/* End spinlock test & set */"
	: "=&r"(ret), "=&r"(temp), "=m"(*spinlock)
	: "m"(*spinlock)
        : "memory");

  return ret;
}

#define release(spinlock) \
  __asm__ __volatile__("mb" : : : "memory"); \
  *spinlock = 0

#endif /* ALPHA */

#ifdef RS6K_ARCH

#define testandset(int *spinlock) \
  (*(spinlock) ? 1 : (*(spinlock)=1,0))

#define release(spinlock) (*(spinlock) = 0)

#endif

#ifdef MIPS_ARCH

static __inline__ long int testandset(int *spinlock)
{
  long int ret, temp;

  __asm__ __volatile__(
        ".set\tmips2\n"
        "1:\tll\t%0,0(%2)\n\t"
        "bnez\t%0,2f\n\t"
        ".set\tnoreorder\n\t"
        "li\t%1,1\n\t"
        "sc\t%1,0(%2)\n\t"
        ".set\treorder\n\t"
        "beqz\t%1,1b\n\t"
        "2:\t.set\tmips0\n\t"
        : "=&r"(ret), "=&r" (temp)
        : "r"(spinlock)
        : "memory");

  return ret;
}

#define release(spinlock) (*(spinlock) = 0)

#endif /* MIPS */

#ifdef PPP_ARCH

#define sync() __asm__ __volatile__ ("sync")

static __inline__ int __compare_and_swap (long int *p, long int oldval, long int newval)
{
  int ret;

  sync();
  __asm__ __volatile__(
		       "0:    lwarx %0,0,%1 ;"
		       "      xor. %0,%3,%0;"
		       "      bne 1f;"
		       "      stwcx. %2,0,%1;"
		       "      bne- 0b;"
		       "1:    "
	: "=&r"(ret)
	: "r"(p), "r"(newval), "r"(oldval)
	: "cr0", "memory");
  sync();
  return ret == 0;
}

#define testandset(spinlock) __compare_and_swap(spinlock, 0, 1)

#define release(spinlock) (*(spinlock) = 0)

#endif /* POWERPC */


/* 
 * Generic Part.
 */


typedef unsigned marcel_lock_t;

#define MARCEL_LOCK_INIT   0

void marcel_lock_init(marcel_lock_t *lock);

static __inline__ void marcel_lock_acquire(marcel_lock_t *lock)
{
#ifdef SMP
  unsigned counter = 0;

  while(testandset(lock)) {
    if(++counter > MARCEL_SPIN_ITERATIONS)
      SCHED_YIELD();
  }
#endif
}

static __inline__ unsigned marcel_lock_tryacquire(marcel_lock_t *lock) __attribute__ ((unused));
static __inline__ unsigned marcel_lock_tryacquire(marcel_lock_t *lock)
{
#ifdef SMP
  return !testandset(lock);
#else
  return 1;
#endif
}
static __inline__ void marcel_lock_release(marcel_lock_t *lock)
{
#ifdef SMP
  release(lock);
#endif
}

#endif
