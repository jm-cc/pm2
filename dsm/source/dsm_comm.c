
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/* Options: DSM_COMM_TRACE, ASSERT, INSTRUMENT */
#include <fcntl.h>
#include <sys/mman.h>

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
//#define MINIMIZE_PACKS_ON_PAGE_TRANSFER
//#define MINIMIZE_PACKS_ON_DIFF_TRANSFER

#ifndef DO_NOT_MINIMIZE_PACKS_ON_PAGE_TRANSFER
#define MINIMIZE_PACKS_ON_PAGE_TRANSFER
#endif

#ifndef DO_NOT_MINIMIZE_PACKS_ON_DIFF_TRANSFER
#define MINIMIZE_PACKS_ON_DIFF_TRANSFER
#endif

/* tie in to the Hyperion instrumentation */


#ifdef INSTRUMENT
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
int hbrc_diffs_out_cnt = 0;
int hbrc_diffs_in_cnt = 0;

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
  hbrc_diffs_out_cnt = 0;
  hbrc_diffs_in_cnt = 0;
}


void dsm_rpc_dump_instrumentation(void)
{
  tfprintf(stderr, "%d pf_handler_calls\n",
    dsm_pf_handler_calls);
  tfprintf(stderr, "%d readPageReq_in\n",
    hyp_readPage_in_cnt);
#if 0
  tfprintf(stderr, "%d readMPage_in\n",
    hyp_readMPage_in_cnt);
#endif
  tfprintf(stderr, "%d readPageReq_out\n",
    hyp_readPage_out_cnt);
  tfprintf(stderr, "%d writePageReq_in\n",
    hyp_writePage_in_cnt);
#if 0
  tfprintf(stderr, "%d writeMPage_in\n",
    hyp_writeMPage_in_cnt);
#endif
  tfprintf(stderr, "%d writePageReq_out\n",
    hyp_writePage_out_cnt);
  tfprintf(stderr, "%d Page_in\n",
    hyp_sendPage_in_cnt);
#if 0
  tfprintf(stderr, "%d sendMRPage_in\n",
    hyp_sendMRPage_in_cnt);
  tfprintf(stderr, "%d sendMWPage_in\n",
    hyp_sendMWPage_in_cnt);
#endif
  tfprintf(stderr, "%d Page_out\n",
    hyp_sendPage_out_cnt);
  tfprintf(stderr, "%d invalidate_in\n",
    hyp_invalidate_in_cnt);
  tfprintf(stderr, "%d invalidate_out\n",
    hyp_invalidate_out_cnt);
  tfprintf(stderr, "%d invalidateAck_in\n",
    hyp_invalidateAck_in_cnt);
  tfprintf(stderr, "%d invalidateAck_out\n",
    hyp_invalidateAck_out_cnt);
#if 0
  tfprintf(stderr, "%d sendDiffs_in\n",
    hyp_sendDiffs_in_cnt);
  tfprintf(stderr, "%d sendMDiffs_in\n",
    hyp_sendMDiffs_in_cnt);
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
#endif
  tfprintf(stderr, "%d hbrc_diffs_out\n",
    hbrc_diffs_out_cnt);
  tfprintf(stderr, "%d hbrc_diffs_in\n",
    hbrc_diffs_in_cnt);

}
#endif


static marcel_mutex_t dsm_comm_lock;


void dsm_comm_init()
{
   marcel_mutex_init(&dsm_comm_lock, NULL);
}


void dsm_send_page_req(dsm_node_t dest_node, dsm_page_index_t index, dsm_node_t req_node, dsm_access_t req_access, int tag)
{
 LOG_IN();

#ifdef INSTRUMENT
 if (req_access == READ_ACCESS)
   hyp_readPage_out_cnt++;
else
   hyp_writePage_out_cnt++;
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
      pm2_rawrpc_begin(dest_node, DSM_LRPC_READ_PAGE_REQ, NULL);
      break;
    }
  case WRITE_ACCESS: 
    {
      pm2_rawrpc_begin(dest_node, DSM_LRPC_WRITE_PAGE_REQ, NULL);
      break;
    }
  default:
    MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR);
  }
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(dsm_page_index_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&req_node, sizeof(dsm_node_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&tag, sizeof(int));
  pm2_rawrpc_end();

 LOG_OUT();
}

