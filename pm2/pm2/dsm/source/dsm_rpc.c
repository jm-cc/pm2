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
/* READ_PAGE_REQ */

PACK_REQ_STUB(DSM_LRPC_READ_PAGE_REQ)
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->req_node, sizeof(dsm_node_t));
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_READ_PAGE_REQ)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->req_node, sizeof(dsm_node_t));
END_STUB

PACK_RES_STUB(DSM_LRPC_READ_PAGE_REQ)
END_STUB

UNPACK_RES_STUB(DSM_LRPC_READ_PAGE_REQ)
END_STUB

/**************************************************/
/* WRITE_PAGE_REQ */

PACK_REQ_STUB(DSM_LRPC_WRITE_PAGE_REQ)
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->req_node, sizeof(dsm_node_t));
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_WRITE_PAGE_REQ)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->req_node, sizeof(dsm_node_t));
END_STUB

PACK_RES_STUB(DSM_LRPC_WRITE_PAGE_REQ)
END_STUB

UNPACK_RES_STUB(DSM_LRPC_WRITE_PAGE_REQ)
END_STUB

/**************************************************/
/* SEND_PAGE */

PACK_REQ_STUB(DSM_LRPC_SEND_PAGE)
     mad_pack_byte(MAD_IN_HEADER, (char *)&(arg->addr), sizeof(void *));
     mad_pack_byte(MAD_IN_HEADER, (char *)&(arg->page_size), sizeof(unsigned long));
     mad_pack_byte(MAD_IN_HEADER, (char *)&(arg->reply_node), sizeof(dsm_node_t));
     mad_pack_byte(MAD_IN_HEADER, (char *)&(arg->access), sizeof(dsm_access_t));
     mad_pack_byte(MAD_IN_HEADER, (char *)(arg->addr), arg->page_size);   
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_SEND_PAGE)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&(arg->addr), sizeof(void *));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&(arg->page_size), sizeof(unsigned long));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&(arg->reply_node), sizeof(dsm_node_t));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&(arg->access), sizeof(dsm_access_t));
     //     fprintf(stderr, "lock_task: I am %p \n", marcel_self());  
     lock_task();
     dsm_protect_page(arg->addr, WRITE_ACCESS); // to enable the page to be coppied
     mad_unpack_byte(MAD_IN_HEADER, (char *)(arg->addr), arg->page_size);
     dsm_protect_page(arg->addr, dsm_get_access(dsm_page_index(arg->addr)));
     unlock_task();
END_STUB

PACK_RES_STUB(DSM_LRPC_SEND_PAGE)
END_STUB

UNPACK_RES_STUB(DSM_LRPC_SEND_PAGE)
END_STUB

/**************************************************/
/* INVALIDATE_REQ */

PACK_REQ_STUB(DSM_LRPC_INVALIDATE_REQ)
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->req_node, sizeof(dsm_node_t));
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->new_owner, sizeof(dsm_node_t));
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_INVALIDATE_REQ)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->req_node, sizeof(dsm_node_t));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->new_owner, sizeof(dsm_node_t));
END_STUB

PACK_RES_STUB(DSM_LRPC_INVALIDATE_REQ)
END_STUB

UNPACK_RES_STUB(DSM_LRPC_INVALIDATE_REQ)
END_STUB

/**************************************************/
/* INVALIDATE_ACK */

PACK_REQ_STUB(DSM_LRPC_INVALIDATE_ACK)
     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_INVALIDATE_ACK)
     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
END_STUB

PACK_RES_STUB(DSM_LRPC_INVALIDATE_ACK)
END_STUB

UNPACK_RES_STUB(DSM_LRPC_INVALIDATE_ACK)
END_STUB

/**************************************************/
/* SEND_DIFFS */

PACK_REQ_STUB(DSM_LRPC_SEND_DIFFS)
     void *addr;
     int size;

     mad_pack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     while ((addr = dsm_get_next_modified_data(arg->index, &size)) != NULL)
       {
	 mad_pack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));
	 mad_pack_byte(MAD_IN_HEADER, (char *)&size, sizeof(int));
	 mad_pack_byte(MAD_IN_HEADER, (char *)addr, size);
       }
    /* send the NULL value to terminate */ 
    mad_pack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));   
END_STUB

UNPACK_REQ_STUB(DSM_LRPC_SEND_DIFFS)
     void *addr;
     int size;

     mad_unpack_byte(MAD_IN_HEADER, (char *)&arg->index, sizeof(unsigned long));
     mad_unpack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));     
     while (addr != NULL)
       {
	 mad_unpack_byte(MAD_IN_HEADER, (char *)&size, sizeof(int));
	 mad_unpack_byte(MAD_IN_HEADER, (char *)addr, size); 
	 mad_unpack_byte(MAD_IN_HEADER, (char *)&addr, sizeof(void *));
       }
END_STUB

PACK_RES_STUB(DSM_LRPC_SEND_DIFFS)
     
END_STUB

UNPACK_RES_STUB(DSM_LRPC_SEND_DIFFS)
END_STUB

/**************************************************/

EXTERN_LRPC_SERVICE(DSM_LRPC_READ_PAGE_REQ);
EXTERN_LRPC_SERVICE(DSM_LRPC_WRITE_PAGE_REQ);
EXTERN_LRPC_SERVICE(DSM_LRPC_SEND_PAGE);
EXTERN_LRPC_SERVICE(DSM_LRPC_INVALIDATE_REQ);
EXTERN_LRPC_SERVICE(DSM_LRPC_INVALIDATE_ACK);
EXTERN_LRPC_SERVICE(DSM_LRPC_SEND_DIFFS);

void dsm_pm2_init_rpc()
{
  DECLARE_LRPC(DSM_LRPC_READ_PAGE_REQ);
  DECLARE_LRPC(DSM_LRPC_WRITE_PAGE_REQ);
  DECLARE_LRPC(DSM_LRPC_SEND_PAGE);
  DECLARE_LRPC(DSM_LRPC_INVALIDATE_REQ);
  DECLARE_LRPC(DSM_LRPC_INVALIDATE_ACK);
  DECLARE_LRPC(DSM_LRPC_SEND_DIFFS);
}
