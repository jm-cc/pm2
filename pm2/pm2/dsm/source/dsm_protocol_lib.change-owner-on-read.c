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


#include <stdio.h>
#include "marcel.h" // tfprintf 
#include "pm2.h"
#include "dsm_const.h" // dsm_node_t
#include "dsm_page_manager.h"
#include "dsm_comm.h"
#include "dsm_protocol_lib.h"

//#define DEBUG

void DSM_RF_ASK_FOR_READ_COPY(unsigned long index)
{
#ifdef DEBUG
  tfprintf(stderr, "call to dsm_ask_for_read_copy(%d)\n", index);
#endif

  dsm_lock_page(index);
  if (dsm_get_pending_access(index) == NO_ACCESS) // there is no pending access
    {
      dsm_set_pending_access(index, READ_ACCESS);
      dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), READ_ACCESS);
      dsm_set_prob_owner(index, dsm_self()); // new
    } // else, there is already a pending access and the page will soon be there
#ifdef DEBUG2
  tfprintf(stderr, "RF: I'll now wait for page %d, owner is %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
  dsm_wait_for_page(index);
#ifdef DEBUG
  tfprintf(stderr, "I got page %d\n", index);
#endif
/*    dsm_set_prob_owner(index, dsm_get_father(index)); */
/*    dsm_set_access(index, READ_ACCESS); */
  dsm_unlock_page(index);
#ifdef DEBUG
  tfprintf(stderr, "Read handler for page %d ended\n", index);
#endif
}

void DSM_MIGRATE(unsigned long index)
{
  pm2_enable_migration();
  pm2_freeze();
  pm2_migrate(marcel_self(), dsm_get_prob_owner(index));
}

void DSM_WF_ASK_FOR_WRITE(unsigned long index)
{
#ifdef DEBUG
  tfprintf(stderr, "call to dsm_ask_for_write(%d)\n", index);
#endif
  dsm_lock_page(index);
 if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
	{
	  dsm_invalidate_copyset(index, dsm_self());
	  dsm_set_access(index, WRITE_ACCESS);
	}
 else // I am not owner
   {
     if (dsm_get_pending_access(index) < WRITE_ACCESS) // there is no pending write access
       {
	 dsm_set_pending_access(index, WRITE_ACCESS);
	 dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), WRITE_ACCESS);
	 dsm_set_prob_owner(index, dsm_self()); // new
       }
      // else, there is already a pending access and the page will soon be there
#ifdef DEBUG2
  tfprintf(stderr, "WF: I'll now wait for page %d, owner is %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
  dsm_wait_for_page(index);
#ifdef DEBUG
  tfprintf(stderr, "I got page %d\n", index);
#endif
   }
  dsm_unlock_page(index);
#ifdef DEBUG
  tfprintf(stderr, "Write handler for page %d ended\n", index);
#endif
}


void DSM_RF_REMOTE_READ(unsigned long index)
{
#ifdef DEBUG
  tfprintf(stderr, "call to dsm_remote_read(%d)\n", index);
#endif
}


void DSM_RS_SEND_READ_COPY(unsigned long index, dsm_node_t req_node)
{
#ifdef DEBUG
  tfprintf(stderr, "entering the read server(%d)\n", index);
#endif
  dsm_lock_page(index);
  if (dsm_get_access(index) != NO_ACCESS) // there is a local copy of the page
    {
      dsm_add_to_copyset(index, req_node);
      dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
      dsm_send_page(req_node, index, dsm_self(), READ_ACCESS);
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) != NO_ACCESS && !dsm_remote_requests_pending(index))
      {
#ifdef DEBUG_QUEUE
	tfprintf(stderr, "RS: storing req(%d) from node %d\n", index, req_node);
#endif
	dsm_set_prob_owner(index,req_node); 
	dsm_store_page_request(index, req_node, READ_ACCESS);	
      }
    else // no access nor pending access? then forward req to prob owner!
      {
#ifdef DEBUG2
      tfprintf(stderr, "RS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
      dsm_set_prob_owner(index,req_node); // req_node will have better information
                                          // about the true owner 
      dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS);
    }
  dsm_unlock_page(index);
}


void DSM_WS_SEND_PAGE_FOR_WRITE(unsigned long index, dsm_node_t req_node)
{
#ifdef DEBUG
  tfprintf(stderr, "entering the write server(%d), req_node = %d\n", index, req_node);
#endif
  dsm_lock_page(index);
  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
      dsm_invalidate_copyset(index, req_node);
      dsm_send_page(req_node, index, dsm_self(), WRITE_ACCESS);
      dsm_set_access(index, NO_ACCESS); // the local copy will be invalidated
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) == WRITE_ACCESS && !dsm_remote_requests_pending(index))
      {
#ifdef DEBUG_QUEUE
      tfprintf(stderr, "WS: storing req(%d) from node %d\n", index, req_node);
#endif
	dsm_store_page_request(index, req_node, WRITE_ACCESS);
      }
    else // no access nor pending access? then forward req to prob owner!
    {
#ifdef DEBUG2
      tfprintf(stderr, "WS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
      dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, WRITE_ACCESS);
    }
  dsm_set_prob_owner(index,req_node); // req_node will have better information
                                      // about the true owner 
  dsm_unlock_page(index);
}


void DSM_IS_INVALIDATE(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
  int i, copyset_size = dsm_get_copyset_size(index);

#ifdef DEBUG2
  tfprintf(stderr, "entering the invalidate server(%d), req_node = %d, new_owner =%d\n", index, req_node, new_owner);
#endif

  if (copyset_size > 0)
    {
      dsm_node_t node;
      boolean new_owner_is_in_copyset = FALSE;

      for (i = 0; i < copyset_size; i++) 
	{
	  node = dsm_get_next_from_copyset(index);
	  if (node != new_owner) 
	    {
#ifdef DEBUG2
	      tfprintf(stderr,"Sending invalidate request to node %d (I am %p)\n", node, marcel_self());
#endif
	      dsm_send_invalidate_req(node, index, dsm_self(), new_owner);
	    }
	  else
	    new_owner_is_in_copyset = TRUE;
	}
	  // the copyset is empty now
      dsm_wait_ack(index, new_owner_is_in_copyset?copyset_size - 1: copyset_size);
    }

  if(dsm_self() != dsm_get_prob_owner(index))
    {
      dsm_set_access(index, NO_ACCESS);
#ifdef DEBUG2
      tfprintf(stderr,"Sending invalidate ack to node %d (I am %p)\n", req_node, marcel_self());
#endif
      dsm_send_invalidate_ack(req_node, index);
    }

  dsm_set_prob_owner(index, new_owner);
#ifdef DEBUG2
  tfprintf(stderr, "exiting the invalidate server(%d)\n", index);
#endif
}





