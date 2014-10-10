/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOMAN_H
#define PIOMAN_H

/** @defgroup pioman PIOMan API
 * @{
 */

#if defined(PIOMAN_PTHREAD)
#  include <pthread.h>
#  include <semaphore.h>
#  include <sched.h>
#  define PIOMAN_MULTITHREAD
#  define PIOMAN_LOCK_PTHREAD
#  ifndef PM2_TOPOLOGY
#    define PIOMAN_TOPOLOGY_NONE 1
#  else
#    include <hwloc.h>
#    define PIOMAN_TOPOLOGY_HWLOC 1
#  endif
#  ifdef MCKERNEL
#    define PIOMAN_SEM_COND 1
#  endif /* MCKERNEL */
// #define PIOMAN_SEM_COND 1
#  define PIOMAN_PTHREAD_SPINLOCK
#elif defined(PIOMAN_MARCEL)
#  include <marcel.h>
#  define PIOMAN_MULTITHREAD
#  define PIOMAN_LOCK_MARCEL
#  ifndef MA__NUMA
#    define PIOMAN_TOPOLOGY_NONE 1
#  else
#    define PIOMAN_TOPOLOGY_MARCEL
#  endif
#else
#  undef  PIOMAN_MULTITHREAD
#  define PIOMAN_LOCK_NONE
#  define PIOMAN_TOPOLOGY_NONE 1
#endif

extern void pioman_init(int*argc, char**argv);
extern void pioman_exit(void);


/** @defgroup polling_points Polling constants. Define the polling points.
 * @{
 */
/** poll on timer */
#define PIOM_POLL_AT_TIMER_SIG  1
/** poll on explicit yield */
#define PIOM_POLL_AT_YIELD      2
/** poll on libc entry */
#define PIOM_POLL_AT_LIB_ENTRY  4
/** poll when LWP is idle */
#define PIOM_POLL_AT_IDLE       8
/** poll at context switch */
#define PIOM_POLL_AT_CTX_SWITCH 16
/** poll when explicitely asked for */
#define PIOM_POLL_WHEN_FORCED   32
/** @} */

/** Polling point. May be called from the application to force polling,
 * from marcel hooks, from timer handler.
 * @return 0 if we didn't need to poll and 1 otherwise
 */
extern int piom_check_polling(unsigned polling_point);

#include "piom_lock.h"
#include "piom_sem.h"
#include "piom_ltask.h"
#include "piom_io_task.h"

/** @} */

#endif /* PIOMAN_H */

