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

/* Options: DSM_COMM_TRACE, DSM_PROT_TRACE, DSM_QUEUE_TRACE */

#include <stdio.h>

#include "marcel.h" // tfprintf 
#include "pm2.h"
#include "dsm_const.h" 
#include "dsm_page_manager.h"
#include "dsm_comm.h"
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h"
#include "dsm_page_size.h"
#include "assert.h"

//#define DSM_PROT_TRACE
//#define DSM_COMM_TRACE
//#define DSM_QUEUE_TRACE


#ifdef DSM_PROT_TRACE
#define ENTER() fprintf(stderr, "[%p] -> %s\n", marcel_self(), __FUNCTION__)
#define EXIT() fprintf(stderr, "[%p] <- %s\n", marcel_self(), __FUNCTION__)
#else
#define ENTER()
#define EXIT()
#endif

void dsmlib_rf_ask_for_read_copy(dsm_page_index_t index)
{
   dsm_lock_page(index);

  ENTER();

  if (dsm_get_access(index) >= READ_ACCESS)
    {
      dsm_unlock_page(index);
      return;
    }

  if (dsm_get_pending_access(index) == NO_ACCESS) // there is no pending access
    {
      dsm_set_pending_access(index, READ_ACCESS);
#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "RF: Send page req (%d) to %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
      dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), READ_ACCESS, 0);
    } // else, there is already a pending access and the page will soon be there
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "RF: I'll now wait for page %d, owner is %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
  dsm_wait_for_page(index);
#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "RF: I got page %d (I am %p) \n", index, marcel_self());
#endif
  dsm_unlock_page(index);
  EXIT();
}

void dsmlib_migrate_thread(dsm_page_index_t index)
{

  ENTER();

  pm2_enable_migration();
  pm2_freeze();
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "I will migrate to %d (I am %p)\n", dsm_get_prob_owner(index), marcel_self());
#endif
  pm2_migrate(marcel_self(), dsm_get_prob_owner(index));

  EXIT();
}

void dsmlib_wf_ask_for_write_access(dsm_page_index_t index)
{
   dsm_lock_page(index);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "call to dsm_ask_for_write(%d)\n", index);
#endif

  if (dsm_get_access(index) == WRITE_ACCESS)
  {
      dsm_unlock_page(index);
      return;
  }
  ENTER();

  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "Write handler for page %d started : a = %d(I am %p)\n", index,((atomic_t *)dsm_get_page_addr(index))->counter, marcel_self());
#endif
      dsmlib_is_invalidate_internal(index, dsm_self(), dsm_self());
      dsm_set_access(index, WRITE_ACCESS);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "Write handler for page %d ended : a = %d(I am %p)\n", index,((atomic_t *)dsm_get_page_addr(index))->counter, marcel_self());
#endif
    }
  else // I am not owner
    {
      if (dsm_get_pending_access(index) < WRITE_ACCESS) // there is no pending write access
	{
	  dsm_set_pending_access(index, WRITE_ACCESS);
#ifdef DSM_COMM_TRACE
	  tfprintf(stderr, "WF: Send page req (%d) to %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
     	  dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), WRITE_ACCESS, 0);
	}
  // else, there is already a pending access and the page will soon be there
#ifdef DSM_PROT_TRACE
      tfprintf(stderr, "WF: I'll now wait for page %d, owner is %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
      dsm_wait_for_page(index);
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "WF: I got page %d (I am %p)\n", index, marcel_self());
#endif
    }
  dsm_unlock_page(index);
  EXIT();
}


static void _internal_dsmlib_rs_send_read_copy(dsm_page_index_t index, dsm_node_t req_node, int tag)
{
#ifdef DSM_PROT_TRACE
  fprintf(stderr, "entering the internal read server(%ld), access = %d, req_node = %d\n", index, dsm_get_access(index), req_node);
#endif

  if (!dsm_next_owner_is_set(index))
    {
      if (dsm_get_access(index) >= READ_ACCESS) // there is a local copy of the page
	{
	  dsm_add_to_copyset(index, req_node);
//	        if (dsm_get_access(index) != WRITE_ACCESS)
//	  	dsm_set_access(index, WRITE_ACCESS);
	  dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
	  dsm_send_page(req_node, index, READ_ACCESS, tag);
//           dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
         }
      else // no access; maybe a pending access ?
        if (dsm_get_pending_access(index) != NO_ACCESS)
	  {
#ifdef DSM_QUEUE_TRACE
	    tfprintf(stderr, "RS: storing req(%d) from node %d, pending access = %d\n", index, req_node, dsm_get_pending_access(index));
	    assert(req_node != dsm_self());
#endif
	    dsm_store_pending_request(index, req_node);
	  }
	else // no access nor pending access? then forward req to prob owner!
	{
#ifdef DSM_COMM_TRACE
	  tfprintf(stderr, "RS: forwarding req(%ld) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
	  dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS, tag);
	}
    }
  else
if (req_node == dsm_get_next_owner(index))
  {
	  dsm_add_to_copyset(index, req_node);
//	        if (dsm_get_access(index) != WRITE_ACCESS)
//	  	dsm_set_access(index, WRITE_ACCESS);
	  dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
	  dsm_send_page(req_node, index, READ_ACCESS, tag);
//           dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
  }
else
    {
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "RS: forwarding req(%ld) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
      dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS, tag);
    }
  
  EXIT();
}


