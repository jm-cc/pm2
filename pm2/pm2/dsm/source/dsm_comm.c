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

/* Options: DSM_COMM_TRACE, ASSERT, HYP_INSTRUMENT */

#include "pm2.h"
#include "dsm_const.h"
#include "dsm_comm.h"
#include "dsm_page_manager.h"
#include "dsm_page_size.h"
#include "dsm_protocol_policy.h"
#include "dsm_rpc.h"
#include "assert.h"

//#define DSM_COMM_TRACE
//#define ASSERT

/* tie in to the Hyperion instrumentation */
#define HYP_INSTRUMENT 1
#ifdef HYP_INSTRUMENT
int dsm_pf_handler_calls = 0;
int hyp_readPage_in_cnt = 0;
int hyp_readMPage_in_cnt = 0;
int hyp_readPage_out_cnt = 0;
int hyp_writePage_in_cnt = 0;
int hyp_writeMPage_in_cnt = 0;
int hyp_writePage_out_cnt = 0;
int hyp_sendPage_in_cnt = 0;
int hyp_sendMRPage_in_cnt = 0;
int hyp_sendMWPage_in_cnt = 0;
int hyp_sendPage_out_cnt = 0;
int hyp_invalidate_in_cnt = 0;
int hyp_invalidate_out_cnt = 0;
int hyp_invalidateAck_in_cnt = 0;
int hyp_invalidateAck_out_cnt = 0;
int hyp_sendDiffs_in_cnt = 0;
int hyp_sendMDiffs_in_cnt = 0;
int hyp_sendDiffs_out_cnt = 0;
int hyp_lrpcLock_out_cnt = 0;
int hyp_lrpcLock_in_cnt = 0;
int hyp_lrpcUnlock_out_cnt = 0;
int hyp_lrpcUnlock_in_cnt = 0;

void dsm_rpc_clear_instrumentation(void)
{
  dsm_pf_handler_calls = 0;
  hyp_readPage_in_cnt = 0;
  hyp_readMPage_in_cnt = 0;
  hyp_readPage_out_cnt = 0;
  hyp_writePage_in_cnt = 0;
  hyp_writeMPage_in_cnt = 0;
  hyp_writePage_out_cnt = 0;
  hyp_sendPage_in_cnt = 0;
  hyp_sendMRPage_in_cnt = 0;
  hyp_sendMWPage_in_cnt = 0;
  hyp_sendPage_out_cnt = 0;
  hyp_invalidate_in_cnt = 0;
  hyp_invalidate_out_cnt = 0;
  hyp_invalidateAck_in_cnt = 0;
  hyp_invalidateAck_out_cnt = 0;
  hyp_sendDiffs_in_cnt = 0;
  hyp_sendMDiffs_in_cnt = 0;
  hyp_sendDiffs_out_cnt = 0;
  hyp_lrpcLock_out_cnt = 0;
  hyp_lrpcLock_in_cnt = 0;
  hyp_lrpcUnlock_out_cnt = 0;
  hyp_lrpcUnlock_in_cnt = 0;
}

