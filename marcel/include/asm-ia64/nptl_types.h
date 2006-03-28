/* Copyright (C) 2003, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2003.

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

#section macros

#define __SIZEOF_LPT_ATTR_T 56
#define __SIZEOF_LPT_MUTEX_T 40
#define __SIZEOF_LPT_MUTEXATTR_T 4
#define __SIZEOF_LPT_COND_T 48
#define __SIZEOF_LPT_CONDATTR_T 4
#define __SIZEOF_LPT_RWLOCK_T 56
#define __SIZEOF_LPT_RWLOCKATTR_T 8
#define __SIZEOF_LPT_BARRIER_T 32
#define __SIZEOF_LPT_BARRIERATTR_T 4

#section types
/* Thread identifiers.  The structure of the attribute type is not
   exposed on purpose.  */
typedef unsigned long int lpt_t;

#section structures
#depend "marcel_fastlock.h[structures]"
typedef union
{
  char __size[__SIZEOF_LPT_ATTR_T];
  long int __align;
} lpt_attr_t;


/* Data structures for mutex handling.  The structure of the attribute
   type is not exposed on purpose.  */
typedef union
{
  struct
  {
    int __dummy;
    unsigned int __count;
    int __owner;
    unsigned int __nusers;
    /* KIND must stay at this position in the structure to maintain
       binary compatibility.  */
    int __kind;
    struct _lpt_fastlock __lock;
  } __data;
  char __size[__SIZEOF_LPT_MUTEX_T];
  long int __align;
} lpt_mutex_t;

struct lpt_mutexattr
{       
        int __mutexkind;
};

typedef union
{
  struct lpt_mutexattr __data;
  char __size[__SIZEOF_LPT_MUTEXATTR_T];
  int __align;
} lpt_mutexattr_t;


/* Data structure for conditional variable handling.  The structure of
   the attribute type is not exposed on purpose.  */
typedef union
{
  struct
  {
    struct _lpt_fastlock __lock;
    p_marcel_task_t __waiting;
  } __data;
  char __size[__SIZEOF_LPT_COND_T];
  long int __align;
} lpt_cond_t;

typedef union
{
  char __size[__SIZEOF_LPT_CONDATTR_T];
  int __align;
} lpt_condattr_t;


/* Keys for thread-specific data */
typedef unsigned int lpt_key_t;


/* Once-only execution */
typedef int lpt_once_t;


#if defined __USE_UNIX98 || defined __USE_XOPEN2K
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
    int __writer;
    int __pad1;
    unsigned long int __pad2;
    unsigned long int __pad3;
    /* FLAGS must stay at this position in the structure to maintain
       binary compatibility.  */
    unsigned int __flags;
  } __data;
  char __size[__SIZEOF_LPT_RWLOCK_T];
  long int __align;
} lpt_rwlock_t;

typedef union
{
  char __size[__SIZEOF_LPT_RWLOCKATTR_T];
  long int __align;
} lpt_rwlockattr_t;
#endif


#ifdef __USE_XOPEN2K
/* POSIX spinlock data type.  */
typedef volatile int lpt_spinlock_t;


/* POSIX barriers data type.  The structure of the type is
   deliberately not exposed.  */
typedef union
{
  char __size[__SIZEOF_LPT_BARRIER_T];
  long int __align;
} lpt_barrier_t;

typedef union
{
  char __size[__SIZEOF_LPT_BARRIERATTR_T];
  int __align;
} lpt_barrierattr_t;
#endif


