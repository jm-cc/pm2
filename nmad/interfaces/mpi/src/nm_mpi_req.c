/*
 * NewMadeleine
 * Copyright (C) 2014-2017 (see AUTHORS file)
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
NM_MPI_ALIAS(MPI_Test_cancelled,   mpi_test_cancelled);
NM_MPI_ALIAS(MPI_Cancel,           mpi_cancel);
NM_MPI_ALIAS(MPI_Start,            mpi_start);
NM_MPI_ALIAS(MPI_Startall,         mpi_startall);

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
  nm_mpi_request_list_cell_init(p_req);
  p_req->status = 0;
#ifdef NMAD_PROFILE
  __sync_fetch_and_add(&nm_mpi_profile.cur_req_total, 1);
  if(nm_mpi_profile.cur_req_total > nm_mpi_profile.max_req_total)
    {
      nm_mpi_profile.max_req_total = nm_mpi_profile.cur_req_total;
    }
#endif /* NMAD_PROFILE */
  return p_req;
}

/** allocate a send request and fill all fields with default values */
__PUK_SYM_INTERNAL
nm_mpi_request_t*nm_mpi_request_alloc_send(nm_mpi_request_type_t type, int count, const void*sbuf,
                                           struct nm_mpi_datatype_s*p_datatype, int tag, struct nm_mpi_communicator_s*p_comm)
{
  assert(type & NM_MPI_REQUEST_SEND);
  struct nm_mpi_request_s*p_req = nm_mpi_request_alloc();
  p_req->request_type       = type;
  p_req->count              = count;
  p_req->sbuf               = sbuf;
  p_req->p_datatype         = p_datatype;
  p_req->user_tag           = tag;
  p_req->p_comm             = p_comm;
  p_req->status             = NM_MPI_STATUS_NONE;
#ifdef NMAD_PROFILE
  __sync_fetch_and_add(&nm_mpi_profile.cur_req_send, 1);
  if(nm_mpi_profile.cur_req_send > nm_mpi_profile.max_req_send)
    {
      nm_mpi_profile.max_req_send = nm_mpi_profile.cur_req_send;
    }
#endif /* NMAD_PROFILE */
  return p_req;
}

/** allocate a recv request and fill all fields with default values */
__PUK_SYM_INTERNAL
nm_mpi_request_t*nm_mpi_request_alloc_recv(int count, void*rbuf,
                                           struct nm_mpi_datatype_s*p_datatype, int tag, struct nm_mpi_communicator_s*p_comm)
{
  struct nm_mpi_request_s*p_req = nm_mpi_request_alloc();
  p_req->request_type       = NM_MPI_REQUEST_RECV;
  p_req->count              = count;
  p_req->rbuf               = rbuf;
  p_req->p_datatype         = p_datatype;
  p_req->user_tag           = tag;
  p_req->p_comm             = p_comm;
  p_req->status             = NM_MPI_STATUS_NONE;
#ifdef NMAD_PROFILE
  __sync_fetch_and_add(&nm_mpi_profile.cur_req_recv, 1);
  if(nm_mpi_profile.cur_req_recv > nm_mpi_profile.max_req_recv)
    {
      nm_mpi_profile.max_req_recv = nm_mpi_profile.cur_req_recv;
    }
#endif /* NMAD_PROFILE */
  return p_req;
}

__PUK_SYM_INTERNAL
void nm_mpi_request_free(nm_mpi_request_t*p_req)
{
#ifdef NMAD_PROFILE
  __sync_fetch_and_sub(&nm_mpi_profile.cur_req_total, 1);
  if(p_req->request_type & NM_MPI_REQUEST_RECV)
    {
      __sync_fetch_and_sub(&nm_mpi_profile.cur_req_recv, 1);
    }
  else if(p_req->request_type & NM_MPI_REQUEST_SEND)
    {
      __sync_fetch_and_sub(&nm_mpi_profile.cur_req_send, 1);
    }
  else
    {
      fprintf(stderr, "# unexpected request type = 0x%x\n", p_req->request_type);
    }
#endif /* NMAD_PROFILE */
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

static int nm_mpi_set_status(nm_mpi_request_t*p_req, struct mpi_status_s*status)
{
  int err = MPI_SUCCESS;
  status->MPI_TAG = p_req->user_tag;
  status->MPI_ERROR = p_req->request_error;

  if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      if(p_req->request_source == MPI_ANY_SOURCE)
	{
	  nm_gate_t p_gate;
	  err = nm_sr_recv_source(nm_mpi_communicator_get_session(p_req->p_comm), &p_req->request_nmad, &p_gate);
	  status->MPI_SOURCE = nm_mpi_communicator_get_dest(p_req->p_comm, p_gate);
	}
      else
	{
	  status->MPI_SOURCE = p_req->request_source;
	}
      if(p_req->user_tag == MPI_ANY_TAG)
	{
	  nm_tag_t nm_tag;
	  nm_sr_request_get_tag(&p_req->request_nmad, &nm_tag);
	  status->MPI_TAG = (int)nm_tag;
	}
    }
  nm_len_t _size = 0;
  nm_sr_request_get_size(&(p_req->request_nmad), &_size);
  status->size = _size;
  status->cancelled = p_req->status & NM_MPI_REQUEST_CANCELLED;
  return err;
}


