
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

#ifdef MA__LWPS
#define LOCK_PREFIX "lock\n\t"
#else
#define LOCK_PREFIX ""
#endif

#define __atomic_fool_gcc(x) (*(volatile struct { int a[100]; } *)x)

#if defined(MA__LWPS) || defined(MA__ACT)
typedef struct { volatile int counter; } atomic_t;
#else
typedef struct { int counter; } atomic_t;
#endif

#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)
#define atomic_set(v,i)		(((v)->counter) = (i))

static __inline__ 
void atomic_inc(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_inc(volatile atomic_t *v)
{
	__asm__ __volatile__(
		LOCK_PREFIX "incl %0"
		:"=m" (__atomic_fool_gcc(v))
		:"m" (0));
}

static __inline__ 
void atomic_dec(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_dec(volatile atomic_t *v)
{
	__asm__ __volatile__(
		LOCK_PREFIX "decl %0"
		:"=m" (__atomic_fool_gcc(v))
		:"m" (0));
}

#else /* X86_ARCH */

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

#endif /* X86_ARCH */

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

static __inline__ unsigned marcel_lock_tryacquire(marcel_lock_t *lock) __attribute__ ((unused));
static __inline__ unsigned marcel_lock_tryacquire(marcel_lock_t *lock)
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

#endif