void dsm_rpc_dump_instrumentation(void)
{
  tfprintf(stderr, "%d pf_handler_calls\n",
    dsm_pf_handler_calls);
#if 0
  tfprintf(stderr, "%d readPage_in\n",
    hyp_readPage_in_cnt);
  tfprintf(stderr, "%d readMPage_in\n",
    hyp_readMPage_in_cnt);
  tfprintf(stderr, "%d readPage_out\n",
    hyp_readPage_out_cnt);
#endif
  tfprintf(stderr, "%d writePage_in\n",
    hyp_writePage_in_cnt);
  tfprintf(stderr, "%d writeMPage_in\n",
    hyp_writeMPage_in_cnt);
  tfprintf(stderr, "%d writePage_out\n",
    hyp_writePage_out_cnt);
  tfprintf(stderr, "%d sendPage_in\n",
    hyp_sendPage_in_cnt);
  tfprintf(stderr, "%d sendMRPage_in\n",
    hyp_sendMRPage_in_cnt);
  tfprintf(stderr, "%d sendMWPage_in\n",
    hyp_sendMWPage_in_cnt);
  tfprintf(stderr, "%d sendPage_out\n",
    hyp_sendPage_out_cnt);
#if 0
  tfprintf(stderr, "%d invalidate_in\n",
    hyp_invalidate_in_cnt);
  tfprintf(stderr, "%d invalidate_out\n",
    hyp_invalidate_out_cnt);
  tfprintf(stderr, "%d invalidateAck_in\n",
    hyp_invalidateAck_in_cnt);
  tfprintf(stderr, "%d invalidateAck_out\n",
    hyp_invalidateAck_out_cnt);
#endif
  tfprintf(stderr, "%d sendDiffs_in\n",
    hyp_sendDiffs_in_cnt);
  tfprintf(stderr, "%d sendMDiffs_in\n",
    hyp_sendMDiffs_in_cnt);
  tfprintf(stderr, "%d sendDiffs_out\n",
    hyp_sendDiffs_out_cnt);
#if 0
  tfprintf(stderr, "%d lrpcLock_out\n",
    hyp_lrpcLock_out_cnt);
  tfprintf(stderr, "%d lrpcLock_in\n",
    hyp_lrpcLock_in_cnt);
  tfprintf(stderr, "%d lrpcUnlock_out\n",
    hyp_lrpcUnlock_out_cnt);
  tfprintf(stderr, "%d lrpcUnlock_in\n",
    hyp_lrpcUnlock_in_cnt);
#endif
}
#endif

void dsm_send_page_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_access_t req_access, int tag)
{
#ifdef HYP_INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif
#ifdef ASSERT
  assert(dest_node != dsm_self());
#endif
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_page_req(%d, %ld, %s)\n", dest_node, index,
  req_access == READ_ACCESS ? "READ_ACCESS" : "WRITE_ACCESS");
#endif
  switch(req_access){
  case READ_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_READ_PAGE_REQ, NULL);
      break;
    }
  case WRITE_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_WRITE_PAGE_REQ, NULL);
      break;
    }
  default:
    RAISE(PROGRAM_ERROR);
  }
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(unsigned long));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&req_node, sizeof(dsm_node_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&tag, sizeof(int));
  pm2_rawrpc_end();
}

void dsm_invalidate_copyset(unsigned long index, dsm_node_t new_owner)
{
  (*dsm_get_invalidate_server(dsm_get_page_protocol(index)))(index, dsm_self(), new_owner);
}

void dsm_send_page(dsm_node_t dest_node, unsigned long index, dsm_access_t access, int tag)
{
  void *addr = dsm_get_page_addr(index);
  unsigned long page_size = dsm_get_page_size(index);
  dsm_node_t reply_node = dsm_self();

#ifdef HYP_INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "dsm_send_page: sending page %ld to node %d!\n", index,
    dest_node);
#endif

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_PAGE, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size,
    sizeof(unsigned long));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node,
    sizeof(dsm_node_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&access,
    sizeof(dsm_access_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
  pm2_rawrpc_end();
#ifdef DSM_COMM_TRACE
  fprintf(stderr, "Page %ld sent -> %d for %s\n", index, dest_node, (access == 1)?"read":"write");
#endif
}


void dsm_send_page_with_user_data(dsm_node_t dest_node, unsigned long index, dsm_access_t access, void *user_addr, int user_length, int tag)
{
  void *addr = dsm_get_page_addr(index);
  unsigned long page_size = dsm_get_page_size(index);
  dsm_node_t reply_node = dsm_self();

#ifdef HYP_INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif

#ifdef DSM_COMM_TRACE
  tfprintf(stderr,
    "dsm_send_page_with_user_data: sending page %ld (addr %p) to node %d"
    " with tag %d!\n",
    index, addr, dest_node, tag);
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
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
  
  pm2_rawrpc_end();
#ifdef DSM_COMM_TRACE
  fprintf(stderr, "Page %ld sent -> %d for %s\n", index, dest_node, (access == 1)?"read":"write");
#endif
}


void dsm_send_invalidate_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
#ifdef HYP_INSTRUMENT
  hyp_invalidate_out_cnt++;
#endif

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "[%s]: sending inv req to node %d for page %ld!\n", __FUNCTION__, dest_node, index);
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

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "[%s]: sending inv ack to node %d for page %ld!\n", __FUNCTION__, dest_node, index);
#endif
  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_INVALIDATE_ACK, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
  pm2_rawrpc_end();
}


