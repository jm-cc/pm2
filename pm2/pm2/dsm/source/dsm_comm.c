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
#include "dsm_page_size.h"
#include "dsm_protocol_policy.h"
#include "dsm_rpc.h"
#include "assert.h"

//#define DEBUG1
//#define DEBUG_HYP
//#define ASSERT

/* tie in to the Hyperion instrumentation */
#define HYP_INSTRUMENT 1
#ifdef HYP_INSTRUMENT
int hyp_readPage_in_cnt = 0;
int hyp_readPage_out_cnt = 0;
int hyp_writePage_in_cnt = 0;
int hyp_writePage_out_cnt = 0;
int hyp_sendPage_in_cnt = 0;
int hyp_sendPage_out_cnt = 0;
int hyp_invalidate_in_cnt = 0;
int hyp_invalidate_out_cnt = 0;
int hyp_invalidateAck_in_cnt = 0;
int hyp_invalidateAck_out_cnt = 0;
int hyp_sendDiffs_in_cnt = 0;
int hyp_sendDiffs_out_cnt = 0;
int hyp_lrpcLock_out_cnt = 0;
int hyp_lrpcLock_in_cnt = 0;
int hyp_lrpcUnlock_out_cnt = 0;
int hyp_lrpcUnlock_in_cnt = 0;

void dsm_rpc_clear_instrumentation(void)
{
  hyp_readPage_in_cnt = 0;
  hyp_readPage_out_cnt = 0;
  hyp_writePage_in_cnt = 0;
  hyp_writePage_out_cnt = 0;
  hyp_sendPage_in_cnt = 0;
  hyp_sendPage_out_cnt = 0;
  hyp_invalidate_in_cnt = 0;
  hyp_invalidate_out_cnt = 0;
  hyp_invalidateAck_in_cnt = 0;
  hyp_invalidateAck_out_cnt = 0;
  hyp_sendDiffs_in_cnt = 0;
  hyp_sendDiffs_out_cnt = 0;
  hyp_lrpcLock_out_cnt = 0;
  hyp_lrpcLock_in_cnt = 0;
  hyp_lrpcUnlock_out_cnt = 0;
  hyp_lrpcUnlock_in_cnt = 0;
}

void dsm_rpc_dump_instrumentation(void)
{
  tfprintf(stderr, "%d readPage_in\n",
    hyp_readPage_in_cnt);
  tfprintf(stderr, "%d readPage_out\n",
    hyp_readPage_out_cnt);
  tfprintf(stderr, "%d writePage_in\n",
    hyp_writePage_in_cnt);
  tfprintf(stderr, "%d writePage_out\n",
    hyp_writePage_out_cnt);
  tfprintf(stderr, "%d sendPage_in\n",
    hyp_sendPage_in_cnt);
  tfprintf(stderr, "%d sendPage_out\n",
    hyp_sendPage_out_cnt);
  tfprintf(stderr, "%d invalidate_in\n",
    hyp_invalidate_in_cnt);
  tfprintf(stderr, "%d invalidate_out\n",
    hyp_invalidate_out_cnt);
  tfprintf(stderr, "%d invalidateAck_in\n",
    hyp_invalidateAck_in_cnt);
  tfprintf(stderr, "%d invalidateAck_out\n",
    hyp_invalidateAck_out_cnt);
 tfprintf(stderr, "%d sendDiffs_in\n",
    hyp_sendDiffs_in_cnt);
  tfprintf(stderr, "%d sendDiffs_out\n",
    hyp_sendDiffs_out_cnt);
  tfprintf(stderr, "%d lrpcLock_out\n",
    hyp_lrpcLock_out_cnt);
  tfprintf(stderr, "%d lrpcLock_in\n",
    hyp_lrpcLock_in_cnt);
  tfprintf(stderr, "%d lrpcUnlock_out\n",
    hyp_lrpcUnlock_out_cnt);
  tfprintf(stderr, "%d lrpcUnlock_in\n",
    hyp_lrpcUnlock_in_cnt);
}
#endif

void dsm_send_page_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_access_t req_access)
{
#ifdef HYP_INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif
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

#ifdef HYP_INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif

#ifdef DEBUG_HYP
  tfprintf(stderr, "dsm_send_page: sending page %ld!\n", index);
#endif

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_PAGE, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size,
    sizeof(unsigned long));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node,
    sizeof(dsm_node_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&access,
    sizeof(dsm_access_t));
#ifdef DSM_SEND_PAGE_CHEAPER
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
#else
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, page_size); 
#endif
  pm2_rawrpc_end();
#ifdef DEBUG3
  fprintf(stderr, "Page %d sent -> %d for %s, a = %d \n", index, dest_node, (access == 1)?"read":"write", ((atomic_t *)addr)->counter);
#endif
}


void dsm_send_page_with_user_data(dsm_node_t dest_node, unsigned long index, dsm_access_t access, void *user_addr, int user_length)
{
  void *addr = dsm_get_page_addr(index);
  unsigned long page_size = dsm_get_page_size(index);
  dsm_node_t reply_node = dsm_self();

#ifdef HYP_INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif

#ifdef DEBUG_HYP
  tfprintf(stderr, "dsm_send_page: sending page %ld!\n", index);
#endif

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_PAGE, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size,
    sizeof(unsigned long));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node,
    sizeof(dsm_node_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&access,
    sizeof(dsm_access_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)user_addr, user_length);
#ifdef DSM_SEND_PAGE_CHEAPER
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
#else
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, page_size); 
#endif
  
  pm2_rawrpc_end();
#ifdef DEBUG3
  fprintf(stderr, "Page %d sent -> %d for %s, a = %d \n", index, dest_node, (access == 1)?"read":"write", ((atomic_t *)addr)->counter);
