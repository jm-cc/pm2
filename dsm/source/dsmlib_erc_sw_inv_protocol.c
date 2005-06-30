
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

//#include <assert.h>
#include "dsmlib_erc_sw_inv_protocol.h"
#include "dsmlib_belist.h"
#include "dsm_protocol_policy.h"
#include "dsm_protocol_lib.h"
#include "dsm_page_manager.h"
/* the following is useful for "token_lock_id_t" */
#include "token_lock.h"

//#define DSM_PROT_TRACE
//#define DSM_QUEUE_TRACE


//#ifdef DSM_PROT_TRACE
#ifdef DSM_PROT_TRACE
#define ENTER() fprintf(stderr, "[%p] -> %s\n", marcel_self(), __FUNCTION__)
#define EXIT() fprintf(stderr, "[%p] <- %s\n", marcel_self(), __FUNCTION__)
#else
#define ENTER()
#define EXIT()
#endif

be_list erc_sw_inv_list;

void dsmlib_erc_sw_inv_init(dsm_proto_t protocol_number)
{

  ENTER();

  erc_sw_inv_list = init_be_list(10);
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"ERC: Initial  printing\n");
  fprintf_be_list(erc_sw_inv_list);
  fprintf(stderr,"Done printing\n");
  fprintf(stderr,"Total ststic pages: %lu\n", dsm_get_nb_static_pages() + dsm_get_nb_pseudo_static_pages());
#endif
 
  EXIT();

  return;
}

void dsmlib_erc_sw_inv_rfh(dsm_page_index_t index)
{
  return dsmlib_rf_ask_for_read_copy(index);
}


void dsmlib_erc_sw_inv_wfh(dsm_page_index_t index)
{  
  ENTER();

  dsm_lock_page(index);
  if (dsm_get_access(index) == WRITE_ACCESS)
    {
      dsm_unlock_page(index);
      EXIT();
      return;
    }

  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
#ifdef DSM_PROT_TRACE
      fprintf(stderr, "Adding to belist: %lu\n", index);
#endif
      add_to_be_list(&erc_sw_inv_list, index);
      dsm_set_access(index, WRITE_ACCESS);
    }
  else // I am not owner
    {
      if (dsm_get_pending_access(index) < WRITE_ACCESS) // there is no pending write access
	{
	  dsm_set_pending_access(index, WRITE_ACCESS);
     	  dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), WRITE_ACCESS, 0);
	}
  // else, there is already a pending access and the page will soon be there
      dsm_wait_for_page(index);
    }

  dsm_unlock_page(index);
  EXIT();
}

static void _internal_dsmlib_erc_sw_inv_rs(dsm_page_index_t index, dsm_node_t req_node, int arg)
{
  dsm_access_t access;
  
  ENTER();
  
  if (!dsm_next_owner_is_set(index))
    {
      if ((access = dsm_get_access(index)) >= READ_ACCESS) // there is a local copy of the page
       {
	 dsm_add_to_copyset(index, req_node);
	 //      dsm_set_access(index,WRITE_ACCESS);
	 dsm_set_access(index, READ_ACCESS);
	 dsm_send_page(req_node, index, READ_ACCESS, arg);
	 //      if (access < WRITE_ACCESS) 
	 //	dsm_set_access(index, READ_ACCESS);
       }
      else // no access; maybe a pending access ?
	if (dsm_get_pending_access(index) != NO_ACCESS)
	  {
	    dsm_store_pending_request(index, req_node);
	  }
	else // no access nor pending access? then forward req to prob owner!
	  {
	    dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS, arg);
	  }
    }
  else
    {
      if (req_node == dsm_get_next_owner(index))
	{
	  dsm_add_to_copyset(index, req_node);
	  //      dsm_set_access(index,WRITE_ACCESS);
	  dsm_set_access(index, READ_ACCESS);
	  dsm_send_page(req_node, index, READ_ACCESS, arg);
	  //      if (access < WRITE_ACCESS) 
	  //	dsm_set_access(index, READ_ACCESS);
	}
      else
	dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS, arg);
    }
  
  EXIT();
  
}

void dsmlib_erc_sw_inv_rs(dsm_page_index_t index, dsm_node_t req_node, int arg)
{
  ENTER();
  
  dsm_lock_page(index);
  _internal_dsmlib_erc_sw_inv_rs(index,req_node, arg);
  dsm_unlock_page(index);
  
  EXIT();
}