static void DSM_LRPC_READ_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  unsigned long index;
  int tag;

#ifdef HYP_INSTRUMENT
  hyp_readPage_in_cnt++;
#endif

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(unsigned long));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&tag, sizeof(int));
  pm2_rawrpc_waitdata(); 
  (*dsm_get_read_server(dsm_get_page_protocol(index)))(index, node, tag);
}


void DSM_LRPC_READ_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_READ_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_WRITE_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  unsigned long index;
  int tag;

#ifdef HYP_INSTRUMENT
  hyp_writePage_in_cnt++;
#endif
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "DSM_LRPC_WRITE_PAGE_REQ_threaded_func called\n");
#endif

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(unsigned long));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&tag, sizeof(int));
  pm2_rawrpc_waitdata(); 
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "DSM_LRPC_WRITE_PAGE_REQ_threaded_func waitdata done"
                 " for node %ld\n", node);
#endif
  (*dsm_get_write_server(dsm_get_page_protocol(index)))(index, node, tag);
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
  int tag;

#ifdef HYP_INSTRUMENT
  hyp_sendPage_in_cnt++;
#endif
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "DSM_LRPC_SEND_PAGE_threaded_func called\n");
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&access, sizeof(dsm_access_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&tag, sizeof(int));

  index = dsm_page_index(addr);

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "addr = %p; page_size is %ld; reply_node is %d; access is %d;"
	   " tag is %d; index is %ld;\n", addr, page_size, reply_node, access, tag,
	   index);
#endif

  if (dsm_empty_page_entry(index))
    RAISE(PROGRAM_ERROR);

  if (dsm_get_expert_receive_page_server(dsm_get_page_protocol(index)))
  {
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "receiving page %ld: calling expert server\n", index);
#endif

    (*dsm_get_expert_receive_page_server(dsm_get_page_protocol(index)))(addr, access, reply_node, page_size, tag);
  }
  else
    {
      dsm_access_t old_access = dsm_get_access(index);
      
//      if (old_access != WRITE_ACCESS)
	dsm_protect_page(addr, WRITE_ACCESS); // to enable the page to be copied
      pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "unpacking page %d, addr = %p, size = %d, access = %d, reply = %d\n", index, addr, page_size, access, reply_node);
#endif
      pm2_rawrpc_waitdata(); 
      
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "calling receive page server\n", index);
#endif
      if (access != WRITE_ACCESS)
	dsm_protect_page(addr, old_access);
      /* now call the protocol-specific server */
      (*dsm_get_receive_page_server(dsm_get_page_protocol(index)))(addr, access, reply_node, tag);
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
  (*dsm_get_invalidate_server(dsm_get_page_protocol(index)))(index, req_node, new_owner);
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
#ifdef DSM_COMM_TRACE
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


#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "dsm_send_diffs: calling pm2_pack_completion: c.proc = %d, &c.sem= %p\n", c.proc, &c.sem);
#endif
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs: calling pm2_rawrpc_end\n");
#endif
  pm2_rawrpc_end();

#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs: calling pm2_completion_wait (I am %p)\n", marcel_self());
#endif
  pm2_completion_wait(&c);

#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs: returning\n");
#endif
}

/*
 *  pjh July 2000
 *  Send the initial message, but don't wait for the ACK. Caller must also
 *  call dsm_send_diffs_wait. Between the two calls the caller can therefore
 *  do something like unlock the page, which may be necessary to avoid
 *  deadlock. Note that the caller must provide the address of a
 *  pm2_completion_t variable.
 */