void dsmlib_rs_send_read_copy(dsm_page_index_t index, dsm_node_t req_node, int tag)
{
#ifdef DSM_PROT_TRACE
  fprintf(stderr, "entering the read server(%ld), access = %d, req_node = %d\n", index, dsm_get_access(index), req_node);
#endif
  ENTER();

  dsm_lock_page(index);
  _internal_dsmlib_rs_send_read_copy(index, req_node, tag);
  dsm_unlock_page(index);

  EXIT();
}


static void _internal_dsmlib_ws_send_page_for_write_access(dsm_page_index_t index, dsm_node_t req_node, int tag)
{

#ifdef DSM_PROT_TRACE
  fprintf(stderr, "entering the write server(%ld), req_node = %d\n", index, req_node);
#endif

     if (dsm_next_owner_is_set(index))
       {
	 if(dsm_get_next_owner(index) == req_node)
	   {
	     if (dsm_get_access(index) == READ_ACCESS)//20/10/99
	         dsmlib_is_invalidate_internal(index, dsm_self(), req_node);
//            if (dsm_get_access(index) != WRITE_ACCESS)
//      	dsm_set_access(index, WRITE_ACCESS);
      dsm_set_access(index, READ_ACCESS); // the local copy will become read-only
      dsm_send_page(req_node, index, WRITE_ACCESS, tag);
      dsm_set_access(index, NO_ACCESS); // the local copy will be invalidated
      dsm_set_prob_owner(index, req_node); // req_node will soon be owner
	   }
	 else
	   {
#ifdef DSM_COMM_TRACE
	     tfprintf(stderr, "WS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
	     dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, WRITE_ACCESS, tag);
	     dsm_set_prob_owner(index, req_node); // req_node will soon be owner		    
	   }
       }
     else
       if (dsm_get_prob_owner(index) == dsm_self())
	 {
	     if (dsm_get_access(index) == READ_ACCESS)//20/10/99
	         dsmlib_is_invalidate_internal(index, dsm_self(), req_node);
      //      if (dsm_get_access(index) != WRITE_ACCESS)
      //	dsm_set_access(index, WRITE_ACCESS);
      dsm_set_access(index, READ_ACCESS); // the local copy will become read-only
      dsm_send_page(req_node, index, WRITE_ACCESS, tag);
      dsm_set_access(index, NO_ACCESS); // the local copy will be invalidated
      dsm_set_prob_owner(index, req_node); // req_node will soon be owner
	 }
       else
       if (dsm_get_pending_access(index) == WRITE_ACCESS)
	 {
	    assert(req_node != dsm_self());
#ifdef DSM_QUEUE_TRACE
	    tfprintf(stderr, "WS: setting next owner (%d) as node %d\n", index, req_node);
#endif
	    dsm_set_next_owner(index, req_node);
	    dsm_set_prob_owner(index, req_node); // req_node will soon be owner
	  }
       else
	 {
#ifdef DSM_COMM_TRACE
	   tfprintf(stderr, "WS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
	   dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, WRITE_ACCESS, tag);
	   dsm_set_prob_owner(index, req_node); // req_node will soon be owner
	 }

  EXIT();
}


void dsmlib_ws_send_page_for_write_access(dsm_page_index_t index, dsm_node_t req_node, int tag)
{
#ifdef DSM_PROT_TRACE
  fprintf(stderr, "entering the write server(%ld), req_node = %d\n", index, req_node);
#endif
  ENTER();

  dsm_lock_page(index);
  _internal_dsmlib_ws_send_page_for_write_access(index, req_node, tag);
  dsm_unlock_page(index);

  EXIT();
}

void dsmlib_is_invalidate(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner)
{
  dsm_lock_page(index);
  dsmlib_is_invalidate_internal(index, req_node, new_owner);
  dsm_unlock_page(index);
}

