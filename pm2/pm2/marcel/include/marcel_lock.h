
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

#ifndef MARCEL_LOCK_EST_DEF
#define MARCEL_LOCK_EST_DEF

#include "sys/marcel_flags.h"
#include "pm2_testandset.h"

#define MARCEL_SPIN_ITERATIONS 100

#ifdef X86_ARCH
/* begining of X86_ARCH section */

#ifdef MA__LWPS
#define LOCK_PREFIX "lock; "
#else
#define LOCK_PREFIX ""
#endif

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int counter; } atomic_t;

#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)
#define atomic_set(v,i)		(((v)->counter) = (i))

static __inline__ 
void atomic_inc(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_inc(volatile atomic_t *v)
{
        __asm__ __volatile__(
                LOCK_PREFIX "incl %0"
                :"=m" (v->counter)
                :"m" (v->counter));
}

static __inline__ 
void atomic_dec(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_dec(volatile atomic_t *v)
{
        __asm__ __volatile__(
                LOCK_PREFIX "decl %0"
                :"=m" (v->counter)
                :"m" (v->counter));
}

/* end of X86_ARCH section */
#elif defined(ALPHA_ARCH)
/* begining of ALPHA_ARCH section */

#define __atomic_fool_gcc(x) (*(struct { int a[100]; } *)x)

typedef int atomic_t;

#define ATOMIC_INIT(i)	        (i)
#define atomic_read(v)		(*(v))
#define atomic_set(v, i)	(*(v) = (i))

extern __inline__ void atomic_add(atomic_t i, volatile atomic_t * v)
{
        unsigned long temp;
        __asm__ __volatile__(
                "\n1:\t"
                "ldl_l %0,%1\n\t"
                "addl %0,%2,%0\n\t"
                "stl_c %0,%1\n\t"
                "beq %0,1b\n"
                "2:"
                :"=&r" (temp),
                 "=m" (__atomic_fool_gcc(v))
                :"Ir" (i),
                 "m" (__atomic_fool_gcc(v)));
}

extern __inline__ void atomic_sub(atomic_t i, volatile atomic_t * v)
{
        unsigned long temp;
        __asm__ __volatile__(
                "\n1:\t"
                "ldl_l %0,%1\n\t"
                "subl %0,%2,%0\n\t"
                "stl_c %0,%1\n\t"
                "beq %0,1b\n"
                "2:"
                :"=&r" (temp),
                 "=m" (__atomic_fool_gcc(v))
                :"Ir" (i),
                 "m" (__atomic_fool_gcc(v)));
}

#define atomic_inc(v) atomic_add(1,(v))
#define atomic_dec(v) atomic_sub(1,(v))

/* end of ALPHA_ARCH section */
#else
/* begining of generic (fallback) section */

typedef unsigned atomic_t;

#define ATOMIC_INIT(i)	        (i)
#define atomic_read(v)		(*(v))
#define atomic_set(v, i)	(*(v) = (i))

static __inline__ 
void atomic_inc(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_inc(volatile atomic_t *v)
{
  (*v)++;
}

static __inline__ 
void atomic_dec(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_dec(volatile atomic_t *v)
{
  (*v)--;
}

/* end of begining (fallback) section */
#endif

/* 
 * Generic Part.
 */

typedef unsigned marcel_lock_t;

#define MARCEL_LOCK_INIT_UNLOCKED   0
#define MARCEL_LOCK_INIT_LOCKED     1
#define MARCEL_LOCK_INIT            MARCEL_LOCK_INIT_UNLOCKED

void marcel_lock_init(marcel_lock_t *lock);

static __inline__ void marcel_lock_acquire(marcel_lock_t *lock)
{
#ifdef MA__LWPS
  unsigned counter = 0;

  while(pm2_spinlock_testandset(lock)) {
    if(++counter > MARCEL_SPIN_ITERATIONS) {
      SCHED_YIELD();
      counter=0;
    }
  }
#endif
}

static __inline__ 
unsigned marcel_lock_tryacquire(marcel_lock_t *lock) __attribute__ ((unused));
static __inline__ 
unsigned marcel_lock_tryacquire(marcel_lock_t *lock)
{
#ifdef MA__LWPS
  return !pm2_spinlock_testandset(lock);
#else
  return 1;
#endif
}
static __inline__ void marcel_lock_release(marcel_lock_t *lock)
{
#ifdef MA__LWPS
  pm2_spinlock_release(lock);
#endif
}

static __inline__ unsigned marcel_lock_locked(marcel_lock_t *lock)
{
  return *lock;
}

#endif


