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

#include "dsm_protocol_lib.h" /* dsm_rf_action_t, etc */

typedef struct _dsm_protocol_t
{
  dsm_rf_action_t read_fault;
  dsm_wf_action_t write_fault;
  dsm_rs_action_t read_server;
  dsm_ws_action_t write_server;
  dsm_is_action_t invalidate_server;
  dsm_rp_action_t receive_page_server;
  dsm_erp_action_t expert_receive_page_server;
} dsm_protocol_t;

extern dsm_protocol_t dsmlib_hyp_java_prot;
extern dsm_protocol_t dsmlib_ddm_li_hudak_prot;
extern dsm_protocol_t dsmlib_migrate_thread_prot;

void dsm_init_protocol_table(dsm_protocol_t *protocol);

void dsm_set_page_protocol(unsigned long index, dsm_protocol_t *protocol);

dsm_protocol_t *dsm_get_page_protocol(unsigned long index);

void dsm_set_read_fault_action(unsigned long index, dsm_rf_action_t action);

dsm_rf_action_t dsm_get_read_fault_action(unsigned long index);

void dsm_set_write_fault_action(unsigned long index, dsm_wf_action_t action);

dsm_wf_action_t dsm_get_write_fault_action(unsigned long index);

void dsm_set_read_server(unsigned long index, dsm_rs_action_t action);

dsm_rs_action_t dsm_get_read_server(unsigned long index);

void dsm_set_write_server(unsigned long index, dsm_ws_action_t action);

dsm_ws_action_t dsm_get_write_server(unsigned long index);

void dsm_set_invalidate_server(unsigned long index, dsm_is_action_t action);

dsm_is_action_t dsm_get_invalidate_server(unsigned long index);

void dsm_set_receive_page_server(unsigned long index, dsm_rp_action_t action);

dsm_rp_action_t dsm_get_receive_page_server(unsigned long index);

void dsm_set_expert_receive_page_server(unsigned long index, dsm_erp_action_t action);

dsm_erp_action_t dsm_get_expert_receive_page_server(unsigned long index);

#endif

