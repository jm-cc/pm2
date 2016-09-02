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
//#  define PIOMAN_MUTEX_SPINLOCK
//#  define PIOMAN_MINI_SPINLOCK
#elif defined(PIOMAN_MARCEL)
#  include <marcel.h>
#  define PIOMAN_MULTITHREAD
#  ifndef MA__NUMA
#    define PIOMAN_TOPOLOGY_NONE 1
#  else
#    define PIOMAN_TOPOLOGY_MARCEL
#  endif
#elif defined(PIOMAN_ABT)
#  include <abt.h>
#  define PIOMAN_TOPOLOGY_NONE 1
#  define PIOMAN_MULTITHREAD
#elif defined(PIOMAN)
#  undef  PIOMAN_MULTITHREAD
#  define PIOMAN_TOPOLOGY_NONE 1
#else
#  error "PIOMan: PIOMAN flags are not defined."
#endif

extern void pioman_init(int*argc, char**argv);
extern void pioman_exit(void);


/** @defgroup polling_points Polling constants. Define the polling points.
 * @{
 */
/** poll on timer */
#define PIOM_POLL_POINT_TIMER  0x01
/** poll on explicit yield */
#define PIOM_POLL_POINT_YIELD  0x02
/** poll when cpu is idle */
#define PIOM_POLL_POINT_IDLE   0x08
/** poll when explicitely asked for by the application */
#define PIOM_POLL_POINT_FORCED 0x10
/** poll in a busy wait */
#define PIOM_POLL_POINT_BUSY   0x20
/** @} */

/** Polling point forced from the application.
 */
extern void piom_polling_force(void);

/** Schedule tasks from local or all queues (depending on 'point')
 * @internal function exported for inlining
 */
extern void piom_ltask_schedule(int point);

#include "piom_lock.h"
#include "piom_ltask.h"
#include "piom_io_task.h"

/** @} */

#endif /* PIOMAN_H */

