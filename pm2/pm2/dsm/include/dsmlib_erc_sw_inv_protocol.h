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

              Eager Release Consistency Implementation
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

