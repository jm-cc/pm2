
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

#ifndef PM2_THREAD_EST_DEF
#define PM2_THREAD_EST_DEF

#include "marcel.h"

typedef void (*pm2_func_t)(void *arg);

void pm2_thread_init(void);
void pm2_thread_exit(void);

marcel_t pm2_thread_create(pm2_func_t func, void *arg);
marcel_t pm2_service_thread_create(pm2_func_t func, void *arg);

#endif
