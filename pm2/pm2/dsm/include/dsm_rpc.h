/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
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

#ifndef DSM_RPC_IS_DEF
#define DSM_RPC_IS_DEF

#include "dsm_const.h"
#include "dsm_mutex.h"


BEGIN_LRPC_LIST
  DSM_LRPC_READ_PAGE_REQ,//11
  DSM_LRPC_WRITE_PAGE_REQ,//12
  DSM_LRPC_SEND_PAGE,//13
  DSM_LRPC_INVALIDATE_REQ,//14
  DSM_LRPC_INVALIDATE_ACK,//15
  DSM_LRPC_SEND_DIFFS,//16
  DSM_LRPC_LOCK,//17
  DSM_LRPC_UNLOCK//18
END_LRPC_LIST

/*******************************************************/
 
LRPC_DECL_REQ(DSM_LRPC_READ_PAGE_REQ, unsigned long index; dsm_node_t req_node;);
LRPC_DECL_RES(DSM_LRPC_READ_PAGE_REQ,);
 
/*******************************************************/

LRPC_DECL_REQ(DSM_LRPC_WRITE_PAGE_REQ, unsigned long index; dsm_node_t req_node;);
LRPC_DECL_RES(DSM_LRPC_WRITE_PAGE_REQ,);
 
/*******************************************************/

LRPC_DECL_REQ(DSM_LRPC_SEND_PAGE, void *addr; unsigned long page_size; dsm_node_t reply_node; dsm_access_t access;);
LRPC_DECL_RES(DSM_LRPC_SEND_PAGE,);
  
/*******************************************************/

LRPC_DECL_REQ(DSM_LRPC_INVALIDATE_REQ, unsigned long index; dsm_node_t req_node; dsm_node_t new_owner;);
LRPC_DECL_RES(DSM_LRPC_INVALIDATE_REQ,);
  
/*******************************************************/

LRPC_DECL_REQ(DSM_LRPC_INVALIDATE_ACK, unsigned long index;);
LRPC_DECL_RES(DSM_LRPC_INVALIDATE_ACK,);
  
/*******************************************************/

LRPC_DECL_REQ(DSM_LRPC_SEND_DIFFS, unsigned long index;);
LRPC_DECL_RES(DSM_LRPC_SEND_DIFFS,);
  
/*******************************************************/

LRPC_DECL_REQ(DSM_LRPC_LOCK, dsm_mutex_t *mutex;);
LRPC_DECL_RES(DSM_LRPC_LOCK,);
  
/*******************************************************/

LRPC_DECL_REQ(DSM_LRPC_UNLOCK, dsm_mutex_t *mutex;);
LRPC_DECL_RES(DSM_LRPC_UNLOCK,);
  
/*******************************************************/

void dsm_pm2_init_rpc(void);

#endif


