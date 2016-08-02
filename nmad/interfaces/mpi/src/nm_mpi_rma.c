/*
 * NewMadeleine
 * Copyright (C) 2016 (see AUTHORS file)
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


#include "nm_mpi_private.h"
#include <assert.h>
#include <errno.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);

#define NM_MPI_WIN_ERROR(win_id, errcode) (MPI_Win_call_errhandler(win_id, errcode), errcode)

/* ** Datatype management for communication **************** */

/* Simple typedef to define the format of a RMA request (MPI_Get) */
typedef struct nm_data_mpi_datatype_header_s nm_mpi_rma_request_datatype_t;
static nm_mpi_datatype_t*p_rma_request_datatype;
static MPI_Datatype rma_request_datatype;

/* Structure for shared memory passive requests */

typedef struct nm_mpi_rma_request_info_s {
  const void*p_origin_addr;
  nm_mpi_datatype_t*p_origin_datatype;
  int origin_count;
  void*p_target_addr;
  nm_mpi_datatype_t*p_target_datatype;
  int target_count;
  void*p_result_addr;
  nm_mpi_datatype_t*p_result_datatype;
  int result_count;
  const void*p_comp_addr;
} nm_mpi_rma_request_info_t;

/* ** Static function prototypes *************************** */

static void nm_mpi_rma_request_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref);
static void nm_mpi_rma_request_flush_destroy_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info,
						     void*ref);
static void nm_mpi_rma_request_free_destroy_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info,
						    void*ref);
static void nm_mpi_rma_request_free_flush_destroy_monitor(nm_sr_event_t event,
							  const nm_sr_event_info_t*info, void*ref);
static void nm_mpi_rma_request_flush_destroy_unlock_monitor(nm_sr_event_t event,
							    const nm_sr_event_info_t*info, void*ref);
static void nm_mpi_rma_request_free_flush_destroy_unlock_monitor(nm_sr_event_t event,
								 const nm_sr_event_info_t*info,
								 void*ref);
static void nm_mpi_rma_request_flush_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info,
					     void*ref);
static int  nm_mpi_rma_isend(nm_mpi_request_t*p_req, struct nm_data_s*p_data, nm_mpi_window_t*p_win,
			     const size_t header_size);
static int  nm_mpi_rma_is_inbound_target(nm_mpi_request_t*p_req, const nm_sr_event_info_t*p_info,
					 nm_mpi_window_t*p_win);
static int  nm_mpi_rma_is_inbound_origin(nm_mpi_request_t*p_req, nm_mpi_window_t*p_win, int*is_inbound);
static int  nm_mpi_rma_is_inbound_local(const MPI_Aint disp, const int count, const int rank,
					const size_t size, const nm_mpi_window_t*p_win);
static int  nm_mpi_rma_pack_send_response(nm_mpi_request_t*p_req);
static int  nm_mpi_rma_recv_accumulate(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win);
static int  nm_mpi_rma_copy_send_response(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win);
static int  nm_mpi_rma_send_element(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win);
static int  nm_mpi_rma_do_comp_and_swap(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win);

/* ** Message passing versions of RMA operations *********** */

static int  nm_mpi_put(const void *origin_addr, int origin_count, nm_mpi_datatype_t*p_origin_datatype,
		       int target_rank, MPI_Aint target_disp, int target_count,
		       nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
		       nm_mpi_request_t*p_win_req);
static int  nm_mpi_get(void *origin_addr, int origin_count, nm_mpi_datatype_t*p_origin_datatype,
		       int target_rank, MPI_Aint target_disp, int target_count,
		       nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
		       nm_mpi_request_t*p_req_in);
static int  nm_mpi_accumulate(const void *origin_addr, int origin_count,
			      nm_mpi_datatype_t*p_origin_datatype, int target_rank,
			      MPI_Aint target_disp, int target_count,
			      nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
			      nm_mpi_window_t*p_win, nm_mpi_request_t*p_req_out);
static int  nm_mpi_get_accumulate(const void *origin_addr, int origin_count,
				  nm_mpi_datatype_t*p_origin_datatype, void *result_addr,
				  int result_count, nm_mpi_datatype_t*p_result_datatype,
				  int target_rank, MPI_Aint target_disp, int target_count,
				  nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
				  nm_mpi_window_t*p_win, nm_mpi_request_t*p_req_out);
static int  nm_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
				nm_mpi_datatype_t*p_datatype, int target_rank, MPI_Aint target_disp,
				MPI_Op op, nm_mpi_window_t*p_win, nm_mpi_request_t*p_req_in);
static int  nm_mpi_compare_and_swap(const void *origin_addr, const void *compare_addr,
				    void *result_addr, nm_mpi_datatype_t*p_datatype, int target_rank,
				    MPI_Aint target_disp, nm_mpi_window_t*p_win,
				    nm_mpi_request_t*p_req);

/* ** Shared memory versions of RMA operations ************* */

static int  nm_mpi_put_shm(const void *origin_addr, int origin_count,
			   nm_mpi_datatype_t*p_origin_datatype, int target_rank, MPI_Aint target_disp,
			   int target_count, nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
			   nm_mpi_request_t*p_req);
static int  nm_mpi_get_shm(void *origin_addr, int origin_count, nm_mpi_datatype_t*p_origin_datatype,
			   int target_rank, MPI_Aint target_disp, int target_count,
			   nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
			   nm_mpi_request_t*p_req);
static int  nm_mpi_accumulate_shm(const void *origin_addr, int origin_count,
				  nm_mpi_datatype_t*p_origin_datatype, int target_rank,
				  MPI_Aint target_disp, int target_count,
				  nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
				  nm_mpi_window_t*p_win, nm_mpi_request_t*p_req);
static int  nm_mpi_get_accumulate_shm(const void *origin_addr, int origin_count,
				      nm_mpi_datatype_t*p_origin_datatype, void *result_addr,
				      int result_count, nm_mpi_datatype_t*p_result_datatype,
				      int target_rank, MPI_Aint target_disp, int target_count,
				      nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
				      nm_mpi_window_t*p_win, nm_mpi_request_t*p_req);
static int  nm_mpi_fetch_and_op_shm(const void *origin_addr, void *result_addr, int count,
				    nm_mpi_datatype_t*p_datatype, int target_rank, MPI_Aint target_disp,
				    MPI_Op op, nm_mpi_window_t*p_win, nm_mpi_request_t*p_req);
static int  nm_mpi_compare_and_swap_shm(const void *origin_addr, const void *compare_addr,
					void *result_addr, nm_mpi_datatype_t*p_datatype,
					int target_rank,MPI_Aint target_disp, nm_mpi_window_t*p_win,
					nm_mpi_request_t*p_req);
  
extern int  nm_mpi_win_unlock(nm_mpi_window_t*p_win, int from, nm_mpi_win_epoch_t*p_epoch);
extern void nm_mpi_win_enqueue_pending(nm_mpi_request_t*p_req, nm_mpi_window_t*p_win);


/* ********************************************************* */
/* Aliases */

NM_MPI_ALIAS(MPI_Put, mpi_put);
NM_MPI_ALIAS(MPI_Get, mpi_get);
NM_MPI_ALIAS(MPI_Accumulate, mpi_accumulate);
NM_MPI_ALIAS(MPI_Get_accumulate, mpi_get_accumulate);
NM_MPI_ALIAS(MPI_Fetch_and_op, mpi_fetch_and_op);
NM_MPI_ALIAS(MPI_Compare_and_swap, mpi_compare_and_swap);
NM_MPI_ALIAS(MPI_Rput, mpi_rput);
NM_MPI_ALIAS(MPI_Rget, mpi_rget);
NM_MPI_ALIAS(MPI_Raccumulate, mpi_raccumulate);
NM_MPI_ALIAS(MPI_Rget_accumulate, mpi_rget_accumulate);

/* ********************************************************* */

/* ** RMA library initialization functions ***************** */

__PUK_SYM_INTERNAL
void nm_mpi_rma_init(void)
{
  /** Initialise rma_request_datatype */
  nm_mpi_rma_request_datatype_t bar;
  const int count = 3;
  int aob[3];          /**< Array of blocklengths */
  MPI_Datatype aot[3]; /**< Array of types */
  MPI_Aint aod[3];     /**< Array of displacement */
  aob[0] = 1; aot[0] = MPI_AINT; aod[0] = 0;
  aob[1] = 1; aot[1] = MPI_UINT32_T;  aod[1] = (void*)&bar.datatype_hash - (void*)&bar;
  aob[2] = 1; aot[2] = MPI_INT;  aod[2] = (void*)&bar.count - (void*)&bar;
  MPI_Type_struct(count, aob, aod, aot, &rma_request_datatype);
  p_rma_request_datatype = nm_mpi_datatype_get(rma_request_datatype);
  p_rma_request_datatype->name = strdup("RMA request datatype");
  MPI_Type_commit(&rma_request_datatype);
}

__PUK_SYM_INTERNAL
void nm_mpi_rma_exit(void)
{
  MPI_Type_free(&rma_request_datatype);
  p_rma_request_datatype = NULL;
}

/* ** Request monitoring functions ************************* */

