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

/* GRAPH */

PACK_REQ_STUB(GRAPH)
END_STUB

UNPACK_REQ_STUB(GRAPH)
END_STUB

PACK_RES_STUB(GRAPH)
END_STUB

UNPACK_RES_STUB(GRAPH)
END_STUB


/* SYNC_DISPLAY */

PACK_REQ_STUB(SYNC_DISPLAY)
     mad_pack_int(MAD_IN_HEADER, &arg->regul, 1);
END_STUB

UNPACK_REQ_STUB(SYNC_DISPLAY)
     mad_unpack_int(MAD_IN_HEADER, &arg->regul, 1);
END_STUB

PACK_RES_STUB(SYNC_DISPLAY)
END_STUB

UNPACK_RES_STUB(SYNC_DISPLAY)
END_STUB


/* SPY */

PACK_REQ_STUB(SPY)
END_STUB

UNPACK_REQ_STUB(SPY)
END_STUB

PACK_RES_STUB(SPY)
END_STUB

UNPACK_RES_STUB(SPY)
END_STUB

/* LOAD */

PACK_REQ_STUB(LOAD)
     mad_pack_int(MAD_IN_HEADER, &arg->load, 1);
END_STUB

UNPACK_REQ_STUB(LOAD)
     mad_unpack_int(MAD_IN_HEADER, &arg->load, 1);
END_STUB

PACK_RES_STUB(LOAD)
END_STUB

UNPACK_RES_STUB(LOAD)
END_STUB