void dsm_send_diffs_start(unsigned long index, dsm_node_t dest_node,
                          pm2_completion_t* c)
{
  void *addr;
  int size;
  
#ifdef HYP_INSTRUMENT
  hyp_sendDiffs_out_cnt++;
#endif

  pm2_completion_init(c, NULL, NULL);

  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_DIFFS, NULL);

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index,
    sizeof(unsigned long));
  while ((addr = dsm_get_next_modified_data(index, &size)) != NULL)
    {
#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "dsm_send_diffs_start: %p %d\n", addr, size);
#endif
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, size);
    }
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));


#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "dsm_send_diffs_start: calling pm2_pack_completion: c->proc = %d, &c->sem= %p\n", c->proc, &c->sem);
#endif
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,c);
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs_start: calling pm2_rawrpc_end\n");
#endif
  pm2_rawrpc_end();
}

void dsm_send_diffs_wait(pm2_completion_t* c)
{
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs_wait: calling pm2_completion_wait (I am %p)\n", marcel_self());
#endif
  pm2_completion_wait(c);

#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs_wait: returning\n");
#endif
}


/* pjh: must be synchronous
 *      and therefore cannot be "quick"
 *
 *      I still worry about the atomicity of the unpacks. Could this thread
 *      lose hand while in the process of unpacking a field? We can't have
 *      another thread see a partial update.
 *
 *      June 2000: Raymond told me that Madeleine unpacks using 32-bit stores
 *                 so we should be okay.
 */
void DSM_LRPC_SEND_DIFFS_threaded_func(void)
{
  unsigned long index;
  void *addr;
  int size;
  pm2_completion_t c;

#ifdef HYP_INSTRUMENT
  hyp_sendDiffs_in_cnt++;
#endif
#ifdef DSM_COMM_TRACE
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: entered\n");
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index,
    sizeof(unsigned long));
#ifdef DSM_COMM_TRACE
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %ld\n", index);
#endif
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  while (addr != NULL)
  {
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
#ifdef DSM_COMM_TRACE
      tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %p %d\n", addr, size);
#endif
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, size);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  }

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);

  pm2_rawrpc_waitdata();

#ifdef DSM_COMM_TRACE
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: msg unpacked; call pm2_completion_signal\n");
#endif

  /* no actual work to do, but unload the diffs which was done above */

  pm2_completion_signal(&c);

#ifdef DSM_COMM_TRACE
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: returning (signalled c.proc = %d, &c.sem = %p\n", c.proc, &c.sem);
#endif
}

void DSM_LRPC_SEND_DIFFS_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_DIFFS_threaded_func, NULL);
}

/* Using message aggregation to improve performance: */


void DSM_LRPC_SEND_MULTIPLE_DIFFS_threaded_func(void)
{
  unsigned long index;
  void *addr;
  int size;
  pm2_completion_t c;

#ifdef HYP_INSTRUMENT
  hyp_sendMDiffs_in_cnt++;
#endif
#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_MULTIPLE_DIFFS_func: entered\n");
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(unsigned long));
  while (index != -1)
    {
#ifdef DEBUG_HYP
      tfprintf(stderr,"DSM_LRPC_SEND_MULTIPLE_DIFFS_func: %ld\n", index);
#endif
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
      while (addr != NULL)
	{
	  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
#ifdef DEBUG_HYP
	  tfprintf(stderr,"DSM_LRPC_SEND_MULTIPLE_DIFFS_func: %p %d\n", addr, size);
#endif
	  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, size);
	  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
	}
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(unsigned long));
    }

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);

  pm2_rawrpc_waitdata();

#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_MULTIPLE_DIFFS_func: msg unpacked; call pm2_completion_signal\n");
#endif

  /* no actual work to do, but unload the diffs which was done above */

  pm2_completion_signal(&c);

