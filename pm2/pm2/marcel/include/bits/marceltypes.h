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

#if !defined _MARCEL_H
# error "Never include <bits/marceltypes.h> directly; use <marcel_pthread.h> instead."
#endif

#ifndef _BITS_MARCELTYPES_H
#define _BITS_MARCELTYPES_H	1

#define __need_schedparam
#include <bits/sched.h>

typedef int __marcel_atomic_lock_t;

/* Fast locks (not abstract because mutexes and conditions aren't abstract). */
struct _marcel_fastlock
{
  long int __status;   /* "Free" or "taken" or head of waiting list */
  __marcel_atomic_lock_t __spinlock;  /* Used by compare_and_swap emulation. Also,
			  adaptive SMP lock stores spin count here. */
};

#ifndef _MARCEL_DESCR_DEFINED
/* Thread descriptors */
typedef struct _marcel_descr_struct *_marcel_descr;
# define _MARCEL_DESCR_DEFINED
#endif

/* VP mask: useful for selecting the set of "forbiden" LWP for a given thread */
typedef unsigned long marcel_vpmask_t;

/* Attributes for threads.  */
typedef struct __marcel_attr_s
{
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
  /* marcel attributs */
  //unsigned stack_size;
  //char *stack_base;
  //int /*boolean*/ detached;
  unsigned user_space;
  /*boolean*/int immediate_activation;
  unsigned not_migratable;
  unsigned not_deviatable;
  //int sched_policy;
  /*boolean*/int rt_thread;
  marcel_vpmask_t vpmask;
  int flags;
} marcel_attr_t;

/* Conditions (not abstract because of MARCEL_COND_INITIALIZER */
typedef struct
{
  struct _marcel_fastlock __c_lock; /* Protect against concurrent access */
  _marcel_descr __c_waiting;        /* Threads waiting on this condition */
} marcel_cond_t;


/* Attribute for conditionally variables.  */
typedef struct
{
  int __dummy;
} marcel_condattr_t;

/* Keys for thread-specific data */
typedef unsigned int marcel_key_t;


/* Mutexes (not abstract because of MARCEL_MUTEX_INITIALIZER).  */
/* (The layout is unnatural to maintain binary compatibility
    with earlier releases of LinuxThreads.) */
typedef struct
{
  int __m_reserved;               /* Reserved for future use */
  int __m_count;                  /* Depth of recursive locking */
  _marcel_descr __m_owner;       /* Owner thread (if recursive or errcheck) */
  int __m_kind;                   /* Mutex kind: fast, recursive or errcheck */
  struct _marcel_fastlock __m_lock; /* Underlying fast lock */
} marcel_mutex_t;


/* Attribute for mutex.  */
typedef struct
{
  int __mutexkind;
} marcel_mutexattr_t;


/* Once-only execution */
typedef int marcel_once_t;


#ifdef __USE_UNIX98
/* Read-write locks.  */
typedef struct _marcel_rwlock_t
{
  struct _marcel_fastlock __rw_lock; /* Lock to guarantee mutual exclusion */
  int __rw_readers;                   /* Number of readers */
  _marcel_descr __rw_writer;         /* Identity of writer, or NULL if none */
  _marcel_descr __rw_read_waiting;   /* Threads waiting for reading */
  _marcel_descr __rw_write_waiting;  /* Threads waiting for writing */
  int __rw_kind;                      /* Reader/Writer preference selection */
  int __rw_pshared;                   /* Shared between processes or not */
} marcel_rwlock_t;


/* Attribute for read-write locks.  */
typedef struct
{
  int __lockkind;
  int __pshared;
} marcel_rwlockattr_t;
#endif

#ifdef __USE_XOPEN2K
/* POSIX spinlock data type.  */
typedef volatile int marcel_spinlock_t;

/* POSIX barrier. */
typedef struct {
  struct _marcel_fastlock __ba_lock; /* Lock to guarantee mutual exclusion */
  int __ba_required;                  /* Threads needed for completion */
  int __ba_present;                   /* Threads waiting */
  _marcel_descr __ba_waiting;        /* Queue of waiting threads */
} marcel_barrier_t;

/* barrier attribute */
typedef struct {
  int __pshared;
} marcel_barrierattr_t;

#endif


/* Thread identifiers */
typedef struct _marcel_desc_struct *marcel_t;
/* typedef unsigned long int marcel_t; */

#endif	/* bits/marceltypes.h */
