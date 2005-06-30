
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

#ifndef DSM_PROT_LIB_IS_DEF
#define DSM_PROT_LIB_IS_DEF

#include "dsm_const.h" 
#include "dsm_page_manager.h"   /* for type "dsm_page_index_t" */


void dsmlib_rf_ask_for_read_copy(dsm_page_index_t index);

void dsmlib_migrate_thread(dsm_page_index_t index);

void dsmlib_wf_ask_for_write_access(dsm_page_index_t index);

void dsmlib_rs_send_read_copy(dsm_page_index_t index, dsm_node_t req_node, int tag);

void dsmlib_ws_send_page_for_write_access(dsm_page_index_t index, dsm_node_t req_node, int tag);

void dsmlib_is_invalidate(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner);
void dsmlib_is_invalidate_internal(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner);

void dsmlib_rp_validate_page(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag);

#endif




