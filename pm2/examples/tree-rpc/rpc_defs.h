
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

#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include "pm2.h"

#ifndef PM2_RPC_DEFS_C
extern
#endif
BEGIN_LRPC_LIST
  DICHOTOMY
END_LRPC_LIST


/* FAC */

LRPC_DECL_REQ(DICHOTOMY, int inf; int sup;)
LRPC_DECL_RES(DICHOTOMY, int res;)


#endif
