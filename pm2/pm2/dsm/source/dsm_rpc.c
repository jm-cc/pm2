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
#include <dsm_rpc.h>
#include "dsm_page_manager.h"

/**************************************************/
/* SEND_DIFFS */
/*
PACK_REQ_STUB(DSM_LRPC_SEND_DIFFS)
     void *addr;
     int size;

     old_mad_pack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     while ((addr = dsm_get_next_modified_data(arg->index, &size)) != NULL)
       {
	 old_mad_pack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));
	 old_mad_pack_byte(MAD_IN_HEADER, (char *)&size, sizeof(int));
	 old_mad_pack_byte(MAD_IN_HEADER, (char *)addr, size);
       }
    // send the NULL value to terminate 
    old_mad_pack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));   
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_SEND_DIFFS)
     void *addr;
     int size;

     old_mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     old_mad_unpack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));     
     while (addr != NULL)
       {
	 old_mad_unpack_byte(MAD_IN_HEADER, (char *)&size, sizeof(int));
	 old_mad_unpack_byte(MAD_IN_HEADER, (char *)addr, size); 
	 old_mad_unpack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));
       }
END_STUB

PACK_RES_STUB(DSM_LRPC_SEND_DIFFS)
     
END_STUB

UNPACK_RES_STUB(DSM_LRPC_SEND_DIFFS)
END_STUB
*/

/**************************************************/
/* DSM_LRPC_LOCK */

PACK_REQ_STUB(DSM_LRPC_LOCK)
     old_mad_pack_byte(MAD_IN_HEADER, (char *)&arg->mutex, sizeof(dsm_mutex_t *));
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_LOCK)
     old_mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->mutex, sizeof(dsm_mutex_t *));
END_STUB

PACK_RES_STUB(DSM_LRPC_LOCK)
END_STUB

UNPACK_RES_STUB(DSM_LRPC_LOCK)
END_STUB

/**************************************************/
/* DSM_LRPC_UNLOCK */

PACK_REQ_STUB(DSM_LRPC_UNLOCK)
     old_mad_pack_byte(MAD_IN_HEADER, (char *)&arg->mutex, sizeof(dsm_mutex_t *));
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_UNLOCK)
     old_mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->mutex, sizeof(dsm_mutex_t *));
END_STUB

PACK_RES_STUB(DSM_LRPC_UNLOCK)
END_STUB

UNPACK_RES_STUB(DSM_LRPC_UNLOCK)
END_STUB

/**************************************************/

//EXTERN_LRPC_SERVICE(DSM_LRPC_READ_PAGE_REQ);
extern void DSM_LRPC_READ_PAGE_REQ_func(void);
extern void DSM_LRPC_WRITE_PAGE_REQ_func(void);
extern void DSM_LRPC_SEND_PAGE_func(void);
extern void DSM_LRPC_INVALIDATE_REQ_func(void);
extern void DSM_LRPC_INVALIDATE_ACK_func(void);
extern void DSM_LRPC_SEND_DIFFS_func(void);
//extern void DSM_LRPC_LOCK_func(void);
//extern void DSM_LRPC_UNLOCK_func(void);
//extern void DSM_LRPC_MUTEX_ACK_func(void);
EXTERN_LRPC_SERVICE(DSM_LRPC_LOCK);
EXTERN_LRPC_SERVICE(DSM_LRPC_UNLOCK);

void dsm_pm2_init_rpc()
{
  pm2_rawrpc_register(&DSM_LRPC_READ_PAGE_REQ, DSM_LRPC_READ_PAGE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_WRITE_PAGE_REQ, DSM_LRPC_WRITE_PAGE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_SEND_PAGE, DSM_LRPC_SEND_PAGE_func);
  pm2_rawrpc_register(&DSM_LRPC_INVALIDATE_REQ, DSM_LRPC_INVALIDATE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_INVALIDATE_ACK, DSM_LRPC_INVALIDATE_ACK_func);
  pm2_rawrpc_register(&DSM_LRPC_SEND_DIFFS, DSM_LRPC_SEND_DIFFS_func);
  //  pm2_rawrpc_register(&DSM_LRPC_LOCK, DSM_LRPC_LOCK_func);
  //  pm2_rawrpc_register(&DSM_LRPC_UNLOCK, DSM_LRPC_UNLOCK_func);
  //  pm2_rawrpc_register(&DSM_LRPC_MUTEX_ACK, DSM_LRPC_MUTEX_ACK_func);
  DECLARE_LRPC(DSM_LRPC_LOCK);
  DECLARE_LRPC(DSM_LRPC_UNLOCK);
}