/* ********************************************************* */

int mpi_request_free(MPI_Request*request)
{
  nm_mpi_request_t*p_req = nm_mpi_request_get(*request);
  int rc = nm_mpi_request_test(p_req);
  if(rc == NM_ESUCCESS || rc == -NM_ENOTPOSTED)
    {
      nm_mpi_request_free(p_req);
    }
  else
    {
      /* request is not completed yet. Enqueue it for future deletion.
       */
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
  nm_mpi_request_complete(p_req, request);
  return err;
}

int mpi_waitall(int count, MPI_Request*array_of_requests, MPI_Status*array_of_statuses)
{
  if(count < 0)
    return MPI_ERR_COUNT;
  else if(count == 0)
    return MPI_SUCCESS;
  nm_sr_request_t**p_nm_requests = malloc(count * sizeof(nm_sr_request_t*));
  int err = NM_ESUCCESS;
  int i;
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
      if(p_req == NULL)
	{
	  free(p_nm_requests);
	  return MPI_ERR_REQUEST;
	}
      p_nm_requests[i] = &p_req->request_nmad;
    }
  nm_sr_request_wait_all(p_nm_requests, count);
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
      if(array_of_statuses != MPI_STATUS_IGNORE)
	{
	  err = nm_mpi_set_status(p_req, &array_of_statuses[i]);
	}
      nm_mpi_request_complete(p_req, &array_of_requests[i]);
      if(err != NM_ESUCCESS)
	{
	  break;
	}
    }
  free(p_nm_requests);
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

