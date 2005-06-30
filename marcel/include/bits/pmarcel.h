/* Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _BITS_PMARCELTYPES_H
#define _BITS_PMARCELTYPES_H	1

#define __SIZEOF_PMARCEL_ATTR_T 36
#define __SIZEOF_PMARCEL_MUTEX_T 24
#define __SIZEOF_PMARCEL_MUTEXATTR_T 4
#define __SIZEOF_PMARCEL_COND_T 48
#define __SIZEOF_PMARCEL_COND_COMPAT_T 12
#define __SIZEOF_PMARCEL_CONDATTR_T 4
#define __SIZEOF_PMARCEL_RWLOCK_T 32
#define __SIZEOF_PMARCEL_RWLOCKATTR_T 8
#define __SIZEOF_PMARCEL_BARRIER_T 20
#define __SIZEOF_PMARCEL_BARRIERATTR_T 4


/* Thread identifiers.  The structure of the attribute type is not
   exposed on purpose.  */
typedef struct __opaque_pmarcel *pmarcel_t;


typedef union
{
  char __size[__SIZEOF_PMARCEL_ATTR_T];
  long int __align;
} pmarcel_attr_t;


/* Data structures for mutex handling.  The structure of the attribute
   type is not exposed on purpose.  */
typedef union
{
  struct
  {
    int __lock;
    unsigned int __count;
    struct pmarcel *__owner;
    /* KIND must stay at this position in the structure to maintain
       binary compatibility.  */
    int __kind;
  } __data;
  char __size[__SIZEOF_PMARCEL_MUTEX_T];
  long int __align;
} pmarcel_mutex_t;

typedef union
{
  char __size[__SIZEOF_PMARCEL_MUTEXATTR_T];
  long int __align;
} pmarcel_mutexattr_t;


/* Data structure for conditional variable handling.  The structure of
   the attribute type is not exposed on purpose.  */
typedef union
{
  struct
  {
    int __lock;
    unsigned long long int __total_seq;
    unsigned long long int __wakeup_seq;
    unsigned long long int __woken_seq;
  } __data;
  char __size[__SIZEOF_PMARCEL_COND_T];
  long long int __align;
} pmarcel_cond_t;

typedef union
{
  char __size[__SIZEOF_PMARCEL_CONDATTR_T];
  long int __align;
} pmarcel_condattr_t;


/* Keys for thread-specific data */
typedef unsigned int pmarcel_key_t;


/* Once-only execution */
typedef int pmarcel_once_t;


#ifdef __USE_UNIX98
/* Data structure for read-write lock variable handling.  The
   structure of the attribute type is not exposed on purpose.  */
typedef union
{
  struct
  {
    int __lock;
    unsigned int __nr_readers;
    unsigned int __readers_wakeup;
    unsigned int __writer_wakeup;
    unsigned int __nr_readers_queued;
    unsigned int __nr_writers_queued;
    /* FLAGS must stay at this position in the structure to maintain
       binary compatibility.  */
    unsigned int __flags;
    pmarcel_t __writer;
  } __data;
  char __size[__SIZEOF_PMARCEL_RWLOCK_T];
  long int __align;
} pmarcel_rwlock_t;

typedef union
{
  char __size[__SIZEOF_PMARCEL_RWLOCKATTR_T];
  long int __align;
} pmarcel_rwlockattr_t;
#endif


#ifdef __USE_XOPEN2K
/* POSIX spinlock data type.  */
typedef volatile int pmarcel_spinlock_t;


/* POSIX barriers data type.  The structure of the type is
   deliberately not exposed.  */
typedef union
{
  char __size[__SIZEOF_PMARCEL_BARRIER_T];
  long int __align;
} pmarcel_barrier_t;

typedef union
{
  char __size[__SIZEOF_PMARCEL_BARRIERATTR_T];
  int __align;
} pmarcel_barrierattr_t;
#endif


#endif	/* bits/pmarceltypes.h */
