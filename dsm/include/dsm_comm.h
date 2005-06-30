
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

#ifndef DSM_COMM_IS_DEF
#define DSM_COMM_IS_DEF

#include "dsm_const.h" // dsm_access_t, dsm_node_t
#include "dsm_page_manager.h"
#include "pm2.h" //pm2_completion_init, etc.

#define INSTRUMENT 1
#undef INSTRUMENT

void dsm_send_page_req(dsm_node_t dest_node, dsm_page_index_t index, dsm_node_t req_node, dsm_access_t req_access, int tag);

void dsm_invalidate_copyset(dsm_page_index_t index, dsm_node_t new_owner);

void dsm_send_page(dsm_node_t dest_node, dsm_page_index_t index, dsm_access_t access, int tag);

void dsm_send_page_with_user_data(dsm_node_t dest_node, dsm_page_index_t index, dsm_access_t access, void *user_addr, int user_length, int tag);

void dsm_send_invalidate_req(dsm_node_t dest_node, dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner);

void dsm_send_invalidate_ack(dsm_node_t dest_node, dsm_page_index_t index);

#define RECEIVE_PAGE_FILE "/tmp/dsm_pm2_dynamic_page"
void dsm_unpack_page(void *addr, unsigned long size);

#ifdef INSTRUMENT
void dsm_rpc_dump_instrumentation(void);
#endif

void dsm_comm_init();

/***********************  Hyperion stuff: ****************************/

void dsm_send_diffs(dsm_page_index_t index, dsm_node_t dest_node);

void dsm_send_diffs_start(dsm_page_index_t index, dsm_node_t dest_node, pm2_completion_t* c);
void dsm_send_diffs_wait(pm2_completion_t* c);

#define dsm_begin_send_multiple_diffs(dest_node)  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_MULTIPLE_DIFFS, NULL)

static __inline__ void dsm_pack_diffs(dsm_page_index_t index) __attribute__ ((unused));
static __inline__ void dsm_pack_diffs(dsm_page_index_t index)
{
  void *addr;
  int size;

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t)); 
  while ((addr = dsm_get_next_modified_data(index, &size)) != NULL) 
    { 
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *)); 
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int)); 
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, size); 
    } 
  // send the NULL value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *)); 
}

static __inline__  void dsm_end_send_diffs()  __attribute__ ((unused));
static __inline__  void dsm_end_send_diffs() 
{ 
  pm2_completion_t c; 
  int end = -1; 
  
  pm2_completion_init(&c, NULL, NULL); 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&end, sizeof(int)); 
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c); 
  pm2_rawrpc_end(); 
#ifdef DEBUG_HYP 
  tfprintf(stderr, "dsm_send_multiple_diffs: calling pm2_completion_wait (I am %p)\n", marcel_self()); 
#endif 
  pm2_completion_wait(&c); 
}

static __inline__ void dsm_begin_send_multiple_page_req(dsm_node_t dest_node, dsm_node_t req_node, dsm_access_t req_access, int tag)  __attribute__ ((unused));
static __inline__ void dsm_begin_send_multiple_page_req(dsm_node_t dest_node, dsm_node_t req_node, dsm_access_t req_access, int tag)
{
 switch(req_access){
  case READ_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_MULTIPLE_READ_PAGE_REQ, NULL);
      break;
    }
  case WRITE_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ, NULL);
      break;
    }
  default:
    RAISE(PROGRAM_ERROR);
 }
 pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&req_node, sizeof(dsm_node_t));
 pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
}

#define dsm_pack_page_req(index) pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(dsm_page_index_t))

static __inline__ void  dsm_end_send_multiple_page_req()  __attribute__ ((unused));
static __inline__ void  dsm_end_send_multiple_page_req()
{
 dsm_page_index_t end = -1;

 pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&end, sizeof(dsm_page_index_t));
 pm2_rawrpc_end();
}

static __inline__ void dsm_begin_send_multiple_pages(dsm_node_t dest_node, dsm_access_t access, int tag) __attribute__ ((unused));
static __inline__ void dsm_begin_send_multiple_pages(dsm_node_t dest_node, dsm_access_t access, int tag)
{
  dsm_node_t reply_node = dsm_self();

  switch(access){
  case READ_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_MULTIPLE_PAGES_READ, NULL);
      break;
    }
  case WRITE_ACCESS: 
    {
      pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE, NULL);
      break;
    }
  default:
    RAISE(PROGRAM_ERROR);
 }
 pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&reply_node, sizeof(dsm_node_t));
 pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&tag, sizeof(int));
}

static __inline__ void dsm_pack_page(dsm_page_index_t index) __attribute__ ((unused));
static __inline__ void dsm_pack_page(dsm_page_index_t index)
{
  void *addr = dsm_get_page_addr(index);
  unsigned long page_size = dsm_get_page_size(index);

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&addr, sizeof(void *));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&page_size, sizeof(unsigned long));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
}


static __inline__ void  dsm_end_send_multiple_pages() __attribute__ ((unused));
static __inline__ void  dsm_end_send_multiple_pages()
{
 void *end = NULL;

 /* Send NULL to terminate */
 pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&end, sizeof(void *));
 pm2_rawrpc_end();
}

#endif




