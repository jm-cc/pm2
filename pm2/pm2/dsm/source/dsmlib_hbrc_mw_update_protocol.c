
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

#include "dsmlib_hbrc_mw_update_protocol.h" 
/* the following is useful for "token_lock_id_t" */
#include "token_lock.h"
#include "dsm_protocol_policy.h"


//#define DSM_PROT_TRACE
#ifdef INSTRUMENT
extern int hbrc_diffs_out_cnt;
extern int hbrc_diffs_in_cnt;
#endif

#ifdef DSM_PROT_TRACE
#define IN tfprintf(stderr,"[%p] -> %s\n", marcel_self(), __FUNCTION__)
#define OUT tfprintf(stderr,"[%p] <- %s\n", marcel_self(), __FUNCTION__)
#else
#define IN
#define OUT
#endif

#define DO_INV 1
#define DONT_INV 0


be_list hbrc_mw_update_list;



void dsmlib_hbrc_mw_update_rfh(dsm_page_index_t index)
{
  IN;
  dsmlib_rf_ask_for_read_copy(index);
  OUT;
}

void dsmlib_hbrc_mw_update_wfh(dsm_page_index_t index)
{
  IN;
  dsm_lock_page(index);
#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) != dsm_self());
#endif // DSM_PROT_TRACE


  if (dsm_get_access(index) == WRITE_ACCESS){
    dsm_unlock_page(index);
    OUT;
    return;
  }
  if(dsm_get_access(index) == NO_ACCESS){
      dsm_unlock_page(index);
      dsmlib_rf_ask_for_read_copy(index);
      OUT;
      return;
  }
  dsm_alloc_twin(index);
  dsm_set_access(index, WRITE_ACCESS);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"page %d will be added to the page list\n", index);
#endif
  add_to_be_list(&hbrc_mw_update_list, index);
#ifdef DSM_PROT_TRACE
  fprintf_be_list(hbrc_mw_update_list);
#endif
  dsm_unlock_page(index);
  OUT;
  return;
}

void dsmlib_hbrc_mw_update_rs(dsm_page_index_t index, dsm_node_t req_node, int arg)
{
  IN;

  //  dsm_lock_page(index);
  dsm_lock_inv(index);
  
  if (dsm_pending_invalidation(index))
    {
       dsm_unlock_page(index);
       dsm_wait_for_inv(index);
       dsm_lock_page(index);
    }

  //  dsm_unlock_inv(index);

#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) == dsm_self());
#endif
  dsm_add_to_copyset(index, req_node);
  dsm_send_page(req_node, index, READ_ACCESS, arg);

  dsm_unlock_page(index);
  OUT;
}

void dsmlib_hbrc_mw_update_ws(dsm_page_index_t index, dsm_node_t req_node, int arg)
{
  IN;

  //  dsm_lock_page(index);
  dsm_lock_inv(index);
  
  if (dsm_pending_invalidation(index))
    {
      dsm_unlock_page(index);
      dsm_wait_for_inv(index);
      dsm_lock_page(index);
    }

  dsm_unlock_inv(index);

#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) == dsm_self());
#endif //DSM_PROT_TRACE
  dsm_add_to_copyset(index, req_node);
  dsm_send_page(req_node, index, WRITE_ACCESS, arg);

  //  dsm_unlock_page(index);
 OUT;
}


void dsmlib_hbrc_mw_update_is(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner)
{
  pm2_completion_t c;

  IN;
  dsm_lock_page(index);

#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"[%s] Entering for page %ld, requested by node %d, New_owner is %d\n", __FUNCTION__, index, req_node, new_owner);
  assert(dsm_self() != dsm_get_prob_owner(index));
#endif // DSM_PROT_TRACE    

  if(dsm_get_twin(index) != NULL && dsm_get_access(index) > NO_ACCESS)
    {
#ifdef DSM_PROT_TRACE
	assert(dsm_get_access(index) == WRITE_ACCESS);
	tfprintf(stderr,"[%s] Invalidate forces pre-release diff send on page %ld...\n", __FUNCTION__, index);
#endif 
	dsm_set_access(index, READ_ACCESS);
	dsmlib_hrbc_start_send_diffs(index, req_node, DONT_INV, &c);
	dsmlib_hrbc_wait_diffs_done(&c);

//	lock_be_list(hbrc_mw_update_list);
#ifdef DSM_PROT_TRACE
	tfprintf(stderr, "IS: will remove %d from page list, req node =%d\n", index, req_node);
#endif //DSM_PROT_TRACE
	remove_if_noted(&hbrc_mw_update_list, index) ;
//	unlock_be_list(hbrc_mw_update_list);

	dsm_free_twin(index);
	dsm_set_access(index, NO_ACCESS);
      }
  dsm_send_invalidate_ack(req_node, index);
  
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "exiting the invalidate server(%d), req node =%d\n", index, req_node);
#endif //DSM_PROT_TRACE
  dsm_unlock_page(index);
  OUT;
  return;
}


