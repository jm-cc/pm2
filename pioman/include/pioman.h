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

#ifdef MARCEL
#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#endif

#if(defined(MARCEL) || defined(PIOM_PTHREAD_LOCK))
#define PIOM_THREAD_ENABLED
#endif

extern void pioman_init(int*argc, char**argv);
extern void pioman_exit(void);

#include "piom_lock_types.h"
#include "piom.h"
#include "piom_log.h"
#include "piom_debug.h"

#include "piom_sem.h"
#include "piom_ltask.h"
#include "piom_io_task.h"
#include "tbx.h"

#endif /* PIOMAN_H */