static void nm_mpi_rma_request_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_request_t  *p_req = (nm_mpi_request_t*)ref;
  nm_tag_t tag;
  nm_sr_request_t*p_nm_req = info->req.p_request;
  nm_sr_request_get_tag(p_nm_req, &tag);
  assert(tag);
  nm_mpi_window_t   *p_win = nm_mpi_window_get(nm_mpi_rma_tag_to_win(tag));
  assert(p_win);
  switch(tag & NM_MPI_TAG_PRIVATE_RMA_MASK_OP) {
  case NM_MPI_TAG_PRIVATE_RMA_GET_REQ:
    nm_mpi_rma_pack_send_response(p_req);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_ACC:
    nm_mpi_rma_recv_accumulate(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_GACC_REQ:
    nm_mpi_rma_copy_send_response(p_req, p_win);
    nm_mpi_rma_recv_accumulate(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_FAO_REQ:
    nm_mpi_rma_send_element(p_req, p_win);
    nm_mpi_rma_recv_accumulate(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_CAS_REQ:
    nm_mpi_rma_send_element(p_req, p_win);
    nm_mpi_rma_do_comp_and_swap(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  }
  __sync_add_and_fetch(&p_req->p_epoch->completed, 1);
  if(nm_mpi_win_completed_epoch(p_req->p_epoch))
    {
      nm_mpi_win_unlock(p_win, p_req->request_source, p_req->p_epoch);
    }
  nm_mpi_datatype_unlock(p_req->p_datatype);
  nm_mpi_request_free(p_req);
}

static void nm_mpi_rma_request_free_flush_destroy_monitor(nm_sr_event_t event,
							  const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_request_t*p_req = (nm_mpi_request_t*)ref;
  FREE_AND_SET_NULL(p_req->rbuf);
  nm_mpi_rma_request_flush_destroy_monitor(event, info, ref);
}

static void nm_mpi_rma_request_free_destroy_monitor(nm_sr_event_t event,
						    const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_request_t*p_req = (nm_mpi_request_t*)ref;
  FREE_AND_SET_NULL(p_req->rbuf);
  nm_mpi_request_free(p_req);
}

static void nm_mpi_rma_request_flush_destroy_monitor(nm_sr_event_t event,
						     const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_rma_request_flush_monitor(event, info, ref);
  nm_mpi_request_t*p_req = (nm_mpi_request_t*)ref;
  nm_mpi_request_free(p_req);
}

static void nm_mpi_rma_request_free_flush_destroy_unlock_monitor(nm_sr_event_t event,
								 const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_request_t*p_req = (nm_mpi_request_t*)ref;
  FREE_AND_SET_NULL(p_req->rbuf);
  nm_mpi_rma_request_flush_destroy_unlock_monitor(event, info, ref);
}

static void nm_mpi_rma_request_flush_destroy_unlock_monitor(nm_sr_event_t event,
							    const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_rma_request_flush_monitor(event, info, ref);
  nm_mpi_request_t*p_req = (nm_mpi_request_t*)ref;
  nm_mpi_window_t *p_win = p_req->p_win;
  assert(p_win);
  if(NM_MPI_WIN_PASSIVE_TARGET_END == p_req->p_epoch->mode
     && nm_mpi_win_completed_epoch(p_req->p_epoch))
    {
      nm_mpi_win_unlock(p_win, p_req->request_source, p_req->p_epoch);
    }
  nm_mpi_request_free(p_req);
}

static void nm_mpi_rma_request_flush_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info,
					     void*ref)
{
  assert(ref);
  nm_mpi_request_t*p_req = (nm_mpi_request_t*)ref;
  __sync_add_and_fetch(&p_req->p_epoch->completed, 1);
  if(NM_SR_EVENT_FINALIZED == event && p_req->p_datatype->id >= _NM_MPI_DATATYPE_OFFSET)
    {
      nm_mpi_datatype_unlock(p_req->p_datatype);
    }
}

/* ** Communication management functions ******************* */

static int nm_mpi_rma_isend(nm_mpi_request_t*p_req, struct nm_data_s*p_data, nm_mpi_window_t*p_win,
			    const size_t header_size)
{
  int err = MPI_SUCCESS;
  if(NULL == p_req->gate) {
    p_req->gate = nm_mpi_communicator_get_gate(p_win->p_comm, p_req->request_source);
    if(p_req->gate == NULL){
      return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
    }
  }
  nm_tag_t     nm_tag    = nm_mpi_rma_win_to_tag(p_win->win_ids[p_req->request_source]);
  nm_tag                |= (nm_tag_t)p_req->user_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  p_req->p_comm                  = p_win->p_comm;
  p_req->request_type            = NM_MPI_REQUEST_SEND;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_req->p_datatype->refcount, 1);
  nm_sr_send_init(p_session, &p_req->request_nmad);
  nm_sr_send_pack_data(p_session, &p_req->request_nmad, p_data);
  nm_sr_send_dest(p_session, &p_req->request_nmad, p_req->gate, nm_tag);
  if(0 < header_size)
    nm_sr_send_header(p_session, &p_req->request_nmad, header_size);
  err = nm_sr_send_submit(p_session, &p_req->request_nmad);
  p_req->request_error = err;
  return err;
}

__PUK_SYM_INTERNAL
void nm_mpi_rma_recv(const nm_sr_event_info_t*p_info, nm_mpi_window_t*p_win)
{
  const nm_tag_t     tag = p_info->recv_unexpected.tag;
  nm_session_t p_session = p_info->recv_unexpected.p_session;
  const nm_gate_t   from = p_info->recv_unexpected.p_gate;
  const size_t       len = p_info->recv_unexpected.len;
  const int       source = nm_mpi_communicator_get_dest(p_win->p_comm, from);
  const int     win_mode = p_win->exposure[source].mode;
  const int  epoch_ready = NM_MPI_WIN_UNUSED != win_mode;
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  p_req->gate                    = from;
  p_req->p_win                   = p_win;
  p_req->p_comm                  = p_win->p_comm;
  p_req->p_epoch                 = &p_win->exposure[source];
  p_req->user_tag                = tag;
  p_req->p_datatype              = nm_mpi_datatype_get(MPI_BYTE);
  p_req->request_type            = NM_MPI_REQUEST_RECV;
  p_req->request_source          = source;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_win->exposure[source].nmsg, 1);
  nm_sr_recv_init(p_session, &p_req->request_nmad);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  nm_sr_recv_match_event(p_session, &p_req->request_nmad, p_info);
  __sync_add_and_fetch(&p_req->p_datatype->refcount, 1);
  if(MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor && !nm_mpi_rma_is_inbound_target(p_req, p_info, p_win)) {
    return;
  }
  switch(tag & NM_MPI_TAG_PRIVATE_RMA_MASK_OP){
  case NM_MPI_TAG_PRIVATE_RMA_PUT:
    {
      struct nm_data_s data;
      /* Retrieve the request as the message header */
      struct nm_data_mpi_datatype_header_s header_msg;
      struct nm_data_s header;
      nm_data_contiguous_set(&header, (struct nm_data_contiguous_s){
	  .ptr = &header_msg, .len = sizeof(struct nm_data_mpi_datatype_header_s)
	    });
      nm_sr_recv_peek(p_session, &p_req->request_nmad, &header);
      /* Set data for unpack */
      nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_hashtable_get(header_msg.datatype_hash);
      assert(p_datatype);
      void*p_base = MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor ? MPI_BOTTOM : p_win->p_base;
      p_req->rbuf       = p_base + header_msg.offset;
      p_req->count      = header_msg.count;
      p_req->p_datatype = p_datatype;
      __sync_add_and_fetch(&p_req->p_datatype->refcount, 1);
      nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
	  .header = header_msg,
	    .data = (struct nm_data_mpi_datatype_s)
	    { .ptr = p_req->rbuf, .p_datatype = p_datatype, .count = header_msg.count }
	});
      nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			    &nm_mpi_rma_request_monitor);
      nm_sr_recv_unpack_data(p_session, &p_req->request_nmad, &data);
      if(epoch_ready){
	nm_sr_recv_post(p_session, &p_req->request_nmad);
      } else {
	nm_mpi_win_enqueue_pending(p_req, p_win);
      }
    }
    break;
  case NM_MPI_TAG_PRIVATE_RMA_GET_REQ:
    {
      struct nm_data_s data;
      nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
	  .data = (struct nm_data_mpi_datatype_s)
	    { .ptr = NULL, .p_datatype = p_req->p_datatype, .count = 0 }
	});
      /* Retrieve the request as the message header */
      struct nm_data_mpi_datatype_header_s header_msg;
      struct nm_data_s header;
      nm_data_contiguous_set(&header, (struct nm_data_contiguous_s){
	  .ptr = &header_msg, .len = sizeof(struct nm_data_mpi_datatype_header_s)
	    });
      nm_sr_recv_peek(p_session, &p_req->request_nmad, &header);
      void*p_base = MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor ? MPI_BOTTOM : p_win->p_base;
      p_req->rbuf       = p_base + header_msg.offset;
      p_req->count      = header_msg.count;
      p_req->p_datatype = nm_mpi_datatype_hashtable_get(header_msg.datatype_hash);
      __sync_add_and_fetch(&p_req->p_datatype->refcount, 1);
      assert(p_req->p_datatype && p_req->p_datatype->id);
      nm_sr_recv_unpack_data(p_session, &p_req->request_nmad, &data);
      nm_sr_recv_post(p_session, &p_req->request_nmad);
      if(epoch_ready){
	nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			      &nm_mpi_rma_request_monitor);
      } else {
	nm_mpi_win_enqueue_pending(p_req, p_win);
      }
    }
    break;
  case NM_MPI_TAG_PRIVATE_RMA_ACC:
  case NM_MPI_TAG_PRIVATE_RMA_GACC_REQ:
  case NM_MPI_TAG_PRIVATE_RMA_FAO_REQ:
  case NM_MPI_TAG_PRIVATE_RMA_CAS_REQ:
    {
      p_req->rbuf  = malloc(len);
      p_req->count = len;
      nm_sr_recv_unpack_contiguous(p_session, &p_req->request_nmad, p_req->rbuf, len);
      nm_sr_recv_post(p_session, &p_req->request_nmad);
      if(epoch_ready){
	nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			      &nm_mpi_rma_request_monitor);
      } else {
	nm_mpi_win_enqueue_pending(p_req, p_win);
      }
    }
    break;
  }
}

