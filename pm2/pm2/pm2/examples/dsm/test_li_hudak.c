/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
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
#include <stdlib.h>
#include "pm2.h"
#include "dsm_comm.h"
#include <sys/mman.h>

BEGIN_DSM_DATA
int c = 7;
END_DSM_DATA

int it, req_counter, page_counter, me, other;
marcel_sem_t sem;
tbx_tick_t t_start, t_end, t_enter_handler, t_start_send_req, t_start_server, t_start_send_page, t_start_receive_server, t_end_receive_server;

double total = 0.0, total_avg = 0.0, total_max = 0.0, total_min = 0x7fffffff;
double segv = 0.0, segv_avg = 0.0, segv_max = 0.0, segv_min = 0x7fffffff;
double prepare_req = 0.0, prepare_req_avg = 0.0, prepare_req_max = 0.0, prepare_req_min = 0x7fffffff;
double process_req = 0.0, process_req_avg = 0.0, process_req_max = 0.0, process_req_min = 0x7fffffff;
double receive_page = 0.0, receive_page_avg = 0.0, receive_page_max = 0.0, receive_page_min = 0x7fffffff;
int cnt_rf = 0, cnt_rs = 0, cnt_rp = 0;
int dump_service;
pm2_completion_t comp;
double t_req, t_page;

//#define  DSM_PROT_TRACE
//#define  DSM_COMM_TRACE


/*************************** Li & Hudak protocol ********************/
void prof_dsmlib_rf_ask_for_read_copy(unsigned long index)
{
  TBX_GET_TICK(t_enter_handler);
  cnt_rf++;
  
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "call to dsm_ask_for_read_copy(%d)\n", index);
#endif

  if (dsm_get_access(index) == READ_ACCESS)
    {
      return;
    }

  if (dsm_get_pending_access(index) == NO_ACCESS) // there is no pending access
    {
      dsm_set_pending_access(index, READ_ACCESS);
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "RF: Send page req (%d) to %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
      TBX_GET_TICK(t_start_send_req);

      dsm_send_page_req(dsm_get_prob_owner(index), index, dsm_self(), READ_ACCESS, 0);
    } // else, there is already a pending access and the page will soon be there
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "RF: I'll now wait for page %d, owner is %d (I am %p)\n", index, dsm_get_prob_owner(index), marcel_self());
#endif
  dsm_wait_for_page(index);
#ifdef DSM_COMM_TRACE
  tfprintf(stderr, "RF: I got page %d (I am %p) \n", index, marcel_self());
#endif
}

void prof_dsmlib_migrate_thread(unsigned long index)
{
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "Starting dsmlib_migrate_thread(%d)\n", index);
#endif
  dsm_unlock_page(index); 
  pm2_enable_migration();
  pm2_freeze();
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "I will migrate to %d (I am %p)\n", dsm_get_prob_owner(index), marcel_self());
#endif
  pm2_migrate(marcel_self(), dsm_get_prob_owner(index));
}

void prof_dsmlib_wf_ask_for_write_access(unsigned long index)
{
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "call to dsm_ask_for_write(%d)\n", index);
#endif

  if (dsm_get_access(index) == WRITE_ACCESS)
      return;

  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "Write handler for page %d started : a = %d(I am %p)\n", index,((atomic_t *)dsm_get_page_addr(index))->counter, marcel_self());
#endif
      dsm_invalidate_copyset(index, dsm_self());
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
}


void prof_dsmlib_rs_send_read_copy(unsigned long index, dsm_node_t req_node, int tag)
{
  TBX_GET_TICK(t_start_server);

  cnt_rs++;

  dsm_lock_page(index);

#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "entering the read server(%d), access = %d\n", index, dsm_get_access(index));
#endif

  if (dsm_get_access(index) >= READ_ACCESS) // there is a local copy of the page
    {
      dsm_add_to_copyset(index, req_node);
      //      lock_task();
      //      if (dsm_get_access(index) != WRITE_ACCESS)
      //	dsm_set_access(index, WRITE_ACCESS);
      dsm_set_access(index, WRITE_ACCESS); // the local copy will be read_only

      TBX_GET_TICK(t_start_send_page);

      dsm_send_page(req_node, index, READ_ACCESS, tag);
      dsm_set_access(index, READ_ACCESS); // the local copy will be read_only
      //      unlock_task();
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) != NO_ACCESS && !dsm_next_owner_is_set(index))
      {
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "RS: storing req(%d) from node %d, pending access = %d\n", index, req_node, dsm_get_pending_access(index));
	//assert(req_node != dsm_self());
