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

______________________________________________________________________________
$Log: isomalloc_rpc.h,v $
Revision 1.2  2000/01/31 15:50:22  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef ISOMALLOC_RPC_IS_DEF
#define ISOMALLOC_RPC_IS_DEF

#include "isomalloc.h"
#include "bitmap.h"

/* #define DEBUG_NEGOCIATION  */


BEGIN_LRPC_LIST
  LRPC_ISOMALLOC_GLOBAL_LOCK,
  LRPC_ISOMALLOC_GLOBAL_UNLOCK,
  LRPC_ISOMALLOC_LOCAL_LOCK,
  LRPC_ISOMALLOC_LOCAL_UNLOCK,
  LRPC_ISOMALLOC_SYNC,
  LRPC_ISOMALLOC_SEND_SLOT_STATUS
END_LRPC_LIST


/*******************************************************/

#ifdef DEBUG_NEGOCIATION 
LRPC_DECL_REQ(LRPC_ISOMALLOC_GLOBAL_LOCK, unsigned int master;);
#else
LRPC_DECL_REQ(LRPC_ISOMALLOC_GLOBAL_LOCK,);
#endif
LRPC_DECL_RES(LRPC_ISOMALLOC_GLOBAL_LOCK,);

 
/*******************************************************/
 
LRPC_DECL_REQ(LRPC_ISOMALLOC_GLOBAL_UNLOCK,);
LRPC_DECL_RES(LRPC_ISOMALLOC_GLOBAL_UNLOCK,);
 
/*******************************************************/
LRPC_DECL_REQ(LRPC_ISOMALLOC_LOCAL_LOCK, unsigned int master;);
LRPC_DECL_RES(LRPC_ISOMALLOC_LOCAL_LOCK,);
 
/*******************************************************/
 
LRPC_DECL_REQ(LRPC_ISOMALLOC_LOCAL_UNLOCK, bitmap_t slot_map;);
LRPC_DECL_RES(LRPC_ISOMALLOC_LOCAL_UNLOCK,);
 
/*******************************************************/
 
LRPC_DECL_REQ(LRPC_ISOMALLOC_SYNC, unsigned int sender;);
LRPC_DECL_RES(LRPC_ISOMALLOC_SYNC,);

/*******************************************************/

LRPC_DECL_REQ(LRPC_ISOMALLOC_SEND_SLOT_STATUS, bitmap_t slot_map; unsigned int sender;);
LRPC_DECL_RES(LRPC_ISOMALLOC_SEND_SLOT_STATUS,);
  
/*******************************************************/
void pm2_isomalloc_init_rpc(void);

#endif