void dsmlib_hbrc_mw_update_rps(void *addr, dsm_access_t access, dsm_node_t reply_node, int arg)
{
  dsm_page_index_t index = dsm_page_index(addr);
  IN;
  dsm_lock_page(index); 

#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) == reply_node);
#endif //DSM_PROT_TRACE
  dsm_set_pending_access(index, NO_ACCESS);
  dsm_set_access(index, READ_ACCESS);
  dsm_signal_page_ready(index);

  dsm_unlock_page(index);
  OUT;
}


void dsmlib_hbrc_acquire()
{
  IN;
  OUT;
  return;
}


void dsmlib_hbrc_release(const token_lock_id_t unused)
{
  dsm_page_index_t index;
  pm2_completion_t c;
  int  invalidated_pages = 0;
  dsm_node_t node;

  IN;

  lock_be_list(hbrc_mw_update_list);

#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "release: will remove first from page list \n");
  fprintf_be_list(hbrc_mw_update_list);
#endif //DSM_PROT_TRACE
  index = remove_first_from_be_list(&hbrc_mw_update_list);
  while(index != (dsm_page_index_t)-1)
    {
      dsm_lock_page(index);
      if(dsm_get_prob_owner(index) != dsm_self())
	{
	  if (dsm_get_twin(index) != NULL)
	    {
#ifdef DSM_PROT_TRACE
	      tfprintf(stderr,"[%s]Processing page: %lu\n",__FUNCTION__, index);
	      assert(dsm_get_access(index) == WRITE_ACCESS);
#endif // DSM_PROT_TRACE
	      dsm_set_access(index, READ_ACCESS);
	      dsmlib_hrbc_start_send_diffs(index, dsm_get_prob_owner(index), DO_INV, &c);
	      dsm_free_twin(index);
	      dsm_set_access(index, NO_ACCESS);
	    }
	  else
	    {
#ifdef DSM_PROT_TRACE
	      tfprintf(stderr,"[%s]Processing page: %lu\n",__FUNCTION__, index);
#endif // DSM_PROT_TRACE
	      dsmlib_hrbc_start_send_empty_diffs(index, dsm_get_prob_owner(index), DO_INV, &c);
	    }
	  dsm_unlock_page(index);
	  dsmlib_hrbc_wait_diffs_done(&c);
	  
	}
      else
	{
	  if (!dsm_pending_invalidation(index))
	    {
	      dsm_set_pending_invalidation(index);
	      node = dsm_get_next_from_copyset(index);
	      
	      while (node != -1)
		{ 
		  if (node != dsm_self()) 
		    {
#ifdef DSM_COMM_TRACE
		      tfprintf(stderr,"IS: Sending invalidate request to node %d (I am %p)\n", node, marcel_self());
#endif
		      dsm_send_invalidate_req(node, index, dsm_self(), dsm_self());
		      invalidated_pages++;
		    }
		  node = dsm_get_next_from_copyset(index);
		}
	      dsm_unlock_page(index);
	      // the copyset is empty now
	      dsm_wait_ack(index, invalidated_pages);
#ifdef DSM_PROT_TRACE
	      tfprintf(stderr,"Got all acks (I am %p)\n", marcel_self());
#endif
	      dsm_clear_pending_invalidation(index);
	      dsm_signal_inv_done(index);
	    }
	  else
	    {
	      dsm_unlock_page(index);
	      
	      dsm_lock_inv(index);
	      dsm_wait_for_inv(index);
	      dsm_unlock_inv(index);
	    }
	}

#ifdef DSM_PROT_TRACE
      tfprintf(stderr, "release: will remove %d from page list\n", index);
      fprintf_be_list(hbrc_mw_update_list);
#endif //DSM_PROT_TRACE
      index = remove_first_from_be_list(&hbrc_mw_update_list);  
    }

  unlock_be_list(hbrc_mw_update_list);
  OUT;
  return;
}