#endif
	dsm_store_pending_request(index, req_node);
      }
    else // no access nor pending access? then forward req to prob owner!
      {
#ifdef DSM_COMM_TRACE
      tfprintf(stderr, "RS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
      dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, READ_ACCESS, tag);
    }
  dsm_unlock_page(index);

  process_req = TBX_TIMING_DELAY(t_start_server, t_start_send_page);
  process_req_avg+=process_req;
  process_req_min = min(process_req, process_req_min);
  process_req_max = max(process_req, process_req_max);
}


void prof_dsmlib_ws_send_page_for_write_access(unsigned long index, dsm_node_t req_node, int tag)
{
  dsm_lock_page(index);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "entering the write server(%d), req_node = %d\n", index, req_node);
#endif
  if (dsm_get_prob_owner(index) == dsm_self()) // I am the owner
    {
      if (dsm_get_access(index) == READ_ACCESS)//20/10/99
	dsm_invalidate_copyset(index, req_node);
      //      lock_task();
      //      if (dsm_get_access(index) != WRITE_ACCESS)
      //	dsm_set_access(index, WRITE_ACCESS);
      dsm_set_access(index, WRITE_ACCESS); // the local copy will become read-only
      dsm_send_page(req_node, index, WRITE_ACCESS, tag);
      dsm_set_access(index, NO_ACCESS); // the local copy will be invalidated
      //      unlock_task();
    }
  else // no access; maybe a pending access ?
    if (dsm_get_pending_access(index) == WRITE_ACCESS && !dsm_next_owner_is_set(index))
      {
#ifdef ASSERT
	assert(req_node != dsm_self());
#endif
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "WS: setting next owner (%d) as node %d\n", index, req_node);
#endif
	dsm_set_next_owner(index, req_node);
      }
    else // no access nor pending access? then forward req to prob owner!
      {
#ifdef DSM_COMM_TRACE
	tfprintf(stderr, "WS: forwarding req(%d) from node %d to node %d, pending access = %d\n", index, req_node, dsm_get_prob_owner(index), dsm_get_pending_access(index));
#endif
	dsm_send_page_req(dsm_get_prob_owner(index), index, req_node, WRITE_ACCESS, tag);
      }
  dsm_set_prob_owner(index, req_node); // req_node will soon be owner
  dsm_unlock_page(index);
}


void prof_dsmlib_is_invalidate(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner)
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

  dsm_set_prob_owner(index, new_owner);
#ifdef DSM_PROT_TRACE
  tfprintf(stderr, "exiting the invalidate server(%d), req node =%d\n", index, req_node);
#endif
}


