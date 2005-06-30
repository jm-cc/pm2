/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

#if !defined _BITS_TYPES_H && !defined _PMARCEL_H
# error "Never include <bits/pmarceltypes.h> directly; use <sys/types.h> instead."
#endif

#ifndef _BITS_PMARCELTYPES_H
#define _BITS_PMARCELTYPES_H	1

#include <sched.h>

typedef int pm__atomic_lock_t;

/* Fast locks (not abstract because mutexes and conditions aren't abstract). */
struct _pmarcel_fastlock
{
  unsigned long __status;	/* "Free" or "taken" or head of waiting list */
  pm__atomic_lock_t __spinlock;	/* Used by compare_and_swap emulation. Also,
				   adaptive SMP lock stores spin count here. */
};

#ifndef _PMARCEL_DESCR_DEFINED
/* Thread descriptors */
typedef struct _pmarcel_descr_struct *_pmarcel_descr;
# define _PMARCEL_DESCR_DEFINED
#endif


/* Attributes for threads.  */
typedef struct __pmarcel_attr_s
{
  int __detachstate;
  int __schedpolicy;
  struct sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
} pmarcel_attr_t;


/* Conditions (not abstract because of PMARCEL_COND_INITIALIZER */
typedef struct
{
  struct _pmarcel_fastlock __c_lock; /* Protect against concurrent access */
  _pmarcel_descr __c_waiting;        /* Threads waiting on this condition */
} pmarcel_cond_t;


/* Attribute for conditionally variables.  */
typedef struct
{
  int __dummy;
} pmarcel_condattr_t;

/* Keys for thread-specific data */
typedef unsigned int pmarcel_key_t;


/* Mutexes (not abstract because of PMARCEL_MUTEX_INITIALIZER).  */
/* (The layout is unnatural to maintain binary compatibility
    with earlier releases of LinuxThreads.) */
typedef struct
{
  int __m_reserved;               /* Reserved for future use */
  int __m_count;                  /* Depth of recursive locking */
  _pmarcel_descr __m_owner;       /* Owner thread (if recursive or errcheck) */
  int __m_kind;                   /* Mutex kind: fast, recursive or errcheck */
  struct _pmarcel_fastlock __m_lock; /* Underlying fast lock */
} pmarcel_mutex_t;


/* Attribute for mutex.  */
typedef struct
{
  int __mutexkind;
} pmarcel_mutexattr_t;


/* Once-only execution */
typedef int pmarcel_once_t;


#ifdef __USE_UNIX98
/* Read-write locks.  */
typedef struct _pmarcel_rwlock_t
{
  struct _pmarcel_fastlock __rw_lock; /* Lock to guarantee mutual exclusion */
  int __rw_readers;                   /* Number of readers */
  _pmarcel_descr __rw_writer;         /* Identity of writer, or NULL if none */
  _pmarcel_descr __rw_read_waiting;   /* Threads waiting for reading */
  _pmarcel_descr __rw_write_waiting;  /* Threads waiting for writing */
  int __rw_kind;                      /* Reader/Writer preference selection */
  int __rw_pshared;                   /* Shared between processes or not */
} pmarcel_rwlock_t;


/* Attribute for read-write locks.  */
typedef struct
{
  int __lockkind;
  int __pshared;
} pmarcel_rwlockattr_t;
#endif

#ifdef __USE_XOPEN2K
/* POSIX spinlock data type.  */
typedef volatile int pmarcel_spinlock_t;

/* POSIX barrier. */
typedef struct {
  struct _pmarcel_fastlock __ba_lock; /* Lock to guarantee mutual exclusion */
  int __ba_required;                  /* Threads needed for completion */
  int __ba_present;                   /* Threads waiting */
  _pmarcel_descr __ba_waiting;        /* Queue of waiting threads */
} pmarcel_barrier_t;

/* barrier attribute */
typedef struct {
  int __pshared;
} pmarcel_barrierattr_t;

#endif


/* Thread identifiers */
typedef unsigned long int pmarcel_t;

#endif	/* bits/pmarceltypes.h */
