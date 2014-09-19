/*
 * NewMadeleine
 * Copyright (C) 2014 (see AUTHORS file)
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
PADICO_MODULE_HOOK(NewMad_Core);


PUK_VECT_TYPE(nm_mpi_request, nm_mpi_request_t*);

/* Custom requests allocator. Inspired by Puk lock-free allocator,
   except that we use int indexes for compatibility with Fortran.
*/
PUK_LFSTACK_TYPE(nm_mpi_request_block, nm_mpi_request_t req __attribute__((aligned(PUK_ALLOCATOR_ALIGN))););
PUK_LFSTACK_TYPE(nm_mpi_request_slice, struct nm_mpi_request_block_lfstack_cell_s block; );
static struct
{
  nm_mpi_request_block_lfstack_t cache;        /**< cache for nm_mpi_request_t blocks */
  nm_mpi_request_slice_lfstack_t slices;       /**< slices of memory containing blocks */
  int slice_size;                              /**< number of blocks per slice */
  MPI_Fint next_id;                            /**< next req id to allocate */
  nm_mpi_request_vect_t request_array;         /**< maps req_id -> nm_mpi_request_t* */
} nm_mpi_request_allocator = { .cache = NULL, .slices = NULL, .slice_size = 256, .next_id = 1, .request_array = NULL };


/* ********************************************************* */

int MPI_Request_free(MPI_Request *request) __attribute__ ((alias ("mpi_request_free")));

int MPI_Waitsome(int incount,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *array_of_indices,
		 MPI_Status *array_of_statuses) __attribute__ ((alias ("mpi_waitsome")));

int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) __attribute__ ((alias ("mpi_wait")));

int MPI_Waitall(int count,
		MPI_Request *array_of_requests,
		MPI_Status *array_of_statuses) __attribute__ ((alias ("mpi_waitall")));

int MPI_Waitany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                MPI_Status *status) __attribute__ ((alias ("mpi_waitany")));

int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status) __attribute__ ((alias ("mpi_test")));

int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                int *flag,
                MPI_Status *status) __attribute__ ((alias ("mpi_testany")));

int MPI_Testall(int count,
		MPI_Request *array_of_requests,
		int *flag,
		MPI_Status *statuses) __attribute__ ((alias ("mpi_testall")));

int MPI_Testsome(int count,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *indices,
		 MPI_Status *statuses) __attribute__ ((alias ("mpi_testsome")));

int MPI_Cancel(MPI_Request *request) __attribute__ ((alias ("mpi_cancel")));

int MPI_Start(MPI_Request *request) __attribute__ ((alias ("mpi_start")));

int MPI_Startall(int count,
                 MPI_Request *array_of_requests) __attribute__ ((alias ("mpi_startall")));

int MPI_Request_is_equal(MPI_Request request1,
			 MPI_Request request2) __attribute__ ((alias ("mpi_request_is_equal")));

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_request_init(void)
{
  nm_mpi_request_allocator.request_array = nm_mpi_request_vect_new();
  /* placeholder so as to never allocate request #0 (easier to debug) */
  nm_mpi_request_vect_push_back(nm_mpi_request_allocator.request_array, (void*)0xDEADBEEF);
}

nm_mpi_request_t*nm_mpi_request_alloc(void)
{
  struct nm_mpi_request_block_lfstack_cell_s*cell = NULL;
 retry:
  cell = nm_mpi_request_block_lfstack_pop(&nm_mpi_request_allocator.cache);
  if(cell == NULL)
    {
      /* cache miss; allocate a new slice */
      int slice_size = nm_mpi_request_allocator.slice_size;
      nm_mpi_request_allocator.slice_size *= 2;
      struct nm_mpi_request_slice_lfstack_cell_s*slice =
	padico_malloc(sizeof(struct nm_mpi_request_slice_lfstack_cell_s) + sizeof(struct nm_mpi_request_block_lfstack_cell_s) * slice_size);
      nm_mpi_request_slice_lfstack_push(&nm_mpi_request_allocator.slices, slice);
      struct nm_mpi_request_block_lfstack_cell_s*blocks = &slice->block;
      int i;
      for(i = 0; i < slice_size; i++)
	{
	  blocks[i].req.request_id = nm_mpi_request_allocator.next_id++;
	  nm_mpi_request_block_lfstack_push(&nm_mpi_request_allocator.cache, &blocks[i]);
	}
      goto retry;
    }
  nm_mpi_request_t*req = &cell->req;
  nm_mpi_request_vect_put(nm_mpi_request_allocator.request_array, req, req->request_id);
  return req;
}

