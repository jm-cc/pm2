/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#include "inlinefunctions/linux_bitops.h"
#include "inlinefunctions/linux_interrupt.h"
#include "inlinefunctions/linux_jiffies.h"
#include "inlinefunctions/linux_spinlock.h"
#include "inlinefunctions/linux_thread_info.h"
#include "inlinefunctions/linux_timer.h"
#include "inlinefunctions/marcel_alloc.h"
#include "inlinefunctions/marcel_descr.h"
#include "inlinefunctions/marcel_keys.h"
#include "inlinefunctions/marcel_sched_generic.h"
#include "inlinefunctions/marcel_sem.h"
#include "inlinefunctions/marcel_threads.h"
#include "inlinefunctions/marcel_timer.h"
#include "inlinefunctions/marcel_topology.h"
#include "inlinefunctions/marcel_fastlock.h"
