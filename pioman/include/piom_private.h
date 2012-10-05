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


#endif /* PIOM_PRIVATE_H */