void prof_dsmlib_rp_validate_page(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag)
{
     unsigned long index = dsm_page_index(addr);
     dsm_node_t node;

     TBX_GET_TICK(t_start_receive_server);
     cnt_rp++;

     dsm_lock_page(index); 

#ifdef DSM_COMM_TRACE
     tfprintf(stderr, "Received page %ld <- %d for %s (I am %p)\n", index, reply_node, (access == 1)?"read":"write", marcel_self());
#endif
     if (access == READ_ACCESS)
       {
	 if (dsm_get_pending_access(index) == READ_ACCESS)
	   {
	     dsm_set_prob_owner(index, reply_node);
	     dsm_set_pending_access(index, NO_ACCESS);
	   }
	 /* Modification of the protocol to enable testing: 7/11/2000 */
	 /*         fprintf(stderr,"it = %d\n", it);
	 if (--it)
	   dsm_set_access(index, NO_ACCESS);
	 else
	   dsm_set_access(index, READ_ACCESS);*/
	 /*         End of modification.

		    Original code:*/
	 dsm_set_access(index, READ_ACCESS);
       }
     else // access = WRITE_ACCESS
       {
	 if (dsm_get_pending_access(index) == WRITE_ACCESS)
	   {
#ifdef DSM_PROT_TRACE
	     tfprintf(stderr,"invalidating copyset\n");
#endif
	     dsm_invalidate_copyset(index, dsm_self());
#ifdef DSM_PROT_TRACE
	     tfprintf(stderr,"invalidated copyset\n");
#endif
	     //lock_task();
	     dsm_set_prob_owner(index, dsm_self());
    	     dsm_set_access(index, WRITE_ACCESS);
	     dsm_set_pending_access(index, NO_ACCESS);
	     //unlock_task();
	   }
	 else
	   {
	     //dsm_set_access(index, WRITE_ACCESS);
	     RAISE (PROGRAM_ERROR);
	   }
       }
     dsm_signal_page_ready(index);
#ifdef DSM_PROT_TRACE
     tfprintf(stderr, "I signalled page ready, (I am %p)\n", marcel_self());
#endif
     // process pending requests
     // should wait for a while ?
     dsm_unlock_page(index);
     while ((node = dsm_get_next_pending_request(index)) != NO_NODE)
      {
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "Processing R-req from node %d (I am %p)\n", node, marcel_self());
#endif
	(*dsm_get_read_server(index))(index, node, 0);
      }
     if(access == WRITE_ACCESS && dsm_next_owner_is_set(index))
      {
	marcel_delay(10);
	node = dsm_get_next_owner(index);
#ifdef DSM_QUEUE_TRACE
	tfprintf(stderr, "Processing W-req from next owner: node %d (I am %p) \n", node, marcel_self());
#endif
	(*dsm_get_write_server(index))(index, node, 0);
	dsm_clear_next_owner(index); // these last 2 calls should be atomic...
      }

     //     TBX_GET_TICK(t_end_receive_server);
}


/************************** end of Li & Hudak protocol ***************/

void startup_func(int argc, char *argv[], void *arg)
{
  it = req_counter = atoi(argv[1]);
}


static void dump_statistics(int full)
{
  fprintf(stderr, "%20s %12s %12s %12s %12s %12s\n", "", "Avg", "Min", "Max", "#", "%Total time");
  fprintf(stderr, "%20s %12s %12s %12s %12s %12s\n", "", "---", "---", "---", "---", "------------");
  fprintf(stderr,"%20s %12.3f %12.3f %12.3f %12d %12.2f%%\n","Seg fault", segv_avg/it, segv_min, segv_max, cnt_rf, segv_avg/total_avg*100);
  fprintf(stderr,"%20s %12.3f %12.3f %12.3f %12d %12.2f%%\n","Prepare request", prepare_req_avg/it, prepare_req_min, prepare_req_max, cnt_rf, prepare_req_avg/total_avg*100);
  if (full)
    fprintf(stderr,"%20s %12.3f %12.3f %12.3f %12d %12.2f%%\n","Transmit request", t_req, t_req, t_req, 0, t_req*it/total_avg*100);
  fprintf(stderr,"%20s %12.3f %12.3f %12.3f %12d %12.2f%%\n","Process request", process_req_avg/it, process_req_min, process_req_max, cnt_rs, process_req_avg/total_avg*100);
  if (full)
    fprintf(stderr,"%20s %12.3f %12.3f %12.3f %12d %12.2f%%\n","Transmit page", t_page, t_page, t_page, 0, t_page*it/total_avg*100);
  fprintf(stderr,"%20s %12.3f %12.3f %12.3f %12d %12.2f%%\n","Install page", receive_page_avg/it, receive_page_min, receive_page_max, cnt_rp, receive_page_avg/total_avg*100);
  fprintf(stderr, "%20s %12s %12s %12s %12s %12s\n", "", "---", "---", "---", "---", "------------");
  fprintf(stderr,"%20s %12.3f %12.3f %12.3f %12s %12.2f%%\n","Total time", total_avg/it, total_min, total_max, "", 100.0);
}

