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

#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include <pm2.h>

BEGIN_LRPC_LIST
  GRAPH,
  SYNC_DISPLAY,
  /* pour la partie regulation : */
  SPY,
  LOAD
END_LRPC_LIST


/* GRAPH */

LRPC_DECL_REQ(GRAPH,)
LRPC_DECL_RES(GRAPH,)

LRPC_DECL_REQ(SYNC_DISPLAY, int regul;)
LRPC_DECL_RES(SYNC_DISPLAY,);

LRPC_DECL_REQ(SPY,)
LRPC_DECL_RES(SPY,)

LRPC_DECL_REQ(LOAD, int load;)
LRPC_DECL_RES(LOAD,);

#endif