void dsm_invalidate_copyset(dsm_page_index_t index, dsm_node_t new_owner)
{
  LOG_IN();

  (*dsm_get_invalidate_server(dsm_get_page_protocol(index)))(index, dsm_self(), new_owner);

  LOG_OUT();
}

typedef struct {
 void *addr;
 unsigned long page_size;
 dsm_node_t reply_node;
 dsm_access_t access;
 int tag;
} page_header_t;

void dsm_send_page(dsm_node_t dest_node, dsm_page_index_t index, dsm_access_t access, int tag)
{
#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER
  page_header_t to_send;

  to_send.addr = dsm_get_page_addr(index);
  to_send.page_size = dsm_get_page_size(index);
  to_send.reply_node = dsm_self();
  to_send.access = access;
  to_send.tag = tag;
#else
  void *addr = dsm_get_page_addr(index);
  unsigned long page_size = dsm_get_page_size(index);
  dsm_node_t reply_node = dsm_self();
#endif

  LOG_IN();

#ifdef INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif

#ifdef ASSERT
  assert(dest_node != dsm_self());
#endif

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "dsm_send_page: sending page %ld to node %d!\n", index,
    dest_node);
#endif

  pm2_rawrpc_begin(dest_node, DSM_LRPC_SEND_PAGE, NULL);
#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&to_send, sizeof(to_send));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)to_send.addr, to_send.page_size); 
#else
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&page_size,
    sizeof(unsigned long));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&reply_node,
    sizeof(dsm_node_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&access,
    sizeof(dsm_access_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
#endif
  pm2_rawrpc_end();
#ifdef DSM_COMM_TRACE
  fprintf(stderr, "Page %ld sent -> %d for %s\n", index, dest_node, (access == 1)?"read":"write");
#endif

  LOG_OUT();
}


void dsm_send_page_with_user_data(dsm_node_t dest_node, dsm_page_index_t index, dsm_access_t access, void *user_addr, int user_length, int tag)
{
#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER
  page_header_t to_send;

  to_send.addr = dsm_get_page_addr(index);
  to_send.page_size = dsm_get_page_size(index);
  to_send.reply_node = dsm_self();
  to_send.access = access;
  to_send.tag = tag;
#else
  void *addr = dsm_get_page_addr(index);
  unsigned long page_size = dsm_get_page_size(index);
  dsm_node_t reply_node = dsm_self();
#endif

 LOG_IN();

#ifdef INSTRUMENT
  hyp_sendPage_out_cnt++;
#endif


  pm2_rawrpc_begin(dest_node, DSM_LRPC_SEND_PAGE, NULL);
#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER
#ifdef DSM_COMM_TRACE
  tfprintf(stderr,
    "dsm_send_page_with_user_data: sending page %ld (addr %p) to node %d"
    " with tag %d!\n",
    index, to_send.addr, dest_node, to_send.tag);
#endif
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&to_send, sizeof(to_send));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)user_addr, user_length);
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)to_send.addr, to_send.page_size); 
#else
#ifdef DSM_COMM_TRACE
  tfprintf(stderr,
    "dsm_send_page_with_user_data: sending page %ld (addr %p) to node %d"
    " with tag %d!\n",
    index, addr, dest_node, tag);
#endif
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&page_size,
    sizeof(unsigned long));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&reply_node,
    sizeof(dsm_node_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&access,
    sizeof(dsm_access_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)user_addr, user_length);
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
#endif
  
  pm2_rawrpc_end();
#ifdef DSM_COMM_TRACE
  fprintf(stderr, "Page %ld sent -> %d for %s\n", index, dest_node, (access == 1)?"read":"write");
#endif

  LOG_OUT();
}


void dsm_send_invalidate_req(dsm_node_t dest_node, dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner)
{

  LOG_IN();

#ifdef INSTRUMENT
  hyp_invalidate_out_cnt++;
#endif

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "[%s]: sending inv req to node %d for page %ld!\n", __FUNCTION__, dest_node, index);
#endif
  pm2_rawrpc_begin(dest_node, DSM_LRPC_INVALIDATE_REQ, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&new_owner, sizeof(dsm_node_t));
  pm2_rawrpc_end();

  LOG_OUT();
}

