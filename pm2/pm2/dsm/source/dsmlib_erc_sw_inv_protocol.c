
//#include <assert.h>
#include "dsmlib_erc_sw_inv_protocol.h"
#include "dsmlib_belist.h"
#include "dsm_protocol_policy.h"
#include "dsm_protocol_lib.h"
#include "dsm_page_manager.h"

be_list erc_sw_inv_list;

void dsmlib_erc_sw_inv_init(int protocol_number){
  erc_sw_inv_list = init_be_list(10);
  fprintf(stderr,"Initial  printing\n");
  fprintf_be_list(erc_sw_inv_list);
  fprintf(stderr,"Done printing\n");
  fprintf(stderr,"Nb total pages: %lu\n", dsm_get_nb_static_pages() + dsm_get_nb_pseudo_static_pages());
  /*  for(i = 0; i < dsm_get_nb_static_pages() + dsm_get_nb_pseudo_static_pages(); i++)
    if(dsm_get_page_protocol(i) == protocol_number){
      count++;
      if(dsm_get_prob_owner(i) == dsm_self())
	dsm_set_access(i, READ_ACCESS);
      else
	dsm_set_access(i, NO_ACCESS);
    }
  fprintf(stderr,"Nb erc pages: %d\n", count);*/
  return;
}

void dsmlib_erc_sw_inv_rfh(unsigned long index){
  fprintf(stderr, "Read Fault Handler called\n");
  return dsmlib_rf_ask_for_read_copy(index);
}


void dsmlib_erc_sw_inv_wfh(unsigned long index)
{  
  fprintf(stderr, "Write Fault Handler called\n");
  if (dsm_get_access(index) == WRITE_ACCESS)
      return;

  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
      fprintf(stderr, "Adding to belist: %lu\n", index);
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
}

void dsmlib_erc_sw_inv_rs(unsigned long index, dsm_node_t req_node, int arg){
  dsm_lock_page(index);
  if (dsm_get_access(index) >= READ_ACCESS) // there is a local copy of the page
    {
      dsm_add_to_copyset(index, req_node);
      dsm_send_page(req_node, index, READ_ACCESS, arg);
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) != NO_ACCESS && !dsm_next_owner_is_set(index))
      {
	dsm_store_pending_request(index, req_node);
      }
    else // no access nor pending access? then forward req to prob owner!
      {
	dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS, arg);
      }
  dsm_unlock_page(index);
}

void dsmlib_erc_sw_inv_ws(unsigned long index, dsm_node_t req_node, int arg){
  fprintf(stderr,"[%s] Entering...\n", __FUNCTION__);
  dsm_lock_page(index);
  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
      if (remove_if_noted(&erc_sw_inv_list, index))
	{
	  // In data-races free programs, this should not happen...
	  //assert(dsm_get_access(index) == WRITE_ACCESS);
	  dsm_invalidate_copyset(index, req_node);
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
  dsm_unlock_page(index);
}

void dsmlib_erc_sw_inv_is(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner){
  fprintf(stderr,"[%s] Entering...\n", __FUNCTION__);
  return dsmlib_is_invalidate(index, req_node, new_owner);
}

void dsmlib_erc_sw_inv_rps(void *addr, dsm_access_t access, dsm_node_t reply_node, int arg){
  unsigned long index = dsm_page_index(addr);
  dsm_node_t node;
  fprintf(stderr,"[%s] Entering...\n", __FUNCTION__);
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
	dsm_invalidate_copyset(index, dsm_self());
	dsm_set_access(index, READ_ACCESS);
      }
    }
  dsm_signal_page_ready(index);
  // process pending requests
  // should wait for a while ?
  dsm_unlock_page(index);
  while ((node = dsm_get_next_pending_request(index)) != NO_NODE)
    {
      (*dsm_get_read_server(index))(index, node, arg);
    }
  if(access == WRITE_ACCESS && dsm_next_owner_is_set(index))
    {
      marcel_delay(10);
      node = dsm_get_next_owner(index);
      (*dsm_get_write_server(index))(index, node, arg);
      dsm_clear_next_owner(index); // these last 2 calls should be atomic...
    }
}

void dsmlib_erc_acquire()
{
  return;
}

void dsmlib_erc_release()
{
  unsigned long index;
  fprintf(stderr,"Start of release\n");
  fprintf_be_list(erc_sw_inv_list);
  fprintf(stderr,"Done printing\n");
  index = remove_first_from_be_list(&erc_sw_inv_list);
  fprintf(stderr,"First One removed: %lu\n", index);
  while(index != (unsigned long)-1)
    {
      fprintf(stderr,"Trying to lock page: %lu\n", index);
      dsm_lock_page(index);
      fprintf(stderr,"Invalidating copyset: %lu\n", index);
      dsm_invalidate_copyset(index, dsm_self());
      fprintf(stderr,"Changing access: %lu\n", index);
      dsm_set_access(index, READ_ACCESS);
      dsm_unlock_page(index);
      index = remove_first_from_be_list(&erc_sw_inv_list);
      fprintf(stderr,"Removed: %ld\n", index);
    }
  fprintf(stderr,"End of release\n");
  return;
}



void dsmlib_erc_add_page(unsigned long index)
{
  if (dsm_get_prob_owner(index) == dsm_self()) 
    dsm_set_access(index, READ_ACCESS);
  else
    dsm_set_access(index, NO_ACCESS);
}






