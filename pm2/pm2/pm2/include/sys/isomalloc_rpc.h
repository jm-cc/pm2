
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

#ifndef ISOMALLOC_RPC_IS_DEF
#define ISOMALLOC_RPC_IS_DEF

#include "isomalloc.h"
#include "bitmap.h"

/* #define DEBUG_NEGOCIATION  */


BEGIN_LRPC_LIST
  LRPC_ISOMALLOC_GLOBAL_LOCK,
  LRPC_ISOMALLOC_GLOBAL_UNLOCK,
  LRPC_ISOMALLOC_LOCAL_LOCK,
  LRPC_ISOMALLOC_LOCAL_UNLOCK,
  LRPC_ISOMALLOC_SYNC,
  LRPC_ISOMALLOC_SEND_SLOT_STATUS
END_LRPC_LIST


/*******************************************************/

#ifdef DEBUG_NEGOCIATION 
LRPC_DECL_REQ(LRPC_ISOMALLOC_GLOBAL_LOCK, unsigned int master;);
#else
LRPC_DECL_REQ(LRPC_ISOMALLOC_GLOBAL_LOCK,);
#endif
LRPC_DECL_RES(LRPC_ISOMALLOC_GLOBAL_LOCK,);

 
/*******************************************************/
 
LRPC_DECL_REQ(LRPC_ISOMALLOC_GLOBAL_UNLOCK,);
LRPC_DECL_RES(LRPC_ISOMALLOC_GLOBAL_UNLOCK,);
 
/*******************************************************/
LRPC_DECL_REQ(LRPC_ISOMALLOC_LOCAL_LOCK, unsigned int master;);
LRPC_DECL_RES(LRPC_ISOMALLOC_LOCAL_LOCK,);
 
/*******************************************************/
 
LRPC_DECL_REQ(LRPC_ISOMALLOC_LOCAL_UNLOCK, bitmap_t slot_map;);
LRPC_DECL_RES(LRPC_ISOMALLOC_LOCAL_UNLOCK,);
 
/*******************************************************/
 
LRPC_DECL_REQ(LRPC_ISOMALLOC_SYNC, unsigned int sender;);
LRPC_DECL_RES(LRPC_ISOMALLOC_SYNC,);

/*******************************************************/

LRPC_DECL_REQ(LRPC_ISOMALLOC_SEND_SLOT_STATUS, bitmap_t slot_map; unsigned int sender;);
LRPC_DECL_RES(LRPC_ISOMALLOC_SEND_SLOT_STATUS,);
  
/*******************************************************/
void pm2_isomalloc_init_rpc(void);

#endif


