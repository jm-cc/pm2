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

#include <pm2.h>
#include <sys/isomalloc_rpc.h>

/**************************************************/
/* GLOBAL_LOCK */

PACK_REQ_STUB(LRPC_ISOMALLOC_GLOBAL_LOCK)
#ifdef DEBUG_NEGOCIATION
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->master, sizeof(unsigned int));
#endif
END_STUB

UNPACK_REQ_STUB(LRPC_ISOMALLOC_GLOBAL_LOCK)
#ifdef DEBUG_NEGOCIATION
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->master, sizeof(unsigned int));
#endif
END_STUB

PACK_RES_STUB(LRPC_ISOMALLOC_GLOBAL_LOCK)
END_STUB

UNPACK_RES_STUB(LRPC_ISOMALLOC_GLOBAL_LOCK)
END_STUB

/**************************************************/
/* LOCAL_LOCK */

PACK_REQ_STUB(LRPC_ISOMALLOC_LOCAL_LOCK)
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->master, sizeof(unsigned int));
END_STUB

UNPACK_REQ_STUB(LRPC_ISOMALLOC_LOCAL_LOCK)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->master, sizeof(unsigned int));
END_STUB

PACK_RES_STUB(LRPC_ISOMALLOC_LOCAL_LOCK)
END_STUB

UNPACK_RES_STUB(LRPC_ISOMALLOC_LOCAL_LOCK)
END_STUB

/**************************************************/
/* GLOBAL_UNLOCK */

PACK_REQ_STUB(LRPC_ISOMALLOC_GLOBAL_UNLOCK)
END_STUB

UNPACK_REQ_STUB(LRPC_ISOMALLOC_GLOBAL_UNLOCK)
END_STUB

PACK_RES_STUB(LRPC_ISOMALLOC_GLOBAL_UNLOCK)
END_STUB

UNPACK_RES_STUB(LRPC_ISOMALLOC_GLOBAL_UNLOCK)
END_STUB

/**************************************************/
/* LOCAL_UNLOCK */

PACK_REQ_STUB(LRPC_ISOMALLOC_LOCAL_UNLOCK)
     mad_pack_byte(MAD_IN_PLACE, (char *)&arg->slot_map, sizeof(bitmap_t));
END_STUB

UNPACK_REQ_STUB(LRPC_ISOMALLOC_LOCAL_UNLOCK)
     mad_unpack_byte(MAD_IN_PLACE, (char *)&arg->slot_map, sizeof(bitmap_t));
END_STUB

PACK_RES_STUB(LRPC_ISOMALLOC_LOCAL_UNLOCK)
END_STUB

UNPACK_RES_STUB(LRPC_ISOMALLOC_LOCAL_UNLOCK)
END_STUB
/**************************************************/
/* SYNC */

PACK_REQ_STUB(LRPC_ISOMALLOC_SYNC)
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->sender, sizeof(unsigned int));
END_STUB

UNPACK_REQ_STUB(LRPC_ISOMALLOC_SYNC)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->sender, sizeof(unsigned int));
END_STUB

PACK_RES_STUB(LRPC_ISOMALLOC_SYNC)
END_STUB

UNPACK_RES_STUB(LRPC_ISOMALLOC_SYNC)
END_STUB
/**************************************************/
/* SEND SLOT STATUS*/

PACK_REQ_STUB(LRPC_ISOMALLOC_SEND_SLOT_STATUS)
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->sender, sizeof(unsigned int));
     mad_pack_byte(MAD_IN_PLACE, (char *)&arg->slot_map, sizeof(bitmap_t));
END_STUB

UNPACK_REQ_STUB(LRPC_ISOMALLOC_SEND_SLOT_STATUS)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->sender, sizeof(unsigned int));
     mad_unpack_byte(MAD_IN_PLACE, (char *)&arg->slot_map, sizeof(bitmap_t));
END_STUB

PACK_RES_STUB(LRPC_ISOMALLOC_SEND_SLOT_STATUS)
END_STUB

UNPACK_RES_STUB(LRPC_ISOMALLOC_SEND_SLOT_STATUS)
END_STUB

/**************************************************/

EXTERN_LRPC_SERVICE(LRPC_ISOMALLOC_GLOBAL_LOCK);
EXTERN_LRPC_SERVICE(LRPC_ISOMALLOC_GLOBAL_UNLOCK);
EXTERN_LRPC_SERVICE(LRPC_ISOMALLOC_LOCAL_LOCK);
EXTERN_LRPC_SERVICE(LRPC_ISOMALLOC_LOCAL_UNLOCK);
EXTERN_LRPC_SERVICE(LRPC_ISOMALLOC_SYNC);
EXTERN_LRPC_SERVICE(LRPC_ISOMALLOC_SEND_SLOT_STATUS);

void pm2_isomalloc_init_rpc()
{
  DECLARE_LRPC(LRPC_ISOMALLOC_GLOBAL_LOCK);
  DECLARE_LRPC(LRPC_ISOMALLOC_GLOBAL_UNLOCK);
  DECLARE_LRPC(LRPC_ISOMALLOC_LOCAL_LOCK);
  DECLARE_LRPC(LRPC_ISOMALLOC_LOCAL_UNLOCK);
  DECLARE_LRPC(LRPC_ISOMALLOC_SYNC);
  DECLARE_LRPC(LRPC_ISOMALLOC_SEND_SLOT_STATUS);
}
