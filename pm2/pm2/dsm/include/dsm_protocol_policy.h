
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

#ifndef DSM_PROTOCOL_POLICY_H
#define DSM_PROTOCOL_POLICY_H

#include "dsm_const.h"



/* Garbage... to get rid of */
#define DEFAULT_DSM_PROTOCOL -1



/**********************************************************************/
/* PUBLIC TYPE DEFINITIONS                                            */
/**********************************************************************/

/* protocol identifier */
typedef int dsm_proto_t;


/**********************************************************************/
/* GLOBAL VARIABLES                                                   */
/**********************************************************************/

/* pre-defined consistency protocols */
extern dsm_proto_t LI_HUDAK;
extern dsm_proto_t MIGRATE_THREAD;
extern dsm_proto_t ERC;
extern dsm_proto_t HBRC;
extern dsm_proto_t HIERARCH;


/* illegal protocol identifier */
extern const dsm_proto_t NO_PROTOCOL;

/* default protocol as set by dsm_set_default_protocol() */
extern const dsm_proto_t DEFAULT_PROTOCOL;

/* default protocol in case dsm_set_default_protocol() is not called */
#define DEFAULT_DEFAULT_PROTOCOL LI_HUDACK


/**********************************************************************/
/* PUBLIC FUNCTIONS                                                   */
/**********************************************************************/

extern void
dsm_init_protocol_table (void);

extern dsm_proto_t
dsm_create_protocol (dsm_rf_action_t, dsm_wf_action_t, dsm_rs_action_t,
                     dsm_ws_action_t, dsm_is_action_t, dsm_rp_action_t,
                     dsm_erp_action_t, dsm_acq_action_t, dsm_rel_action_t,
                     dsm_pi_action_t, dsm_pa_action_t);

extern int
dsm_get_registered_protocols (void);

extern void
dsm_set_read_fault_action(dsm_proto_t, dsm_rf_action_t);

extern dsm_rf_action_t
dsm_get_read_fault_action(dsm_proto_t);

extern void
dsm_set_write_fault_action(dsm_proto_t, dsm_wf_action_t);

extern dsm_wf_action_t
dsm_get_write_fault_action(dsm_proto_t);

extern void
dsm_set_read_server(dsm_proto_t, dsm_rs_action_t);

extern dsm_rs_action_t
dsm_get_read_server(dsm_proto_t);

extern void
dsm_set_write_server(dsm_proto_t, dsm_ws_action_t);

extern dsm_ws_action_t
dsm_get_write_server(dsm_proto_t);

extern void
dsm_set_invalidate_server(dsm_proto_t, dsm_is_action_t);

extern dsm_is_action_t
dsm_get_invalidate_server(dsm_proto_t);

extern void
dsm_set_receive_page_server(dsm_proto_t, dsm_rp_action_t);

extern dsm_rp_action_t
dsm_get_receive_page_server(dsm_proto_t);

extern void
dsm_set_expert_receive_page_server(dsm_proto_t, dsm_erp_action_t);

extern dsm_erp_action_t
dsm_get_expert_receive_page_server(dsm_proto_t);

extern void
dsm_set_acquire_func(dsm_proto_t, dsm_acq_action_t);

extern dsm_acq_action_t
dsm_get_acquire_func(dsm_proto_t);

extern void
dsm_set_release_func(dsm_proto_t, dsm_rel_action_t);

extern dsm_rel_action_t
dsm_get_release_func(dsm_proto_t);

extern void
dsm_set_prot_init_func(dsm_proto_t, dsm_pi_action_t);

extern dsm_pi_action_t
dsm_get_prot_init_func(dsm_proto_t);

extern void
dsm_set_page_add_func(dsm_proto_t, dsm_pa_action_t);

extern dsm_pa_action_t
dsm_get_page_add_func(dsm_proto_t);

#endif   /* DSM_PROTOCOL_POLICY_H */

