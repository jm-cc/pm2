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

#include "rpc_defs.h"
#include <string.h>

static char ping_buf[MAX_BUF_SIZE] __MAD_ALIGNED__;

/* SAMPLE */

PACK_REQ_STUB(SAMPLE)
   old_old_mad_pack_str(MAD_IN_HEADER, arg->tab);
END_STUB

UNPACK_REQ_STUB(SAMPLE)
   old_old_mad_unpack_str(MAD_IN_HEADER, arg->tab);
END_STUB

PACK_RES_STUB(SAMPLE)
END_STUB

UNPACK_RES_STUB(SAMPLE)
END_STUB


/* INFO */

unsigned BUF_SIZE, NB_PING;

PACK_REQ_STUB(INFO)
   old_old_mad_pack_int(MAD_IN_HEADER, &BUF_SIZE, 1);
   old_old_mad_pack_int(MAD_IN_HEADER, &NB_PING, 1);
END_STUB

UNPACK_REQ_STUB(INFO)
   old_old_mad_unpack_int(MAD_IN_HEADER, &BUF_SIZE, 1);
   old_old_mad_unpack_int(MAD_IN_HEADER, &NB_PING, 1);
END_STUB

PACK_RES_STUB(INFO)
END_STUB

UNPACK_RES_STUB(INFO)
END_STUB


/* PING */

PACK_REQ_STUB(LRPC_PING)
   old_old_mad_pack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

UNPACK_REQ_STUB(LRPC_PING)
   old_old_mad_unpack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

PACK_RES_STUB(LRPC_PING)
   old_old_mad_pack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

UNPACK_RES_STUB(LRPC_PING)
   old_old_mad_unpack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB


/* PING_ASYNC */

PACK_REQ_STUB(LRPC_PING_ASYNC)
   old_old_mad_pack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

UNPACK_REQ_STUB(LRPC_PING_ASYNC)
   old_old_mad_unpack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

PACK_RES_STUB(LRPC_PING_ASYNC)
END_STUB

UNPACK_RES_STUB(LRPC_PING_ASYNC)
END_STUB