void dsm_send_invalidate_ack(dsm_node_t dest_node, dsm_page_index_t index)
{

  LOG_IN();
#ifdef INSTRUMENT
  hyp_invalidateAck_out_cnt++;
#endif

#ifdef DSM_COMM_TRACE
  fprintf(stderr, "[%s]: sending inv ack to node %d for page %ld!\n", __FUNCTION__, dest_node, index);
#endif
  pm2_rawrpc_begin(dest_node, DSM_LRPC_INVALIDATE_ACK, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));
  pm2_rawrpc_end();

  LOG_OUT();
}


static void DSM_LRPC_READ_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  dsm_page_index_t index;
  int tag;

  LOG_IN();

#ifdef INSTRUMENT
  hyp_readPage_in_cnt++;
#endif

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(dsm_page_index_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&tag, sizeof(int));
  pm2_rawrpc_waitdata();
  
  //  dsm_lock_page(index);
  (*dsm_get_read_server(dsm_get_page_protocol(index)))(index, node, tag);
  //  dsm_unlock_page(index);

  LOG_OUT();
}


void DSM_LRPC_READ_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_READ_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_WRITE_PAGE_REQ_threaded_func(void)
{
  dsm_node_t node;
  dsm_page_index_t index;
  int tag;

  LOG_IN();

#ifdef INSTRUMENT
  hyp_writePage_in_cnt++;
#endif
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "DSM_LRPC_WRITE_PAGE_REQ_threaded_func called\n");
#endif

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(dsm_page_index_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&tag, sizeof(int));
  pm2_rawrpc_waitdata(); 
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "DSM_LRPC_WRITE_PAGE_REQ_threaded_func waitdata done"
                 " for node %ld\n", node);
#endif
//  dsm_lock_page(index);
  (*dsm_get_write_server(dsm_get_page_protocol(index)))(index, node, tag);
  //  dsm_unlock_page(index);

  LOG_OUT();
}


void DSM_LRPC_WRITE_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_WRITE_PAGE_REQ_threaded_func, NULL);
}

#define USE_DOUBLE_MAPPING 1

static void DSM_LRPC_SEND_PAGE_threaded_func(void)
{
#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER
  page_header_t to_receive;
#else
  dsm_node_t reply_node;
  dsm_access_t access;
  unsigned long page_size;
  void * addr;
  int tag;
#endif

  dsm_page_index_t index;

  LOG_IN();

#ifdef INSTRUMENT
  hyp_sendPage_in_cnt++;
#endif

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "DSM_LRPC_SEND_PAGE_threaded_func called\n");
#endif

#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&to_receive, sizeof(to_receive));
  index = dsm_page_index(to_receive.addr);

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "addr = %p; page_size is %ld; reply_node is %d; access is %d;"
	   " tag is %d; index is %ld;\n", to_receive.addr, to_receive.page_size, to_receive.reply_node, to_receive.access, to_receive.tag,
	   index);
#endif
#else
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&reply_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&access, sizeof(dsm_access_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&tag, sizeof(int));
  index = dsm_page_index(addr);

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "addr = %p; page_size is %ld; reply_node is %d; access is %d;"
	   " tag is %d; index is %ld;\n", addr, page_size, reply_node, access, tag,
	   index);
#endif
#endif //MINIMIZE_PACKS_ON_PAGE_TRANSFER


  if (dsm_empty_page_entry(index))
    MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR);

  if (dsm_get_expert_receive_page_server(dsm_get_page_protocol(index)))
  {
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "receiving page %ld: calling expert server\n", index);
#endif

#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER
    (*dsm_get_expert_receive_page_server(dsm_get_page_protocol(index)))(to_receive.addr, to_receive.access, to_receive.reply_node, to_receive.page_size, to_receive.tag);
#else
    (*dsm_get_expert_receive_page_server(dsm_get_page_protocol(index)))(addr, access, reply_node, page_size, tag);
#endif
  }
  else
    {
      dsm_access_t old_access = dsm_get_access(index);

#ifdef MINIMIZE_PACKS_ON_PAGE_TRANSFER

#ifdef USE_DOUBLE_MAPPING
       dsm_unpack_page(to_receive.addr, to_receive.page_size);
#else // no double mapping
      /*  Old stuff  */

//     if (old_access != WRITE_ACCESS)

        dsm_protect_page(to_receive.addr, WRITE_ACCESS); // to enable the page to be copied
       pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)to_receive.addr, to_receive.page_size);
	pm2_rawrpc_waitdata(); 

