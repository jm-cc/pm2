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

#error deprecated
#if !defined _MARCEL_H
# error "Never include <bits/marceltypes.h> directly; use <marcel_pthread.h> instead."
#endif

#ifndef _BITS_MARCELTYPES_H
#define _BITS_MARCELTYPES_H	1


#ifndef _MARCEL_DESCR_DEFINED
/* Thread descriptors */
typedef struct _marcel_descr_struct *_marcel_descr;
# define _MARCEL_DESCR_DEFINED
#endif




#ifdef __USE_UNIX98
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
//typedef struct marcel_task *marcel_t;
/* typedef unsigned long int marcel_t; */

#endif	/* bits/marceltypes.h */
