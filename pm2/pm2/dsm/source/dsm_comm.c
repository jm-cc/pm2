/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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

#include "pm2.h"
#include "dsm_const.h"
#include "dsm_comm.h"
#include "dsm_page_manager.h"
#include "dsm_protocol_policy.h"
#include "dsm_rpc.h"
#include "assert.h"


//#define DEBUG1

void dsm_send_page_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_access_t req_access)
{
  assert(dest_node != dsm_self());
  switch(req_access){
  case READ_ACCESS: 
    {
      LRPC_REQ(DSM_LRPC_READ_PAGE_REQ) req;

      req.index = index;
      req.req_node = req_node;
      pm2_async_rpc((int)dest_node, DSM_LRPC_READ_PAGE_REQ, NULL, &req);
      break;
    }
  case WRITE_ACCESS: 
    {
      LRPC_REQ(DSM_LRPC_WRITE_PAGE_REQ) req;

      req.index = index;
      req.req_node = req_node;
      pm2_async_rpc((int)dest_node, DSM_LRPC_WRITE_PAGE_REQ, NULL, &req);
      break;
    }
  default:
    RAISE(PROGRAM_ERROR);
  }
}

void dsm_invalidate_copyset(unsigned long index, dsm_node_t new_owner)
{
  (*dsm_get_invalidate_server(index))(index, dsm_self(), new_owner);
}

void dsm_send_page(dsm_node_t dest_node, unsigned long index, dsm_node_t reply_node, dsm_access_t access)
{
  LRPC_REQ(DSM_LRPC_SEND_PAGE) req;

  req.addr = dsm_get_page_addr(index);
  req.page_size = dsm_get_page_size(index);
  req.reply_node = dsm_self();
  req.access = access;

  pm2_async_rpc((int)dest_node, DSM_LRPC_SEND_PAGE, NULL, &req);
#ifdef DEBUG3
     fprintf(stderr, "Page %d sent -> %d for %s, a = %d \n", index, dest_node, (access == 1)?"read":"write", ((atomic_t *)req.addr)->counter);
#endif
}

void dsm_send_invalidate_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
  LRPC_REQ(DSM_LRPC_INVALIDATE_REQ) req;

  req.index = index;
  req.req_node = req_node;
  req.new_owner = new_owner;
  pm2_async_rpc((int)dest_node, DSM_LRPC_INVALIDATE_REQ, NULL, &req);
}

void dsm_send_invalidate_ack(dsm_node_t dest_node, unsigned long index)
{
  LRPC_REQ(DSM_LRPC_INVALIDATE_ACK) req;

  req.index = index;
  pm2_quick_async_rpc((int)dest_node, DSM_LRPC_INVALIDATE_ACK, NULL, &req);
}


BEGIN_SERVICE(DSM_LRPC_READ_PAGE_REQ)
#ifdef DEBUG1
     fprintf(stderr,"will enter RS...(I am %p)\n", marcel_self());
#endif
     (*dsm_get_read_server(req.index))(req.index, req.req_node);
END_SERVICE(DSM_LRPC_READ_PAGE_REQ)

BEGIN_SERVICE(DSM_LRPC_WRITE_PAGE_REQ)
#ifdef DEBUG1
     fprintf(stderr,"will enter WS...(I am %p)\n", marcel_self());
#endif
     (*dsm_get_write_server(req.index))(req.index, req.req_node);
END_SERVICE(DSM_LRPC_WRITE_PAGE_REQ)

BEGIN_SERVICE(DSM_LRPC_SEND_PAGE)
     (*dsm_get_receive_page_server(dsm_page_index(req.addr)))(req.addr, req.access, req.reply_node);
END_SERVICE(DSM_LRPC_SEND_PAGE)


BEGIN_SERVICE(DSM_LRPC_INVALIDATE_REQ)
     dsm_lock_page(req.index);
     (*dsm_get_invalidate_server(req.index))(req.index, req.req_node, req.new_owner);
     dsm_unlock_page(req.index);
END_SERVICE(DSM_LRPC_INVALIDATE_REQ)


BEGIN_SERVICE(DSM_LRPC_INVALIDATE_ACK)
#ifdef DEBUG2
    tfprintf(stderr, "Received ack(%d)\n", req.index);
#endif
    dsm_signal_ack(req.index, 1);
END_SERVICE(DSM_LRPC_INVALIDATE_ACK)


BEGIN_SERVICE(DSM_LRPC_SEND_DIFFS)
#ifdef DEBUG_HYP
    tfprintf(stderr, "Received diffs(%d)\n", req.index);
#endif
END_SERVICE(DSM_LRPC_SEND_DIFFS)

/***********************  Hyperion stuff: ****************************/

void dsm_send_diffs(unsigned long index, dsm_node_t dest_node)
{
   LRPC_REQ(DSM_LRPC_SEND_DIFFS) req;

  req.index = index;
  pm2_async_rpc((int)dest_node, DSM_LRPC_SEND_DIFFS, NULL, &req);
}
