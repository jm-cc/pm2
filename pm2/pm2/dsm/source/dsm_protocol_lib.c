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
#define HYP_SEND_PAGE_CHEAPER

#include <stdio.h>

#include "marcel.h" // tfprintf 
#include "pm2.h"
#include "dsm_const.h" // dsm_node_t
#include "dsm_page_manager.h"
#include "dsm_comm.h"
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h"
#include "dsm_page_size.h"
#include "assert.h"

//#define DEBUG1

void dsmlib_rf_ask_for_read_copy(unsigned long index)
{

#ifdef DEBUG1
  tfprintf(stderr, "call to dsm_ask_for_read_copy(%d)\n", index);
#endif

  if (dsm_get_access(index) == READ_ACCESS)
    {
      return;
    }

  if (dsm_get_pending_access(index) == NO_ACCESS) // there is no pending access
    {
      dsm_set_pending_access(index, READ_ACCESS);
#ifdef DEBUG2
  tfprintf(stderr, "RF: Send page req (%d) to %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
      dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), READ_ACCESS);
    } // else, there is already a pending access and the page will soon be there
#ifdef DEBUG1
  tfprintf(stderr, "RF: I'll now wait for page %d, owner is %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
  dsm_wait_for_page(index);
#ifdef DEBUG1
  tfprintf(stderr, "RF: I got page %d (I am %p) \n", index, marcel_self());
#endif
}

void dsmlib_migrate_thread(unsigned long index)
{
  dsm_unlock_page(index); 
  pm2_enable_migration();
  pm2_freeze();
  pm2_migrate_self(dsm_get_prob_owner(index));
}

void dsmlib_wf_ask_for_write_access(unsigned long index)
{

#ifdef DEBUG1
  tfprintf(stderr, "call to dsm_ask_for_write(%d)\n", index);
#endif

  if (dsm_get_access(index) == WRITE_ACCESS)
      return;

  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
#ifdef DEBUG3
  tfprintf(stderr, "Write handler for page %d started : a = %d(I am %p)\n", index,((atomic_t *)dsm_get_page_addr(index))->counter, marcel_self());
#endif
      dsm_invalidate_copyset(index, dsm_self());
      dsm_set_access(index, WRITE_ACCESS);
#ifdef DEBUG3
  tfprintf(stderr, "Write handler for page %d ended : a = %d(I am %p)\n", index,((atomic_t *)dsm_get_page_addr(index))->counter, marcel_self());
#endif
    }
  else // I am not owner
    {
      if (dsm_get_pending_access(index) < WRITE_ACCESS) // there is no pending write access
	{
	  dsm_set_pending_access(index, WRITE_ACCESS);
#ifdef DEBUG2
	  tfprintf(stderr, "WF: Send page req (%d) to %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
     	  dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), WRITE_ACCESS);
	}
  // else, there is already a pending access and the page will soon be there
#ifdef DEBUG1
      tfprintf(stderr, "WF: I'll now wait for page %d, owner is %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
      dsm_wait_for_page(index);
#ifdef DEBUG1
      tfprintf(stderr, "WF: I got page %d (I am %p)\n", index, marcel_self());
#endif
    }
}


void dsmlib_rs_send_read_copy(unsigned long index, dsm_node_t req_node)
{
  dsm_lock_page(index);

#ifdef DEBUG1
  tfprintf(stderr, "entering the read server(%d), access = %d\n", index, dsm_get_access(index));
#endif

  if (dsm_get_access(index) >= READ_ACCESS) // there is a local copy of the page
    {
      dsm_add_to_copyset(index, req_node);
      //      lock_task();
      //      if (dsm_get_access(index) != WRITE_ACCESS)
      //	dsm_set_access(index, WRITE_ACCESS);
      dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
      dsm_send_page(req_node, index, READ_ACCESS);
      //      dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
      //      unlock_task();
#ifdef DEBUG3
      tfprintf(stderr,"RS after send page: a = %d\n",((atomic_t *)dsm_get_page_addr(index))->counter);
#endif
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) != NO_ACCESS && !dsm_next_owner_is_set(index))
      {
#ifdef DEBUG_QUEUE
	tfprintf(stderr, "RS: storing req(%d) from node %d, pending access = %d\n", index, req_node, dsm_get_pending_access(index));
	//assert(req_node != dsm_self());
#endif
	dsm_store_pending_request(index, req_node);
      }
    else // no access nor pending access? then forward req to prob owner!
      {
#ifdef DEBUG3
      tfprintf(stderr, "RS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
      dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS);
    }
  dsm_unlock_page(index);
}