#ifdef DSM_COMM_TRACE
	tfprintf(stderr, "unpacking page %d, addr = %p, size = %d, access = %d, reply = %d\n", index, to_receive.addr, to_receive.page_size, to_receive.access, to_receive.reply_node);
#endif

#endif // USE_DOUBLE_MAPPING


	if (to_receive.access != WRITE_ACCESS)
	  dsm_protect_page(to_receive.addr, old_access);

      /* now call the protocol-specific server */
#ifdef DSM_COMM_TRACE
	tfprintf(stderr, "calling receive page server\n", index);
#endif
//	TBX_GET_TICK(t_end);
//	total = TBX_TIMING_DELAY(t_start, t_end);
       // fprintf(stderr,"unpack time = %f\n", total);
	(*dsm_get_receive_page_server(dsm_get_page_protocol(index)))(to_receive.addr, to_receive.access, to_receive.reply_node, to_receive.tag);
    }

#else // do not MINIMIZE_PACKS_ON_PAGE_TRANSFER

#ifdef USE_DOUBLE_MAPPING
       dsm_unpack_page(addr, page_size);
#else
      /*  Old stuff  */

//     if (old_access != WRITE_ACCESS)

        dsm_protect_page(addr, WRITE_ACCESS); // to enable the page to be copied
       pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size);
       pm2_rawrpc_waitdata(); 

#endif // USE_DOUBLE_MAPPING

#ifdef DSM_COMM_TRACE
	tfprintf(stderr, "unpacking page %d, addr = %p, size = %d, access = %d, reply = %d\n", index, addr, page_size, access, reply_node);
#endif

	if (access != WRITE_ACCESS)
	  dsm_protect_page(addr, old_access);

      /* now call the protocol-specific server */
#ifdef DSM_COMM_TRACE
	tfprintf(stderr, "calling receive page server\n", index);
#endif
	TBX_GET_TICK(t_end);
	total = TBX_TIMING_DELAY(t_start, t_end);
	fprintf(stderr,"unpack time = %f\n", total);
	(*dsm_get_receive_page_server(dsm_get_page_protocol(index)))(addr, access, reply_node, tag);
    }
#endif //MINIMIZE_PACKS_ON_PAGE_TRANSFER

  LOG_OUT();
}


void DSM_LRPC_SEND_PAGE_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_PAGE_threaded_func, NULL);
}
 

static void DSM_LRPC_INVALIDATE_REQ_threaded_func(void)
{
  dsm_node_t req_node, new_owner;
  dsm_page_index_t index;

  LOG_IN();

#ifdef INSTRUMENT
  hyp_invalidate_in_cnt++;
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&new_owner, sizeof(dsm_node_t));
  pm2_rawrpc_waitdata(); 
  // dsm_lock_page(index);
  (*dsm_get_invalidate_server(dsm_get_page_protocol(index)))(index, req_node, new_owner);
  // dsm_unlock_page(index);

  LOG_OUT();
}


void DSM_LRPC_INVALIDATE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_INVALIDATE_REQ_threaded_func, NULL);
}


void DSM_LRPC_INVALIDATE_ACK_func(void)
{
  dsm_page_index_t index;
  
  LOG_IN();

#ifdef INSTRUMENT
  hyp_invalidateAck_in_cnt++;
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));
  pm2_rawrpc_waitdata(); 
#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "Received ack(%d)\n", index);
#endif
  dsm_signal_ack(index, 1);

  LOG_OUT();
}

#define ISOADDR_TEMP_AREA_SIZE 0x4000000
#define ISOADDR_AREA_NEW_TOP (ISOADDR_AREA_TOP - ISOADDR_TEMP_AREA_SIZE)

/* a buffer of size 64 MegaBytes... for some reason */
static char buf[64 * 1024 * 1024];

void dsm_unpack_page(void *addr, unsigned long size)
{
  int fd;
  void *system_view;
  
  LOG_IN();

  /* page unpacking must be done in exclusive mode */
  marcel_mutex_lock(&dsm_comm_lock);
  
  /* associate page to a true file */
  fd = open(RECEIVE_PAGE_FILE, O_CREAT | O_RDWR, 0666);
  write(fd, buf, size);
  addr = mmap(addr, size, PROT_NONE, MAP_PRIVATE | MAP_FIXED, fd, 0);
  if(addr == (void *)-1) 
    MARCEL_EXCEPTION_RAISE(STORAGE_ERROR);
  
  /* associate the file to a system view */
  system_view = mmap((void*)ISOADDR_AREA_NEW_TOP, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
		     fd, 0);   
  if(system_view == (void *)-1) 
    MARCEL_EXCEPTION_RAISE(STORAGE_ERROR);
  
  /*unpack page using the system view*/
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)system_view, size); 
  pm2_rawrpc_waitdata(); 
  
  /*close and delete file */
  munmap(system_view, size);
  close(fd);
  unlink(RECEIVE_PAGE_FILE);
  
  marcel_mutex_unlock(&dsm_comm_lock);

  LOG_OUT();
}