void dsmlib_hbrc_add_page(dsm_page_index_t index)
{
  IN;
  dsm_lock_page(index);
  if (dsm_get_prob_owner(index) == dsm_self()) 
    dsm_set_access(index, WRITE_ACCESS);
  else
    dsm_set_access(index, NO_ACCESS);
  dsm_unlock_page(index);
  OUT;
}

void dsmlib_hbrc_mw_update_prot_init(int prot)
{
  IN;

  hbrc_mw_update_list = init_be_list(10);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"Initial  printing\n");
#ifdef DSM_PROT_TRACE
  fprintf_be_list(hbrc_mw_update_list);
#endif
  tfprintf(stderr,"Done printing\n");
  tfprintf(stderr,"Nb total pages: %lu\n", dsm_get_nb_static_pages() + dsm_get_nb_pseudo_static_pages());
#endif // DSM_PROT_TRACE
  OUT;
/*  for(i = 0; i < dsm_get_nb_static_pages() + dsm_get_nb_pseudo_static_pages(); i++)
    if(dsm_get_page_protocol(i) == protocol_number){
      count++;
      if(dsm_get_prob_owner(i) == dsm_self())
	dsm_set_access(i, READ_ACCESS);
      else
	dsm_set_access(i, NO_ACCESS);
    }
#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"Nb erc pages: %d\n", count);
#endif // DSM_PROT_TRACE*/

  return;


}
/*------------------------------------------------------*/


void dsmlib_hrbc_start_send_diffs(dsm_page_index_t index, dsm_node_t dest_node, int invalidate,  pm2_completion_t *c)
{
  int i = 0;
  int self = dsm_self();
  void *start;
  void *twin;
  int stacking_mode = 0;
  int stack_start = 0;
  int stack_size = 0;
  unsigned long page_size;

  IN;

#ifdef INSTRUMENT
  hbrc_diffs_out_cnt++;
#endif
 
  start = dsm_get_page_addr(index);
  twin = dsm_get_twin(index);
#ifdef DSM_PROT_TRACE
//  assert(dsm_get_twin(index) != NULL);
  tfprintf(stderr,"[%s]Sending diffs for %lu from %d to %d\n",__FUNCTION__, index, dsm_self(), dest_node);
#endif // DSM_PROT_TRACE

  pm2_completion_init(c, NULL, NULL);

  page_size = dsm_get_page_size(index);
  pm2_rawrpc_begin(dest_node, DSM_LRPC_HBRC_DIFFS, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&invalidate, sizeof(int));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&self, sizeof(int));

  for(i = 0; i < page_size / 4;i++){
    if(*(((int *)start) + i) != *(((int *)twin) + i)){
      if(stacking_mode){
	stack_size+=sizeof(int);
      }
      else{
	stacking_mode = 1;
        stack_start = i;
	stack_size = 4;
      }
    }
    else{
      if(stacking_mode){
        stacking_mode = 0;
#ifdef DSM_PROT_TRACE
	tfprintf(stderr, "[%s]Packing offset %d, size(in bytes)=%d\n", __FUNCTION__, sizeof(int) * stack_start, stack_size);
#endif //DSM_PROT_TRACE
        pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_start, sizeof(int));
        pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_size, sizeof(int));
        pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)(((int *)start) + stack_start), stack_size);
//	tfprintf(stderr, "Packed\n");
      }
    }
  }
  if(stacking_mode){
#ifdef DSM_PROT_TRACE
    tfprintf(stderr, "[%s]Packing offset %d, size(in words)=%d\n", __FUNCTION__, sizeof(int) * stack_start, stack_size);
#endif //DSM_PROT_TRACE
    pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_start, sizeof(int));
    pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_size, sizeof(int));
    pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)(((int *)start) + stack_start), stack_size);
#ifdef DSM_PROT_TRACE
    tfprintf(stderr, "[%s]Packed\n",__FUNCTION__);
#endif //DSM_PROT_TRACE
  }

  i = -1;
  // send the -1 value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&i, sizeof(int));
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,c);
  pm2_rawrpc_end();

  OUT;
}

void dsmlib_hrbc_wait_diffs_done(pm2_completion_t *c)
{
  IN;

  pm2_completion_wait(c);

  OUT;
}

