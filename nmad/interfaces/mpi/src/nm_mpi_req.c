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


NM_MPI_HANDLE_TYPE(request, nm_mpi_request_t, _NM_MPI_REQUEST_OFFSET, 256);

static struct nm_mpi_handle_request_s nm_mpi_requests;

/* ********************************************************* */

NM_MPI_ALIAS(MPI_Request_free,     mpi_request_free);
NM_MPI_ALIAS(MPI_Wait,             mpi_wait);
NM_MPI_ALIAS(MPI_Waitall,          mpi_waitall);
NM_MPI_ALIAS(MPI_Waitany,          mpi_waitany);
NM_MPI_ALIAS(MPI_Waitsome,         mpi_waitsome);
NM_MPI_ALIAS(MPI_Test,             mpi_test);
NM_MPI_ALIAS(MPI_Testall,          mpi_testall);
NM_MPI_ALIAS(MPI_Testany,          mpi_testany);
NM_MPI_ALIAS(MPI_Testsome,         mpi_testsome);
NM_MPI_ALIAS(MPI_Cancel,           mpi_cancel);
NM_MPI_ALIAS(MPI_Start,            mpi_start);
NM_MPI_ALIAS(MPI_Startall,         mpi_startall);
NM_MPI_ALIAS(MPI_Request_is_equal, mpi_request_is_equal);

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_request_init(void)
{
  nm_mpi_handle_request_init(&nm_mpi_requests);
}

__PUK_SYM_INTERNAL
void nm_mpi_request_exit(void)
{
  nm_mpi_handle_request_finalize(&nm_mpi_requests, &nm_mpi_request_free);
}

/* ********************************************************* */

__PUK_SYM_INTERNAL
nm_mpi_request_t*nm_mpi_request_alloc(void)
{
  nm_mpi_request_t*p_req = nm_mpi_handle_request_alloc(&nm_mpi_requests);
  return p_req;
}

__PUK_SYM_INTERNAL
void nm_mpi_request_free(nm_mpi_request_t*p_req)
{
  /* set request to zero to help debug */
  p_req->request_type = NM_MPI_REQUEST_ZERO;
  nm_mpi_handle_request_free(&nm_mpi_requests, p_req);
}

__PUK_SYM_INTERNAL
nm_mpi_request_t*nm_mpi_request_get(MPI_Fint req_id)
{
  const int id = (int)req_id;
  if(id == MPI_REQUEST_NULL)
    return NULL;
  assert(id >= 0);
  nm_mpi_request_t*p_req = nm_mpi_handle_request_get(&nm_mpi_requests, req_id);
  assert(p_req != NULL);
  assert(p_req->id == id);
  return p_req;
}

static int nm_mpi_set_status(nm_mpi_request_t*p_req, MPI_Status *status)
{
  int err = MPI_SUCCESS;
  status->MPI_TAG = p_req->user_tag;
  status->MPI_ERROR = p_req->request_error;

  if(p_req->request_type == NM_MPI_REQUEST_RECV)
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
      if(p_req->user_tag == MPI_ANY_TAG)
	{
	  nm_tag_t nm_tag;
	  nm_sr_get_rtag(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad, &nm_tag);
	  status->MPI_TAG = (int)nm_tag;
	}
    }
  size_t _size = 0;
  nm_sr_get_size(nm_comm_get_session(p_req->p_comm->p_comm), &(p_req->request_nmad), &_size);
  status->size = _size;
  MPI_NMAD_TRACE("Size %d Size datatype %lu\n", status->size, (unsigned long)nm_mpi_datatype_size(p_req->p_datatype));
  return err;
}


/* ********************************************************* */

int mpi_request_free(MPI_Request*request)
{
  nm_mpi_request_t*p_req = nm_mpi_request_get(*request);
  int rc = nm_mpi_request_test(p_req);
  if(rc == NM_ESUCCESS || rc == -NM_ENOTPOSTED)
    {
      p_req->request_type = NM_MPI_REQUEST_ZERO;
      p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
      nm_mpi_request_free(p_req);
    }
  else
    {
#warning TODO- enqueue task for future deletion
    }
  *request = MPI_REQUEST_NULL;
  return MPI_SUCCESS;
}

int mpi_waitsome(int incount, MPI_Request*array_of_requests, int*outcount, int*array_of_indices, MPI_Status*array_of_statuses)
{
  int count = 0;
  if(incount <= 0)
    {
      *outcount = MPI_UNDEFINED;
      return MPI_SUCCESS;
    }
  while(count == 0)
    {
      int i;
      int count_null = 0;
      for(i = 0; i < incount; i++)
	{
	  int flag;
	  nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
	  if(p_req == NULL || p_req->request_type == NM_MPI_REQUEST_ZERO)
	    {
	      count_null++;
	      continue;
	    }
	  int err = mpi_test(&array_of_requests[i], &flag, &array_of_statuses[i]);
	  if(err != MPI_SUCCESS)
	    {
	      *outcount = MPI_UNDEFINED;
	      return err;
	    }
	  if(flag)
	    {
	      array_of_indices[count] = i;
	      count++;
	    }
	}
      if(count_null == incount)
	{
	  *outcount = MPI_UNDEFINED;
	  return MPI_SUCCESS;
	}
    }
  *outcount = count;
  return MPI_SUCCESS;
}

