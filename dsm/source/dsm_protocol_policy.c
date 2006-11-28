
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


#include <stdio.h>
#include <stdlib.h>
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h"
#include "dsmlib_erc_sw_inv_protocol.h"
#include "dsmlib_hbrc_mw_update_protocol.h"
#include "hierarch_protocol.h"



#define MAX_DSM_PROTOCOLS 15


/**********************************************************************/
/* PUBLIC GLOBAL VARIABLES                                            */
/**********************************************************************/

/* pre-defined consistency protocols */
dsm_proto_t LI_HUDAK;
dsm_proto_t MIGRATE_THREAD;
dsm_proto_t ERC;
dsm_proto_t HBRC;
dsm_proto_t HIERARCH;


/**********************************************************************/
/* PRIVATE DATA STRUCTURES                                            */
/**********************************************************************/

typedef struct _dsm_protocol_t
{
   dsm_rf_action_t read_fault_handler;
   dsm_wf_action_t write_fault_handler;
   dsm_rs_action_t read_server;
   dsm_ws_action_t write_server;
   dsm_is_action_t invalidate_server;
   dsm_rp_action_t receive_page_server;
   dsm_erp_action_t expert_receive_page_server;
   dsm_acq_action_t acquire_func;
   dsm_rel_action_t release_func;
   dsm_pi_action_t prot_init_func;
   dsm_pa_action_t page_add_func;
} dsm_protocol_t;


/**********************************************************************/
/* PRIVATE GLOBAL VARIABLES                                           */
/**********************************************************************/

static int _nb_protocols = 0;
 /* the protocol table */
static dsm_protocol_t dsm_protocol_table[MAX_DSM_PROTOCOLS];

/* default DSM consistency protocol set by dsm_set_default_protocol() */
static dsm_proto_t dsm_default_protocol;


/**********************************************************************/
/* PUBLIC FUNCTIONS                                                   */
/**********************************************************************/

void
dsm_init_protocol_table (void)
{
   /* LI_HUDAK */
   LI_HUDAK = dsm_create_protocol(dsmlib_rf_ask_for_read_copy /* read_fault_handler */,
                                  dsmlib_wf_ask_for_write_access /* write_fault_handler */,
                                  dsmlib_rs_send_read_copy /* read_server */,
                                  dsmlib_ws_send_page_for_write_access /* write_server */,
                                  dsmlib_is_invalidate /* invalidate_server */,
                                  dsmlib_rp_validate_page /* receive_page_server */,
                                  NULL /* expert_receive_page_server */,
                                  NULL /* acquire_func */,
                                  NULL /* release_func */,
                                  NULL /* prot_init_func */,
                                  NULL /* page_add_func */);

   /* MIGRATE_THREAD */
   MIGRATE_THREAD = dsm_create_protocol(dsmlib_migrate_thread /* read_fault_handler */,
                                        dsmlib_migrate_thread /* write_fault_handler */,
                                        NULL /* read_server */,
                                        NULL /* write_server */,
                                        NULL /* invalidate_server */,
                                        NULL /* receive_page_server */,
                                        NULL /* expert_receive_page_server */,
                                        NULL /* acquire_func */,
                                        NULL /* release_func */,
                                        NULL /* prot_init_func */,
                                        NULL /* page_add_func */);

   /* ERC */
   ERC = dsm_create_protocol(dsmlib_erc_sw_inv_rfh /* read_fault_handler */,
                             dsmlib_erc_sw_inv_wfh /* write_fault_handler */,
                             dsmlib_erc_sw_inv_rs /* read_server */,
                             dsmlib_erc_sw_inv_ws /* write_server */,
                             dsmlib_erc_sw_inv_is /* invalidate_server */,
                             dsmlib_erc_sw_inv_rps /* receive_page_server */,
                             NULL /* expert_receive_page_server */,
                             NULL /* acquire_func */,
                             dsmlib_erc_release /* release_func */,
                             dsmlib_erc_sw_inv_init /* prot_init_func */,
                             dsmlib_erc_add_page /* page_add_func */);

   /* HBRC */
   HBRC = dsm_create_protocol(dsmlib_hbrc_mw_update_rfh /* read_fault_handler */,
                              dsmlib_hbrc_mw_update_wfh /* write_fault_handler */,
                              dsmlib_hbrc_mw_update_rs /* read_server */,
                              dsmlib_hbrc_mw_update_ws /* write_server */,
                              dsmlib_hbrc_mw_update_is /* invalidate_server */,
                              dsmlib_hbrc_mw_update_rps /* receive_page_server */,
                              NULL /* expert_receive_page_server */,
                              NULL /* acquire_func */,
                              dsmlib_hbrc_release /* release_func */,
                              dsmlib_hbrc_mw_update_prot_init /* prot_init_func */,
                              dsmlib_hbrc_add_page /* page_add_func */);

  /* Hierarchical consistency protocol: HIERARCH; to be used in
   * conjunction with the token_locks */
  HIERARCH = dsm_create_protocol(hierarch_proto_read_fault_handler,
                                 hierarch_proto_write_fault_handler,
                                 NULL /* bypass the read server
                                         implemented in DSM-PM2 */,
                                 NULL /* bypass the write server
                                         implemented in DSM-PM2 */,
                                 NULL /* bypass the invalidate server
                                         implemented in DSM-PM2 */,
                                 hierarch_proto_receive_page_server,
                                 NULL /* no expert_receive_page_server */,
                                 hierarch_proto_acquire_func,
                                 hierarch_proto_release_func,
                                 hierarch_proto_initialization,
                                 hierarch_proto_page_add_func);

  /* By default, the default consistency protocol is
   * "DSM_DEFAULT_DEFAULT_PROTOCOL"; this line must stay after all the
   * pre-defined protocols have been defined. */
  dsm_set_default_protocol(DSM_DEFAULT_DEFAULT_PROTOCOL);

  return;
}

