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

              Home-Based Release Consistency Implementation
                          Vincent Bernardi

                      2000 All Rights Reserved


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

#include "dsmlib_hbrc_mw_update_protocol.h" 

be_list hbrc_mw_update_list;

void dsmlib_hbrc_mw_update_rfh(unsigned long index)
{
#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) != dsm_self());
#endif // DSM_PROT_TRACE
     return dsmlib_rf_ask_for_read_copy(index);
}

void dsmlib_hbrc_mw_update_wfh(unsigned long index)
{
#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) != dsm_self());
#endif // DSM_PROT_TRACE

  if (dsm_get_access(index) == WRITE_ACCESS)
      return;
  if(dsm_get_access(index) == NO_ACCESS)
      return dsmlib_rf_ask_for_read_copy(index);

  dsm_alloc_twin(index);
  dsm_set_access(index, WRITE_ACCESS);
  add_to_be_list(&hbrc_mw_update_list, index);
  return;
}

void dsmlib_hbrc_mw_update_rs(unsigned long index, dsm_node_t req_node, int arg)
{
  dsm_lock_page(index);
#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) == dsm_self());
#endif
  dsm_add_to_copyset(index, req_node);
  dsm_send_page(req_node, index, READ_ACCESS, arg);
  dsm_unlock_page(index);
}

void dsmlib_hbrc_mw_update_ws(unsigned long index, dsm_node_t req_node, int arg)
{
  dsm_lock_page(index);
#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) == dsm_self());
#endif //DSM_PROT_TRACE
  dsm_add_to_copyset(index, req_node);
  dsm_send_page(req_node, index, WRITE_ACCESS, arg);
  dsm_unlock_page(index);
}

void dsmlib_hbrc_mw_update_is(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
#ifdef DSM_PROT_TRACE
    fprintf(stderr,"[%s] Entering for page %d, requested by node %d, New_owner is %d\n", __FUNCTION__, index, req_node, new_owner);
#endif // DSM_PROT_TRACE    
    if(dsm_self() == dsm_get_prob_owner(index))
      {
	dsmlib_hrbc_invalidate_copyset(index, req_node, pm2_self());
	dsm_send_invalidate_ack(req_node, index);
	return;
      }
    if(dsm_self() != dsm_get_prob_owner(index))
      {
	dsm_lock_page(index);
	if(dsm_get_twin(index) != NULL)
	  {
#ifdef DSM_PROT_TRACE
	    assert(dsm_get_access(index) == WRITE_ACCESS);
	    fprintf(stderr,"[%s] Invalidate forces pre-release diff send on page %d...\n", __FUNCTION__, index);
#endif // DSM_PROT_TRACE
	    dsmlib_hrbc_send_diffs(index, req_node);
	    assert(remove_if_noted(&hbrc_mw_update_list, index) == 1);
	    dsm_free_twin(index);
	  }
	    
	dsm_set_access(index, NO_ACCESS);
	dsm_send_invalidate_ack(req_node, index);
        dsm_unlock_page(index);
      }
#ifdef DSM_PROT_TRACE
    tfprintf(stderr, "exiting the invalidate server(%d), req node =%d\n", index, req_node);
#endif //DSM_PROT_TRACE
}

void dsmlib_hbrc_mw_update_rps(void *addr, dsm_access_t access, dsm_node_t reply_node, int arg)
{
  unsigned long index = dsm_page_index(addr);
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"[%s] Entering...\n", __FUNCTION__);
#endif //DSM_PROT_TRACE
  dsm_lock_page(index); 

#ifdef DSM_PROT_TRACE
  assert(dsm_get_prob_owner(index) == reply_node);
#endif //DSM_PROT_TRACE
  dsm_set_pending_access(index, NO_ACCESS);
  dsm_set_access(index, READ_ACCESS);
  dsm_signal_page_ready(index);
  dsm_unlock_page(index);
}

void dsmlib_hbrc_acquire()
{
  return;
}

void dsmlib_hbrc_release()
{
  unsigned long index;
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"[%s]Start of release\n",__FUNCTION__);
  fprintf_be_list(hbrc_mw_update_list);
  fprintf(stderr,"[%s]Done printing\n",__FUNCTION__);
#endif // DSM_PROT_TRACE
  index = remove_first_from_be_list(&hbrc_mw_update_list);
  while(index != (unsigned long)-1)
    {
      if(dsm_get_prob_owner(index) != dsm_self())
	{
#ifdef DSM_PROT_TRACE
	  fprintf(stderr,"[%s]Processing page: %lu\n",__FUNCTION__, index);
#endif // DSM_PROT_TRACE
	  dsm_lock_page(index);
	  assert(dsm_get_twin(index) != NULL);
#ifdef DSM_PROT_TRACE
	  assert(dsm_get_access(index) == WRITE_ACCESS);
#endif // DSM_PROT_TRACE
	  dsmlib_hrbc_send_diffs(index, dsm_get_prob_owner(index));
	  dsm_free_twin(index);
	  dsm_send_invalidate_req(dsm_get_prob_owner(index), index, dsm_self(), dsm_get_prob_owner(index));
	  dsm_wait_ack(index, 1);
	  dsm_set_access(index, NO_ACCESS);
	  dsm_unlock_page(index);
	}
      else
	dsmlib_hrbc_invalidate_copyset(index, dsm_self(), dsm_self());
#ifdef DSM_PROT_TRACE
      fprintf(stderr,"[%s]Processed\n",__FUNCTION__);
#endif // DSM_PROT_TRACE
      index = remove_first_from_be_list(&hbrc_mw_update_list);
    }