void DSM_HRBC_DIFFS_threaded_func(void)
{
  dsm_page_index_t index;
  int i,  invalidate, invalidated_pages = 0;
  int size;
  pm2_completion_t c;
  void *start;
  dsm_node_t node, req_node;

  IN;
#ifdef INSTRUMENT
  hbrc_diffs_in_cnt++;
#endif 
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index,
    sizeof(dsm_page_index_t));
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&invalidate, sizeof(int));

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&req_node, sizeof(int));
#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"I will process diffs from %d for page %ld , invalidate = %d\n", req_node, index, invalidate);
#endif

  dsm_lock_page(index);

  start = dsm_get_page_addr(index);
  
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&i, sizeof(void *));
  while (i != -1)
  {
    pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
    pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)((int *)start + i), size);
#ifdef DSM_PROT_TRACE
    tfprintf(stderr,"[%s] Received %d bytes diff at %d offset\n", __FUNCTION__, size, i * sizeof(int));
#endif //DSM_PROT_TRACE

    pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&i, sizeof(void *));
  }

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);

  pm2_rawrpc_waitdata();
#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"finished processing diffs (%ld) from node %d\n", index, req_node);
#endif

  if (invalidate == DO_INV)
    {
    if (!dsm_pending_invalidation(index))
      {
	dsm_set_pending_invalidation(index);
	node = dsm_get_next_from_copyset(index);
	
      while (node != -1)
	{ 
	  if (node != req_node) 
	    {
#ifdef DSM_COMM_TRACE
	      tfprintf(stderr,"IS: Sending invalidate request to node %d (I am %p)\n", node, marcel_self());
#endif
	      dsm_send_invalidate_req(node, index, dsm_self(), dsm_self());
	      invalidated_pages++;
	    }
	  node = dsm_get_next_from_copyset(index);
	}
      dsm_unlock_page(index);
      // the copyset is empty now
      dsm_wait_ack(index, invalidated_pages);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"Got all acks (I am %p)\n", marcel_self());
#endif
      dsm_clear_pending_invalidation(index);
      dsm_signal_inv_done(index);
      }
    else
      {
	dsm_unlock_page(index);
	
	dsm_lock_inv(index);
	dsm_wait_for_inv(index);
	dsm_unlock_inv(index);
      }
    }
  else
    dsm_unlock_page(index);

  pm2_completion_signal(&c);

  OUT;
  return;
}

void DSM_LRPC_HBRC_DIFFS_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_HRBC_DIFFS_threaded_func, NULL);
}



void dsmlib_hrbc_invalidate_copyset(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner)
{
  int invalidated_pages = 0;
  dsm_node_t node;

  IN;

  node = dsm_get_next_from_copyset(index);

  while (node != -1)
    { 
      if (node != req_node) 
	{
#ifdef DSM_COMM_TRACE
	  tfprintf(stderr,"IS: Sending invalidate request to node %d (I am %p)\n", node, marcel_self());
#endif
	  dsm_send_invalidate_req(node, index, dsm_self(), new_owner);
	  invalidated_pages++;
	}
//      dsm_lock_page(index);
      node = dsm_get_next_from_copyset(index);
//      dsm_unlock_page(index);
    }
  // the copyset is empty now
       dsm_wait_ack(index, invalidated_pages);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr,"Got all acks (I am %p)\n", marcel_self());
#endif
  OUT;
}



void dsmlib_hrbc_start_send_empty_diffs(dsm_page_index_t index, dsm_node_t dest_node, int invalidate,  pm2_completion_t *c)
{
  int i = 0;
  int self = dsm_self();

  IN;
 
#ifdef DSM_PROT_TRACE
//  assert(dsm_get_twin(index) != NULL);
  tfprintf(stderr,"[%s]Sending empty diffs for %lu from %d to %d\n",__FUNCTION__, index, dsm_self(), dest_node);
#endif // DSM_PROT_TRACE

  pm2_completion_init(c, NULL, NULL);

  pm2_rawrpc_begin(dest_node, DSM_LRPC_HBRC_DIFFS, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(dsm_page_index_t));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&invalidate, sizeof(int));
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&self, sizeof(int));

  i = -1;
  // send the -1 value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&i, sizeof(int));
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, c);
  pm2_rawrpc_end();

  OUT;
}











