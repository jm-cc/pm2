/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#ifdef MM_MAMI_ENABLED

#ifndef MM_MAMI_MARCEL_PRIVATE_H
#define MM_MAMI_MARCEL_PRIVATE_H

#include "pm2_common.h"

extern
int _mami_update_stats(marcel_entity_t *entity,
                       int source,
                       int dest,
                       size_t size);

#endif /* MM_MAMI_MARCEL_PRIVATE_H */
#endif /* MM_MAMI_ENABLED */