
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


#ifndef DSM_ERC_SW_INV_IS_DEF
#define DSM_ERC_SW_INV_IS_DEF

#include "marcel.h" // tfprintf 
#include "pm2.h"
#include "dsm_const.h" 
#include "dsm_page_manager.h"
#include "dsm_comm.h"
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h"
#include "dsm_page_size.h"
#include "dsm_mutex.h"
#include "assert.h"

void dsmlib_erc_sw_inv_init(int protocol_number);

void dsmlib_erc_sw_inv_rfh(unsigned long index);

void dsmlib_erc_sw_inv_wfh(unsigned long index);

void dsmlib_erc_sw_inv_rs(unsigned long index, dsm_node_t req_node, int arg);

void dsmlib_erc_sw_inv_ws(unsigned long index, dsm_node_t req_node, int arg);

void dsmlib_erc_sw_inv_is(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);

void dsmlib_erc_sw_inv_rps(void *addr, dsm_access_t access, dsm_node_t reply_node, int arg);

void dsmlib_erc_acquire();

void dsmlib_erc_release();

void dsmlib_erc_add_page(unsigned long index);

#endif