void nm_mpi_request_free(nm_mpi_request_t*req)
{
  struct nm_mpi_request_block_lfstack_cell_s*cell = container_of(req, struct nm_mpi_request_block_lfstack_cell_s, req);
  const int id = req->request_id;
  req->request_type = NM_MPI_REQUEST_ZERO;
  nm_mpi_request_vect_put(nm_mpi_request_allocator.request_array, NULL, id);
  nm_mpi_request_block_lfstack_push(&nm_mpi_request_allocator.cache, cell);
}

nm_mpi_request_t*nm_mpi_request_get(MPI_Fint req_id)
{
  const int id = (int)req_id;
  if(id == MPI_REQUEST_NULL)
    return NULL;
  assert(id >= 0);
  nm_mpi_request_t*req = nm_mpi_request_vect_at(nm_mpi_request_allocator.request_array, id);
  assert(req != NULL);
  assert(req->request_id == id);
  return req;
}

static int nm_mpi_set_status(nm_mpi_request_t *p_req, MPI_Status *status)
{
  int err = MPI_SUCCESS;
  status->MPI_TAG = p_req->user_tag;
  status->MPI_ERROR = p_req->request_error;

  if(p_req->request_type == NM_MPI_REQUEST_RECV ||
      p_req->request_type == NM_MPI_REQUEST_PACK_RECV)
    {
      if(p_req->request_source == MPI_ANY_SOURCE)
	{
	  nm_gate_t p_gate;
	  err = nm_sr_recv_source(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad, &p_gate);
	  status->MPI_SOURCE = nm_mpi_communicator_get_dest(p_req->p_comm, p_gate);
	}
      else 
	{
	  status->MPI_SOURCE = p_req->request_source;
	}
    }
  size_t _size = 0;
  nm_sr_get_size(nm_comm_get_session(p_req->p_comm->p_comm), &(p_req->request_nmad), &_size);
  status->size = _size;
  MPI_NMAD_TRACE("Size %d Size datatype %lu\n", status->size, (unsigned long)nm_mpi_datatype_size(p_req->p_datatype));
  if(nm_mpi_datatype_size(p_req->p_datatype) != 0)
    {
      status->count = status->size / nm_mpi_datatype_size(p_req->p_datatype);
    }
  else
    {
      status->count = -1;
    }
  return err;
}


/* ********************************************************* */

int mpi_request_free(MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  MPI_NMAD_LOG_IN();
  p_req->request_type = NM_MPI_REQUEST_ZERO;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  if(p_req->contig_buffer != NULL)
    {
      FREE_AND_SET_NULL(p_req->contig_buffer);
    }
  nm_mpi_request_free(p_req);
  *request = MPI_REQUEST_NULL;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_waitsome(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
#warning Implement MPI_Waitsome !
  return MPI_ERR_UNKNOWN;
}

int mpi_wait(MPI_Request *request, MPI_Status *status)
{
  nm_mpi_request_t*p_req = nm_mpi_request_get(*request);
  int err = NM_ESUCCESS;
  if(p_req == NULL)
    {
      err = MPI_ERR_REQUEST;
      goto out; 
    }
  err = nm_mpi_request_wait(p_req);
  if(status != MPI_STATUS_IGNORE)
    {
      err = nm_mpi_set_status(p_req, status);
    }
  nm_mpi_request_complete(p_req);
  nm_mpi_request_free(p_req);
 out:
  *request = MPI_REQUEST_NULL;
  return err;
}

int mpi_waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses)
{
  int err = NM_ESUCCESS;
  int i;
  if(array_of_statuses == MPI_STATUSES_IGNORE)
    {
      for (i = 0; i < count; i++)
	{
	  err = mpi_wait(&(array_of_requests[i]), MPI_STATUS_IGNORE);
	  if(err != NM_ESUCCESS)
	    goto out;
	}
    }
  else
    {
      for (i = 0; i < count; i++)
	{
	  err = mpi_wait(&(array_of_requests[i]), &(array_of_statuses[i]));
	  if(err != NM_ESUCCESS)
	    goto out;
	}
    }
 out:
  return err;
}

