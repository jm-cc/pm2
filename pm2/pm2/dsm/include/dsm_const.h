
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

#ifndef DSM_CONST_IS_DEF
#define DSM_CONST_IS_DEF

/* the following is useful for "token_lock_id_t" */
#include "token_lock.h"

typedef enum
{ NO_ACCESS, READ_ACCESS, WRITE_ACCESS, UNKNOWN_ACCESS } dsm_access_t;

typedef unsigned long int dsm_page_index_t;

typedef int dsm_node_t;

extern const dsm_node_t NOBODY;



/* error codes returned by some functions */
enum
{
   /* this dummy value is to make sure the compiler will choose type
   * "int" for this enumeration (instead of "unsigned int") */
   DSM_ERR_DUMMY = -1,
   DSM_SUCCESS = 0,
   DSM_ERR_MEMORY,      /* memory exhausted */
   DSM_ERR_ILLEGAL,     /* illegal call / operation */
   DSM_ERR_NOT_INIT,    /* utilization of an object not initialized */
   DSM_ERR_MISC,        /* miscellaneous errors */
};


typedef void (*dsm_rf_action_t)(dsm_page_index_t index); // read fault handler

typedef void (*dsm_wf_action_t)(dsm_page_index_t index); // write fault handler

typedef void (*dsm_rs_action_t)(dsm_page_index_t index, dsm_node_t req_node, int tag); // read server

typedef void (*dsm_ws_action_t)(dsm_page_index_t index, dsm_node_t req_node, int tag); // write server

typedef void (*dsm_is_action_t)(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner); // invalidate server

typedef void (*dsm_rp_action_t)(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag); // receive page server

typedef void (*dsm_erp_action_t)(void *addr, dsm_access_t access, dsm_node_t reply_node, unsigned long page_size, int tag); // expert receive page server

typedef void (*dsm_acq_action_t)(const token_lock_id_t); // acquire func

typedef void (*dsm_rel_action_t)(const token_lock_id_t); // release func

typedef void (*dsm_pi_action_t)(int prot_id); // protocol init func

typedef void (*dsm_pa_action_t)(dsm_page_index_t index); // page add func

#endif


