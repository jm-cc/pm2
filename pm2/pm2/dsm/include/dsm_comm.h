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

#ifndef DSM_COMM_IS_DEF
#define DSM_COMM_IS_DEF

#include "dsm_const.h" // dsm_access_t, dsm_node_t
#include "dsm_page_manager.h"

#define DSM_SEND_PAGE_CHEAPER

void dsm_send_page_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_access_t req_access);

void dsm_invalidate_copyset(unsigned long index, dsm_node_t new_owner);

void dsm_send_page(dsm_node_t dest_node, unsigned long index, dsm_access_t access);

void dsm_send_page_with_user_data(dsm_node_t dest_node, unsigned long index, dsm_access_t access, void *user_addr, int user_length);

void dsm_send_invalidate_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);

void dsm_send_invalidate_ack(dsm_node_t dest_node, unsigned long index);

/***********************  Hyperion stuff: ****************************/

void dsm_send_diffs(unsigned long index, dsm_node_t dest_node);

#endif