__PUK_SYM_INTERNAL
void nm_mpi_rma_handle_passive(nm_mpi_request_t*p_req)
{
  nm_mpi_window_t *p_win = p_req->p_win;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_tag_t tag;
  nm_sr_request_get_tag(&p_req->request_nmad, &tag);
  assert(tag);
  switch(tag & NM_MPI_TAG_PRIVATE_RMA_MASK_OP) {
  case NM_MPI_TAG_PRIVATE_RMA_PUT:
    nm_sr_recv_post(p_session, &p_req->request_nmad);
    return;
  case NM_MPI_TAG_PRIVATE_RMA_GET_REQ:
    nm_mpi_rma_pack_send_response(p_req);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_ACC:
    nm_mpi_rma_recv_accumulate(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_GACC_REQ:
    nm_mpi_rma_copy_send_response(p_req, p_win);
    nm_mpi_rma_recv_accumulate(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_FAO_REQ:
    nm_mpi_rma_send_element(p_req, p_win);
    nm_mpi_rma_recv_accumulate(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  case NM_MPI_TAG_PRIVATE_RMA_CAS_REQ:
    nm_mpi_rma_send_element(p_req, p_win);
    nm_mpi_rma_do_comp_and_swap(p_req, p_win);
    FREE_AND_SET_NULL(p_req->rbuf);
    break;
  }
  __sync_add_and_fetch(&p_req->p_epoch->completed, 1);
  if(nm_mpi_win_completed_epoch(p_req->p_epoch))
    {
      nm_mpi_win_unlock(p_win, p_req->request_source, p_req->p_epoch);
    }
  nm_mpi_datatype_unlock(p_req->p_datatype);
  nm_mpi_request_free(p_req);
}

static int nm_mpi_rma_is_inbound_target(nm_mpi_request_t*p_req, const nm_sr_event_info_t*p_info,
					nm_mpi_window_t*p_win)
{
  int is_inbound;
  const nm_tag_t     tag = p_info->recv_unexpected.tag;
  nm_session_t p_session = p_info->recv_unexpected.p_session;
  const nm_gate_t   from = p_info->recv_unexpected.p_gate;
  const size_t       len = p_info->recv_unexpected.len;
  const int       source = nm_mpi_communicator_get_dest(p_win->p_comm, from);
  /* Check message header to verify boundaries of data */
  struct nm_data_mpi_datatype_header_s header_msg;
  struct nm_data_s header;
  nm_data_contiguous_set(&header, (struct nm_data_contiguous_s){
      .ptr = &header_msg, .len = sizeof(struct nm_data_mpi_datatype_header_s)
	});
  nm_sr_recv_peek(p_session, &p_req->request_nmad, &header);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_hashtable_get(header_msg.datatype_hash);
  is_inbound = nm_mpi_win_addr_is_valid((void*)header_msg.offset,
					header_msg.count * p_datatype->size, p_win);
  nm_mpi_request_t*p_req_valid = nm_mpi_request_alloc();
  p_req_valid->gate                    = from;
  p_req_valid->p_win                   = p_win;
  p_req_valid->count                   = 1;
  p_req_valid->p_comm                  = p_win->p_comm;
  p_req_valid->p_epoch                 = &p_win->exposure[source];
  p_req_valid->user_tag                = (tag & ~0x080000F0) | 0x0400000F;
  p_req_valid->p_datatype              = nm_mpi_datatype_get(MPI_INT);
  p_req_valid->request_type            = NM_MPI_REQUEST_SEND;
  p_req_valid->request_source          = source;
  p_req_valid->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  ((int*)p_req_valid->static_buf)[0]   = is_inbound;
  p_req_valid->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_req_valid->p_datatype->refcount, 1);
  nm_tag_t tag_out = nm_mpi_rma_win_to_tag(p_win->win_ids[source]);
  tag_out |= (nm_tag_t)p_req_valid->user_tag & (NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0xFFFFFF);
  __sync_add_and_fetch(&p_win->exposure[source].nmsg, 1);
  nm_sr_send_init(p_session, &p_req_valid->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req_valid->request_nmad, &p_req_valid->static_buf, sizeof(int));
  nm_sr_request_set_ref(&p_req_valid->request_nmad, p_req_valid);
  nm_sr_request_monitor(p_session, &p_req_valid->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_flush_destroy_unlock_monitor);
  nm_sr_send_dest(p_session, &p_req_valid->request_nmad, from, tag_out);
  p_req_valid->request_error = nm_sr_send_submit(p_session, &p_req_valid->request_nmad);
  if(!is_inbound) { /* ERROR ON RANGE */
    p_req->rbuf       = malloc(len);
    p_req->count      = len;
    p_req->p_datatype = nm_mpi_datatype_get(MPI_BYTE);
    nm_sr_recv_unpack_contiguous(p_session, &p_req->request_nmad, p_req->rbuf, len);
    nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_rma_request_free_flush_destroy_unlock_monitor);
    nm_sr_recv_post(p_session, &p_req->request_nmad);	
  }
  return is_inbound;
}

static int nm_mpi_rma_is_inbound_origin(nm_mpi_request_t*p_req_in, nm_mpi_window_t*p_win, int*is_inbound)
{
  if(MPI_WIN_FLAVOR_DYNAMIC != p_win->flavor)
    return (*is_inbound = 1, MPI_SUCCESS);
  const nm_tag_t tag_mask  = NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC | 0xFFFF00;
  nm_tag_t nm_tag = nm_mpi_rma_win_to_tag(p_win->win_ids[p_req_in->request_source]);
  nm_tag |= (nm_tag_t)((p_req_in->user_tag & ~0x080000F0) | 0x0400000F) & (NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0xFFFFFF);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_sr_request_t req;
  int source = p_req_in->request_source;
  nm_sr_recv_init(p_session, &req);
  nm_sr_recv_unpack_contiguous(p_session, &req, is_inbound, sizeof(int));
  nm_sr_recv_irecv(p_session, &req, p_req_in->gate, nm_tag, tag_mask);
  __sync_add_and_fetch(&p_win->access[source].nmsg, 1);
  nm_sr_rwait(p_session, &req);
  __sync_add_and_fetch(&p_win->access[source].completed, 1);
  return MPI_SUCCESS;
}

static int nm_mpi_rma_is_inbound_local(const MPI_Aint disp, const int count, const int rank,
				       const size_t size, const nm_mpi_window_t*p_win)
{
  int is_inbound = (MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor) || (disp + (count * size) <= p_win->size[rank]);
  return is_inbound;
}

static int nm_mpi_rma_pack_send_response(nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t *p_win = p_req->p_win;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  /* Send back response */
  nm_mpi_request_t*p_resp = nm_mpi_request_alloc();
  p_resp->sbuf            = p_req->sbuf;
  p_resp->count           = p_req->count;
  p_resp->gate            = NULL;
  p_resp->p_win           = p_win;
  p_resp->p_epoch         = &p_win->exposure[p_req->request_source];
  p_resp->user_tag        = p_req->user_tag | NM_MPI_TAG_PRIVATE_RMA_REQ_RESP; /**< keeps the seq */
  p_resp->p_datatype      = p_req->p_datatype;
  p_resp->request_source  = p_req->request_source;
  struct nm_data_s data;
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){
      .ptr = p_resp->rbuf, .p_datatype = p_resp->p_datatype, .count = p_resp->count
	});
  __sync_add_and_fetch(&p_win->exposure[p_req->request_source].nmsg, 1);
  err = nm_mpi_rma_isend(p_resp, &data, p_win, 0);
  nm_sr_request_set_ref(&p_resp->request_nmad, p_resp);
  nm_sr_request_monitor(p_session, &p_resp->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_flush_destroy_unlock_monitor);
  return err;
}

static int nm_mpi_rma_copy_send_response(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  if(p_req->gate == NULL){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_tag_t     nm_tag    = nm_mpi_rma_win_to_tag(p_win->win_ids[p_req->request_source]);
  int          dest      = nm_mpi_communicator_get_dest(p_win->p_comm, p_req->gate);
  assert(dest == p_req->request_source);
  nm_mpi_rma_request_datatype_t*p_req_info = p_req->rbuf;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_hashtable_get(p_req_info->datatype_hash);
  assert(p_datatype);
  struct nm_data_s data;
  void*p_base = MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor ? MPI_BOTTOM : p_win->p_base;
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){
      .ptr = p_base + p_req_info->offset, .p_datatype = p_datatype, .count = p_req_info->count
	});
  nm_len_t size = nm_data_size(&data);
  nm_mpi_request_t*p_resp = nm_mpi_request_alloc();
  p_resp->rbuf            = malloc(size);
  p_resp->gate            = p_req->gate;
  p_resp->count           = size;
  p_resp->p_win           = p_win;
  p_resp->p_comm          = p_win->p_comm;
  p_resp->p_epoch         = &p_win->exposure[dest];
  p_resp->user_tag        = p_req->user_tag | NM_MPI_TAG_PRIVATE_RMA_REQ_RESP; /**< keeps the seq */
  p_resp->p_datatype      = nm_mpi_datatype_get(MPI_BYTE);
  p_resp->request_type    = NM_MPI_REQUEST_SEND;
  p_resp->request_source  = dest;
  p_resp->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_resp->request_persistent_type = NM_MPI_REQUEST_ZERO;
  nm_tag |= (nm_tag_t)p_resp->user_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  __sync_add_and_fetch(&p_datatype->refcount, 1);
  /* Copy data */
  nm_mpi_spin_lock(&p_win->lock);
  nm_mpi_datatype_pack(p_resp->rbuf, p_win->p_base + p_req_info->offset, p_datatype, p_req_info->count);
  nm_mpi_spin_unlock(&p_win->lock);
  /* Send copied data from contiguous buffer */
  nm_sr_send_init(p_session, &p_resp->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_resp->request_nmad, p_resp->rbuf, p_resp->count);
  nm_sr_request_set_ref(&p_resp->request_nmad, p_resp);
  nm_sr_request_monitor(p_session, &p_resp->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_free_flush_destroy_unlock_monitor);
  err = nm_sr_send_isend(p_session, &p_resp->request_nmad, p_resp->gate, nm_tag);
  __sync_add_and_fetch(&p_win->exposure[dest].nmsg, 1);
  return err;
}

static int nm_mpi_rma_recv_accumulate(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  MPI_Op op = nm_mpi_rma_tag_to_mpi_op(p_req->user_tag);
  if(MPI_NO_OP == op) return err;
  nm_mpi_operator_t*p_operator = nm_mpi_operator_get(op);
  assert(p_operator);
  nm_mpi_rma_request_datatype_t*p_datatype_header = p_req->rbuf;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_hashtable_get(p_datatype_header->datatype_hash);
  assert(p_datatype);
  void*inbuf  = p_req->rbuf + sizeof(nm_mpi_rma_request_datatype_t);
  void*p_base = MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor ? MPI_BOTTOM : p_win->p_base;
  void*outbuf = p_base + p_datatype_header->offset;
  int count   = p_datatype_header->count;
  nm_mpi_spin_lock(&p_win->lock);
  nm_mpi_operator_apply(&inbuf, outbuf, count, p_datatype, p_operator);
  nm_mpi_spin_unlock(&p_win->lock);  
  return err;
}

static int  nm_mpi_rma_send_element(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  int dest = nm_mpi_communicator_get_dest(p_win->p_comm, p_req->gate);
  assert(dest == p_req->request_source);
  nm_mpi_rma_request_datatype_t*p_req_info = p_req->rbuf;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_hashtable_get(p_req_info->datatype_hash);
  assert(p_datatype);
  nm_mpi_request_t*p_resp = nm_mpi_request_alloc();
  p_resp->gate            = NULL;
  p_resp->count           = 1;
  p_resp->p_win           = p_win;
  p_resp->p_epoch         = &p_win->exposure[dest];
  p_resp->user_tag        = p_req->user_tag | NM_MPI_TAG_PRIVATE_RMA_REQ_RESP; /**< keeps the seq */
  p_resp->p_datatype      = p_datatype;
  p_resp->request_source  = dest;
  struct nm_data_s data;
  void*p_base = MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor ? MPI_BOTTOM : p_win->p_base;
  nm_mpi_spin_lock(&p_win->lock);
  memcpy(&p_resp->static_buf, p_base + p_req_info->offset, p_datatype->size);
  nm_mpi_spin_unlock(&p_win->lock);
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){
      .ptr = &p_resp->static_buf, .p_datatype = p_datatype, .count = 1
	});
  __sync_add_and_fetch(&p_win->exposure[dest].nmsg, 1);
  err = nm_mpi_rma_isend(p_resp, &data, p_win, 0);
  nm_sr_request_set_ref(&p_resp->request_nmad, p_resp);
  nm_sr_request_monitor(p_session, &p_resp->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_flush_destroy_unlock_monitor);
  return err;
}