/**********************************************************************/
dsm_proto_t
dsm_get_default_protocol (void)
{
   return dsm_default_protocol;
}

void
dsm_set_default_protocol (const dsm_proto_t protocol)
{
   assert ( protocol != DSM_NO_PROTOCOL );
   assert ( protocol != DSM_DEFAULT_PROTOCOL );
   dsm_default_protocol = protocol;
}


dsm_proto_t
dsm_create_protocol (dsm_rf_action_t const read_fault_handler,
                     dsm_wf_action_t const write_fault_handler,
                     dsm_rs_action_t const read_server,
                     dsm_ws_action_t const write_server,
                     dsm_is_action_t const invalidate_server,
                     dsm_rp_action_t const receive_page_server,
                     dsm_erp_action_t const expert_receive_page_server,
                     dsm_acq_action_t const acquire_func,
                     dsm_rel_action_t const release_func,
                     dsm_pi_action_t const prot_init_func,
                     dsm_pa_action_t const page_add_func)
{
   dsm_protocol_table[_nb_protocols].read_fault_handler = read_fault_handler;
   dsm_protocol_table[_nb_protocols].write_fault_handler = write_fault_handler;
   dsm_protocol_table[_nb_protocols].read_server = read_server;
   dsm_protocol_table[_nb_protocols].write_server = write_server;
   dsm_protocol_table[_nb_protocols].invalidate_server = invalidate_server;
   dsm_protocol_table[_nb_protocols].receive_page_server = receive_page_server;
   dsm_protocol_table[_nb_protocols].expert_receive_page_server = expert_receive_page_server;
   dsm_protocol_table[_nb_protocols].acquire_func = acquire_func;
   dsm_protocol_table[_nb_protocols].release_func = release_func;
   dsm_protocol_table[_nb_protocols].prot_init_func = prot_init_func;
   dsm_protocol_table[_nb_protocols].page_add_func = page_add_func;

   return (dsm_proto_t)(_nb_protocols++);
}


int
dsm_get_registered_protocols (void)
{
   return _nb_protocols;
}


void
dsm_set_read_fault_action (const dsm_proto_t key, dsm_rf_action_t const action)
{
   dsm_protocol_table[key].read_fault_handler = action;
}


dsm_rf_action_t
dsm_get_read_fault_action (const dsm_proto_t key)
{
   return dsm_protocol_table[key].read_fault_handler;
}


void
dsm_set_write_fault_action (const dsm_proto_t key, dsm_wf_action_t const action)
{
   dsm_protocol_table[key].write_fault_handler = action;
}


dsm_wf_action_t
dsm_get_write_fault_action (const dsm_proto_t key)
{
   return dsm_protocol_table[key].write_fault_handler;
}


void
dsm_set_read_server (const dsm_proto_t key, dsm_rs_action_t const action)
{
   dsm_protocol_table[key].read_server = action;
}


dsm_rs_action_t
dsm_get_read_server (const dsm_proto_t key)
{
   return dsm_protocol_table[key].read_server;
}


void
dsm_set_write_server (const dsm_proto_t key, dsm_ws_action_t const action)
{
   dsm_protocol_table[key].write_server = action;
}


dsm_ws_action_t
dsm_get_write_server (const dsm_proto_t key)
{
   return dsm_protocol_table[key].write_server;
}


void
dsm_set_invalidate_server (const dsm_proto_t key, dsm_is_action_t const action)
{
   dsm_protocol_table[key].invalidate_server = action;
}


dsm_is_action_t
dsm_get_invalidate_server (const dsm_proto_t key)
{
   return dsm_protocol_table[key].invalidate_server;
}


void
dsm_set_receive_page_server (const dsm_proto_t key, dsm_rp_action_t const action)
{
   dsm_protocol_table[key].receive_page_server = action;
}


dsm_rp_action_t
dsm_get_receive_page_server (const dsm_proto_t key)
{
   return dsm_protocol_table[key].receive_page_server;
}


void
dsm_set_expert_receive_page_server (const dsm_proto_t key, dsm_erp_action_t const action)
{
   dsm_protocol_table[key].expert_receive_page_server = action;
}


dsm_erp_action_t
dsm_get_expert_receive_page_server (const dsm_proto_t key)
{
   return dsm_protocol_table[key].expert_receive_page_server;
}


void
dsm_set_acquire_func (const dsm_proto_t key, dsm_acq_action_t const action)
{
   dsm_protocol_table[key].acquire_func = action;
}


dsm_acq_action_t
dsm_get_acquire_func (const dsm_proto_t key)
{
   return dsm_protocol_table[key].acquire_func;
}


void
dsm_set_release_func (const dsm_proto_t key, dsm_rel_action_t const action)
{
   dsm_protocol_table[key].release_func = action;
}


dsm_rel_action_t
dsm_get_release_func (const dsm_proto_t key)
{
   return dsm_protocol_table[key].release_func;
}


void
dsm_set_prot_init_func (const dsm_proto_t key, dsm_pi_action_t const action)
{
   dsm_protocol_table[key].prot_init_func = action;
}


dsm_pi_action_t
dsm_get_prot_init_func (const dsm_proto_t key)
{
   return dsm_protocol_table[key].prot_init_func;
}


void
dsm_set_page_add_func (const dsm_proto_t key, dsm_pa_action_t const action)
{
   dsm_protocol_table[key].page_add_func = action;
}


dsm_pa_action_t
dsm_get_page_add_func (const dsm_proto_t key)
{
   return dsm_protocol_table[key].page_add_func;
}