#ifdef DSM_PROT_TRACE
      fprintf(stderr,"[%s]Work is done now\n",__FUNCTION__);
#endif // DSM_PROT_TRACE
}


void dsmlib_hbrc_add_page(unsigned long index)
{ 
  if (dsm_get_prob_owner(index) == dsm_self()) 
    dsm_set_access(index, WRITE_ACCESS);
  else
    dsm_set_access(index, NO_ACCESS);
}

void dsmlib_hbrc_mw_update_prot_init(unsigned long index)
{
  int i, count = 0;
  hbrc_mw_update_list = init_be_list(10);
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"Initial  printing\n");
  fprintf_be_list(hbrc_mw_update_list);
  fprintf(stderr,"Done printing\n");
  fprintf(stderr,"Nb total pages: %lu\n", dsm_get_nb_static_pages() + dsm_get_nb_pseudo_static_pages());
#endif // DSM_PROT_TRACE

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
/*------------------------------------------------------*/


void dsmlib_hrbc_send_diffs(unsigned long index, dsm_node_t dest_node)
{
  pm2_completion_t c;
  int i = 0;
  void *start;
  void *twin;
  int stacking_mode = 0;
  int stack_start = 0;
  int stack_size = 0;

  start = dsm_get_page_addr(index);
  twin = dsm_get_twin(index);
#ifdef DSM_PROT_TRACE
  assert(dsm_get_twin(index) != NULL);
  fprintf(stderr,"[%s]Sending diffs for %lu from %d to %d\n",__FUNCTION__, index, dsm_self(), dest_node); 
#endif // DSM_PROT_TRACE

  pm2_completion_init(&c, NULL, NULL);
  pm2_rawrpc_begin((int)dest_node, DSM_LRPC_HBRC_DIFFS, NULL);
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index, sizeof(unsigned long));
  for(i = 0; i < (DSM_PAGE_SIZE / 4);i++){
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
	tfprintf(stderr, "[%s]Packing offset %d, size(in words) %d\n", sizeof(int) * stack_start, stack_size);
#endif //DSM_PROT_TRACE
        pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_start, sizeof(int));
        pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_size, sizeof(int));
        pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)(((int *)start) + stack_start), stack_size);
	tfprintf(stderr, "[%s]Packed\n");
      }
    }
  }
  if(stacking_mode){
#ifdef DSM_PROT_TRACE
    tfprintf(stderr, "[%s]Packing offset %d, size(in words) %d\n", sizeof(int) * stack_start, stack_size);
#endif //DSM_PROT_TRACE
    pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_start, sizeof(int));
    pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&stack_size, sizeof(int));
    pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)(((int *)start) + stack_start), stack_size);
#ifdef DSM_PROT_TRACE
    tfprintf(stderr, "[%s]Packed\n");
#endif //DSM_PROT_TRACE
  }

  i = -1;
  // send the -1 value to terminate 
  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&i, sizeof(int));
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
  pm2_rawrpc_end();
  pm2_completion_wait(&c);
}



void DSM_HRBC_DIFFS_threaded_func(void)
{
  unsigned long index;
  int i;
  int size;
  pm2_completion_t c;
  void *start;

#ifdef DSM_PROT_TRACE
  fprintf(stderr,"[%s] Entering...\n", __FUNCTION__);
#endif //DSM_PROT_TRACE

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&index,
    sizeof(unsigned long));
  start = dsm_get_page_addr(index);
  
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&i, sizeof(void *));
  while (i != -1)
  {
    pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&size, sizeof(int));
    pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)((int *)start + i), size);
#ifdef DSM_PROT_TRACE
    tfprintf(stderr,"[%s] Received %d words diff at %d offset, first word is %d\n", __FUNCTION__, size, i * sizeof(int), *((int *)start + i));
#endif //DSM_PROT_TRACE

    pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&i, sizeof(void *));
  }

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);

  pm2_rawrpc_waitdata();

  pm2_completion_signal(&c);
}

void DSM_LRPC_HBRC_DIFFS_func(void)
{
  pm2_thread_create((pm2_func_t) DSM_HRBC_DIFFS_threaded_func, NULL);
}



void dsmlib_hrbc_invalidate_copyset(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
{
  int i, copyset_size = dsm_get_copyset_size(index);
  int invalidated_pages = 0;

#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "[%s] Entering, %d node in copyset\n", __FUNCTION__, copyset_size);
#endif

  if (copyset_size > 0)
    {
      dsm_node_t node;
      for (i = 0; i < copyset_size; i++) 
	{
	  node = dsm_get_next_from_copyset(index);
	  if ( node!= req_node) 
	    {
#ifdef DSM_COMM_TRACE
	      tfprintf(stderr,"IS: Sending invalidate request to node %d (I am %p)\n", node, marcel_self());
#endif
	      dsm_send_invalidate_req(node, index, dsm_self(), new_owner);
	      invalidated_pages++;
	    }
	}
	  // the copyset is empty now
      dsm_wait_ack(index, invalidated_pages);
#ifdef DSM_PROT_TRACE
  fprintf(stderr,"Got all acks (I am %p)\n", marcel_self());
#endif
    }

#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "exiting the invalidate server(%d), req node =%d\n", index, req_node);
#endif
}