/***********************  Hyperion stuff: ****************************/

typedef struct {
 void *addr;
 int size;
} diff_header_t;

/* pjh: This must be synchronous! */
void dsm_send_diffs(dsm_page_index_t index, dsm_node_t dest_node)
{
#ifdef MINIMIZE_PACKS_ON_DIFF_TRANSFER
  diff_header_t header, empty_header = {0,0};
#else
  void *addr;
  int size;
#endif
  pm2_completion_t c;
  
  LOG_IN();

#ifdef INSTRUMENT
  hyp_sendDiffs_out_cnt++;
#endif

  pm2_completion_init(&c, NULL, NULL);

  pm2_rawrpc_begin(dest_node, DSM_LRPC_SEND_DIFFS, NULL);

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t));

#ifdef MINIMIZE_PACKS_ON_DIFF_TRANSFER
  while ((header.addr = dsm_get_next_modified_data(index, &header.size)) != NULL)
    {
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&header, sizeof(header));
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)header.addr, header.size);
    }
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&empty_header, sizeof(empty_header));
#else
  while ((addr = dsm_get_next_modified_data(index, &size)) != NULL)
    {
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, size);
    }
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
#endif

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

  LOG_OUT();
}

/*
 *  pjh July 2000
 *  Send the initial message, but don't wait for the ACK. Caller must also
 *  call dsm_send_diffs_wait. Between the two calls the caller can therefore
 *  do something like unlock the page, which may be necessary to avoid
 *  deadlock. Note that the caller must provide the address of a
 *  pm2_completion_t variable.
 */
void dsm_send_diffs_start(dsm_page_index_t index, dsm_node_t dest_node,
                          pm2_completion_t* c)
{
#ifdef MINIMIZE_PACKS_ON_DIFF_TRANSFER
  diff_header_t header, empty_header = {0,0};
#else
  void *addr;
  int size;
#endif
  
  LOG_IN();

#ifdef INSTRUMENT
  hyp_sendDiffs_out_cnt++;
#endif

  pm2_completion_init(c, NULL, NULL);

  pm2_rawrpc_begin(dest_node, DSM_LRPC_SEND_DIFFS, NULL);

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t));
#ifdef MINIMIZE_PACKS_ON_DIFF_TRANSFER
  while ((header.addr = dsm_get_next_modified_data(index, &header.size)) != NULL)
    {
#ifdef DSM_COMM_TRACE
      tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %p %d\n", header.addr, header.size);
#endif
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&header, sizeof(header));
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)header.addr, header.size);
    }
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&empty_header, sizeof(empty_header));
#else
  while ((addr = dsm_get_next_modified_data(index, &size)) != NULL)
    {
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, size);
    }
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
#endif

#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "dsm_send_diffs_start: calling pm2_pack_completion: c->proc = %d, &c->sem= %p\n", c->proc, &c->sem);
#endif
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,c);
#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs_start: calling pm2_rawrpc_end\n");
#endif
  pm2_rawrpc_end();

  LOG_OUT();
}

void dsm_send_diffs_wait(pm2_completion_t* c)
{
  LOG_IN();

#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs_wait: calling pm2_completion_wait (I am %p)\n", marcel_self());
#endif
  pm2_completion_wait(c);

#ifdef DSM_COMM_TRACE
tfprintf(stderr, "dsm_send_diffs_wait: returning\n");
#endif

  LOG_OUT();
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
  dsm_page_index_t index;
  pm2_completion_t c;
#ifdef MINIMIZE_PACKS_ON_DIFF_TRANSFER
  diff_header_t header;
#else
  void *addr;
  int size;
#endif

  LOG_IN();

#ifdef INSTRUMENT
  hyp_sendDiffs_in_cnt++;
#endif
#ifdef DSM_COMM_TRACE
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: entered\n");
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t));
#ifdef DSM_COMM_TRACE
  tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %ld\n", index);
