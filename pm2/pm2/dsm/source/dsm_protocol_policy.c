
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


#include <stdio.h>
#include <malloc.h>
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h" 

//#define TRACE_PROT

/* Global data structures for the protocol policy module */

#define NB_BUILT_IN_PROTOCOLS 2

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

static int _nb_protocols = NB_BUILT_IN_PROTOCOLS;
static dsm_protocol_t dsm_protocol_table[MAX_DSM_PROTOCOLS]; /* the protocol table */


/* Public functions */

void dsm_init_protocol_table()
{

  /* 0: LI_HUDAK */
  dsm_protocol_table[0].read_fault_handler = dsmlib_rf_ask_for_read_copy;
  dsm_protocol_table[0].write_fault_handler = dsmlib_wf_ask_for_write_access;
  dsm_protocol_table[0].read_server = dsmlib_rs_send_read_copy;
  dsm_protocol_table[0].write_server = dsmlib_ws_send_page_for_write_access;
  dsm_protocol_table[0].invalidate_server = dsmlib_is_invalidate;
  dsm_protocol_table[0].receive_page_server = dsmlib_rp_validate_page;

  /* 1: MIGRATE_THREAD */
  dsm_protocol_table[1].read_fault_handler = dsmlib_migrate_thread;
  dsm_protocol_table[1].write_fault_handler = dsmlib_migrate_thread;
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