static int  nm_mpi_rma_do_comp_and_swap(const nm_mpi_request_t*p_req, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  nm_mpi_rma_request_datatype_t*p_req_info = p_req->rbuf;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_hashtable_get(p_req_info->datatype_hash);
  assert(p_datatype);
  void*inbuf  = p_req->rbuf + sizeof(nm_mpi_rma_request_datatype_t);
  void*p_base = MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor ? MPI_BOTTOM : p_win->p_base;
  nm_mpi_spin_lock(&p_win->lock);
  if(!memcmp(inbuf, p_base + p_req_info->offset, p_datatype->size)) {
    memcpy(p_base + p_req_info->offset, inbuf + p_datatype->size, p_datatype->size);
  }
  nm_mpi_spin_unlock(&p_win->lock);
  return err;
}

/* ********************************************************* */

static int nm_mpi_put(const void *origin_addr, int origin_count, nm_mpi_datatype_t*p_origin_datatype,
		      int target_rank, MPI_Aint target_disp, int target_count,
		      nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
		      nm_mpi_request_t*p_req_out)
{
  int err = MPI_SUCCESS;
  int is_inbound;
  target_disp *= p_win->disp_unit[target_rank];
  if(!nm_mpi_rma_is_inbound_local(target_disp, target_count, target_rank, p_target_datatype->size, p_win)){
    return p_req_out->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor) {
    return nm_mpi_put_shm(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp,
			  target_count, p_target_datatype, p_win, p_req_out);
  }
  nm_mpi_win_send_datatype(p_target_datatype, target_rank, p_win);
  const uint16_t seq = nm_mpi_win_get_next_seq(p_win);
  p_req_out->sbuf           = origin_addr;
  p_req_out->gate           = NULL;
  p_req_out->count          = origin_count;
  p_req_out->p_win          = p_win;
  p_req_out->p_epoch        = &p_win->access[target_rank];
  p_req_out->user_tag       = nm_mpi_rma_create_usertag(seq, NM_MPI_TAG_PRIVATE_RMA_PUT);
  p_req_out->p_datatype     = p_origin_datatype;
  p_req_out->request_source = target_rank;
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  struct nm_data_s data;
  nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
      .header = (struct nm_data_mpi_datatype_header_s)
	{ .offset = target_disp, .datatype_hash = p_target_datatype->hash, .count = target_count },
	.data = (struct nm_data_mpi_datatype_s)
	   { .ptr = (void*)origin_addr, .p_datatype = p_origin_datatype, .count = origin_count }
    });
  err = nm_mpi_rma_isend(p_req_out, &data, p_win, sizeof(struct nm_data_mpi_datatype_header_s));
  nm_sr_request_set_ref(&p_req_out->request_nmad, p_req_out);
  nm_mpi_rma_is_inbound_origin(p_req_out, p_win, &is_inbound);
  if(!is_inbound) {
    p_req_out->request_type = NM_MPI_REQUEST_CANCELLED;
    err = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  nm_mpi_datatype_unlock(p_target_datatype);
  p_req_out->request_error = err;
  return err;
}

static int nm_mpi_get(void *origin_addr, int origin_count, nm_mpi_datatype_t*p_origin_datatype,
		      int target_rank, MPI_Aint target_disp, int target_count,
		      nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
		      nm_mpi_request_t*p_req_in)
{
  int err = MPI_SUCCESS;
  int is_inbound;
  target_disp *= p_win->disp_unit[target_rank];
  if(!nm_mpi_rma_is_inbound_local(target_disp, target_count, target_rank, p_target_datatype->size, p_win)){
    return p_req_in->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor) {
    return nm_mpi_get_shm(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp,
			  target_count, p_target_datatype, p_win, p_req_in);
  }
  nm_mpi_win_send_datatype(p_target_datatype, target_rank, p_win);
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  const nm_tag_t tag_mask  = NM_MPI_TAG_PRIVATE_RMA_MASK;
  nm_tag_t nm_tag;
  struct nm_data_s data;
  nm_session_t   p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const uint16_t      seq  = nm_mpi_win_get_next_seq(p_win);
  const int     target_win = p_win->win_ids[target_rank];
  /* Adding the handler for the target response to the request */
  nm_tag = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_REQ_RESP);/**< keeps the seq */
  p_req_in->gate                    = gate;
  p_req_in->rbuf                    = origin_addr;
  p_req_in->count                   = origin_count;
  p_req_in->p_win                   = p_win;
  p_req_in->p_comm                  = p_win->p_comm;
  p_req_in->p_epoch                 = &p_win->access[target_rank];
  p_req_in->user_tag                = (int)nm_tag;
  p_req_in->p_datatype              = p_origin_datatype;
  p_req_in->request_type            = NM_MPI_REQUEST_RECV;
  p_req_in->request_source          = target_rank;
  p_req_in->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req_in->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_origin_datatype->refcount, 1);
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){ .ptr = origin_addr, .p_datatype = p_origin_datatype, .count = origin_count });
  nm_sr_recv_init(p_session, &p_req_in->request_nmad);
  nm_sr_recv_unpack_data(p_session, &p_req_in->request_nmad, &data);
  nm_sr_request_set_ref(&p_req_in->request_nmad, p_req_in);
  err = nm_sr_recv_irecv(p_session, &p_req_in->request_nmad, p_req_in->gate, nm_tag, tag_mask);
  p_req_in->request_error = err;
  /* Send request */
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  nm_mpi_request_t*p_req_out = nm_mpi_request_alloc();
  nm_tag = nm_mpi_rma_create_tag(target_win, seq, NM_MPI_TAG_PRIVATE_RMA_GET_REQ);
  p_req_out->sbuf       = NULL;
  p_req_out->gate       = NULL;
  p_req_out->count      = 0;
  p_req_out->p_win      = p_win;
  p_req_out->p_epoch    = &p_win->access[target_rank];
  p_req_out->user_tag   = nm_tag;
  p_req_out->p_datatype = p_origin_datatype;
  p_req_out->request_source = target_rank;
  nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
      .header = (struct nm_data_mpi_datatype_header_s)
	{ .offset = target_disp, .datatype_hash = p_target_datatype->hash, .count = target_count },
	.data = (struct nm_data_mpi_datatype_s)
	   { .ptr = NULL, .p_datatype = p_target_datatype, .count = 0 }
    });
  err = nm_mpi_rma_isend(p_req_out, &data, p_win, sizeof(struct nm_data_mpi_datatype_header_s));
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  nm_mpi_rma_is_inbound_origin(p_req_out, p_win, &is_inbound);
  if(!is_inbound) {
    nm_sr_rcancel(p_session, &p_req_in->request_nmad);
    p_req_in->request_type = NM_MPI_REQUEST_CANCELLED;
    err = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  } else {
    __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  }
  nm_mpi_datatype_unlock(p_target_datatype);
  nm_sr_request_set_ref(&p_req_out->request_nmad, p_req_out);
  nm_sr_request_monitor(p_session, &p_req_out->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_free_flush_destroy_monitor);
  p_req_in->request_error = err;
  return err;
}

static int nm_mpi_accumulate(const void *origin_addr, int origin_count,
			     nm_mpi_datatype_t*p_origin_datatype, int target_rank,
			     MPI_Aint target_disp, int target_count,
			     nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
			     nm_mpi_window_t*p_win, nm_mpi_request_t*p_req_out)
{
  int err = MPI_SUCCESS;
  int is_inbound;
  target_disp *= p_win->disp_unit[target_rank];
  if(!nm_mpi_rma_is_inbound_local(target_disp, target_count, target_rank, p_target_datatype->size, p_win)){
    return p_req_out->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  if (MPI_WIN_FLAVOR_SHARED == p_win->flavor) {
    return nm_mpi_accumulate_shm(origin_addr, origin_count, p_origin_datatype, target_rank,
				 target_disp, target_count, p_target_datatype, op, p_win, p_req_out);
  }
  nm_mpi_win_send_datatype(p_target_datatype, target_rank, p_win);
  struct nm_data_s data;
  nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
      .header = (struct nm_data_mpi_datatype_header_s)
	{ .offset = target_disp, .datatype_hash = p_target_datatype->hash, .count = target_count },
	.data = (struct nm_data_mpi_datatype_s)
	   { .ptr = (void*)origin_addr, .p_datatype = p_origin_datatype, .count = origin_count }
    });
  const uint16_t seq = nm_mpi_win_get_next_seq(p_win);
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  p_req_out->sbuf           = origin_addr;
  p_req_out->gate           = NULL;
  p_req_out->count          = origin_count;
  p_req_out->p_win          = p_win;
  p_req_out->p_epoch        = &p_win->access[target_rank];
  p_req_out->user_tag       = nm_mpi_rma_create_usertag(seq, NM_MPI_TAG_PRIVATE_RMA_ACC);
  p_req_out->user_tag      |= nm_mpi_rma_mpi_op_to_tag(op);
  p_req_out->p_datatype     = p_origin_datatype;
  p_req_out->request_source = target_rank;
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  err = nm_mpi_rma_isend(p_req_out, &data, p_win, sizeof(struct nm_data_mpi_datatype_header_s));
  nm_sr_request_set_ref(&p_req_out->request_nmad, p_req_out);
  nm_mpi_rma_is_inbound_origin(p_req_out, p_win, &is_inbound);
  if(!is_inbound) {
    p_req_out->request_type = NM_MPI_REQUEST_CANCELLED;
    err = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  nm_mpi_datatype_unlock(p_target_datatype);
  p_req_out->request_error = err;
  return err;
}