#endif

#ifdef MINIMIZE_PACKS_ON_DIFF_TRANSFER
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&header, sizeof(header));
//#ifdef DSM_COMM_TRACE
#ifdef DSM_COMM_TRACE
      tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %p %d\n", header.addr, header.size);
#endif
  while (header.addr != NULL)
  {
      pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)header.addr, header.size);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&header, sizeof(header));
  }

#else
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  while (addr != NULL)
  {
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
#ifdef DSM_COMM_TRACE
      tfprintf(stderr,"DSM_LRPC_SEND_DIFFS_func: %p %d\n", addr, size);
#endif
      pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, size);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  }
#endif //MINIMIZE_PACKS_ON_DIFF_TRANSFER
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

  LOG_OUT();
}

void DSM_LRPC_SEND_DIFFS_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_DIFFS_threaded_func, NULL);
}

/* Using message aggregation to improve performance: */


void DSM_LRPC_SEND_MULTIPLE_DIFFS_threaded_func(void)
{
  dsm_page_index_t index;
  void *addr;
  int size;
  pm2_completion_t c;

  LOG_IN();

#ifdef INSTRUMENT
  hyp_sendMDiffs_in_cnt++;
#endif
#ifdef DEBUG_HYP
  tfprintf(stderr,"DSM_LRPC_SEND_MULTIPLE_DIFFS_func: entered\n");
#endif

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t));
  while (index != NO_PAGE)
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
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t));
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

  LOG_OUT();
}


void DSM_LRPC_SEND_MULTIPLE_DIFFS_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_MULTIPLE_DIFFS_threaded_func, NULL);
}


static void DSM_LRPC_MULTIPLE_READ_PAGE_REQ_threaded_func(void)
{
  dsm_node_t req_node;
  dsm_page_index_t index;
  int tag;

  LOG_IN();

#ifdef INSTRUMENT
  hyp_readMPage_in_cnt++;
#endif
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));

  dsm_begin_send_multiple_pages(req_node, READ_ACCESS, tag);

  while (index != NO_PAGE)
    {
      //      dsm_lock_page(index);
      (*dsm_get_read_server(index))(index, req_node, tag);
      //      dsm_unlock_page(index);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));
    }
  pm2_rawrpc_waitdata(); 

  dsm_end_send_multiple_pages();

  LOG_OUT();
}


void DSM_LRPC_MULTIPLE_READ_PAGE_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_LRPC_MULTIPLE_READ_PAGE_REQ_threaded_func, NULL);
}


static void DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ_threaded_func(void)
{
  dsm_node_t req_node;
  dsm_page_index_t index;
  int tag;

  LOG_IN();

#ifdef INSTRUMENT
  hyp_writeMPage_in_cnt++;
#endif
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));

  dsm_begin_send_multiple_pages(req_node, WRITE_ACCESS, tag);

  while (index != NO_PAGE)
    {
      //      dsm_lock_page(index);
      (*dsm_get_write_server(index))(index, req_node, tag);
      //      dsm_unlock_page(index);
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t));
    }
  pm2_rawrpc_waitdata(); 

  dsm_end_send_multiple_pages();

  LOG_OUT();
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
  dsm_page_index_t index = NO_PAGE;
  int tag;

  LOG_IN();

#ifdef INSTRUMENT
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
	MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR);
      
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
    }

  /* Terminate: */
  if (index != NO_PAGE)
    (*dsm_get_expert_receive_page_server(index))((void *)-1, READ_ACCESS, reply_node, page_size, tag);

#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_READ_threaded_func ended\n");
#endif

  LOG_OUT();
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
  dsm_page_index_t index = NO_PAGE;
  int tag;

  LOG_IN();

#ifdef INSTRUMENT
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
	MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR);
      
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
    }

  /* Terminate: */
  if (index != NO_PAGE)
    (*dsm_get_expert_receive_page_server(index))((void *)-1, READ_ACCESS, reply_node, page_size, tag);
  
#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_threaded_func ended\n");
#endif

  LOG_OUT();
}


void DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_func(void)
{
#ifdef DEBUG_HYP
  tfprintf(stderr, "DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_func called\n");
#endif
  pm2_thread_create((pm2_func_t) DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_threaded_func, NULL);
}
 
