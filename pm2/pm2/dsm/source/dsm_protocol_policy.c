
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
#include <malloc.h>
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h" 
#include "dsmlib_erc_sw_inv_protocol.h"
#include "dsmlib_hbrc_mw_update_protocol.h"

//#define TRACE_PROT

/* Global data structures for the protocol policy module */

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

#define MAX_DSM_PROTOCOLS 15

static int _nb_protocols = 0;
static dsm_protocol_t dsm_protocol_table[MAX_DSM_PROTOCOLS]; /* the protocol table */

int LI_HUDAK;
int MIGRATE_THREAD;
int ERC;
int HBRC;
int HIERARCH;

/* Public functions */

void dsm_init_protocol_table()
{
  /* LI_HUDAK */
  LI_HUDAK = dsm_create_protocol(dsmlib_rf_ask_for_read_copy, // read_fault_handler 
		      dsmlib_wf_ask_for_write_access, // write_fault_handler 
		      dsmlib_rs_send_read_copy, // read_server
		      dsmlib_ws_send_page_for_write_access, // write_server 
		      dsmlib_is_invalidate, // invalidate_server 
		      dsmlib_rp_validate_page, // receive_page_server 
		      NULL, // expert_receive_page_server
		      NULL, // acquire_func 
		      NULL, // release_func 
		      NULL, // prot_init_func 
		      NULL  // page_add_func 
		      );

  /* MIGRATE_THREAD */
  MIGRATE_THREAD = dsm_create_protocol(dsmlib_migrate_thread, // read_fault_handler 
		      dsmlib_migrate_thread, // write_fault_handler
		      NULL,// read_server
		      NULL,// write_server 
		      NULL, // invalidate_server 
		      NULL, // receive_page_server 
		      NULL, // expert_receive_page_server
		      NULL, // acquire_func 
		      NULL, // release_func 
		      NULL, // prot_init_func 
		      NULL  // page_add_func 
		      );

  /* ERC */
  ERC = dsm_create_protocol(dsmlib_erc_sw_inv_rfh, // read_fault_handler 
		      dsmlib_erc_sw_inv_wfh, // write_fault_handler 
		      dsmlib_erc_sw_inv_rs, // read_server
		      dsmlib_erc_sw_inv_ws, // write_server 
		      dsmlib_erc_sw_inv_is, // invalidate_server 
		      dsmlib_erc_sw_inv_rps, // receive_page_server 
		      NULL, // expert_receive_page_server
		      NULL, // acquire_func 
		      dsmlib_erc_release, // release_func 
		      dsmlib_erc_sw_inv_init, // prot_init_func 
		      dsmlib_erc_add_page  // page_add_func 
		      );

  /* HBRC */
  HBRC = dsm_create_protocol(dsmlib_hbrc_mw_update_rfh, // read_fault_handler 
			     dsmlib_hbrc_mw_update_wfh, // write_fault_handler 
			     dsmlib_hbrc_mw_update_rs, // read_server
			     dsmlib_hbrc_mw_update_ws, // write_server 
			     dsmlib_hbrc_mw_update_is, // invalidate_server 
			     dsmlib_hbrc_mw_update_rps, // receive_page_server 
			     NULL, // expert_receive_page_server
			     NULL, // acquire_func 
			     dsmlib_hbrc_release, // release_func 
			     dsmlib_hbrc_mw_update_prot_init, // prot_init_func 
			     dsmlib_hbrc_add_page  // page_add_func 
			     );
  
  /* Hierarchical consistency protocol: HIERARCH */
  /* HIERARCH = dsm_create_protocol(); */
}


