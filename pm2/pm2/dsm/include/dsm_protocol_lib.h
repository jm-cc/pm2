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

#ifndef DSM_PROT_LIB_IS_DEF
#define DSM_PROT_LIB_IS_DEF

#include "dsm_const.h" 

void dsmlib_rf_ask_for_read_copy(unsigned long index);

void dsmlib_migrate_thread(unsigned long index);

void dsmlib_wf_ask_for_write_access(unsigned long index);

void dsmlib_rs_send_read_copy(unsigned long index, dsm_node_t req_node, int tag);

void dsmlib_ws_send_page_for_write_access(unsigned long index, dsm_node_t req_node, int tag);

void dsmlib_is_invalidate(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);

void dsmlib_rp_validate_page(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag);

#endif




