
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#ifndef MARCEL_ALLOC_EST_DEF
#define MARCEL_ALLOC_EST_DEF

#include "sys/isomalloc_archdep.h"

void marcel_slot_init(void);

void *marcel_slot_alloc(void);

void marcel_slot_free(void *addr);

void marcel_slot_exit(void);

#endif