static void _internal_dsmlib_erc_sw_inv_ws(dsm_page_index_t index, dsm_node_t req_node, int arg)
{
  
  ENTER();
  
  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
      if (remove_if_noted(&erc_sw_inv_list, index))
	{
	  // In data-races free programs, this should not happen...
	  //assert(dsm_get_access(index) == WRITE_ACCESS);
	  dsmlib_is_invalidate_internal(index, dsm_self(), req_node);
	}
      dsm_set_access(index, READ_ACCESS);
      dsm_send_page(req_node, index, WRITE_ACCESS, arg);
      dsm_set_access(index, NO_ACCESS);
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) == WRITE_ACCESS && !dsm_next_owner_is_set(index))
      {
	//assert(req_node != dsm_self());
	dsm_set_next_owner(index, req_node);
      }
    else // no access nor pending access? then forward req to prob owner!
      {
	dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, WRITE_ACCESS, arg);
      }
  dsm_set_prob_owner(index,req_node); // req_node will soon be owner
  
  EXIT();
}


void dsmlib_erc_sw_inv_ws(dsm_page_index_t index, dsm_node_t req_node, int arg)
{
  ENTER();

  dsm_lock_page(index);
  _internal_dsmlib_erc_sw_inv_ws(index,req_node, arg);
  dsm_unlock_page(index);

  EXIT();

}

void dsmlib_erc_sw_inv_is(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner){
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"[%s] Entering...\n", __FUNCTION__);
#endif
  return dsmlib_is_invalidate(index, req_node, new_owner);
}

void dsmlib_erc_sw_inv_rps(void *addr, dsm_access_t access, dsm_node_t reply_node, int arg)
{
  dsm_page_index_t index = dsm_page_index(addr);
  dsm_node_t node;

  ENTER();

  dsm_lock_page(index); 

  if (access == READ_ACCESS)
    {
      if (dsm_get_pending_access(index) == READ_ACCESS)
	{
	  dsm_set_prob_owner(index, reply_node);
	  dsm_set_pending_access(index, NO_ACCESS);
	}
      dsm_set_access(index, READ_ACCESS);
    }
  else // access = WRITE_ACCESS
    {
      if (dsm_get_pending_access(index) == WRITE_ACCESS)
	{
	  dsm_set_prob_owner(index, dsm_self());
	  //dsm_invalidate_copyset(index, dsm_self());
	  dsm_set_access(index, READ_ACCESS);
	  dsm_set_pending_access(index, NO_ACCESS);
	}
      else{
	fprintf(stderr, "Something REALLY wrong happened\n");
	dsm_set_prob_owner(index, dsm_self());
	dsmlib_is_invalidate_internal(index, dsm_self(), dsm_self());
	dsm_set_access(index, READ_ACCESS);
      }
    }
  dsm_signal_page_ready(index);
  // process pending requests
  // should wait for a while ?
  while ((node = dsm_get_next_pending_request(index)) != NOBODY)
    {
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "Processing R-req(%d) from node %d (I am %p)\n", index, node, marcel_self());
#endif
      _internal_dsmlib_erc_sw_inv_rs(index, node, arg);
    }
  if(access == WRITE_ACCESS && dsm_next_owner_is_set(index))
    {
      marcel_delay(60);
      node = dsm_get_next_owner(index);
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "Processing W-req (%d)from next owner: node %d (I am %p) \n", index, node, marcel_self());
#endif
      _internal_dsmlib_erc_sw_inv_ws(index, node, arg);
      dsm_clear_next_owner(index); // these last 2 calls should be atomic...
    }
  dsm_unlock_page(index);

  EXIT();
}

void dsmlib_erc_acquire()
{
  return;
}

void dsmlib_erc_release(const token_lock_id_t unused)
{
  dsm_page_index_t index;

  ENTER();

#ifdef DSM_PROT_TRACE
  fprintf(stderr,"Start of release\n");
  fprintf_be_list(erc_sw_inv_list);
  fprintf(stderr,"Done printing\n");
#endif
  index = remove_first_from_be_list(&erc_sw_inv_list);
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"First One removed: %lu\n", index);
#endif
  while(index != (dsm_page_index_t)-1)
    {
//      fprintf(stderr,"Trying to lock page: %lu\n", index);
      dsm_lock_page(index);
//      fprintf(stderr,"Invalidating copyset: %lu\n", index);
      dsmlib_is_invalidate_internal(index, dsm_self(), dsm_self());
//      fprintf(stderr,"Changing access: %lu\n", index);
      dsm_set_access(index, READ_ACCESS);
      dsm_unlock_page(index);
      index = remove_first_from_be_list(&erc_sw_inv_list);
//      fprintf(stderr,"Removed: %ld\n", index);
    }
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"End of release\n");
#endif
  EXIT();
  return;
}



void dsmlib_erc_add_page(dsm_page_index_t index)
{
  ENTER();

  if (dsm_get_prob_owner(index) == dsm_self()) 
    dsm_set_access(index, READ_ACCESS);
  else
    dsm_set_access(index, NO_ACCESS);

  EXIT();
}