static int nm_mpi_get_accumulate(const void *origin_addr, int origin_count,
				 nm_mpi_datatype_t*p_origin_datatype, void *result_addr,
				 int result_count, nm_mpi_datatype_t*p_result_datatype,
				 int target_rank, MPI_Aint target_disp, int target_count,
				 nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
				 nm_mpi_window_t*p_win, nm_mpi_request_t*p_req_in)
{
  int err = MPI_SUCCESS;
  int is_inbound;
  target_disp *= p_win->disp_unit[target_rank];
  if(!nm_mpi_rma_is_inbound_local(target_disp, target_count, target_rank, p_target_datatype->size, p_win)){
    return p_req_in->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor) {
    return nm_mpi_get_accumulate_shm(origin_addr, origin_count, p_origin_datatype, result_addr,
				     result_count, p_result_datatype, target_rank, target_disp,
				     target_count, p_target_datatype, op, p_win, p_req_in);
  }
  nm_mpi_win_send_datatype(p_target_datatype, target_rank, p_win);
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  const nm_tag_t tag_mask = NM_MPI_TAG_PRIVATE_RMA_MASK;
  nm_session_t  p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const uint16_t      seq = nm_mpi_win_get_next_seq(p_win);
  const int    target_win = p_win->win_ids[target_rank];
  const nm_tag_t   tag_op = nm_mpi_rma_mpi_op_to_tag(op);
  nm_tag_t nm_tag;
  struct nm_data_s data;
  /* Adding the handler for the target response to the request */
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_REQ_RESP);/**< keeps the seq */
  nm_tag |= tag_op;
  p_req_in->gate                    = gate;
  p_req_in->rbuf                    = result_addr;
  p_req_in->count                   = result_count;
  p_req_in->p_win                   = p_win;
  p_req_in->p_comm                  = p_win->p_comm;
  p_req_in->p_epoch                 = &p_win->access[target_rank];
  p_req_in->user_tag                = (int)nm_tag;
  p_req_in->p_datatype              = p_result_datatype;
  p_req_in->request_type            = NM_MPI_REQUEST_RECV;
  p_req_in->request_source          = target_rank;
  p_req_in->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req_in->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_result_datatype->refcount, 1);
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){ .ptr = result_addr, .p_datatype = p_result_datatype, .count = result_count });
  nm_sr_recv_init(p_session, &p_req_in->request_nmad);
  nm_sr_recv_unpack_data(p_session, &p_req_in->request_nmad, &data);
  nm_sr_request_set_ref(&p_req_in->request_nmad, p_req_in);
  err = nm_sr_recv_irecv(p_session, &p_req_in->request_nmad, gate, nm_tag, tag_mask);
  p_req_in->request_error = err;
  /* Send request */
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  nm_tag = nm_mpi_rma_create_tag(target_win, seq, NM_MPI_TAG_PRIVATE_RMA_GACC_REQ) | tag_op;
  if(MPI_NO_OP == op) origin_count = 0;
  nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
      .header = (struct nm_data_mpi_datatype_header_s)
	{ .offset = target_disp, .datatype_hash = p_target_datatype->hash, .count = target_count },
	.data = (struct nm_data_mpi_datatype_s)
	   { .ptr = (void*)origin_addr, .p_datatype = p_origin_datatype, .count = origin_count }
    });
  nm_mpi_request_t*p_req_out = nm_mpi_request_alloc();
  p_req_out->sbuf            = origin_addr;
  p_req_out->gate            = NULL;
  p_req_out->count           = origin_count;
  p_req_out->p_win           = p_win;
  p_req_out->p_epoch         = &p_win->access[target_rank];
  p_req_out->user_tag        = (int)nm_tag;
  p_req_out->p_datatype      = p_origin_datatype;
  p_req_out->request_source  = target_rank;
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  err = nm_mpi_rma_isend(p_req_out, &data, p_win, sizeof(struct nm_data_mpi_datatype_header_s));
  nm_mpi_rma_is_inbound_origin(p_req_out, p_win, &is_inbound);
  if(!is_inbound) {
    nm_sr_rcancel(p_session, &p_req_in->request_nmad);
    p_req_in->request_type = NM_MPI_REQUEST_CANCELLED;
    err = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  } else {
    __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  }
  nm_sr_request_set_ref(&p_req_out->request_nmad, p_req_out);
  nm_sr_request_monitor(p_session, &p_req_out->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_flush_destroy_monitor);
  nm_mpi_datatype_unlock(p_target_datatype);
  p_req_in->request_error = err;
  return err;
}

static int nm_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
			       nm_mpi_datatype_t*p_datatype, int target_rank, MPI_Aint target_disp,
			       MPI_Op op, nm_mpi_window_t*p_win, nm_mpi_request_t*p_req_in)
{
  int err = MPI_SUCCESS;
  int count = NULL == origin_addr && MPI_NO_OP == op ? 0 : 1;
  int is_inbound;
  target_disp *= p_win->disp_unit[target_rank];
  if(!nm_mpi_rma_is_inbound_local(target_disp, 1, target_rank, p_datatype->size, p_win)){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor) {
    return nm_mpi_fetch_and_op_shm(origin_addr, result_addr, count, p_datatype, target_rank, target_disp,
				   op, p_win, p_req_in);
  }
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return p_req_in->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  nm_session_t  p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_tag_t nm_tag, op_tag = nm_mpi_rma_mpi_op_to_tag(op);
  const nm_tag_t tag_mask = NM_MPI_TAG_PRIVATE_RMA_MASK;
  uint16_t seq = nm_mpi_win_get_next_seq(p_win);
  struct nm_data_s data;
  /* Adding the handler for the target response to the request */
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_REQ_RESP);/**< keeps the seq */
  nm_tag |= op_tag;
  p_req_in->gate                    = gate;
  p_req_in->rbuf                    = result_addr;
  p_req_in->count                   = 1;
  p_req_in->p_win                   = p_win;
  p_req_in->p_comm                  = p_win->p_comm;
  p_req_in->p_epoch                 = &p_win->access[target_rank];
  p_req_in->user_tag                = (int)nm_tag;
  p_req_in->p_datatype              = p_datatype;
  p_req_in->request_type            = NM_MPI_REQUEST_RECV;
  p_req_in->request_source          = target_rank;
  p_req_in->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req_in->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_datatype->refcount, 1);
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){
      .ptr = result_addr, .p_datatype = p_datatype, .count = 1
	});
  nm_sr_recv_init(p_session, &p_req_in->request_nmad);
  nm_sr_recv_unpack_data(p_session, &p_req_in->request_nmad, &data);
  nm_sr_request_set_ref(&p_req_in->request_nmad, p_req_in);
  err = nm_sr_recv_irecv(p_session, &p_req_in->request_nmad, p_req_in->gate, nm_tag, tag_mask);
  p_req_in->request_error = err;
  /* Send request */
  nm_mpi_request_t*p_req_out = nm_mpi_request_alloc();
  p_req_out->sbuf            = origin_addr;
  p_req_out->gate            = NULL;
  p_req_out->count           = 1;
  p_req_out->p_win           = p_win;
  p_req_out->p_epoch         = &p_win->access[target_rank];
  p_req_out->user_tag        = nm_mpi_rma_create_usertag(seq, NM_MPI_TAG_PRIVATE_RMA_FAO_REQ) | op_tag;
  p_req_out->p_datatype      = p_datatype;
  p_req_out->request_source  = target_rank;
  nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
      .header = (struct nm_data_mpi_datatype_header_s)
	{ .offset = target_disp, .datatype_hash = p_datatype->hash, .count = 1 },
	.data = (struct nm_data_mpi_datatype_s)
	   { .ptr = (void*)origin_addr, .p_datatype = p_datatype, .count = count }
    });
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  err = nm_mpi_rma_isend(p_req_out, &data, p_win, sizeof(struct nm_data_mpi_datatype_header_s));
  nm_mpi_rma_is_inbound_origin(p_req_out, p_win, &is_inbound);
  nm_sr_request_set_ref(&p_req_out->request_nmad, p_req_out);
  nm_sr_request_monitor(p_session, &p_req_out->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_flush_destroy_monitor);
  return p_req_in->request_error = (is_inbound ? err : MPI_ERR_RMA_RANGE);
}

static int nm_mpi_compare_and_swap(const void *origin_addr, const void *compare_addr,
				   void *result_addr, nm_mpi_datatype_t*p_datatype, int target_rank,
				   MPI_Aint target_disp, nm_mpi_window_t*p_win, nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  int is_inbound;
  target_disp *= p_win->disp_unit[target_rank];
  if(!nm_mpi_rma_is_inbound_local(target_disp, 1, target_rank, p_datatype->size, p_win)){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_RANGE);
  }
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor) {
    return nm_mpi_compare_and_swap_shm(origin_addr, compare_addr, result_addr, p_datatype, target_rank,
				       target_disp, p_win, p_req);
  }
  nm_tag_t nm_tag;
  struct nm_data_s data;
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  nm_session_t  p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const nm_tag_t tag_mask = NM_MPI_TAG_PRIVATE_RMA_MASK;
  uint16_t seq = nm_mpi_win_get_next_seq(p_win);
  /* Adding the handler for the target response to the request */
  nm_tag = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_REQ_RESP);/**< keeps the seq */
  p_req->gate                    = gate;
  p_req->rbuf                    = result_addr;
  p_req->count                   = 1;
  p_req->p_win                   = p_win;
  p_req->p_comm                  = p_win->p_comm;
  p_req->p_epoch                 = &p_win->access[target_rank];
  p_req->user_tag                = (int)nm_tag;
  p_req->p_datatype              = p_datatype;
  p_req->request_type            = NM_MPI_REQUEST_RECV;
  p_req->request_source          = target_rank;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_datatype->refcount, 1);
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){ .ptr = result_addr, .p_datatype = p_datatype, .count = 1 });
  nm_sr_recv_init(p_session, &p_req->request_nmad);
  nm_sr_recv_unpack_data(p_session, &p_req->request_nmad, &data);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  err = nm_sr_recv_irecv(p_session, &p_req->request_nmad, p_req->gate, nm_tag, tag_mask);
  p_req->request_error = err;
  /* Send request */
  nm_mpi_request_t*p_req_out = nm_mpi_request_alloc();
  p_req_out->rbuf            = malloc(2 * p_datatype->size);
  p_req_out->gate            = NULL;
  p_req_out->count           = 2;
  p_req_out->p_win           = p_win;
  p_req_out->p_epoch         = &p_win->access[target_rank];
  p_req_out->user_tag        = nm_mpi_rma_create_usertag(seq, NM_MPI_TAG_PRIVATE_RMA_CAS_REQ);
  p_req_out->p_datatype      = p_datatype;
  p_req_out->request_source  = target_rank;
  memcpy(&((char*)p_req_out->rbuf)[0], compare_addr, p_datatype->size);
  memcpy(&((char*)p_req_out->rbuf)[p_datatype->size], origin_addr, p_datatype->size);
  nm_data_mpi_datatype_wrapper_set(&data, (struct nm_data_mpi_datatype_wrapper_s) {
      .header = (struct nm_data_mpi_datatype_header_s)
	{ .offset = target_disp, .datatype_hash = p_datatype->hash, .count = 1 },
	.data = (struct nm_data_mpi_datatype_s)
	   { .ptr = p_req_out->rbuf, .p_datatype = p_datatype, .count = 2 }
    });
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  err = nm_mpi_rma_isend(p_req_out, &data, p_win, sizeof(struct nm_data_mpi_datatype_header_s));
  nm_mpi_rma_is_inbound_origin(p_req_out, p_win, &is_inbound);
  nm_sr_request_set_ref(&p_req_out->request_nmad, p_req_out);
  nm_sr_request_monitor(p_session, &p_req_out->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_free_flush_destroy_monitor);
  return p_req->request_error = (is_inbound ? err : MPI_ERR_RMA_RANGE);
}

/* ********************************************************* */

int mpi_put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
	    int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
	    MPI_Win win)
{
  int err = MPI_SUCCESS;
  if(0 > origin_count || 0 > target_count)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_ACTIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING)
       & p_win->access[target_rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && origin_count) || (NULL == p_target_datatype && target_count))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if(!p_origin_datatype->committed || !p_target_datatype->committed)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  err = nm_mpi_put(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp,
		   target_count, p_target_datatype, p_win, p_req);
  if(MPI_SUCCESS == err) {
    nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_rma_request_flush_destroy_monitor);
  }
  return err;
}

