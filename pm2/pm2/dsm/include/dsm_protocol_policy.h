/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#ifndef DSM_PROT_POL_IS_DEF
#define DSM_PROT_POL_IS_DEF

#include "dsm_const.h"

#define LI_HUDAK 0
#define MIGRATE_THREAD 1
#define DEFAULT_DSM_PROTOCOL -1

void dsm_init_protocol_table();

int dsm_create_protocol( dsm_rf_action_t read_fault_handler,
			 dsm_wf_action_t write_fault_handler,
			 dsm_rs_action_t read_server,
			 dsm_ws_action_t write_server,
			 dsm_is_action_t invalidate_server,
			 dsm_rp_action_t receive_page_server,
			 dsm_erp_action_t expert_receive_page_server,
			 dsm_acq_action_t acquire_func,
			 dsm_rel_action_t release_func,
			 dsm_pi_action_t prot_init_func,
			 dsm_pa_action_t page_add_func);

int dsm_registered_protocols();

void dsm_set_read_fault_action(int key, dsm_rf_action_t action);

dsm_rf_action_t dsm_get_read_fault_action(int key);

void dsm_set_write_fault_action(int key, dsm_wf_action_t action);

dsm_wf_action_t dsm_get_write_fault_action(int key);

void dsm_set_read_server(int key, dsm_rs_action_t action);

dsm_rs_action_t dsm_get_read_server(int key);

void dsm_set_write_server(int key, dsm_ws_action_t action);

dsm_ws_action_t dsm_get_write_server(int key);

void dsm_set_invalidate_server(int key, dsm_is_action_t action);

dsm_is_action_t dsm_get_invalidate_server(int key);

void dsm_set_receive_page_server(int key, dsm_rp_action_t action);

dsm_rp_action_t dsm_get_receive_page_server(int key);

void dsm_set_expert_receive_page_server(int key, dsm_erp_action_t action);

dsm_erp_action_t dsm_get_expert_receive_page_server(int key);

dsm_acq_action_t dsm_get_acquire_func(int key);

void dsm_set_acquire_func(int key, dsm_acq_action_t action);

dsm_rel_action_t dsm_get_release_func(int key);

void dsm_set_release_func(int key, dsm_rel_action_t action);

void dsm_set_prot_init_func(int key, dsm_pi_action_t action);

dsm_pi_action_t dsm_get_prot_init_func(int key);

void dsm_set_page_add_func(int key, dsm_pa_action_t action);

dsm_pa_action_t dsm_get_page_add_func(int key);

#endif