static void threaded_dump_func(void *arg)
{
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &comp);
  pm2_rawrpc_waitdata(); 

  pm2_completion_signal_begin(&comp); 
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&process_req_avg, sizeof(double));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&process_req_min, sizeof(double));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&process_req_max, sizeof(double));
  pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &cnt_rs, 1);
  pm2_completion_signal_end(); 
  fprintf(stderr, "sending stats: done\n");
}

static void dump_func(void)
{
  pm2_thread_create(threaded_dump_func, NULL);
}

void handler(void *arg)
{
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&process_req_avg, sizeof(double));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&process_req_min, sizeof(double));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&process_req_max, sizeof(double));
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &cnt_rs, 1);
}


int pm2_main(int argc, char **argv)
{
  int prot, i;
  volatile int local;

#ifdef PROFILE
  profile_activate(FUT_ENABLE, PM2_PROF_MASK | DSM_PROF_MASK);
#endif
  pm2_push_startup_func(startup_func, NULL);
  pm2_set_dsm_page_distribution(DSM_CENTRALIZED, 1);
  pm2_rawrpc_register(&dump_service, dump_func);

  prot = dsm_create_protocol(prof_dsmlib_rf_ask_for_read_copy,
			     prof_dsmlib_wf_ask_for_write_access,
                             prof_dsmlib_rs_send_read_copy,
                             prof_dsmlib_ws_send_page_for_write_access,
                             prof_dsmlib_is_invalidate,
                             prof_dsmlib_rp_validate_page,
                             NULL, 
                             NULL, 
                             NULL, 
                             NULL, 
                             NULL 
                             );
  dsm_set_default_protocol(prot);
 
  pm2_init(&argc, argv);

  if (argc < 2)
    {
      fprintf(stderr, "Usage: test_li_hudak <iterations> [<send_req time> <send_page_time>]\n");
      exit(1);
    }

  if(pm2_self() == 0) 
    { /* master process */
      
      pm2_completion_init(&comp, handler, NULL);

      for (i = 0; i < it; i++)
	{
	  TBX_GET_TICK(t_start);
	  
	  local = c;
	  
	  TBX_GET_TICK(t_end);
	  
	  if (cnt_rf)
	    {
	      total = TBX_TIMING_DELAY(t_start, t_end);
	      total_avg+=total;
	      total_min = min(total, total_min);
	      total_max = max(total, total_max);
	      
	      segv = TBX_TIMING_DELAY(t_start, t_enter_handler);
	      segv_avg+=segv;
	      segv_min = min(segv, segv_min);
	      segv_max = max(segv, segv_max);
	      
	      prepare_req = TBX_TIMING_DELAY(t_enter_handler, t_start_send_req);
	      prepare_req_avg+=prepare_req;
	      prepare_req_min = min(prepare_req, prepare_req_min);
	      prepare_req_max = max(prepare_req, prepare_req_max);
	    }
	  else
	    {
	      segv_min = 0.0;
	      prepare_req_min = 0.0;
	      total_min = 0.0;
	    }
	  
	  if (cnt_rp)
	    {
	      receive_page = TBX_TIMING_DELAY(t_start_receive_server, t_end);
	      receive_page_avg+=receive_page;
	      receive_page_min = min(receive_page, receive_page_min);
	      receive_page_max = max(receive_page, receive_page_max);
	    }
	  else
	    receive_page_min = 0.0;

	  dsm_set_access(0, NO_ACCESS);
	}

      pm2_rawrpc_begin(1, dump_service, NULL);
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&comp);
      pm2_rawrpc_end();
      pm2_completion_wait(&comp);

      if (argc > 2)
	{
	  t_req = atof(argv[2]);
	  t_page = atof(argv[3]);
	}

      if (!cnt_rs)
	process_req_min = 0.0;
	  
      dump_statistics(argc==4);
      pm2_halt();
    }

  pm2_exit();

#ifdef PROFILE
  profile_stop();
#endif

  fprintf(stderr, "Main is ending\n");
  return 0;
}
