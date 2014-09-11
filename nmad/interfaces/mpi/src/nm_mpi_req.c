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
  req->request_type = MPI_REQUEST_ZERO;
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


int mpi_waitsome(int incount,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *array_of_indices,
		 MPI_Status *array_of_statuses) {
#warning Implement MPI_Waitsome !
  return MPI_ERR_UNKNOWN;
}

int mpi_wait(MPI_Request *request,
	     MPI_Status *status) {
  nm_mpi_request_t  *p_req = nm_mpi_request_get(*request);
  int                       err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  if(p_req == NULL)
    {
      err = MPI_ERR_REQUEST;
      goto out; 
    }
  err = mpir_wait(p_req);

  if (status != MPI_STATUS_IGNORE) {
      err = mpir_set_status(p_req, status);
  }

  if (p_req->request_persistent_type == MPI_REQUEST_ZERO) {
    p_req->request_type = MPI_REQUEST_ZERO;
  }
  if (p_req->request_ptr != NULL) {
    FREE_AND_SET_NULL(p_req->request_ptr);
  }

  // Release one active communication for that type
  if (p_req->request_datatype > MPI_PACKED) {
    err = nm_mpi_datatype_unlock(p_req->request_datatype);
  }
  
  nm_mpi_request_free(p_req);
 out:
  *request = MPI_REQUEST_NULL;
  
  MPI_NMAD_TRACE("Request completed\n");
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_waitall(int count,
		MPI_Request *array_of_requests,
		MPI_Status *array_of_statuses) {
  int err = NM_ESUCCESS;
  int i;

  MPI_NMAD_LOG_IN();

  if (array_of_statuses == MPI_STATUSES_IGNORE) {
    for (i = 0; i < count; i++) {
      err =  MPI_Wait(&(array_of_requests[i]), MPI_STATUS_IGNORE);
      if (err != NM_ESUCCESS)
        goto out;
    }
  }
  else {
    for (i = 0; i < count; i++) {
      err =  MPI_Wait(&(array_of_requests[i]), &(array_of_statuses[i]));
      if (err != NM_ESUCCESS)
        goto out;
    }
  }

 out:
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_waitany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                MPI_Status *status) {
  int flag, i, err;
  int count_null;

  MPI_NMAD_LOG_IN();

  while (1) {
    count_null = 0;
    for(i=0 ; i<count ; i++) {
      nm_mpi_request_t *p_req = nm_mpi_request_get(array_of_requests[i]);
      if (p_req->request_type == MPI_REQUEST_ZERO) {
        count_null++;
        continue;
      }
      err = MPI_Test(&(array_of_requests[i]), &flag, status);
      if (flag) {
        *rqindex = i;

	MPI_NMAD_LOG_OUT();
        return err;
      }
    }
    if (count_null == count) {
      *rqindex = MPI_UNDEFINED;
      MPI_NMAD_LOG_OUT();
      return MPI_SUCCESS;
    }
  }
}

int mpi_test(MPI_Request *request,
             int *flag,
             MPI_Status *status) {
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  int             err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();
  err = mpir_test(p_req);

  if (err == NM_ESUCCESS) {
    *flag = 1;

    if (status != MPI_STATUS_IGNORE) {
      err = mpir_set_status(p_req, status);
    }

    if (p_req->request_persistent_type == MPI_REQUEST_ZERO) {
      p_req->request_type = MPI_REQUEST_ZERO;
    }
    if (p_req->request_ptr != NULL) {
      FREE_AND_SET_NULL(p_req->request_ptr);
    }

    // Release one active communication for that type
    if (p_req->request_datatype > MPI_PACKED) {
      err = nm_mpi_datatype_unlock(p_req->request_datatype);
    }
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_testany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                int *flag,
                MPI_Status *status) {
  int i, err = 0;
  int count_inactive = 0;
  nm_mpi_request_t *p_req;

  MPI_NMAD_LOG_IN();

  for(i=0 ; i<count ; i++) {
    p_req = nm_mpi_request_get(array_of_requests[i]);
    if (p_req->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = MPI_Test(&(array_of_requests[i]), flag, status);
    if (*flag == 1) {
      *rqindex = i;
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }

  *rqindex = MPI_UNDEFINED;
  if (count_inactive == count) {
    *flag = 1;
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_testall(int count,
		MPI_Request *array_of_requests,
		int *flag,
		MPI_Status *statuses) {
  int i, err = 0;
  int count_inactive = 0;
  nm_mpi_request_t *p_req;

  MPI_NMAD_LOG_IN();

  /* flag is true only if all requests have completed. Otherwise, flag is
   * false and neither the array_of_requests nor the array_of_statuses is
   * modified.
   */
  for(i=0 ; i<count ; i++) {
    p_req = nm_mpi_request_get(array_of_requests[i]);
    if (p_req->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = mpir_test(p_req);
    if(err != NM_ESUCCESS) {
      /* at least one request is not completed */
      *flag = 0;
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }

  /* all the requests are completed */
  for(i=0 ; i<count ; i++) {
    p_req = nm_mpi_request_get(array_of_requests[i]);
    if (p_req->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = MPI_Test(&(array_of_requests[i]), flag, &(statuses[i]));
    if(*flag != 1) {
      /* at least one request is not completed */
      ERROR("Error during MPI_Testall: request #%d should be completed, but it is not !", i);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }

  }

  *flag = 1;

  MPI_NMAD_LOG_OUT();
  return err;
}

/* TODO : handle the case where statuses == MPI_STATUSES_IGNORE */
int mpi_testsome(int count,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *indices,
		 MPI_Status *statuses) {
  int i, flag, err = 0;
  int count_inactive = 0;
  nm_mpi_request_t *p_req;

  MPI_NMAD_LOG_IN();

  *outcount = 0;

  for(i=0 ; i<count ; i++) {
    p_req = nm_mpi_request_get(array_of_requests[i]);
    if (p_req->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = mpi_test(&(array_of_requests[i]), &flag, &(statuses[*outcount]));
    if (flag == 1) {
      indices[*outcount] = i;
      (*outcount)++;
    }
  }

  if (count_inactive == count)
    *outcount = MPI_UNDEFINED;
  
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_cancel(MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  int             err = NM_ESUCCESS;
  
  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Request is cancelled\n");
  err = mpir_cancel(p_req);
  p_req->request_type = MPI_REQUEST_CANCELLED;
  if (p_req->contig_buffer != NULL)
    {
      FREE_AND_SET_NULL(p_req->contig_buffer);
    }
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_start(MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  int err;
  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Start request\n");

  p_req->request_type = p_req->request_persistent_type;

  err =  mpir_start(p_req);

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_startall(int count, MPI_Request *array_of_requests)
{
  int i;
  int err = MPI_SUCCESS;

  MPI_NMAD_LOG_IN();
  for(i=0 ; i<count ; i++) {
    err = MPI_Start(&(array_of_requests[i]));
    if (err != MPI_SUCCESS) {
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_request_is_equal(MPI_Request request1, MPI_Request request2)
{
  nm_mpi_request_t *p_req1 = nm_mpi_request_get(request1);
  nm_mpi_request_t *p_req2 = nm_mpi_request_get(request2);
  if (p_req1->request_type == MPI_REQUEST_ZERO) {
    return (p_req1->request_type == p_req2->request_type);
  }
  else if (p_req2->request_type == MPI_REQUEST_ZERO) {
    return (p_req1->request_type == p_req2->request_type);
  }
  else {
    return (p_req1 == p_req2);
  }
}

