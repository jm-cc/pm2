
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

#ifndef PM2_TESTANDSET_EST_DEF
#define PM2_TESTANDSET_EST_DEF

/* 
 * Architecture-dependant definitions.
 * Most are extracted from Linux kernel sources & LinuxThreads sources.
 */

#ifdef X86_ARCH
static __inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock) __attribute__ ((unused));
static __inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  unsigned ret;

  __asm__ __volatile__(
       "xchgl %0, %1"
       : "=r"(ret), "=m"(*spinlock)
       : "0"(1), "m"(*spinlock)
       : "memory");

  return ret;
}

#define pm2_spinlock_release(spinlock) (*(spinlock) = 0)

#endif /* X86_ARCH */

#ifdef SPARC_ARCH
static __inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  char ret = 0;

  __asm__ __volatile__("ldstub [%0], %1"
	: "=r"(spinlock), "=r"(ret)
	: "0"(spinlock), "1" (ret) : "memory");

  return (unsigned)ret;
}

#define pm2_spinlock_release(spinlock) \
  __asm__ __volatile__("stbar\n\tstb %1,%0" : "=m"(*(spinlock)) : "r"(0));
#endif /* SPARC_ARCH */

#ifdef ALPHA_ARCH
static __inline__ long unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  long unsigned ret, temp;

  __asm__ __volatile__(
	"\n1:\t"
	"ldl_l %0,%3\n\t"
	"bne %0,2f\n\t"
	"or $31,1,%1\n\t"
	"stl_c %1,%2\n\t"
	"beq %1,1b\n"
	"2:\tmb\n"
	: "=&r"(ret), "=&r"(temp), "=m"(*spinlock)
	: "m"(*spinlock)
        : "memory");

  return ret;
}

#define pm2_spinlock_release(spinlock) \
  __asm__ __volatile__("mb" : : : "memory"); \
  *spinlock = 0

#endif /* ALPHA_ARCH */

#ifdef RS6K_ARCH
#define pm2_spinlock_testandset(volatile unsigned *spinlock) \
  (*(spinlock) ? 1 : (*(spinlock)=1,0))

#define pm2_spinlock_release(spinlock) (*(spinlock) = 0)
#endif /* RS6K_ARCH */

#ifdef MIPS_ARCH
static __inline__ long unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  long unsigned ret, temp;

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

#define pm2_spinlock_release(spinlock) (*(spinlock) = 0)
#endif /* MIPS_ARCH */

#ifdef PPC_ARCH
#define sync() __asm__ __volatile__ ("sync")

static __inline__ unsigned __compare_and_swap (long unsigned *p, long unsigned oldval, long unsigned newval)
{
  unsigned ret;

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

#define pm2_spinlock_testandset(spinlock) __compare_and_swap(spinlock, 0, 1)

#define pm2_spinlock_release(spinlock) (*(spinlock) = 0)
#endif /* PPC_ARCH */

#endif /* TESTANDSET_EST_DEF */