int mpi_get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
	    int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
	    MPI_Win win)
{
  int err = MPI_SUCCESS;
  if(0 > origin_count || 0 > target_count)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_ACTIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING)
       & p_win->access[target_rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && origin_count) || (NULL == p_target_datatype && target_count))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if(!p_origin_datatype->committed || !p_target_datatype->committed)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int    comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t     p_session  = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_mpi_request_t*p_req_resp = nm_mpi_request_alloc();
  err = nm_mpi_get(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp,
		   target_count, p_target_datatype, p_win, p_req_resp);
  if(MPI_SUCCESS == err) {
    nm_sr_request_monitor(p_session, &p_req_resp->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_rma_request_flush_destroy_monitor);
  } else if (MPI_ERR_RMA_RANGE == err) {
    nm_mpi_request_free(p_req_resp);
  }
  return err;
}

int mpi_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
		   int target_rank, MPI_Aint target_disp, int target_count,
		   MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  int err = MPI_SUCCESS;
  if(0 > origin_count || 0 > target_count)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  if(MPI_OP_NULL == op || MPI_NO_OP == op || _NM_MPI_OP_OFFSET <= op)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_OP);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_ACTIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING)
       & p_win->access[target_rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && origin_count) || (NULL == p_target_datatype && target_count))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if(!p_origin_datatype->committed || !p_target_datatype->committed)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  err = nm_mpi_accumulate(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp,
			  target_count, p_target_datatype, op, p_win, p_req);
  if(MPI_SUCCESS == err) {
    nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_rma_request_flush_destroy_monitor);
  }
  return err;
}

int mpi_get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
		       void *result_addr, int result_count, MPI_Datatype result_datatype,
		       int target_rank, MPI_Aint target_disp, int target_count,
		       MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  int err = MPI_SUCCESS;
  if(0 > origin_count || 0 > target_count || 0 > result_count)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  if(MPI_OP_NULL == op || _NM_MPI_OP_OFFSET <= op)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_OP);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_ACTIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING)
       & p_win->access[target_rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_result_datatype = nm_mpi_datatype_get(result_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && MPI_NO_OP != op)
     || (NULL == p_result_datatype && result_count) || (NULL == p_target_datatype && target_count))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if((p_origin_datatype && !p_origin_datatype->committed)
     || !p_result_datatype->committed || !p_target_datatype->committed)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t     p_session  = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_mpi_request_t*p_req_resp = nm_mpi_request_alloc();
  if(MPI_NO_OP == op) {
    err = nm_mpi_get(result_addr, result_count, p_result_datatype, target_rank, target_disp,
		     target_count, p_target_datatype, p_win, p_req_resp);
  } else {
    err = nm_mpi_get_accumulate(origin_addr, origin_count, p_origin_datatype, result_addr,
				result_count, p_result_datatype, target_rank, target_disp,
				target_count, p_target_datatype, op, p_win, p_req_resp);
  }
  if(MPI_SUCCESS == err) {
    nm_sr_request_monitor(p_session, &p_req_resp->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_rma_request_flush_destroy_monitor);
  } else if (MPI_ERR_RMA_RANGE == err) {
    nm_mpi_request_free(p_req_resp);
  }
  return err;
}

int mpi_fetch_and_op(const void *origin_addr, void *result_addr,
		     MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
		     MPI_Op op, MPI_Win win)
{
  int err = MPI_SUCCESS;
  if(MPI_OP_NULL == op || _NM_MPI_OP_OFFSET <= op)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_OP);
  if(_NM_MPI_DATATYPE_OFFSET <= datatype)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_ACTIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING)
       & p_win->access[target_rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(NULL == p_datatype || !p_datatype->committed)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t  p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_mpi_request_t*p_req_in = nm_mpi_request_alloc();
  err = nm_mpi_fetch_and_op(origin_addr, result_addr, p_datatype, target_rank, target_disp, op,
			    p_win, p_req_in);
  if(MPI_SUCCESS == err) {
    __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
    nm_sr_request_monitor(p_session, &p_req_in->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_rma_request_flush_destroy_monitor);
  } else {
    nm_sr_rcancel(p_session, &p_req_in->request_nmad);
    p_req_in->request_type = NM_MPI_REQUEST_CANCELLED;
    nm_mpi_request_free(p_req_in);
  }
  return err;
}

int mpi_compare_and_swap(const void *origin_addr, const void *compare_addr,
			 void *result_addr, MPI_Datatype datatype, int target_rank,
			 MPI_Aint target_disp, MPI_Win win)
{
  int err = MPI_SUCCESS;
  if(_NM_MPI_DATATYPE_OFFSET <= datatype)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  switch(datatype) {
  case MPI_FLOAT: case MPI_DOUBLE: case MPI_REAL: case MPI_DOUBLE_PRECISION: case MPI_LONG_DOUBLE:
  case MPI_REAL4: case MPI_REAL8: case MPI_REAL16: case MPI_COMPLEX: case MPI_C_COMPLEX:
  case MPI_C_DOUBLE_COMPLEX: case MPI_C_LONG_DOUBLE_COMPLEX: case MPI_DOUBLE_COMPLEX:
  case MPI_COMPLEX4: case MPI_COMPLEX8: case MPI_COMPLEX16: case MPI_COMPLEX32: case MPI_2REAL:
  case MPI_2DOUBLE_PRECISION: case MPI_2INTEGER: case MPI_FLOAT_INT: case MPI_DOUBLE_INT:
  case MPI_LONG_INT: case MPI_2INT: case MPI_SHORT_INT: case MPI_LONG_DOUBLE_INT:
    /* 
       case MPI_C_FLOAT_COMPLEX: case MPI_CXX_FLOAT_COMPLEX: case MPI_CXX_DOUBLE_COMPLEX:
       case MPI_CXX_LONG_DOUBLE_COMPLEX: 
    */
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
    break;
  }
  nm_mpi_window_t  *p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_ACTIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING)
       & p_win->access[target_rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(NULL == p_datatype || !p_datatype->committed)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  nm_session_t  p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_mpi_request_t*p_req_in = nm_mpi_request_alloc();
  err = nm_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, p_datatype, target_rank,
				target_disp, p_win, p_req_in);
  if(MPI_ERR_RMA_RANGE == err) {
    nm_sr_rcancel(p_session, &p_req_in->request_nmad);
    p_req_in->request_type = NM_MPI_REQUEST_CANCELLED;
    nm_mpi_request_free(p_req_in);
  } else {
    __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
    nm_sr_request_monitor(p_session, &p_req_in->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_rma_request_flush_destroy_monitor);
  }
  return err;  
}

int mpi_rput(const void *origin_addr, int origin_count,
	     MPI_Datatype origin_datatype, int target_rank,
	     MPI_Aint target_disp, int target_count,
	     MPI_Datatype target_datatype, MPI_Win win,
	     MPI_Request *request)
{
  int err = MPI_SUCCESS;
  if(!request)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_REQUEST);
  nm_mpi_request_t*p_req_out = nm_mpi_request_alloc();
  *request = p_req_out->id;
  assert(*request);
  if(0 > origin_count || 0 > target_count)
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return p_req_out->request_error = MPI_ERR_WIN;
  if(!((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING) & p_win->access[target_rank].mode))
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && origin_count) || (NULL == p_target_datatype && target_count))
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if(!p_origin_datatype->committed || !p_target_datatype->committed)
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return p_req_out->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  p_req_out->p_win       = p_win;
  p_req_out->p_epoch     = &p_win->access[target_rank];
  err = nm_mpi_put(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp, target_count,
		   p_target_datatype, p_win, p_req_out);
  p_req_out->request_error = err;
  if(MPI_SUCCESS == err) {
    nm_sr_request_monitor(p_session, &p_req_out->request_nmad, NM_SR_EVENT_SEND_COMPLETED,
			  &nm_mpi_rma_request_flush_monitor);
  }
  return err;
}

int mpi_rget(void *origin_addr, int origin_count,
	     MPI_Datatype origin_datatype, int target_rank,
	     MPI_Aint target_disp, int target_count,
	     MPI_Datatype target_datatype, MPI_Win win,
	     MPI_Request *request)
{
  int err = MPI_SUCCESS;
  if(!request)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_REQUEST);
  nm_mpi_request_t*p_req_resp = nm_mpi_request_alloc();
  *request = p_req_resp->id;
  assert(*request);
  if(0 > origin_count || 0 > target_count)
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return p_req_resp->request_error = MPI_ERR_WIN;
  if(!((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING) & p_win->access[target_rank].mode))
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && origin_count) || (NULL == p_target_datatype && target_count))
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if(!p_origin_datatype->committed || !p_target_datatype->committed)
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  p_req_resp->p_win      = p_win;
  p_req_resp->p_epoch    = &p_win->access[target_rank];
  err = nm_mpi_get(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp, target_count,
		   p_target_datatype, p_win, p_req_resp);
  p_req_resp->request_error = err;
  if(MPI_SUCCESS == err) {
    nm_sr_request_set_ref(&p_req_resp->request_nmad, p_req_resp);
    nm_sr_request_monitor(p_session, &p_req_resp->request_nmad, NM_SR_EVENT_RECV_COMPLETED,
			  &nm_mpi_rma_request_flush_monitor);
  }
  return err;
}

int mpi_raccumulate(const void *origin_addr, int origin_count,
		    MPI_Datatype origin_datatype, int target_rank,
		    MPI_Aint target_disp, int target_count,
		    MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
		    MPI_Request *request)
{
  int err = MPI_SUCCESS;
  if(!request)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_REQUEST);
  nm_mpi_request_t*p_req_out = nm_mpi_request_alloc();
  *request = p_req_out->id;
  assert(*request);
  if(0 > origin_count || 0 > target_count)
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  if(MPI_OP_NULL == op || MPI_NO_OP == op || _NM_MPI_OP_OFFSET <= op)
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_OP);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return p_req_out->request_error = MPI_ERR_WIN;
  if(!((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING) & p_win->access[target_rank].mode))
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && origin_count) || (NULL == p_target_datatype && target_count))
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if(!p_origin_datatype->committed || !p_target_datatype->committed)
    return p_req_out->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return p_req_out->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  p_req_out->p_win       = p_win;
  p_req_out->p_epoch     = &p_win->access[target_rank];
  err = nm_mpi_accumulate(origin_addr, origin_count, p_origin_datatype, target_rank, target_disp,
			  target_count, p_target_datatype, op, p_win, p_req_out);
  p_req_out->request_error = err;
  if(MPI_SUCCESS == err) {
    nm_sr_request_set_ref(&p_req_out->request_nmad, p_req_out);
    nm_sr_request_monitor(p_session, &p_req_out->request_nmad, NM_SR_EVENT_SEND_COMPLETED,
			  &nm_mpi_rma_request_flush_monitor);
  }
  return err;
}