int dsm_create_protocol(dsm_rf_action_t read_fault_handler,
			dsm_wf_action_t write_fault_handler,
			dsm_rs_action_t read_server,
			dsm_ws_action_t write_server,
			dsm_is_action_t invalidate_server,
			dsm_rp_action_t receive_page_server,
			dsm_erp_action_t expert_receive_page_server,
			dsm_acq_action_t acquire_func,
			dsm_rel_action_t release_func,
			dsm_pi_action_t prot_init_func,
			dsm_pa_action_t page_add_func
			)
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

 _nb_protocols++;
 
 return _nb_protocols - 1 ;
}


int dsm_registered_protocols()
{
  return _nb_protocols;
}


void dsm_set_read_fault_action(int key, dsm_rf_action_t action)
{
  dsm_protocol_table[key].read_fault_handler = action;
}


dsm_rf_action_t dsm_get_read_fault_action(int key)
{
  return dsm_protocol_table[key].read_fault_handler;
}


void dsm_set_write_fault_action(int key, dsm_wf_action_t action)
{
  dsm_protocol_table[key].write_fault_handler = action;
}


dsm_wf_action_t dsm_get_write_fault_action(int key)
{
  return dsm_protocol_table[key].write_fault_handler;
}


void dsm_set_read_server(int key, dsm_rs_action_t action)
{
  dsm_protocol_table[key].read_server = action;
}


dsm_rs_action_t dsm_get_read_server(int key)
{
  return dsm_protocol_table[key].read_server;
}


void dsm_set_write_server(int key, dsm_ws_action_t action)
{
  dsm_protocol_table[key].write_server = action;
}


dsm_ws_action_t dsm_get_write_server(int key)
{
  return dsm_protocol_table[key].write_server;
}


void dsm_set_invalidate_server(int key, dsm_is_action_t action)
{
  dsm_protocol_table[key].invalidate_server = action;
}


dsm_is_action_t dsm_get_invalidate_server(int key)
{
  return dsm_protocol_table[key].invalidate_server;
}


void dsm_set_receive_page_server(int key, dsm_rp_action_t action)
{
  dsm_protocol_table[key].receive_page_server = action;
}


dsm_rp_action_t dsm_get_receive_page_server(int key)
{
  return dsm_protocol_table[key].receive_page_server;
}


void dsm_set_expert_receive_page_server(int key, dsm_erp_action_t action)
{
  dsm_protocol_table[key].expert_receive_page_server = action;
}


dsm_erp_action_t dsm_get_expert_receive_page_server(int key)
{
  return dsm_protocol_table[key].expert_receive_page_server;
}


void dsm_set_acquire_func(int key, dsm_acq_action_t action)
{
  dsm_protocol_table[key].acquire_func = action;
}


dsm_acq_action_t dsm_get_acquire_func(int key)
{
#ifdef TRACE_PROT
  fprintf(stderr,"Entering %s\n", __FUNCTION__);
#endif
  return dsm_protocol_table[key].acquire_func;
}


void dsm_set_release_func(int key, dsm_rel_action_t action)
{
  dsm_protocol_table[key].release_func = action;
}


dsm_rel_action_t dsm_get_release_func(int key)
{
#ifdef TRACE_PROT
  fprintf(stderr,"[%s]: Entering...\n", __FUNCTION__);
#endif
  return dsm_protocol_table[key].release_func;
}


void dsm_set_prot_init_func(int key, dsm_pi_action_t action)
{
  dsm_protocol_table[key].prot_init_func = action;
}


dsm_pi_action_t dsm_get_prot_init_func(int key)
{
#ifdef TRACE_PROT
  fprintf(stderr,"[%s]: Entering...\n", __FUNCTION__);
#endif
  return dsm_protocol_table[key].prot_init_func;
}


void dsm_set_page_add_func(int key, dsm_pa_action_t action)
{
  dsm_protocol_table[key].page_add_func = action;
}


dsm_pa_action_t dsm_get_page_add_func(int key)
{
#ifdef TRACE_PROT
 fprintf(stderr,"[%s]: Entering...\n", __FUNCTION__);
#endif
  return dsm_protocol_table[key].page_add_func;
}

