
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

#ifndef DSM_RPC_IS_DEF
#define DSM_RPC_IS_DEF

#include "dsm_const.h"
#include "dsm_mutex.h"


#ifndef PM2_DSM_RPC_C
extern
#endif
BEGIN_LRPC_LIST
  DSM_LRPC_READ_PAGE_REQ,
  DSM_LRPC_WRITE_PAGE_REQ,
  DSM_LRPC_SEND_PAGE,
  DSM_LRPC_INVALIDATE_REQ,
  DSM_LRPC_INVALIDATE_ACK,
  DSM_LRPC_SEND_DIFFS,
  DSM_LRPC_LOCK,
  DSM_LRPC_UNLOCK,
  DSM_LRPC_MULTIPLE_READ_PAGE_REQ,
  DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ,
  DSM_LRPC_SEND_MULTIPLE_PAGES_READ,
  DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE,
  DSM_LRPC_SEND_MULTIPLE_DIFFS,
  DSM_LRPC_HBRC_DIFFS,
  TOKEN_LOCK_RECV,
  TOKEN_LOCK_RELEASE_NOTIFICATION,
  TOKEN_LOCK_REQUEST,
  TOKEN_LOCK_MANAGER_SERVER,
  HIERARCH_INVALIDATE,
  HIERARCH_DIFF_ACK,
  HIERARCH_DIFF,
  HIERARCH_INV_ACK,
  HIERARCH_PAGE_PROVIDER
END_LRPC_LIST


void dsm_pm2_init_rpc(void);

#endif


