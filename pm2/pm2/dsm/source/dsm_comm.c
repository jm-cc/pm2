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
//#define DEBUG_HYP
#define ASSERT

void dsm_send_page_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_access_t req_access)
{
#ifdef ASSERT
  assert(dest_node != dsm_self());
#endif
  switch(req_access){
  case READ_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_READ_PAGE_REQ, NULL);
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
      pm2_rawrpc_end();
      break;
    }
  case WRITE_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_WRITE_PAGE_REQ, NULL);
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
      pm2_rawrpc_end();
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

void dsm_send_page(dsm_node_t dest_node, unsigned long index, dsm_access_t access)
{
  void *addr = dsm_get_page_addr(index);
  unsigned long page_size = dsm_get_page_size(index);
  dsm_node_t reply_node = dsm_self();

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_PAGE, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node, sizeof(dsm_node_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&access, sizeof(dsm_access_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, page_size); 
  pm2_rawrpc_end();
#ifdef DEBUG3
  fprintf(stderr, "Page %d sent -> %d for %s, a = %d \n", index, dest_node, (access == 1)?"read":"write", ((atomic_t *)addr)->counter);
#endif
}

void dsm_send_invalidate_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_INVALIDATE_REQ, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&new_owner, sizeof(dsm_node_t));
  pm2_rawrpc_end();
}

void dsm_send_invalidate_ack(dsm_node_t dest_node, unsigned long index)
{
  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_INVALIDATE_ACK, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_rawrpc_end();
}


static void DSM_LRPC_READ_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  unsigned long index;

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&node, sizeof(dsm_node_t));
  pm2_rawrpc_waitdata(); 
  (*dsm_get_read_server(index))(index, node);
}


void DSM_LRPC_READ_PAGE_REQ_func(void)
{
  pm2_thread_create(DSM_LRPC_READ_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_WRITE_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  unsigned long index;

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&node, sizeof(dsm_node_t));
  pm2_rawrpc_waitdata(); 
  (*dsm_get_write_server(index))(index, node);
}


void DSM_LRPC_WRITE_PAGE_REQ_func(void)
{
  pm2_thread_create(DSM_LRPC_WRITE_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_SEND_PAGE_threaded_func(void)
{
  dsm_node_t reply_node;
  unsigned long page_size;
  void * addr;
  dsm_access_t access, old_access;
  unsigned long index;

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&access, sizeof(dsm_access_t));

  index = dsm_page_index(addr);
  old_access = dsm_get_access(index);

  //     fprintf(stderr, "lock_task: I am %p \n", marcel_self());  
  lock_task();
  dsm_protect_page(addr, WRITE_ACCESS); // to enable the page to be copied
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, page_size); 
  if (old_access != access)
    dsm_protect_page(addr, old_access);
  unlock_task();
  pm2_rawrpc_waitdata(); 
  (*dsm_get_receive_page_server(index))(addr, access, reply_node);
}


void DSM_LRPC_SEND_PAGE_func(void)
{
  pm2_thread_create(DSM_LRPC_SEND_PAGE_threaded_func, NULL);
}
 

static void DSM_LRPC_INVALIDATE_REQ_threaded_func(void)
{
  dsm_node_t req_node, new_owner;
  unsigned long index;

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&new_owner, sizeof(dsm_node_t));
  pm2_rawrpc_waitdata(); 
  dsm_lock_page(index);
  (*dsm_get_invalidate_server(index))(index, req_node, new_owner);
  dsm_unlock_page(index);
}


void DSM_LRPC_INVALIDATE_REQ_func(void)
{
  pm2_thread_create(DSM_LRPC_INVALIDATE_REQ_threaded_func, NULL);
}


void DSM_LRPC_INVALIDATE_ACK_func(void)
{
  unsigned long index;
  
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_rawrpc_waitdata(); 
#ifdef DEBUG2
  tfprintf(stderr, "Received ack(%d)\n", index);
#endif
  dsm_signal_ack(index, 1);
}


/***********************  Hyperion stuff: ****************************/

void dsm_send_diffs(unsigned long index, dsm_node_t dest_node)
{
  void *addr;
  int size;
  
  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_DIFFS, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(unsigned long));
  while ((addr = dsm_get_next_modified_data(index, &size)) != NULL)
    {
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, size);
    }
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));   
  pm2_rawrpc_end();
}


void DSM_LRPC_SEND_DIFFS_func(void)
{
#ifdef DEBUG_HYP
  tfprintf(stderr, "Received diffs(%d)\n", req.index);
#endif
}