int mpi_wait(MPI_Request*request, MPI_Status*status)
{
  nm_mpi_request_t*p_req = nm_mpi_request_get(*request);
  if(p_req == NULL)
    {
      return MPI_ERR_REQUEST;
    }
  int err = nm_mpi_request_wait(p_req);
  if(status != MPI_STATUS_IGNORE)
    {
      err = nm_mpi_set_status(p_req, status);
    }
  nm_mpi_request_complete(p_req);
  nm_mpi_request_free(p_req);
  *request = MPI_REQUEST_NULL;
  return err;
}

int mpi_waitall(int count, MPI_Request*array_of_requests, MPI_Status*array_of_statuses)
{
  int err = NM_ESUCCESS;
  int i;
  for(i = 0; i < count; i++)
    {
      err = mpi_wait(&(array_of_requests[i]),
		     (array_of_statuses == MPI_STATUSES_IGNORE) ? MPI_STATUS_IGNORE : &array_of_statuses[i]);
      if(err != NM_ESUCCESS)
	{
	  break;
	}
    }
  return err;
}

int mpi_waitany(int count, MPI_Request*array_of_requests, int*rqindex, MPI_Status*status)
{
  int err = MPI_SUCCESS;
  for(;;)
    {
      int i;
      int count_null = 0;
      for(i = 0; i < count; i++)
	{
	  int flag;
	  nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
	  if(p_req == NULL || p_req->request_type == NM_MPI_REQUEST_ZERO)
	    {
	      count_null++;
	      continue;
	    }
	  err = mpi_test(&(array_of_requests[i]), &flag, status);
	  if(flag)
	    {
	      *rqindex = i;
	      return err;
	    }
	}
      if(count_null == count)
	{
	  *rqindex = MPI_UNDEFINED;
	  return MPI_SUCCESS;
	}
    }
}

int mpi_test(MPI_Request *request, int *flag, MPI_Status *status)
{
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  if(p_req == NULL)
    {
      *flag = 1;
      return MPI_SUCCESS;
    }
  int err = nm_mpi_request_test(p_req);
  if(err == NM_ESUCCESS)
    {
      *flag = 1;
      if(status != MPI_STATUS_IGNORE)
	{
	  err = nm_mpi_set_status(p_req, status);
	}
      nm_mpi_request_complete(p_req);
      nm_mpi_request_free(p_req);
      *request = MPI_REQUEST_NULL;
    }
  else
    { /* err == -NM_EAGAIN */
      *flag = 0;
    }
  return MPI_SUCCESS;
}

int mpi_testany(int count, MPI_Request*array_of_requests, int*rqindex, int*flag, MPI_Status*status)
{
  int err = MPI_SUCCESS;
  int count_inactive = 0;
  int i;
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
      if(p_req == NULL || p_req->request_type == NM_MPI_REQUEST_ZERO)
	{
	  count_inactive++;
	  continue;
	}
      err = MPI_Test(&(array_of_requests[i]), flag, status);
      if(*flag == 1)
	{
	  *rqindex = i;
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
      if(p_req == NULL)
	{
	  *flag = 0;
	  return MPI_ERR_REQUEST;
	}
      if(p_req->request_type == NM_MPI_REQUEST_ZERO)
	{
	  count_inactive++;
	  continue;
	}
      int rc = nm_mpi_request_test(p_req);
      if(rc != NM_ESUCCESS)
	{
	  /* at least one request is not completed */
	  *flag = 0;
	  return MPI_SUCCESS;
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
      if(p_req == NULL || p_req->request_type == NM_MPI_REQUEST_ZERO)
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
      err = nm_sr_rcancel(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_SEND)
    {
    }
  else 
    {
      ERROR("Request type %d incorrect\n", p_req->request_type);
    }
  p_req->request_type = NM_MPI_REQUEST_CANCELLED;
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
  // Release one active communication for that type
  if(p_req->p_datatype->id >= _NM_MPI_DATATYPE_OFFSET)
    {
      nm_mpi_datatype_unlock(p_req->p_datatype);
    }
}

__PUK_SYM_INTERNAL
int nm_mpi_request_test(nm_mpi_request_t*p_req)
 {
  int err = NM_ESUCCESS;
  if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      err = nm_sr_rtest(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_SEND)
    {
      err = nm_sr_stest(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
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
    }
  else if(p_req->request_type == NM_MPI_REQUEST_SEND) 
    {
      err = nm_sr_swait(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    }
  else
    {
      ERROR("Waiting operation invalid for request type %d\n", p_req->request_type);
    }
  return err;
}