#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_MULTIPLE_DIFFS_func: returning (signalled c.proc = %d, c.c_ptr = %p\n", c.proc, c.c_ptr);
#endif
}


void DSM_LRPC_SEND_MULTIPLE_DIFFS_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_MULTIPLE_DIFFS_threaded_func, NULL);
}


static void DSM_LRPC_MULTIPLE_READ_PAGE_REQ_threaded_func(void)
{
  dsm_node_t req_node;
  unsigned long index;
  int tag;

#ifdef HYP_INSTRUMENT
  hyp_readMPage_in_cnt++;
#endif
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));

  dsm_begin_send_multiple_pages(req_node, READ_ACCESS, tag);

  while (index != -1)
    {
      (*dsm_get_read_server(index))(index, req_node, tag);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
    }
  pm2_rawrpc_waitdata(); 

  dsm_end_send_multiple_pages();
}


void DSM_LRPC_MULTIPLE_READ_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_MULTIPLE_READ_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ_threaded_func(void)
{
  dsm_node_t req_node;
  unsigned long index;
  int tag;

#ifdef HYP_INSTRUMENT
  hyp_writeMPage_in_cnt++;
#endif
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));

  dsm_begin_send_multiple_pages(req_node, WRITE_ACCESS, tag);

  while (index != -1)
    {
      (*dsm_get_write_server(index))(index, req_node, tag);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long));
    }
  pm2_rawrpc_waitdata(); 

  dsm_end_send_multiple_pages();
}


void DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_SEND_MULTIPLE_PAGES_READ_threaded_func(void)
{
  dsm_node_t reply_node;
  unsigned long page_size;
  void * addr;
  unsigned long index = -1;
  int tag;

#ifdef HYP_INSTRUMENT
  hyp_sendMRPage_in_cnt++;
#endif
#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_READ_threaded_func called\n");
#endif
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&tag, sizeof(int));

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  
  while (addr != NULL)
    {
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
      
      index = dsm_page_index(addr);
      
      if (dsm_get_expert_receive_page_server(index))
	{
#ifdef DEBUG_HYP
	  tfprintf(stderr, "receiving page %ld: calling expert server\n", index);
#endif
	  (*dsm_get_expert_receive_page_server(index))(addr, READ_ACCESS, reply_node, page_size, tag);
	}
      else
	RAISE(PROGRAM_ERROR);
      
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
    }

  /* Terminate: */
  if (index != -1)
    (*dsm_get_expert_receive_page_server(index))((void *)-1, READ_ACCESS, reply_node, page_size, tag);

#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_READ_threaded_func ended\n");
#endif
}


void DSM_LRPC_SEND_MULTIPLE_PAGES_READ_func(void)
{
#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_READ_func called\n");
#endif
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_MULTIPLE_PAGES_READ_threaded_func, NULL);
}
 

static void DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_threaded_func()
{
  dsm_node_t reply_node;
  unsigned long page_size;
  void *addr;
  unsigned long index = -1;
  int tag;

#ifdef HYP_INSTRUMENT
  hyp_sendMWPage_in_cnt++;
#endif
#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_threaded_func called\n");
#endif
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&reply_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&tag, sizeof(int));

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  
  while (addr != NULL)
    {
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
      
      index = dsm_page_index(addr);
      
      if (dsm_get_expert_receive_page_server(index))
	{
#ifdef DEBUG_HYP
	  tfprintf(stderr, "receiving page %ld: calling expert server\n", index);
#endif
	  (*dsm_get_expert_receive_page_server(index))(addr, WRITE_ACCESS, reply_node, page_size, tag);
	}
      else
	RAISE(PROGRAM_ERROR);
      
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
    }

  /* Terminate: */
  if (index != -1)
    (*dsm_get_expert_receive_page_server(index))((void *)-1, READ_ACCESS, reply_node, page_size, tag);
  
#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_threaded_func ended\n");
#endif
}


void DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_func(void)
{
#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_func called\n");
#endif
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_threaded_func, NULL);
}
 
