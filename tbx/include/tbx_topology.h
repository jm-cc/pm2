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


#ifndef __TBX_TOPOLOGY_H__
#define __TBX_TOPOLOGY_H__


#ifdef PM2_TOPOLOGY

#include <hwloc.h>

extern hwloc_topology_t topology;
extern char *synthetic_topology_description;

void tbx_topology_init(int argc, char *argv[]);
void tbx_topology_destroy(void);

#endif				/* PM2_TOPOLOGY */


#endif /** __TBX_TOPOLOGY_H__ **/
