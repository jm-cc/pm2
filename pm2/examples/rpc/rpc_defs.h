
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
	SAMPLE,
        INFO,
	LRPC_PING,
	LRPC_PING_ASYNC,
	LRPC_PING_QUICK_ASYNC
END_LRPC_LIST


/* SAMPLE */

typedef char sample_tab[64];

LRPC_DECL_REQ(SAMPLE, sample_tab tab;)
LRPC_DECL_RES(SAMPLE,)


/* INFO */

extern unsigned BUF_SIZE;
extern unsigned NB_PING;

LRPC_DECL_REQ(INFO,);
LRPC_DECL_RES(INFO,);

/* PING */

#define MAX_BUF_SIZE    16384

#define BUF_PACKING     MAD_IN_PLACE

LRPC_DECL_REQ(LRPC_PING,)
LRPC_DECL_RES(LRPC_PING,)


/* PING_ASYNC */

LRPC_DECL_REQ(LRPC_PING_ASYNC,)
LRPC_DECL_RES(LRPC_PING_ASYNC,)


#endif