int mpi_waitany(int count, MPI_Request *array_of_requests, int *rqindex, MPI_Status *status)
{
  int err = MPI_SUCCESS;
  for(;;)
    {
      int flag, i;
      int count_null = 0;
      for(i = 0; i < count; i++)
	{
	  nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
	  if(p_req->request_type == NM_MPI_REQUEST_ZERO)
	    {
	      count_null++;
	      continue;
	    }
	  err = MPI_Test(&(array_of_requests[i]), &flag, status);
	  if(flag)
	    {
	      *rqindex = i;
	      return err;
	    }
	}
      if(count_null == count)
	{
	  *rqindex = MPI_UNDEFINED;
	  MPI_NMAD_LOG_OUT();
	  return MPI_SUCCESS;
	}
    }
}

int mpi_test(MPI_Request *request, int *flag, MPI_Status *status)
{
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  int err = nm_mpi_request_test(p_req);
  if(err == NM_ESUCCESS)
    {
      *flag = 1;
      if(status != MPI_STATUS_IGNORE)
	{
	  err = nm_mpi_set_status(p_req, status);
	}
      nm_mpi_request_complete(p_req);
    }
  else
    { /* err == -NM_EAGAIN */
      *flag = 0;
    }
  return err;
}

int mpi_testany(int count, MPI_Request*array_of_requests, int*rqindex, int*flag, MPI_Status*status)
{
  int err = MPI_SUCCESS;
  int count_inactive = 0;
  int i;
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
      if(p_req->request_type == NM_MPI_REQUEST_ZERO)
	{
	  count_inactive++;
	  continue;
	}
      err = MPI_Test(&(array_of_requests[i]), flag, status);
      if(*flag == 1)
	{
	  *rqindex = i;
	  MPI_NMAD_LOG_OUT();
	  return err;
	}
    }
  *rqindex = MPI_UNDEFINED;
  if(count_inactive == count)
    {
      *flag = 1;
    }
  return err;
}

int mpi_testall(int count, MPI_Request*array_of_requests, int*flag, MPI_Status*statuses)
{
  int i, err =  MPI_SUCCESS;
  int count_inactive = 0;
  /* flag is true only if all requests have completed. Otherwise, flag is
   * false and neither the array_of_requests nor the array_of_statuses is
   * modified.
   */
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
      if(p_req->request_type == NM_MPI_REQUEST_ZERO)
	{
	  count_inactive++;
	  continue;
	}
      err = nm_mpi_request_test(p_req);
      if(err != NM_ESUCCESS)
	{
	  /* at least one request is not completed */
	  *flag = 0;
	  return err;
	}
    }
  /* all the requests are completed */
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
      if(p_req->request_type == NM_MPI_REQUEST_ZERO)
	{
	  count_inactive++;
	  continue;
	}
      err = MPI_Test(&(array_of_requests[i]), flag, &(statuses[i]));
      if(*flag != 1)
	{
	  /* at least one request is not completed */
	  ERROR("Error during MPI_Testall: request #%d should be completed, but it is not !", i);
	  return MPI_ERR_INTERN;
	}
    }
  *flag = 1;
  return err;
}

/* TODO : handle the case where statuses == MPI_STATUSES_IGNORE */
int mpi_testsome(int count, MPI_Request*array_of_requests, int*outcount, int*indices, MPI_Status *statuses)
{
  int i, flag, err = MPI_SUCCESS;
  int count_inactive = 0;
  *outcount = 0;
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t *p_req = nm_mpi_request_get(array_of_requests[i]);
      if(p_req->request_type == NM_MPI_REQUEST_ZERO)
	{
	  count_inactive++;
	  continue;
	}
      err = mpi_test(&(array_of_requests[i]), &flag, &(statuses[*outcount]));
      if(flag == 1)
	{
	  indices[*outcount] = i;
	  (*outcount)++;
	}
    }
  if(count_inactive == count)
    *outcount = MPI_UNDEFINED;
  return err;
}

