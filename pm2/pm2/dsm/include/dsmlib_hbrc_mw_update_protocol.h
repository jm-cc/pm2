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

              Home-Based Release Consistency Implementation
                          Vincent Bernardi

                      2000 All Rights Reserved


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


#ifndef DSM_HBRC_MW_UPDATE_IS_DEF
#define DSM_HBRC_MW_UPDATE_IS_DEF

#include <assert.h>
#include "marcel.h" // tfprintf 
#include "pm2.h"
#include "dsm_const.h" 
#include "dsm_page_manager.h"
#include "dsm_comm.h"
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h"
#include "dsmlib_belist.h"
#include "dsm_page_size.h"
#include "dsm_mutex.h"


void dsmlib_hbrc_mw_update_init(int protocol_number);

void dsmlib_hbrc_mw_update_rfh(unsigned long index);

void dsmlib_hbrc_mw_update_wfh(unsigned long index);

void dsmlib_hbrc_mw_update_rs(unsigned long index, dsm_node_t req_node, int arg);

void dsmlib_hbrc_mw_update_ws(unsigned long index, dsm_node_t req_node, int arg);

void dsmlib_hbrc_mw_update_is(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);

void dsmlib_hbrc_mw_update_rps(void *addr, dsm_access_t access, dsm_node_t reply_node, int arg);

void dsmlib_hbrc_acquire();

void dsmlib_hbrc_release();

void dsmlib_hbrc_add_page(unsigned long index);

void dsmlib_hbrc_mw_update_prot_init(unsigned long index);

void dsmlib_hrbc_send_diffs(unsigned long index, dsm_node_t dest_node);

void DSM_HRBC_DIFFS_threaded_func(void);

void DSM_LRPC_HBRC_DIFFS_func(void);

void dsmlib_hrbc_invalidate_copyset(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);
#endif










