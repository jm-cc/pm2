/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2012 "the PM2 team" (see AUTHORS file)
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

/** @file internal PIOMan definitions.
 */

#ifndef PIOM_PRIVATE_H
#define PIOM_PRIVATE_H

#ifdef PIOMAN_TRACE
#include <GTG.h>
#endif /* PIOMAN_TRACE */

#include <tbx.h>
#include "piom_lfqueue.h"
#include "piom_log.h"
#include "pioman.h"

/** Try to schedule a task
 * Returns the task that have been scheduled (or NULL if no task)
 */
TBX_INTERNAL void *piom_ltask_schedule(void);

/** initialize ltask system */
TBX_INTERNAL void piom_init_ltasks(void);

/** destroy internal structures, stop task execution, etc. */
TBX_INTERNAL void piom_exit_ltasks(void);

/* todo: get a dynamic value here !
 * it could be based on:
 * - application hints
 * - the history of previous request
 * - compiler hints
 */

TBX_INTERNAL struct piom_parameters_s
{
    int busy_wait_usec;     /**< time to do a busy wait before blocking, in usec; default: 5 */
    int busy_wait_granularity; /**< number of busy wait loops between timestamps to amortize clock_gettime() */
    int enable_progression; /**< whether to enable background progression (idle thread and sighandler); default 1 */
    int idle_granularity;   /**< in usec. */
    int timer_period;       /**< period for timer-based polling (in usec); default: 4000 */
    int spare_lwp;          /**< number of spare LWPs for blocking calls; default: 0 */
} piom_parameters;

#endif /* PIOM_PRIVATE_H */