int mpi_cancel(MPI_Request *request)
{
  nm_mpi_request_t*p_req = nm_mpi_request_get(*request);
  int err = MPI_SUCCESS;
  if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      mpir_dec_nb_incoming_msg();
      err = nm_sr_rcancel(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_SEND)
    {
      mpir_dec_nb_outgoing_msg();
    }
  else 
    {
      ERROR("Request type %d incorrect\n", p_req->request_type);
    }
  p_req->request_type = NM_MPI_REQUEST_CANCELLED;
  if(p_req->contig_buffer != NULL)
    {
      FREE_AND_SET_NULL(p_req->contig_buffer);
    }
  return err;
}

int mpi_start(MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  int err = MPI_SUCCESS;
  p_req->request_type = p_req->request_persistent_type;
  if(p_req->request_persistent_type == NM_MPI_REQUEST_SEND)
    {
      err = nm_mpi_isend_start(p_req);
    }
  else if(p_req->request_persistent_type == NM_MPI_REQUEST_RECV)
    {
      err = nm_mpi_irecv_start(p_req);
    }
  else 
    {
      ERROR("Unknown persistent request type: %d\n", p_req->request_persistent_type);
      err = MPI_ERR_INTERN;
    }
  return err;
}

int mpi_startall(int count, MPI_Request *array_of_requests)
{
  int err = MPI_SUCCESS;
  int i;
  for(i = 0; i < count; i++)
    {
      err = MPI_Start(&(array_of_requests[i]));
      if(err != MPI_SUCCESS)
	{
	  return err;
	}
    }
  return err;
}

int mpi_request_is_equal(MPI_Request request1, MPI_Request request2)
{
  nm_mpi_request_t *p_req1 = nm_mpi_request_get(request1);
  nm_mpi_request_t *p_req2 = nm_mpi_request_get(request2);
  if(p_req1->request_type == NM_MPI_REQUEST_ZERO)
    {
      return (p_req1->request_type == p_req2->request_type);
    }
  else if(p_req2->request_type == NM_MPI_REQUEST_ZERO)
    {
      return (p_req1->request_type == p_req2->request_type);
    }
  else
    {
      return (p_req1 == p_req2);
    }
}

/* ********************************************************* */

/** notify request completion */
__PUK_SYM_INTERNAL
void nm_mpi_request_complete(nm_mpi_request_t*p_req)
{
  if(p_req->request_persistent_type == NM_MPI_REQUEST_ZERO)
    {
      p_req->request_type = NM_MPI_REQUEST_ZERO;
    }
  if(p_req->request_ptr != NULL)
    {
      FREE_AND_SET_NULL(p_req->request_ptr);
    }
  // Release one active communication for that type
  if(!p_req->p_datatype->basic)
    {
      nm_mpi_datatype_unlock(p_req->p_datatype);
    }
}

__PUK_SYM_INTERNAL
int nm_mpi_request_test(nm_mpi_request_t*p_req)
 {
  int err = MPI_SUCCESS;
  if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      err = nm_sr_rtest(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_SEND)
    {
      err = nm_sr_stest(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_PACK_RECV)
    {
      nm_pack_cnx_t *connection = &(p_req->request_cnx);
      err = nm_test_end_unpacking(connection);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_PACK_SEND)
    {
      nm_pack_cnx_t *connection = &(p_req->request_cnx);
      err = nm_test_end_packing(connection);
    }
  else
    {
     ERROR("Request type %d invalid\n", p_req->request_type);
    }
  return err;
}

__PUK_SYM_INTERNAL
int nm_mpi_request_wait(nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      err = nm_sr_rwait(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
      nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
      if(!p_datatype->is_contig && p_req->contig_buffer) 
	{
	  nm_mpi_datatype_split(p_req);
	}
    }
  else if(p_req->request_type == NM_MPI_REQUEST_SEND) 
    {
      err = nm_sr_swait(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
      if(p_req->request_persistent_type == NM_MPI_REQUEST_ZERO)
	{
	  if(p_req->contig_buffer != NULL)
	    {
	      FREE_AND_SET_NULL(p_req->contig_buffer);
	    }
	}
    }
  else if(p_req->request_type == NM_MPI_REQUEST_PACK_RECV)
    {
      nm_pack_cnx_t *connection = &(p_req->request_cnx);
      err = nm_end_unpacking(connection);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_PACK_SEND)
    {
      nm_pack_cnx_t *connection = &(p_req->request_cnx);
      err = nm_end_packing(connection);
    }
  else
    {
      ERROR("Waiting operation invalid for request type %d\n", p_req->request_type);
    }
  return err;
}