#endif
}


void dsm_send_invalidate_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
#ifdef HYP_INSTRUMENT
  hyp_invalidate_out_cnt++;
#endif

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_INVALIDATE_REQ, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&new_owner, sizeof(dsm_node_t));
  pm2_rawrpc_end();
}

void dsm_send_invalidate_ack(dsm_node_t dest_node, unsigned long index)
{
#ifdef HYP_INSTRUMENT
  hyp_invalidateAck_out_cnt++;
#endif

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_INVALIDATE_ACK, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_rawrpc_end();
}


static void DSM_LRPC_READ_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  unsigned long index;

#ifdef HYP_INSTRUMENT
  hyp_readPage_in_cnt++;
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&node, sizeof(dsm_node_t));
  pm2_rawrpc_waitdata(); 
  (*dsm_get_read_server(index))(index, node);
}


void DSM_LRPC_READ_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_READ_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_WRITE_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  unsigned long index;

#ifdef HYP_INSTRUMENT
  hyp_writePage_in_cnt++;
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&node, sizeof(dsm_node_t));
  pm2_rawrpc_waitdata(); 
  (*dsm_get_write_server(index))(index, node);
}


void DSM_LRPC_WRITE_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_WRITE_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_SEND_PAGE_threaded_func(void)
{
  dsm_node_t reply_node;
  dsm_access_t access;
  unsigned long page_size;
  void * addr;
  unsigned long index;

#ifdef HYP_INSTRUMENT
  hyp_sendPage_in_cnt++;
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&access, sizeof(dsm_access_t));

  index = dsm_page_index(addr);

  if (dsm_get_expert_receive_page_server(index))
    (*dsm_get_expert_receive_page_server(index))(addr, access, reply_node, page_size);
  else
    {
      dsm_access_t old_access = dsm_get_access(index);
      
      if (old_access != WRITE_ACCESS)
	dsm_protect_page(addr, WRITE_ACCESS); // to enable the page to be copied
#ifdef DSM_SEND_PAGE_CHEAPER
      pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
#else
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, page_size); 
#endif
      pm2_rawrpc_waitdata(); 
      
#ifdef DEBUG_HYP
      tfprintf(stderr, "unpack page %ld: calling page server\n", index);
#endif
      if (access != WRITE_ACCESS)
	dsm_protect_page(addr, old_access);
      /* now call the protocol-specific server */
      (*dsm_get_receive_page_server(index))(addr, access, reply_node);
    }
}


void DSM_LRPC_SEND_PAGE_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_PAGE_threaded_func, NULL);
}
 

static void DSM_LRPC_INVALIDATE_REQ_threaded_func(void)
{
  dsm_node_t req_node, new_owner;
  unsigned long index;

#ifdef HYP_INSTRUMENT
  hyp_invalidate_in_cnt++;
#endif

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
  pm2_thread_create((pm2_func_t) DSM_LRPC_INVALIDATE_REQ_threaded_func, NULL);
}


void DSM_LRPC_INVALIDATE_ACK_func(void)
{
  unsigned long index;
  
#ifdef HYP_INSTRUMENT
  hyp_invalidateAck_in_cnt++;
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_rawrpc_waitdata(); 
#ifdef DEBUG2
  tfprintf(stderr, "Received ack(%d)\n", index);
#endif
  dsm_signal_ack(index, 1);
}


/***********************  Hyperion stuff: ****************************/

/* pjh: This must be synchronous! */
void dsm_send_diffs(unsigned long index, dsm_node_t dest_node)
{
  void *addr;
  int size;
  pm2_completion_t c;
  
#ifdef HYP_INSTRUMENT
  hyp_sendDiffs_out_cnt++;
#endif

  pm2_completion_init(&c, NULL, NULL);

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_DIFFS, NULL);

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index,
    sizeof(unsigned long));
  while ((addr = dsm_get_next_modified_data(index, &size)) != NULL)
    {
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, size);
    }
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));


#ifdef DEBUG_HYP
  tfprintf(stderr, "dsm_send_diffs: calling pm2_pack_completion: c.proc = %d, c.sem_ptr = %p\n", c.proc, c.sem_ptr);
#endif
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
#ifdef DEBUG_HYP
tfprintf(stderr, "dsm_send_diffs: calling pm2_rawrpc_end\n");
#endif
  pm2_rawrpc_end();

#ifdef DEBUG_HYP
tfprintf(stderr, "dsm_send_diffs: calling pm2_completion_wait (I am %p)\n", marcel_self());
#endif
  pm2_completion_wait(&c);

#ifdef DEBUG_HYP
tfprintf(stderr, "dsm_send_diffs: returning\n");
#endif
}


/* pjh: must be synchronous
 *
 *      I still worry about the atomicity of the unpacks. Could this thread
 *      lose hand while in the process of unpacking a field? We can't have
 *      another thread see a partial update.
 */
void DSM_LRPC_SEND_DIFFS_func(void)
{
  unsigned long index;
  void *addr;
  int size;
  pm2_completion_t c;

#ifdef HYP_INSTRUMENT
  hyp_sendDiffs_in_cnt++;
#endif
#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: entered\n");
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index,
    sizeof(unsigned long));
#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %ld\n", index);
#endif
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  while (addr != NULL)
  {
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
#ifdef DEBUG_HYP
      tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %p %d\n", addr, size);
#endif
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, size);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  }

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);

  pm2_rawrpc_waitdata();

#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: msg unpacked; call pm2_completion_signal\n");
#endif

  /* no actual work to do, but unload the diffs which was done above */

  pm2_completion_signal(&c);

#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: returning (signalled c.proc = %d, c.sem_ptr = %p\n", c.proc, c.sem_ptr);
#endif
}


