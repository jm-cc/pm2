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

/* This file comes from the Glibc's NPTL, specifically
   `sysdeps/unix/sysv/linux/ARCH/bits/pthreadtypes.h'.  */

#section macros

#define __SIZEOF_LPT_ATTR_T 56
#define __SIZEOF_LPT_MUTEX_T 40
#define __OFFSETOF_LPT_MUTEX_KIND 16
#define __SIZEOF_LPT_MUTEXATTR_T 4
#define __SIZEOF_LPT_COND_T 48
#define __SIZEOF_LPT_CONDATTR_T 4
#define __SIZEOF_LPT_RWLOCK_T 56
#define __OFFSETOF_LPT_RWLOCK_FLAGS 48
#define __SIZEOF_LPT_RWLOCKATTR_T 8
#define __SIZEOF_LPT_BARRIER_T 32
#define __SIZEOF_LPT_BARRIERATTR_T 4

#section types
/* Thread identifiers.  The structure of the attribute type is not
   exposed on purpose.  */
// On n'exporte pas cela: on utilise des pointeurs, qui sont abi-compatibles avec cela.
//typedef unsigned long int lpt_t;

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
  // replaced the two ints by our marcel_t
    //int __dummy;
    unsigned int __count;
    //int __owner;
    unsigned int __nusers;
    marcel_t __owner;
    /* KIND must stay at this position in the structure to maintain
       binary compatibility with static initializers.  */
    int __kind;
    struct _lpt_fastlock __lock;
  } __data;
  char __size[__SIZEOF_LPT_MUTEX_T];
  long int __align;
} lpt_mutex_t;

struct lpt_mutexattr
{       
        int mutexkind;
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


/* Data structure for read-write lock variable handling.  The
   structure of the attribute type is not exposed on purpose.  */
typedef union
{
  struct
  {
    long int __lock;
    struct _lpt_fastlock __readers_wakeup;
    struct _lpt_fastlock __writer_wakeup;
    unsigned int __nr_readers_queued;
    unsigned int __nr_writers_queued;
    marcel_t __writer;
    unsigned int __nr_readers;
    int __shared;
    /* FLAGS must stay at this position in the structure to maintain
       binary compatibility with static initializers.  */
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


/* POSIX spinlock data type.  */
typedef volatile int lpt_spinlock_t;

/* POSIX barriers data type. */
  struct lpt_barrier
  {
    unsigned int init_count;
    struct _lpt_fastlock lock;
    ma_atomic_t leftB;
    ma_atomic_t leftE;
  };

/* POSIX barriers data type.  The structure of the type is
   deliberately not exposed.  */
typedef union
{
  char __size[__SIZEOF_LPT_BARRIER_T];
  long int __align;
} lpt_barrier_t;

  struct lpt_barrierattr
  {
    int pshared;
  };

typedef union
{
  char __size[__SIZEOF_LPT_BARRIERATTR_T];
  int __align;
} lpt_barrierattr_t;


/* Check that the size of our data structures is compatible with those of
   NPTL.  */

MA_VERIFY (sizeof (lpt_attr_t) <= __SIZEOF_LPT_ATTR_T);
MA_VERIFY (sizeof (lpt_mutex_t) <= __SIZEOF_LPT_MUTEX_T);
MA_VERIFY (tbx_offset_of (lpt_mutex_t,__data.__kind) == __OFFSETOF_LPT_MUTEX_KIND);
MA_VERIFY (sizeof (lpt_mutexattr_t) <= __SIZEOF_LPT_MUTEXATTR_T);
MA_VERIFY (sizeof (lpt_cond_t) <= __SIZEOF_LPT_COND_T);
MA_VERIFY (sizeof (lpt_condattr_t) <= __SIZEOF_LPT_CONDATTR_T);
MA_VERIFY (sizeof (lpt_rwlock_t) <= __SIZEOF_LPT_RWLOCK_T);
MA_VERIFY (tbx_offset_of (lpt_rwlock_t,__data.__flags) == __OFFSETOF_LPT_RWLOCK_FLAGS);
MA_VERIFY (sizeof (lpt_rwlockattr_t) <= __SIZEOF_LPT_RWLOCKATTR_T);
MA_VERIFY (sizeof (lpt_barrier_t) <= __SIZEOF_LPT_BARRIER_T);
MA_VERIFY (sizeof (lpt_barrierattr_t) <= __SIZEOF_LPT_BARRIERATTR_T);
MA_VERIFY (sizeof (lpt_barrierattr_t) <= __SIZEOF_LPT_BARRIERATTR_T);

#section marcel_macros
#define __ma_cleanup_fct_attribute
