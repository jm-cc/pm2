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

/* PING */

PACK_REQ_STUB(LRPC_PING)
   mad_pack_int(MAD_BY_COPY, &arg->nb, 1);
END_STUB

UNPACK_REQ_STUB(LRPC_PING)
   mad_unpack_int(MAD_BY_COPY, &arg->nb, 1);
END_STUB

PACK_RES_STUB(LRPC_PING)
END_STUB

UNPACK_RES_STUB(LRPC_PING)
END_STUB


/* SAMPLE */

PACK_REQ_STUB(LRPC_SAMPLE)
   mad_pack_str(MAD_IN_PLACE, arg->tab);
END_STUB

UNPACK_REQ_STUB(LRPC_SAMPLE)
   mad_unpack_str(MAD_IN_PLACE, arg->tab);
END_STUB

PACK_RES_STUB(LRPC_SAMPLE)
END_STUB

UNPACK_RES_STUB(LRPC_SAMPLE)
END_STUB