int mpi_test(MPI_Request*request, int*flag, MPI_Status*status)
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
      nm_mpi_request_complete(p_req, request);
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
      if((array_of_requests[i] != MPI_REQUEST_NULL) && (p_req == NULL))
	{
	  *flag = 0;
	  return MPI_ERR_REQUEST;
	}
      else if(p_req == NULL || p_req->request_type == NM_MPI_REQUEST_ZERO)
	{
	  count_inactive++;
	}
      else
	{
	  int rc = nm_mpi_request_test(p_req);
	  if(rc != NM_ESUCCESS)
	    {
	      /* at least one request is not completed */
	      *flag = 0;
	      return MPI_SUCCESS;
	    }
	}
    }
  /* all the requests are completed */
  for(i = 0; i < count; i++)
    {
      nm_mpi_request_t*p_req = nm_mpi_request_get(array_of_requests[i]);
      if((p_req == NULL) || (p_req->request_type == NM_MPI_REQUEST_ZERO))
	{
	  count_inactive++;
	  continue;
	}
      err = MPI_Test(&(array_of_requests[i]), flag, &(statuses[i]));
      if(*flag != 1)
	{
	  /* at least one request is not completed */
	  NM_MPI_FATAL_ERROR("Error during MPI_Testall: request #%d should be completed, but it is not !", i);
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

int mpi_cancel(MPI_Request*request)
{
  int err = MPI_SUCCESS;
  nm_mpi_request_t*p_req = nm_mpi_request_get(*request);
  if(!p_req)
    return MPI_ERR_REQUEST;
  if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      int rc = nm_sr_rcancel(nm_mpi_communicator_get_session(p_req->p_comm), &p_req->request_nmad);
      if(rc == NM_ESUCCESS || rc == -NM_ECANCELED)
	{
	  /* successfully cancelled or already cancelled */
	  p_req->status |= NM_MPI_REQUEST_CANCELLED;
	  err = MPI_SUCCESS;
	}
      else if(rc == -NM_EINPROGRESS || rc == -NM_EALREADY)
	{
	  /* too late for cancel, recv will be successfull: return succes, do not mark as cancelled */
	  err = MPI_SUCCESS;
	}
      else
	{
	  /* cannot cancel for another reason */
	  NM_MPI_WARNING("MPI_Cancel unexpected error %d. Cannot cancel.", rc);
	  err = MPI_ERR_UNKNOWN;
	}
    }
  else if(p_req->request_type & NM_MPI_REQUEST_SEND)
    {
      if(nm_sr_stest(nm_mpi_communicator_get_session(p_req->p_comm), &p_req->request_nmad) == NM_ESUCCESS)
	{
	  /* already successfull: return success */
	  err = MPI_SUCCESS;
	}
      else
	{
	  /* cancel for send requests is not implemented in nmad.
	   * 1. issue a warning
	   * 2. mark as cancelled so as to unblock the next wait to avoid deadlock in case of rdv
	   * 3. return an error code
	   */
	  NM_MPI_WARNING("MPI_Cancel called for send- not supported.");
	  p_req->status |= NM_MPI_REQUEST_CANCELLED;
	  err = MPI_ERR_UNSUPPORTED_OPERATION;
	}
    }
  else
    {
      NM_MPI_FATAL_ERROR("Request type %d incorrect\n", p_req->request_type);
    }
  return err;
}

int mpi_test_cancelled(const MPI_Status*status, int*flag)
{
  *flag = status->cancelled;
  return MPI_SUCCESS;
}

int mpi_start(MPI_Request*request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_get(*request);
  int err = MPI_SUCCESS;
  assert(p_req != NULL);
  assert(p_req->request_type != NM_MPI_REQUEST_ZERO);
  assert(p_req->status & NM_MPI_REQUEST_PERSISTENT);
  if(p_req->request_type == NM_MPI_REQUEST_SEND)
    {
      err = nm_mpi_isend_start(p_req);
    }
  else if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      err = nm_mpi_irecv_start(p_req);
    }
  else
    {
      NM_MPI_FATAL_ERROR("Unknown persistent request type: %d\n", p_req->request_type);
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

/* ********************************************************* */
/* ** core test/wait/complete functions used to build MPI level primitives */

/** notify request completion & free request */
__PUK_SYM_INTERNAL
void nm_mpi_request_complete(nm_mpi_request_t*p_req, MPI_Request*request)
{
  if(!(p_req->status & NM_MPI_REQUEST_PERSISTENT))
    {
      /* Release one active communication for that type */
      if(p_req->p_datatype->id >= _NM_MPI_DATATYPE_OFFSET)
        {
          nm_mpi_datatype_ref_dec(p_req->p_datatype);
        }
      if(request)
        {
          assert(*request == p_req->id);
          *request = MPI_REQUEST_NULL;
        }
      nm_mpi_request_free(p_req);
    }
}

__PUK_SYM_INTERNAL
int nm_mpi_request_test(nm_mpi_request_t*p_req)
 {
  int err = NM_ESUCCESS;
  if(p_req->status & NM_MPI_REQUEST_CANCELLED)
    {
      err = MPI_SUCCESS;
    }
  else if(p_req->request_type == NM_MPI_REQUEST_RECV)
    {
      err = nm_sr_rtest(nm_mpi_communicator_get_session(p_req->p_comm), &p_req->request_nmad);
    }
  else if(p_req->request_type & NM_MPI_REQUEST_SEND)
    {
      err = nm_sr_stest(nm_mpi_communicator_get_session(p_req->p_comm), &p_req->request_nmad);
    }
  else
    {
     NM_MPI_FATAL_ERROR("Request type %d invalid\n", p_req->request_type);
    }
  return err;
}

__PUK_SYM_INTERNAL
int nm_mpi_request_wait(nm_mpi_request_t*p_req)
{
  int err = MPI_SUCCESS;
  if(p_req->status & NM_MPI_REQUEST_CANCELLED)
    {
      err = MPI_SUCCESS;
    }
  else if((p_req->request_type == NM_MPI_REQUEST_RECV) || (p_req->request_type & NM_MPI_REQUEST_SEND))
    {
      nm_sr_request_wait(&p_req->request_nmad);
      err = MPI_SUCCESS;
    }
  else
    {
      NM_MPI_FATAL_ERROR("Waiting operation invalid for request type %d\n", p_req->request_type);
    }
  return err;
}