int mpi_rget_accumulate(const void *origin_addr, int origin_count,
			MPI_Datatype origin_datatype, void *result_addr,
			int result_count, MPI_Datatype result_datatype,
			int target_rank, MPI_Aint target_disp, int target_count,
			MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
			MPI_Request *request)
{
  int err = MPI_SUCCESS;
  if(!request)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_REQUEST);
  nm_mpi_request_t*p_req_resp = nm_mpi_request_alloc();
  *request = p_req_resp->id;
  assert(*request);
  if(0 > origin_count || 0 > target_count || 0 > result_count)
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_COUNT);
  if(MPI_OP_NULL == op || _NM_MPI_OP_OFFSET <= op)
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_OP);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return p_req_resp->request_error = MPI_ERR_WIN;
  if(!((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING) & p_win->access[target_rank].mode))
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  nm_mpi_datatype_t*p_origin_datatype = nm_mpi_datatype_get(origin_datatype);
  nm_mpi_datatype_t*p_result_datatype = nm_mpi_datatype_get(result_datatype);
  nm_mpi_datatype_t*p_target_datatype = nm_mpi_datatype_get(target_datatype);
  if((NULL == p_origin_datatype && (MPI_NO_OP != op || origin_datatype))
     || (NULL == p_result_datatype && result_count) || (NULL == p_target_datatype && target_count))
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  if(!p_origin_datatype->committed || !p_result_datatype->committed || !p_target_datatype->committed)
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(win, MPI_ERR_TYPE);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(0 > target_rank || comm_size <= target_rank || MPI_ANY_SOURCE == target_rank)
    return p_req_resp->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  p_req_resp->p_win      = p_win;
  p_req_resp->p_epoch    = &p_win->access[target_rank];
  err = nm_mpi_get_accumulate(origin_addr, origin_count, p_origin_datatype, result_addr,
			      result_count, p_result_datatype, target_rank, target_disp,
			      target_count, p_target_datatype, op, p_win, p_req_resp);
  p_req_resp->request_error = err;
  if(MPI_SUCCESS == err) {
    nm_sr_request_set_ref(&p_req_resp->request_nmad, p_req_resp);
    nm_sr_request_monitor(p_session, &p_req_resp->request_nmad, NM_SR_EVENT_RECV_COMPLETED,
			  &nm_mpi_rma_request_flush_monitor);
  }
  return err;
}

/* ********************************************************* */
/* ** Shared memory RMA operations ************************* */

static int nm_mpi_put_shm(const void *origin_addr, int origin_count, nm_mpi_datatype_t*p_origin_datatype,
			  int target_rank, MPI_Aint target_disp, int target_count,
			  nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
			  nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return p_req->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  /* Prepare request informations */
  void*p_base = ((void**)p_win->p_base)[target_rank] + target_disp;
  const uint16_t seq     = nm_mpi_win_get_next_seq(p_win);
  nm_tag_t nm_tag;
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_PUT);
  nm_tag  = nm_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  nm_tag |= nm_mpi_rma_win_to_tag(p_win->win_ids[target_rank]);
  nm_mpi_rma_request_info_t*p_info_req = malloc(sizeof(nm_mpi_rma_request_info_t));
  p_info_req->p_origin_addr     = origin_addr;
  p_info_req->p_origin_datatype = p_origin_datatype;
  p_info_req->origin_count      = origin_count;
  p_info_req->p_target_addr     = p_base;
  p_info_req->p_target_datatype = p_target_datatype;
  p_info_req->target_count      = target_count;
  p_req->rbuf           = p_info_req;
  p_req->gate           = gate;
  p_req->count          = origin_count;
  p_req->p_win          = p_win;
  p_req->p_epoch        = &p_win->access[target_rank];
  p_req->user_tag       = nm_tag;
  p_req->p_datatype     = p_origin_datatype;
  p_req->request_source = target_rank;
  p_req->request_type            = NM_MPI_REQUEST_SEND;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_origin_datatype->refcount, 1);
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  gate = nm_mpi_communicator_get_gate(p_win->p_comm, p_win->myrank);
  /* Enqueue or start immediately operation */
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_sr_send_init(p_session, &p_req->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  err = nm_sr_send_dest(p_session, &p_req->request_nmad, gate, nm_tag);
  p_req->request_error = err;
  nm_mpi_win_enqueue_pending(p_req, p_win);
  return err;
}

static int nm_mpi_get_shm(void *origin_addr, int origin_count, nm_mpi_datatype_t*p_origin_datatype,
			  int target_rank, MPI_Aint target_disp, int target_count,
			  nm_mpi_datatype_t*p_target_datatype, nm_mpi_window_t*p_win,
			  nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return p_req->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  /* Prepare request informations */
  void*p_base = ((void**)p_win->p_base)[target_rank] + target_disp;
  const uint16_t seq     = nm_mpi_win_get_next_seq(p_win);
  nm_tag_t nm_tag;
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_GET_REQ);
  nm_tag  = nm_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  nm_tag |= nm_mpi_rma_win_to_tag(p_win->win_ids[target_rank]);
  nm_mpi_rma_request_info_t*p_info_req = malloc(sizeof(nm_mpi_rma_request_info_t));
  p_info_req->p_result_addr     = origin_addr;
  p_info_req->p_result_datatype = p_origin_datatype;
  p_info_req->result_count      = origin_count;
  p_info_req->p_target_addr     = p_base;
  p_info_req->p_target_datatype = p_target_datatype;
  p_info_req->target_count      = target_count;
  p_req->rbuf           = p_info_req;
  p_req->gate           = gate;
  p_req->count          = origin_count;
  p_req->p_win          = p_win;
  p_req->p_epoch        = &p_win->access[target_rank];
  p_req->user_tag       = nm_tag;
  p_req->p_datatype     = p_origin_datatype;
  p_req->request_source = target_rank;
  p_req->request_type            = NM_MPI_REQUEST_SEND;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_origin_datatype->refcount, 1);
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  gate = nm_mpi_communicator_get_gate(p_win->p_comm, p_win->myrank);
  /* Enqueue or start immediately operation */
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_sr_send_init(p_session, &p_req->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  err = nm_sr_send_dest(p_session, &p_req->request_nmad, gate, nm_tag);
  p_req->request_error = err;
  nm_mpi_win_enqueue_pending(p_req, p_win);
  return err;
}

static int nm_mpi_accumulate_shm(const void *origin_addr, int origin_count,
				 nm_mpi_datatype_t*p_origin_datatype, int target_rank,
				 MPI_Aint target_disp, int target_count,
				 nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
				 nm_mpi_window_t*p_win, nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return p_req->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  /* Prepare request informations */
  void*p_base = ((void**)p_win->p_base)[target_rank] + target_disp;
  const uint16_t seq     = nm_mpi_win_get_next_seq(p_win);
  nm_tag_t nm_tag, op_tag = nm_mpi_rma_mpi_op_to_tag(op);
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_ACC);
  nm_tag  = nm_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  nm_tag |= nm_mpi_rma_win_to_tag(p_win->win_ids[target_rank]);
  nm_tag |= op_tag;
  nm_mpi_rma_request_info_t*p_info_req = malloc(sizeof(nm_mpi_rma_request_info_t));
  p_info_req->p_origin_addr     = origin_addr;
  p_info_req->p_origin_datatype = p_origin_datatype;
  p_info_req->origin_count      = origin_count;
  p_info_req->p_target_addr     = p_base;
  p_info_req->p_target_datatype = p_target_datatype;
  p_info_req->target_count      = target_count;
  p_req->rbuf           = p_info_req;
  p_req->gate           = gate;
  p_req->count          = origin_count;
  p_req->p_win          = p_win;
  p_req->p_epoch        = &p_win->access[target_rank];
  p_req->user_tag       = nm_tag;
  p_req->p_datatype     = p_origin_datatype;
  p_req->request_source = target_rank;
  p_req->request_type            = NM_MPI_REQUEST_SEND;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_origin_datatype->refcount, 1);
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  gate = nm_mpi_communicator_get_gate(p_win->p_comm, p_win->myrank);
  /* Enqueue or start immediately operation */
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_sr_send_init(p_session, &p_req->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  err = nm_sr_send_dest(p_session, &p_req->request_nmad, gate, nm_tag);
  p_req->request_error = err;
  nm_mpi_win_enqueue_pending(p_req, p_win);
  return err;
}

static int nm_mpi_get_accumulate_shm(const void *origin_addr, int origin_count,
				     nm_mpi_datatype_t*p_origin_datatype, void *result_addr,
				     int result_count, nm_mpi_datatype_t*p_result_datatype,
				     int target_rank, MPI_Aint target_disp, int target_count,
				     nm_mpi_datatype_t*p_target_datatype, MPI_Op op,
				     nm_mpi_window_t*p_win, nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return p_req->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  /* Prepare request informations */
  void*p_base = ((void**)p_win->p_base)[target_rank] + target_disp;
  const uint16_t seq     = nm_mpi_win_get_next_seq(p_win);
  nm_tag_t nm_tag, op_tag = nm_mpi_rma_mpi_op_to_tag(op);
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_GACC_REQ);
  nm_tag  = nm_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  nm_tag |= nm_mpi_rma_win_to_tag(p_win->win_ids[target_rank]);
  nm_tag |= op_tag;
  nm_mpi_rma_request_info_t*p_info_req = malloc(sizeof(nm_mpi_rma_request_info_t));
  p_info_req->p_origin_addr     = origin_addr;
  p_info_req->p_origin_datatype = p_origin_datatype;
  p_info_req->origin_count      = origin_count;
  p_info_req->p_target_addr     = p_base;
  p_info_req->p_target_datatype = p_target_datatype;
  p_info_req->target_count      = target_count;
  p_info_req->p_result_addr     = result_addr;
  p_info_req->p_result_datatype = p_result_datatype;
  p_info_req->result_count      = result_count;
  p_req->rbuf           = p_info_req;
  p_req->gate           = gate;
  p_req->count          = origin_count;
  p_req->p_win          = p_win;
  p_req->p_epoch        = &p_win->access[target_rank];
  p_req->user_tag       = nm_tag;
  p_req->p_datatype     = p_origin_datatype;
  p_req->request_source = target_rank;
  p_req->request_type            = NM_MPI_REQUEST_SEND;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  __sync_add_and_fetch(&p_origin_datatype->refcount, 1);
  __sync_add_and_fetch(&p_target_datatype->refcount, 1);
  __sync_add_and_fetch(&p_result_datatype->refcount, 1);
  __sync_add_and_fetch(&p_win->access[target_rank].nmsg, 1);
  gate = nm_mpi_communicator_get_gate(p_win->p_comm, p_win->myrank);
  /* Enqueue or start immediately operation */
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_sr_send_init(p_session, &p_req->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  err = nm_sr_send_dest(p_session, &p_req->request_nmad, gate, nm_tag);
  p_req->request_error = err;
  nm_mpi_win_enqueue_pending(p_req, p_win);
  return err;
}

