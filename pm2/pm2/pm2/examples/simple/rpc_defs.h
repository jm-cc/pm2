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
	SAMPLE,
        INFO,
	LRPC_PING,
	LRPC_PING_ASYNC,
	LRPC_PING_QUICK_ASYNC
END_LRPC_LIST


/* SAMPLE */

typedef char sample_tab[64];

LRPC_DECL_REQ(SAMPLE, sample_tab tab;)
LRPC_DECL_RES(SAMPLE,)


/* INFO */

extern unsigned BUF_SIZE;
extern unsigned NB_PING;

LRPC_DECL_REQ(INFO,);
LRPC_DECL_RES(INFO,);

/* PING */

#define MAX_BUF_SIZE    16384

#define BUF_PACKING     MAD_IN_PLACE

LRPC_DECL_REQ(LRPC_PING,)
LRPC_DECL_RES(LRPC_PING,)


/* PING_ASYNC */

LRPC_DECL_REQ(LRPC_PING_ASYNC,)
LRPC_DECL_RES(LRPC_PING_ASYNC,)


#endif