void dsmlib_is_invalidate_internal(dsm_page_index_t index, dsm_node_t req_node, dsm_node_t new_owner)
{
  int i, copyset_size = dsm_get_copyset_size(index);

#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "entering the invalidate server(%d), req_node = %d, new_owner =%d (I am %p)\n", index, req_node, new_owner,marcel_self());
#endif

  if (copyset_size > 0)
    {
      dsm_node_t node;
      boolean new_owner_is_in_copyset = FALSE;

      for (i = 0; i < copyset_size; i++) 
	{
	  node = dsm_get_next_from_copyset(index);
	  if (node != new_owner && node!= dsm_self()) 
	    {
#ifdef DSM_COMM_TRACE
	      tfprintf(stderr,"IS: Sending invalidate request to node %d (I am %p)\n", node, marcel_self());
#endif
	      dsm_send_invalidate_req(node, index, dsm_self(), new_owner);
	    }
	  else
	    new_owner_is_in_copyset = TRUE;
	}
	  // the copyset is empty now
      dsm_wait_ack(index, new_owner_is_in_copyset?copyset_size - 1: copyset_size);
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"Got all acks (I am %p)\n", marcel_self());
#endif
    }

  if(dsm_self() != dsm_get_prob_owner(index) && dsm_self() != new_owner)
    {
      dsm_set_access(index, NO_ACCESS);
#ifdef DSM_COMM_TRACE
      tfprintf(stderr,"IS: Sending invalidate ack to node %d (I am %p)\n", req_node, marcel_self());
#endif
      dsm_send_invalidate_ack(req_node, index);
    }
  if(dsm_self() != new_owner)
      dsm_set_prob_owner(index, new_owner); // ?? 15/03
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "exiting the invalidate server(%d), req node =%d\n", index, req_node);
#endif
}


void dsmlib_rp_validate_page(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag)
{
     dsm_page_index_t index = dsm_page_index(addr);
     dsm_node_t node;

     dsm_lock_page(index); 

#ifdef DSM_COMM_TRACE
     tfprintf(stderr, "Received page %ld <- %d for %s (I am %p)\n", index, reply_node, (access == 1)?"read":"write", marcel_self());
#endif
     if (access == READ_ACCESS)
       {	   
	 dsm_set_access(index, READ_ACCESS);
	 if (dsm_get_pending_access(index) == READ_ACCESS)
	   {
	     dsm_set_prob_owner(index, reply_node);
	     dsm_set_pending_access(index, NO_ACCESS);
	   }
       }
     else // access = WRITE_ACCESS
       {
	 if (dsm_get_pending_access(index) == WRITE_ACCESS)
	   {
#ifdef DSM_PROT_TRACE
	     tfprintf(stderr,"invalidating copyset\n");
#endif
	     dsmlib_is_invalidate_internal(index, dsm_self(), dsm_self());
#ifdef DSM_PROT_TRACE
	     tfprintf(stderr,"invalidated copyset\n");
#endif
    	     dsm_set_access(index, WRITE_ACCESS);
	     if ( ! dsm_next_owner_is_set(index))
	       dsm_set_prob_owner(index, dsm_self());
	     dsm_set_pending_access(index, NO_ACCESS);
	   }
	 else
	   {
	     RAISE (PROGRAM_ERROR);
	   }
       }
     dsm_signal_page_ready(index);
#ifdef DSM_PROT_TRACE
     tfprintf(stderr, "I signalled page ready, first value got = %d (I am %p)\n", ((int *)dsm_get_page_addr(index))[0]/*((atomic_t *)dsm_get_page_addr(index))->counter*/, marcel_self());
#endif
     // process pending requests
     // should wait for a while ?
     
//     dsm_unlock_page(index);
          marcel_delay(40);
     //     marcel_yield();
     while ((node = dsm_get_next_pending_request(index)) != NOBODY)
      {
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "Processing R-req(%d) from node %d (I am %p)\n", index, node, marcel_self());
#endif
	_internal_dsmlib_rs_send_read_copy(index, node, 0);
      }
     if(access == WRITE_ACCESS && dsm_next_owner_is_set(index))
      {
	node = dsm_get_next_owner(index);
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "Processing W-req (%d)from next owner: node %d (I am %p) \n", index, node, marcel_self());
#endif
	_internal_dsmlib_ws_send_page_for_write_access(index, node, 0);
	dsm_clear_next_owner(index); // these last 2 calls should be atomic...
      }

     dsm_unlock_page(index);
     
     EXIT();
}