void dsmlib_ws_send_page_for_write_access(unsigned long index, dsm_node_t req_node)
{
  dsm_lock_page(index);
#ifdef DEBUG3
  tfprintf(stderr, "entering the write server(%d), req_node = %d\n", index, req_node);
#endif
  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
      if (dsm_get_access(index) == READ_ACCESS)//20/10/99
	dsm_invalidate_copyset(index, req_node);
#ifdef DEBUG3
      tfprintf(stderr,"WS before send page: a = %d\n",((atomic_t *)dsm_get_page_addr(index))->counter);
#endif
      //      lock_task();
      //      if (dsm_get_access(index) != WRITE_ACCESS)
      //	dsm_set_access(index, WRITE_ACCESS);
      dsm_set_access(index, READ_ACCESS); // the local copy will become read-only
      dsm_send_page(req_node, index, WRITE_ACCESS);
      dsm_set_access(index, NO_ACCESS); // the local copy will be invalidated
      //      unlock_task();
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) == WRITE_ACCESS && !dsm_next_owner_is_set(index))
      {
	assert(req_node != dsm_self());
#ifdef DEBUG_QUEUE
	tfprintf(stderr, "WS: setting next owner (%d) as node %d\n", index, req_node);
#endif
	dsm_set_next_owner(index, req_node);
      }
    else // no access nor pending access? then forward req to prob owner!
      {
#ifdef DEBUG3
	tfprintf(stderr, "WS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
	dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, WRITE_ACCESS);
      }
  dsm_set_prob_owner(index,req_node); // req_node will soon be owner
  dsm_unlock_page(index);
}


void dsmlib_is_invalidate(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
  int i, copyset_size = dsm_get_copyset_size(index);

#ifdef DEBUG3
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
#ifdef DEBUG2
	      tfprintf(stderr,"IS: Sending invalidate request to node %d (I am %p)\n", node, marcel_self());
#endif
	      dsm_send_invalidate_req(node, index, dsm_self(), new_owner);
	    }
	  else
	    new_owner_is_in_copyset = TRUE;
	}
	  // the copyset is empty now
      dsm_wait_ack(index, new_owner_is_in_copyset?copyset_size - 1: copyset_size);
#ifdef DEBUG1
  fprintf(stderr,"Got all acks (I am %p)\n", marcel_self());
#endif
    }

  if(dsm_self() != dsm_get_prob_owner(index) && dsm_self() != new_owner)
    {
      dsm_set_access(index, NO_ACCESS);
#ifdef DEBUG3
      tfprintf(stderr,"IS: Sending invalidate ack to node %d (I am %p)\n", req_node, marcel_self());
#endif
      dsm_send_invalidate_ack(req_node, index);
    }

  dsm_set_prob_owner(index, new_owner);
#ifdef DEBUG3
  tfprintf(stderr, "exiting the invalidate server(%d), req node =%d\n", index, req_node);
#endif
}

void dsmlib_rp_validate_page(void *addr, dsm_access_t access, dsm_node_t reply_node)
{
     unsigned long index = dsm_page_index(addr);
     dsm_node_t node;

     dsm_lock_page(index); 

#ifdef DEBUG3
     tfprintf(stderr, "Received page %ld <- %d for %s (I am %p)\n", index, reply_node, (access == 1)?"read":"write", marcel_self());
#endif
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
#ifdef DEBUG3
	     tfprintf(stderr,"invalidating copyset\n");
#endif
	     dsm_invalidate_copyset(index, dsm_self());
#ifdef DEBUG3
	     tfprintf(stderr,"invalidated copyset\n");
#endif
	     //lock_task();
	     //dsm_set_prob_owner(index, dsm_self());
    	     dsm_set_access(index, WRITE_ACCESS);
	     dsm_set_pending_access(index, NO_ACCESS);
	     //unlock_task();
	   }
	 else
	   dsm_set_access(index, WRITE_ACCESS);
       }
     dsm_signal_page_ready(dsm_page_index(addr));
#ifdef DEBUG3
     tfprintf(stderr, "I signalled page ready, a = %d (I am %p)\n", ((atomic_t *)dsm_get_page_addr(index))->counter, marcel_self());
#endif
     // process pending requests
     // should wait for a while ?
     dsm_unlock_page(index);
     while ((node = dsm_get_next_pending_request(index)) != NO_NODE)
      {
#ifdef DEBUG_QUEUE
	tfprintf(stderr, "Processing R-req from node %d (I am %p)\n", node, marcel_self());
#endif
	(*dsm_get_read_server(index))(index, node);
      }
     if(access == WRITE_ACCESS && dsm_next_owner_is_set(index))
      {
	marcel_delay(10);
	node = dsm_get_next_owner(index);
#ifdef DEBUG_QUEUE
	tfprintf(stderr, "Processing W-req from next owner: node %d (I am %p) \n", node, marcel_self());
#endif
	(*dsm_get_write_server(index))(index, node);
	dsm_clear_next_owner(index); // these last 2 calls should be atomic...
      }
}
/*************  Hyperion protocols *************************/
/* obsolete now: */