static int nm_mpi_fetch_and_op_shm(const void *origin_addr, void *result_addr, int count,
				   nm_mpi_datatype_t*p_datatype, int target_rank, MPI_Aint target_disp,
				   MPI_Op op, nm_mpi_window_t*p_win, nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  /* Prepare request informations */
  void*p_base = ((void**)p_win->p_base)[target_rank] + target_disp;
  const uint16_t seq = nm_mpi_win_get_next_seq(p_win);
  nm_tag_t nm_tag, op_tag = nm_mpi_rma_mpi_op_to_tag(op);
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_FAO_REQ);
  nm_tag  = nm_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  nm_tag |= nm_mpi_rma_win_to_tag(p_win->win_ids[target_rank]);
  nm_tag |= op_tag;
  nm_mpi_rma_request_info_t*p_info_req = malloc(sizeof(nm_mpi_rma_request_info_t));
  p_info_req->p_origin_addr     = origin_addr;
  p_info_req->p_origin_datatype = p_datatype;
  p_info_req->origin_count      = count;
  p_info_req->p_target_addr     = p_base;
  p_info_req->p_target_datatype = p_datatype;
  p_info_req->target_count      = 1;
  p_info_req->p_result_addr     = result_addr;
  p_info_req->p_result_datatype = p_datatype;
  p_info_req->result_count      = 1;
  p_req->rbuf           = p_info_req;
  p_req->gate           = gate;
  p_req->count          = 1;
  p_req->p_win          = p_win;
  p_req->p_epoch        = &p_win->access[target_rank];
  p_req->user_tag       = nm_tag;
  p_req->p_datatype     = p_datatype;
  p_req->request_source = target_rank;
  __sync_add_and_fetch(&p_datatype->refcount, 2);/* Once as origin_datatype, second as result_datatype */
  gate = nm_mpi_communicator_get_gate(p_win->p_comm, p_win->myrank);
  /* Enqueue or start immediately operation */
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_sr_send_init(p_session, &p_req->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
  err = nm_sr_send_dest(p_session, &p_req->request_nmad, gate, nm_tag);
  p_req->request_error = err;
  nm_mpi_win_enqueue_pending(p_req, p_win);
  return err;
}

static int nm_mpi_compare_and_swap_shm(const void *origin_addr, const void *compare_addr,
				       void *result_addr, nm_mpi_datatype_t*p_datatype, int target_rank,
				       MPI_Aint target_disp, nm_mpi_window_t*p_win,
				       nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(gate == NULL){
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  }
  /* Prepare request informations */
  void*p_base = ((void**)p_win->p_base)[target_rank] + target_disp;
  const uint16_t seq = nm_mpi_win_get_next_seq(p_win);
  nm_tag_t nm_tag;
  nm_tag  = nm_mpi_rma_create_tag(p_win->id, seq, NM_MPI_TAG_PRIVATE_RMA_CAS_REQ);
  nm_tag  = nm_tag & NM_MPI_TAG_PRIVATE_RMA_MASK_USER;
  nm_tag |= nm_mpi_rma_win_to_tag(p_win->win_ids[target_rank]);
  nm_mpi_rma_request_info_t*p_info_req = malloc(sizeof(nm_mpi_rma_request_info_t));
  p_info_req->p_origin_addr     = origin_addr;
  p_info_req->p_origin_datatype = p_datatype;
  p_info_req->origin_count      = 1;
  p_info_req->p_target_addr     = p_base;
  p_info_req->p_target_datatype = p_datatype;
  p_info_req->target_count      = 1;
  p_info_req->p_result_addr     = result_addr;
  p_info_req->p_result_datatype = p_datatype;
  p_info_req->result_count      = 1;
  p_info_req->p_comp_addr       = compare_addr;
  p_req->rbuf           = p_info_req;
  p_req->gate           = gate;
  p_req->count          = 1;
  p_req->p_win          = p_win;
  p_req->p_epoch        = &p_win->access[target_rank];
  p_req->user_tag       = nm_tag;
  p_req->p_datatype     = p_datatype;
  p_req->request_source = target_rank;
  __sync_add_and_fetch(&p_datatype->refcount, 2);/* Once as origin_datatype, second as result_datatype */
  gate = nm_mpi_communicator_get_gate(p_win->p_comm, p_win->myrank);
  /* Enqueue or start immediately operation */
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_sr_send_init(p_session, &p_req->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  err = nm_sr_send_dest(p_session, &p_req->request_nmad, gate, nm_tag);
  p_req->request_error = err;
  nm_mpi_win_enqueue_pending(p_req, p_win);
  return err;
}

__PUK_SYM_INTERNAL
void nm_mpi_rma_recv_shm(const nm_sr_event_info_t*p_info, nm_mpi_window_t*p_win)
{
  nm_session_t p_session = p_info->recv_unexpected.p_session;
  const size_t       len = p_info->recv_unexpected.len;
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  p_req->rbuf                    = malloc(len);
  p_req->p_win                   = p_win;
  p_req->p_comm                  = p_win->p_comm;
  p_req->user_tag                = p_info->recv_unexpected.tag;
  p_req->p_datatype              = nm_mpi_datatype_get(MPI_BYTE);
  p_req->request_type            = NM_MPI_REQUEST_RECV;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  nm_sr_recv_init(p_session, &p_req->request_nmad);
  nm_sr_recv_unpack_contiguous(p_session, &p_req->request_nmad, p_req->rbuf, len);
  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
  nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_rma_request_free_destroy_monitor);
  nm_sr_recv_match_event(p_session, &p_req->request_nmad, p_info);
  nm_sr_recv_post(p_session, &p_req->request_nmad);
}

__PUK_SYM_INTERNAL
void nm_mpi_rma_handle_passive_shm(nm_mpi_request_t*p_req)
{
  nm_mpi_window_t *p_win = p_req->p_win;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_tag_t tag;
  nm_sr_request_get_tag(&p_req->request_nmad, &tag);
  assert(tag);
  MPI_Op op = nm_mpi_rma_tag_to_mpi_op(p_req->user_tag);
  nm_mpi_rma_request_info_t*p_info_req = p_req->rbuf;
  struct nm_data_s data;
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){
      .ptr = p_info_req->p_target_addr, .p_datatype = p_info_req->p_target_datatype,
	.count = p_info_req->target_count
	});
  nm_len_t size = nm_data_size(&data);
  void*buf = malloc(size);
  void*tmp = buf;
  switch(tag & NM_MPI_TAG_PRIVATE_RMA_MASK_OP) {
  case NM_MPI_TAG_PRIVATE_RMA_PUT:
    {
      nm_mpi_spin_lock(&p_win->lock);
      nm_mpi_datatype_pack(tmp, p_info_req->p_origin_addr, p_info_req->p_origin_datatype,
			   p_info_req->origin_count);
      nm_mpi_datatype_unpack(tmp, p_info_req->p_target_addr, p_info_req->p_target_datatype,
			     p_info_req->target_count);
      nm_mpi_spin_unlock(&p_win->lock);
    }
    break;
  case NM_MPI_TAG_PRIVATE_RMA_GET_REQ:
    {
      nm_mpi_spin_lock(&p_win->lock);
      nm_mpi_datatype_pack(tmp, p_info_req->p_target_addr, p_info_req->p_target_datatype,
			   p_info_req->target_count);
      nm_mpi_datatype_unpack(tmp, p_info_req->p_result_addr, p_info_req->p_result_datatype,
			     p_info_req->result_count);
      nm_mpi_spin_unlock(&p_win->lock);
    }
    break;
  case NM_MPI_TAG_PRIVATE_RMA_ACC:
    {
      if(MPI_NO_OP == op) p_req->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_INTERN);
      nm_mpi_operator_t*p_operator = nm_mpi_operator_get(op);
      assert(p_operator);
      nm_mpi_spin_lock(&p_win->lock);
      nm_mpi_datatype_pack(tmp, p_info_req->p_origin_addr, p_info_req->p_origin_datatype,
			   p_info_req->origin_count);
      nm_mpi_operator_apply(&tmp, p_info_req->p_target_addr, p_info_req->target_count,
			    p_info_req->p_target_datatype, p_operator);
      nm_mpi_spin_unlock(&p_win->lock);
    }
    break;
  case NM_MPI_TAG_PRIVATE_RMA_GACC_REQ:
    {
      if(MPI_NO_OP == op) p_req->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_INTERN);
      nm_mpi_operator_t*p_operator = nm_mpi_operator_get(op);
      assert(p_operator);
      nm_mpi_spin_lock(&p_win->lock);
      nm_mpi_datatype_pack(tmp, p_info_req->p_target_addr, p_info_req->p_target_datatype,
			   p_info_req->target_count);
      nm_mpi_datatype_unpack(tmp, p_info_req->p_result_addr, p_info_req->p_result_datatype,
			     p_info_req->result_count);
      nm_mpi_datatype_pack(tmp, p_info_req->p_origin_addr, p_info_req->p_origin_datatype,
			   p_info_req->origin_count);
      nm_mpi_operator_apply(&tmp, p_info_req->p_target_addr, p_info_req->target_count,
			    p_info_req->p_target_datatype, p_operator);
      nm_mpi_spin_unlock(&p_win->lock);
      nm_mpi_datatype_unlock(p_info_req->p_target_datatype);
      nm_mpi_datatype_unlock(p_info_req->p_result_datatype);
    }
    break;
  case NM_MPI_TAG_PRIVATE_RMA_FAO_REQ:
    {
      if(MPI_NO_OP == op) p_req->request_error = NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_INTERN);
      nm_mpi_operator_t*p_operator = nm_mpi_operator_get(op);
      assert(p_operator);
      nm_mpi_spin_lock(&p_win->lock);
      memcpy(p_info_req->p_result_addr, p_info_req->p_target_addr, p_info_req->p_target_datatype->size);
      nm_mpi_operator_apply(&p_info_req->p_origin_addr, p_info_req->p_target_addr,
			    p_info_req->origin_count, p_info_req->p_target_datatype, p_operator);
      nm_mpi_spin_unlock(&p_win->lock);
    }
    break;
  case NM_MPI_TAG_PRIVATE_RMA_CAS_REQ:
    {
      nm_mpi_spin_lock(&p_win->lock);
      memcpy(p_info_req->p_result_addr, p_info_req->p_target_addr, p_info_req->p_target_datatype->size);
      if(!memcmp(p_info_req->p_comp_addr, p_info_req->p_target_addr,
		 p_info_req->p_target_datatype->size)) {
	memcpy(p_info_req->p_target_addr, p_info_req->p_origin_addr,
	       p_info_req->p_origin_datatype->size);
      }
      nm_mpi_spin_unlock(&p_win->lock);
    }
    break;
  }
  free(buf);
  nm_sr_send_submit(p_session, &p_req->request_nmad);
  FREE_AND_SET_NULL(p_req->rbuf);
}
