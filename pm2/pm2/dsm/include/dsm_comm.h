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

#ifndef DSM_COMM_IS_DEF
#define DSM_COMM_IS_DEF

#include "dsm_const.h" // dsm_access_t, dsm_node_t
#include "dsm_page_manager.h"


void dsm_send_page_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_access_t req_access, int tag);

void dsm_invalidate_copyset(unsigned long index, dsm_node_t new_owner);

void dsm_send_page(dsm_node_t dest_node, unsigned long index, dsm_access_t access, int tag);

void dsm_send_page_with_user_data(dsm_node_t dest_node, unsigned long index, dsm_access_t access, void *user_addr, int user_length, int tag);

void dsm_send_invalidate_req(dsm_node_t dest_node, unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);

void dsm_send_invalidate_ack(dsm_node_t dest_node, unsigned long index);

#define RECEIVE_PAGE_FILE "/tmp/dsm_pm2_dynamic_page"
void dsm_unpack_page(void *addr, unsigned long size);

/***********************  Hyperion stuff: ****************************/

void dsm_send_diffs(unsigned long index, dsm_node_t dest_node);

void dsm_send_diffs_start(unsigned long index, dsm_node_t dest_node,
                          pm2_completion_t* c);
void dsm_send_diffs_wait(pm2_completion_t* c);

#define dsm_begin_send_multiple_diffs(dest_node)  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_SEND_MULTIPLE_DIFFS, NULL)

static __inline__ void dsm_pack_diffs(unsigned long index) __attribute__ ((unused));
static __inline__ void dsm_pack_diffs(unsigned long index)
{
  void *addr;
  int size;

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(unsigned long)); 
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

#define dsm_pack_page_req(index) pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&index, sizeof(unsigned long))

static __inline__ void  dsm_end_send_multiple_page_req()  __attribute__ ((unused));
static __inline__ void  dsm_end_send_multiple_page_req()
{
 int end = -1;

 pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char*)&end, sizeof(unsigned long));
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

static __inline__ void dsm_pack_page(unsigned long index) __attribute__ ((unused));
static __inline__ void dsm_pack_page(unsigned long index)
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