void dsmlib_ws_hyp_send_page_for_write_access(unsigned long index, dsm_node_t req_node)
{
  dsm_lock_page(index);
#ifdef DEBUG_HYP
  tfprintf(stderr, "entering the write server(%d), req_node = %d\n", index, req_node);
#endif
  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
#ifdef DEBUG_HYP
      tfprintf(stderr,"WS before send page...\n");
#endif
      dsm_send_page(req_node, index, WRITE_ACCESS);
    }
  else // no access? then forward req to prob owner!
    {
#ifdef DEBUG_HYP
      tfprintf(stderr, "WS: forwarding req(%d) from node %d to node %d\n", index, req_node, dsm_get_prob_owner(index));
#endif
      dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, WRITE_ACCESS);
    }
  dsm_unlock_page(index);
#ifdef DEBUG_HYP
  tfprintf(stderr, "exiting the write server(%d), req_node = %d\n", index, req_node);
#endif
}


void dsmlib_erp_hyp_receive_page(void *addr, dsm_access_t access, dsm_node_t reply_node, unsigned long page_size)
{
  unsigned long index;
  /* pjh */
  int wc;
  int ac;
#ifdef DEBUG_HYP
  tfprintf(stderr, "dsmlib_erp_hyp_receive_page called\n");
#endif
  index = dsm_page_index(addr);

  /* is there a later request whose answer I should wait for? */
  /* (for non-Hyperion protocols: keep wc == 0 and ac == -1) */
   dsm_lock_page(index); 

   wc = dsm_get_waiter_count(index);
   ac = dsm_get_page_arrival_count(index);

#ifdef DEBUG_HYP
  tfprintf(stderr, "unpack page %ld: %d %d\n", index, wc, ac);
#endif

  if (wc == (ac +1))  /* keep the page */
  {
    //dsm_access_t old_access = dsm_get_access(index);

#ifdef DSM_SEND_PAGE_CHEAPER
    pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)addr, page_size); 
#else
    pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)addr, page_size); 
#endif
    pm2_rawrpc_waitdata(); 

#ifdef DEBUG_HYP
    tfprintf(stderr, "unpack page %ld: calling page server\n", index);
#endif

#ifdef DEBUG_HYP
    tfprintf(stderr, "Received page %ld <- %d for %s (I am %p)\n", index,
                 reply_node, (access == 1)?"read":"write", marcel_self());
#endif

    dsm_alloc_page_bitmap(index);

    dsm_set_access_without_protect(index, WRITE_ACCESS);
    dsm_set_pending_access(index, NO_ACCESS);
    
    dsm_clear_waiter_count(index);
    dsm_signal_page_ready(dsm_page_index(addr)); /* wakes all waiters */

#ifdef DEBUG_HYP
    tfprintf(stderr, "I signalled page ready, (I am %p)\n", marcel_self());
#endif

    dsm_unlock_page(index); 
  }
  else /* discard the page */
  {
     static char discard[DSM_PAGE_SIZE];
#ifdef DEBUG_HYP
     tfprintf(stderr, "unpack page %ld: discarding page (%d, %d)!\n", index,
       wc, ac+1);
#endif
#ifdef DSM_SEND_PAGE_CHEAPER
     pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, discard, page_size); 
#else
     pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, discard, page_size); 
#endif
     pm2_rawrpc_waitdata(); 

    /* page discarded, but count it */
     dsm_increment_page_arrival_count(index);

     dsm_unlock_page(index); 
  }
}



void dsmlib_rp_hyp_validate_page(void *addr, dsm_access_t access,
dsm_node_t reply_node)
{
  unsigned long index = dsm_page_index(addr);
//  int wc = dsm_get_waiter_count(index);
//  int ac = dsm_get_page_arrival_count(index);

#ifdef DEBUG_HYP
  tfprintf(stderr, "dsmlib_rp_hyp_validate_page called\n");
#endif

  dsm_lock_page(index); 

#ifdef DEBUG_HYP
  tfprintf(stderr, "Received page %ld <- %d for %s (I am %p)\n", index,
                 reply_node, (access == 1)?"read":"write", marcel_self());
#endif

  dsm_alloc_page_bitmap(index);

  dsm_set_access_without_protect(index, WRITE_ACCESS);
  dsm_set_pending_access(index, NO_ACCESS);

  dsm_clear_waiter_count(index);
  dsm_signal_page_ready(dsm_page_index(addr)); /* wakes all waiters */

#ifdef DEBUG_HYP
  tfprintf(stderr, "I signalled page ready, (I am %p)\n", marcel_self());
#endif

  dsm_unlock_page(index);
}
