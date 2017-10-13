/*
 * NewMadeleine
 * Copyright (C) 2012 (see AUTHORS file)
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

/** @file
 * @brief FORTRAN interface implementation
 */

#include "nm_mpi_private.h"
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <tbx.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);

extern int f90_mpi_in_place;
extern int f90_mpi_bottom;
extern int f90_mpi_statuses_ignore;
extern int f90_mpi_status_ignore;

#if defined(NMAD_FORTRAN_TARGET_GFORTRAN) || defined(NMAD_FORTRAN_TARGET_IFORT)
/**
 * Fortran version for MPI_INIT
 */
int mpi_init_(void)
{
  int argc = -1;
  char**argv = NULL;
  tbx_fortran_init(&argc, &argv);
  return MPI_Init(&argc, &argv);
}
#elif defined NMAD_FORTRAN_TARGET_NONE
/* Nothing */
#else
#  error unknown FORTRAN TARGET for Mad MPI
#endif



#ifndef NMAD_FORTRAN_TARGET_NONE
/* Alias Fortran
 */

/**
 * Fortran version for MPI_INIT_THREAD
 */
void mpi_init_thread_(int * required, int *provided, int * ierr)
{
  int argc = -1;
  char**argv = NULL;
  tbx_fortran_init(&argc, &argv);
  *ierr = MPI_Init_thread(&argc, &argv, *required, provided);
}

/**
 * Fortran version for MPI_INITIALIZED
 */
void mpi_initialized_(int *flag, int *ierr)
{
  *ierr = MPI_Initialized(flag);
}

/**
 * Fortran version for MPI_FINALIZE
 */
void mpi_finalize_(void)
{
  tbx_fortran_finalize();
  MPI_Finalize();
}

/**
 * Fortran version for MPI_ABORT
 */
void mpi_abort_(MPI_Comm comm, int errorcode)
{
  MPI_Abort(comm, errorcode);
}

/**
 * Fortran version for MPI_COMM_SIZE
 */
void mpi_comm_size_(int *comm,
                    int *size, int *ierr) {
  *ierr = MPI_Comm_size(*comm, size);
}

/**
 * Fortran version for MPI_COMM_RANK
 */
void mpi_comm_rank_(int *comm,
		    int *rank,
		    int *ierr) {
  *ierr = MPI_Comm_rank(*comm, rank);
}

/**
 * Fortran version for MPI_ATTR_GET
 */
void mpi_attr_get_(int *comm,
		   int *keyval,
		   int *attr_value,
		   int *flag,
		   int *ierr) {
  *ierr = MPI_Attr_get(*comm, *keyval, attr_value, flag);
}

void mpi_attr_put_(int*comm, int*keyval, void*attr_value, int*ierr)
{
  *ierr = MPI_Attr_put(*comm, *keyval, attr_value);
}

/**
 * Fortran version for MPI_GET_PROCESSOR_NAME
 */
void mpi_get_processor_name_(char *name,
			     int *resultlen)
{
  MPI_Get_processor_name(name, resultlen);
}

/*
void mpi_esend_(void *buffer,
                int *count,
                int *datatype,
                int *dest,
                int *tag,
                int *is_completed,
                int *comm,
                int *request,
		int *ierr) {
}
*/

/**
 * Fortran version for MPI_SEND
 */
void mpi_send_(void *buffer,
               int *count,
               int *datatype,
               int *dest,
               int *tag,
               int *comm,
               int *ierr) {
  void * _buffer = buffer==&f90_mpi_bottom ? MPI_BOTTOM : buffer;
  *ierr = MPI_Send(_buffer, *count, *datatype, *dest, *tag, *comm);
}

/**
 * Fortran version for MPI_ISEND
 */
void mpi_isend_(void *buffer,
                int *count,
                int *datatype,
                int *dest,
                int *tag,
                int *comm,
                int *request,
                int *ierr) {
  MPI_Request c_request;
  void * _buffer = buffer==&f90_mpi_bottom ? MPI_BOTTOM : buffer;
  *ierr = MPI_Isend(_buffer, *count, *datatype, *dest, *tag, *comm, &c_request);
  *request = c_request;
}

/**
 * Fortran version for MPI_RSEND
 */
void mpi_rsend_(void *buffer,
                int *count,
                int *datatype,
                int *dest,
                int *tag,
                int *comm,
                int *ierr) {
  *ierr = MPI_Rsend(buffer, *count, *datatype, *dest, *tag, *comm);
}

/**
 * Fortran version for MPI_SSEND
 */
void mpi_ssend_(void *buffer,
		int *count,
                int *datatype,
                int *dest,
                int *tag,
                int *comm,
                int *ierr) {
  *ierr = MPI_Ssend(buffer, *count, *datatype, *dest, *tag, *comm);
}

/**
 * Fortran version for MPI_RECV
 */
void mpi_recv_(void *buffer,
               int *count,
               int *datatype,
               int *source,
               int *tag,
               int *comm,
               int *status,
               int *ierr) {
  MPI_Status _status;
  void * _buffer = buffer==&f90_mpi_bottom ? MPI_BOTTOM : buffer;
  *ierr = MPI_Recv(_buffer, *count, *datatype, *source, *tag, *comm, &_status);
  mpi_status_c2f(&_status, status);
}

/**
 * Fortran version for MPI_IRECV
 */
void mpi_irecv_(void *buffer,
                int *count,
                int *datatype,
                int *source,
                int *tag,
                int *comm,
                int *request,
                int *ierr) {
  MPI_Request c_request;
  void * _buffer = buffer==&f90_mpi_bottom ? MPI_BOTTOM : buffer;
  *ierr = MPI_Irecv(_buffer, *count, *datatype, *source, *tag, *comm, &c_request);
  *request = c_request;
}

/**
 * Fortran version for MPI_SENDRECV
 */
void mpi_sendrecv_(void *sendbuf,
                   int *sendcount,
                   int *sendtype,
                   int *dest,
                   int *sendtag,
                   void *recvbuf,
		   int *recvcount,
                   int *recvtype,
                   int *source,
                   int *recvtag,
                   int *comm,
                   int *status,
                   int *ierr) {
  MPI_Status _status;
  *ierr = MPI_Sendrecv(sendbuf, *sendcount, *sendtype, *dest, *sendtag,
		       recvbuf, *recvcount, *recvtype, *source, *recvtag, *comm,
		       &_status);
  mpi_status_c2f(&_status, status);
}

/**
 * Fortran version for MPI_PACK
 */
void mpi_pack_(void *inbuf,
	       int  *incount,
	       int  *datatype,
	       void *outbuf,
	       int  *outcount,
	       int  *position,
	       int  *comm,
	       int  *ierr) {
  *ierr = MPI_Pack(inbuf, *incount, *datatype, outbuf, *outcount, position, *comm);
}

/**
 * Fortran version for MPI_UNPACK
 */
void mpi_unpack_(void *inbuf,
		 int  *insize,
		 int  *position,
		 void *outbuf,
		 int  *outcount,
		 int  *datatype,
		 int  *comm,
		 int  *ierr) {
  *ierr = MPI_Unpack(inbuf, *insize, position, outbuf, *outcount, *datatype, *comm);
}

/**
 * Fortran stub for MPI_BSEND (not implemented)
 */
void mpi_bsend_(void *buf,
		int *count,
		int *datatype,
		int *dest,
		int *tag,
		int *comm,
		int *ierr) {
  *ierr = MPI_ERR_UNKNOWN;
}

/**
 * Fortran stub for MPI_BUFFER_ATTACH (not implemented)
 */
void mpi_buffer_attach_(void *buffer,
			int *size,
			int *ierr) {
  *ierr = MPI_ERR_UNKNOWN;
}

/**
 * Fortran stub for MPI_BUFFER_DETACH (not implemented)
 */
void mpi_buffer_detach_(void *buffer,
			int *size,
			int *ierr) {
  *ierr = MPI_ERR_UNKNOWN;
}

/**
 * Fortran version for MPI_WAIT
 */
void mpi_wait_(int *request,
               int *status,
               int *ierr) {
  MPI_Status _status;
  *ierr = MPI_Wait(request, &_status);
  mpi_status_c2f(&_status, status);
}

/**
 * Fortran version for MPI_WAIT_ALL
 */
void mpi_waitall_(int *count,
                  int *array_of_requests,
                  int  array_of_statuses[][MPI_STATUS_SIZE],
                  int *ierr) {
  int err = NM_ESUCCESS;
  int i;
  for (i = 0; i < *count; i++) 
    {
      MPI_Status _status;
      err = MPI_Wait(&array_of_requests[i], &_status);
      if ((MPI_Status *)array_of_statuses != MPI_STATUSES_IGNORE) 
	{
	  mpi_status_c2f(&_status, array_of_statuses[i]);
	}
      if (err != NM_ESUCCESS)
        goto out;
    }
 out:
  *ierr = err;
}

/**
 * Fortran version for MPI_WAIT_ANY
 */
void mpi_waitany_(int *count,
		  int *request,
		  int *rqindex,
		  int *status,
		  int *ierr) {
  int err = NM_ESUCCESS;
  int i, flag, count_null;
  MPI_Status _status;

  while(1) {
    count_null = 0;
    for (i = 0; i < *count; i++) {
      nm_mpi_request_t *p_req = nm_mpi_request_get(request[i]);
      if (p_req->request_type == NM_MPI_REQUEST_ZERO) {
        count_null ++;
      }
      else {
        MPI_Test(&request[i], &flag, &_status);
        if (flag) {

          p_req->request_type = NM_MPI_REQUEST_ZERO;
          *rqindex = i;

	  mpi_status_c2f(&_status, status);

          *ierr = err;
          return;
        }
      }
    }
    if (count_null == *count) {
      *rqindex = MPI_UNDEFINED;
      *ierr = MPI_SUCCESS;
      return;
    }
  }
}

/**
 * Fortran version for MPI_TEST
 */
void mpi_test_(int *request,
               int *flag,
               int *status,
               int *ierr) {
  MPI_Status _status;
  *ierr = MPI_Test(request, flag, &_status);
  if (*ierr != MPI_SUCCESS)
    return;
  mpi_status_c2f(&_status, status);
}

/**
 * Fortran version for MPI_TESTANY
 */
void mpi_testany_(int *count,
                  int  array_of_requests[],
                  int *rqindex,
                  int *flag,
                  int *status,
		  int *ierr) {
  int err = NM_ESUCCESS;
  int i, _flag, count_null=0;
  MPI_Status _status;

  for (i = 0; i < *count; i++) {
    nm_mpi_request_t *p_req = nm_mpi_request_get(array_of_requests[i]);
    if (p_req->request_type == NM_MPI_REQUEST_ZERO) {
      count_null ++;
    }
    else {
      err = MPI_Test(&array_of_requests[i], &_flag, &_status);
      if (_flag) {
        *rqindex = i;
        *flag = _flag;
        *ierr = MPI_SUCCESS;

	mpi_status_c2f(&_status, status);
        return;
      }
    }
  }
  *rqindex = MPI_UNDEFINED;
  if (count_null == *count) {
    *flag = 1;
  }
  *ierr = err;
}

/**
 * Fortran version for MPI_IPROBE
 */
void mpi_iprobe_(int *source,
                 int *tag,
                 int *comm,
                 int *flag,
                 int *status,
		 int *ierr) {
  MPI_Status _status;
  *ierr = MPI_Iprobe(*source, *tag, *comm, flag, &_status);

  mpi_status_c2f(&_status, status);
}

/**
 * Fortran version for MPI_PROBE
 */
void mpi_probe_(int *source,
                int *tag,
                int *comm,
                int *status,
		int *ierr) {
  MPI_Status _status;
  *ierr = MPI_Probe(*source, *tag, *comm, &_status);

  mpi_status_c2f(&_status, status);
}

/**
 * Fortran version for MPI_CANCEL
 */
void mpi_cancel_(int *request,
		 int *ierr) {
  *ierr = MPI_Cancel(request);
}

/**
 * Fortran version for MPI_REQUEST_FREE
 */
void mpi_request_free_(int *request,
		       int *ierr) {
  *ierr = MPI_Request_free(request);
}

/**
 * Fortran version for MPI_GET_COUNT
 */
void mpi_get_count_(int *status,
                    int *datatype,
                    int *count,
                    int *ierr) {
  MPI_Status _status;
  mpi_status_f2c(status, &_status);
  *ierr = MPI_Get_count(&_status, *datatype, count);
}

/**
 * Fortran version for MPI_SEND_INIT
 */
void mpi_send_init_(void* buf,
		    int *count,
		    int *datatype,
		    int *dest,
		    int *tag,
		    int *comm,
		    int *request,
		    int *ierr) {
  void * _buf = buf==&f90_mpi_bottom ? MPI_BOTTOM : buf;
  *ierr = MPI_Send_init(_buf, *count, *datatype, *dest, *tag, *comm, request);
}

/**
 * Fortran version for MPI_RECV_INIT
 */
void mpi_recv_init_(void* buf,
		    int *count,
		    int *datatype,
		    int *source,
		    int *tag,
		    int *comm,
		    int *request,
		    int *ierr) {
  void * _buf = buf==&f90_mpi_bottom ? MPI_BOTTOM : buf;
  *ierr = MPI_Recv_init(_buf, *count, *datatype, *source, *tag, *comm, request);
}

/**
 * Fortran version for MPI_START
 */
void mpi_start_(int *request,
		int *ierr) {
  *ierr = MPI_Start(request);
}

/**
 * Fortran version for MPI_START_ALL
 */
void mpi_startall_(int *count,
		   int  array_of_requests[],
		   int *ierr) {
  int i;
  for (i = 0; i < *count; i++) {
    *ierr = MPI_Start(&array_of_requests[i]);
    if (*ierr != MPI_SUCCESS) {
      return;
    }
  }
}

/**
 * Fortran version for MPI_BARRIER
 */
void mpi_barrier_(int *comm,
                  int *ierr) {
  *ierr = MPI_Barrier(*comm);
}

/**
 * Fortran version for MPI_BCAST
 */
void mpi_bcast_(void *buffer,
                int *count,
                int *datatype,
                int *root,
                int *comm,
                int *ierr) {
  *ierr = MPI_Bcast(buffer, *count, *datatype, *root, *comm);
}

/**
 * Fortran version for MPI_GATHER
 */
void mpi_gather_(void *sendbuf,
		 int *sendcount,
		 int *sendtype,
		 void *recvbuf,
		 int *recvcount,
		 int *recvtype,
		 int *root,
		 int *comm,
		 int *ierr) {
  *ierr = MPI_Gather(sendbuf, *sendcount, *sendtype, recvbuf, *recvcount,
		     *recvtype, *root, *comm);
}

/**
 * Fortran version for MPI_GATHERV
 */
void mpi_gatherv_(void *sendbuf,
		  int *sendcount,
		  int *sendtype,
		  void *recvbuf,
		  int *recvcounts,
		  int *displs,
		  int *recvtype,
		  int *root,
		  int *comm,
		  int *ierr) {
  *ierr = MPI_Gatherv(sendbuf, *sendcount, *sendtype, recvbuf, recvcounts,
		      displs, *recvtype, *root, *comm);
}

/**
 * Fortran version for MPI_ALLGATHER
 */
void mpi_allgather_(void *sendbuf,
		    int *sendcount,
		    int *sendtype,
		    void *recvbuf,
		    int *recvcount,
		    int *recvtype,
		    int *comm,
		    int *ierr) {
  *ierr = MPI_Allgather(sendbuf, *sendcount, *sendtype, recvbuf, *recvcount,
			*recvtype, *comm);
}

/**
 * Fortran version for MPI_ALLGATHERV
 */
void mpi_allgatherv_(void *sendbuf,
		     int *sendcount,
		     int *sendtype,
		     void *recvbuf,
		     int *recvcounts,
		     int *displs,
		     int *recvtype,
		     int *comm,
		     int *ierr) {
  *ierr = MPI_Allgatherv(sendbuf, *sendcount, *sendtype, recvbuf, recvcounts,
			 displs, *recvtype, *comm);
}

/*
 * Fortran version for MPI_SCATTER
 */
void mpi_scatter_(void *sendbuf,
		  int *sendcount,
		  int *sendtype,
		  void *recvbuf,
		  int *recvcount,
		  int *recvtype,
		  int *root,
		  int *comm,
		  int *ierr) {
  *ierr = MPI_Scatter(sendbuf, *sendcount, *sendtype, recvbuf, *recvcount,
		      *recvtype, *root, *comm);
}

/** Fortran version of MPI_Scatterv*/
void mpi_scatterv_(void*sendbuf, int*sendcounts, int*displs, int*sendtype,
		   void*recvbuf, int*recvcount, int*recvtype,
		   int*root, int*comm, int*ierr)
{
  *ierr = MPI_Scatterv(sendbuf, sendcounts, displs, *sendtype,
		       recvbuf, *recvcount, *recvtype,
		       *root, *comm);
}
  
/**
 * Fortran version for MPI_ALLTOALL
 */
void mpi_alltoall_(void *sendbuf,
		   int *sendcount,
		   int *sendtype,
		   void *recvbuf,
		   int *recvcount,
		   int *recvtype,
		   int *comm,
		   int *ierr) {
  *ierr = MPI_Alltoall(sendbuf, *sendcount, *sendtype, recvbuf, *recvcount,
		       *recvtype, *comm);
}

/**
 * Fortran version for MPI_ALLTOALLV
 */
void mpi_alltoallv_(void *sendbuf,
                    int *sendcount,
                    int *sdispls,
                    int *sendtype,
                    void *recvbuf,
                    int *recvcount,
                    int *rdispls,
                    int *recvtype,
                    int *comm,
                    int *ierr) {
  *ierr = MPI_Alltoallv(sendbuf, sendcount, sdispls, *sendtype, recvbuf,
			recvcount, rdispls, *recvtype, *comm);
}

/**
 * Fortran version for MPI_OP_CREATE
 */
void mpi_op_create_(MPI_User_function *function TBX_UNUSED,
                    int commute TBX_UNUSED,
                    MPI_Op *op TBX_UNUSED)
{
  NM_MPI_FATAL_ERROR("unimplemented");
}

/**
 * Fortran version for MPI_OP_FREE
 */
void mpi_op_free_(MPI_Op *op)
{
  NM_MPI_FATAL_ERROR("unimplemented");
}

/**
 * Fortran version for MPI_REDUCE
 */
void mpi_reduce_(void *sendbuf,
                 void *recvbuf,
                 int *count,
                 int *datatype,
                 int *op,
                 int *root,
                 int *comm,
                 int *ierr) {
  *ierr = MPI_Reduce(sendbuf, recvbuf, *count, *datatype, *op,
                     *root, *comm);
}

/**
 * Fortran version for MPI_ALLREDUCE
 */
void mpi_allreduce_(void *sendbuf,
                    void *recvbuf,
                    int *count,
                    int *datatype,
                    int *op,
                    int *comm,
                    int *ierr) {
  *ierr = MPI_Allreduce(sendbuf, recvbuf, *count, *datatype, *op, *comm);
}

/**
 * Fortran version for MPI_REDUCE_SCATTER
 */
void mpi_reduce_scatter_(void *sendbuf,
			 void *recvbuf,
			 int *recvcounts,
			 int *datatype,
			 int *op,
			 int *comm,
			 int *ierr) {
  *ierr = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, *datatype,
			     *op, *comm);
}

/**
 * Fortran version for MPI_WTIME
 */
double mpi_wtime_(void) {
  return MPI_Wtime();
}

/**
 * Fortran version for MPI_WTICK
 */
double mpi_wtick_(void) {
  return MPI_Wtick();
}

/*
 * Fortran version for  MPI_ERROR_STRING
 */
void mpi_error_string_(int *errorcode,
		       char *string,
		       int *resultlen,
		       int *ierr) {
  *ierr = MPI_Error_string(*errorcode, string, resultlen);
}

/*
 * Fortran version for  MPI_ERRHANDLER_SET
 */
void mpi_errhandler_set_(int *comm,
			 int *errhandler,
			 int *ierr) {
  *ierr = MPI_Errhandler_set(*comm, *errhandler);
}

/**
 * Fortran version for MPI_GET_VERSION
 */
void mpi_get_version_(int *version,
		      int *subversion,
		      int *ierr) {
  *ierr = MPI_Get_version(version, subversion);
}

/**
 * Fortran version for MPI_GET_ADDRESS
 */
void mpi_get_address_(void *location,
		      uintptr_t *address,
		      int *ierr) {
  MPI_Aint _address;
  *ierr = MPI_Get_address(location, &_address);
  *address = _address;
}

/**
 * Fortran version for MPI_ADDRESS
 */
void mpi_address_(void *location,
		  uintptr_t *address,
		  int *ierr) {
  MPI_Aint _address;
  *ierr = MPI_Address(location, &_address);
  *address = _address;
}

/**
 * Fortran version for MPI_TYPE_SIZE
 */
void mpi_type_size_(int *datatype,
		    int *size,
		    int *ierr) {
  *ierr = MPI_Type_size(*datatype, size);
}

/**
 * Fortran version for MPI_TYPE_GET_EXTENT
 */
void mpi_type_get_extent_(int *datatype,
			  int *lb,
			  int *extent,
			  int *ierr) {
  MPI_Aint _lb, _extent;
  *ierr = MPI_Type_get_extent(*datatype, &_lb, &_extent);
  *lb = _lb;
  *extent = _extent;
}

/**
 * Fortran version for MPI_TYPE_EXTENT
 */
void mpi_type_extent_(int *datatype,
		      int *extent,
		      int *ierr) {
  MPI_Aint _extent;
  *ierr = MPI_Type_extent(*datatype, &_extent);
  *extent = _extent;
}

/**
 * Fortran version for MPI_TYPE_LB
 */
void mpi_type_lb_(int *datatype,
		  int *lb,
		  int *ierr) {
  MPI_Aint _lb;
  *ierr = MPI_Type_lb(*datatype, &_lb);
  *lb = _lb;
}

/**
 * Fortran version for MPI_TYPE_CREATE_RESIZED
 */
void mpi_type_create_resized_(int *oldtype,
			      int *lb,
			      int *extent,
			      int *newtype,
			      int *ierr) {
  MPI_Datatype _newtype;
  *ierr = MPI_Type_create_resized(*oldtype, *lb, *extent, &_newtype);
  *newtype = _newtype;
}

/**
 * Fortran version for MPI_TYPE_COMMIT
 */
void mpi_type_commit_(int *datatype,
		      int *ierr) {
  *ierr = MPI_Type_commit(datatype);
}

/**
 * Fortran version for MPI_TYPE_FREE
 */
void mpi_type_free_(int *datatype,
		    int *ierr) {
  *ierr = MPI_Type_free(datatype);
}

/**
 * Fortran version for MPI_TYPE_CONTIGUOUS
 */
void mpi_type_contiguous_(int *count,
                          int *oldtype,
                          int *newtype,
			  int *ierr) {
  MPI_Datatype _newtype;
  *ierr = MPI_Type_contiguous(*count, *oldtype, &_newtype);
  *newtype = _newtype;
}

/**
 * Fortran version for MPI_TYPE_VECTOR
 */
void mpi_type_vector_(int *count,
                      int *blocklength,
                      int *stride,
                      int *oldtype,
                      int *newtype,
		      int *ierr) {
  MPI_Datatype _newtype;
  *ierr = MPI_Type_vector(*count, *blocklength, *stride, *oldtype, &_newtype);
  *newtype = _newtype;
}

/**
 * Fortran version for MPI_TYPE_HVECTOR
 */
void mpi_type_hvector_(int *count,
                       int *blocklength,
                       int *stride,
                       int *oldtype,
                       int *newtype,
		       int *ierr) {
  MPI_Datatype _newtype;
  *ierr = MPI_Type_hvector(*count, *blocklength, *stride, *oldtype, &_newtype);
  *newtype = _newtype;
}

/**
 * Fortran version for MPI_TYPE_INDEXED
 */
void mpi_type_indexed_(int *count,
                       int *array_of_blocklengths,
                       int *array_of_displacements,
                       int *oldtype,
                       int *newtype,
		       int *ierr) {
  MPI_Datatype _newtype;
  *ierr = MPI_Type_indexed(*count, array_of_blocklengths, array_of_displacements,
			   *oldtype, &_newtype);
  *newtype = _newtype;
}

/**
 * Fortran version for MPI_TYPE_HINDEXED
 */
void mpi_type_hindexed_(int *count,
                        int *array_of_blocklengths,
                        uintptr_t *array_of_displacements,
                        int *oldtype,
                        int *newtype,
			int *ierr) {
  MPI_Datatype _newtype;
  MPI_Aint *_array_of_displacements = malloc(*count * sizeof(MPI_Aint));
  int i;

  for(i=0 ; i<*count ; i++) {
    _array_of_displacements[i] = array_of_displacements[i];
  }
  *ierr = MPI_Type_hindexed(*count, array_of_blocklengths, _array_of_displacements,
			    *oldtype, &_newtype);
  *newtype = _newtype;
  free(_array_of_displacements);
}

/**
 * Fortran version for MPI_TYPE_STRUCT
 */
void mpi_type_struct_(int *count,
                      int *array_of_blocklengths,
                      int *array_of_displacements,
                      int *array_of_types,
                      int *newtype,
		      int *ierr) {
  MPI_Datatype _newtype;
  MPI_Aint *_array_of_displacements = malloc(*count * sizeof(MPI_Aint));
  int i;

  for(i=0 ; i<*count ; i++) {
    _array_of_displacements[i] = array_of_displacements[i];
  }
  *ierr = MPI_Type_struct(*count, array_of_blocklengths, _array_of_displacements,
			  array_of_types, &_newtype);
  *newtype = _newtype;
  free(_array_of_displacements);
}

/** Fortran version for MPI_Comm_create */
void mpi_comm_create_(int*comm, int*group, int*newcomm, int*ierr)
{
  MPI_Comm _newcomm;
  int err = MPI_Comm_create(*comm, *group, &_newcomm);
  *newcomm = _newcomm;
  *ierr = err;
}

/**
 * Fortran version for MPI_COMM_GROUP
 */
void mpi_comm_group_(int *comm,
		     int *group,
		     int *ierr) {
  MPI_Group _group;
  *ierr = MPI_Comm_group(*comm, &_group);
  *group = _group;
}

/**
 * Fortran version for MPI_COMM_SPLIT
 */
void mpi_comm_split_(int *comm,
		     int *color,
		     int *key,
		     int *newcomm,
		     int *ierr) {
  MPI_Comm _newcomm;
  *ierr = MPI_Comm_split(*comm, *color, *key, &_newcomm);
  *newcomm = _newcomm;
}

/**
 * Fortran version for MPI_COMM_DUP
 */
void mpi_comm_dup_(int *comm,
                   int *newcomm,
                   int *ierr) {
  MPI_Comm _newcomm;
  *ierr = MPI_Comm_dup(*comm, &_newcomm);
  *newcomm = _newcomm;
}

/**
 * Fortran version for MPI_COMM_FREE
 */
void mpi_comm_free_(int *comm,
		    int *ierr) {
  MPI_Comm _newcomm = *comm;
  *ierr = MPI_Comm_free(&_newcomm);
  *comm = _newcomm;
}

void mpi_cart_create_(int*_comm_old, int*ndims, int*dims, int*periods, int*reorder, int*_comm_cart, int*ierr)
{
  MPI_Comm comm_old = *_comm_old;
  MPI_Comm comm_cart = MPI_COMM_NULL;
  *ierr = MPI_Cart_create(comm_old, *ndims, dims, periods, *reorder, &comm_cart);
  *_comm_cart = comm_cart;
}

void mpi_cart_coords_(int*_comm, int*rank, int*ndims, int*coords, int*ierr)
{
  MPI_Comm comm = *_comm;
  *ierr = MPI_Cart_coords(comm, *rank, *ndims, coords);
}

void mpi_cart_rank_(int*_comm, int*coords, int*rank, int*ierr)
{
  MPI_Comm comm = *_comm;
  *ierr = MPI_Cart_rank(comm, coords, rank);
}

void mpi_cart_shift_(int*_comm, int*direction, int*displ, int*source, int*dest, int*ierr)
{
  MPI_Comm comm = *_comm;
  MPI_Cart_shift(comm, *direction, *displ, source, dest);
}

/**
 * Fortran version for MPI_GROUP_TRANSLATE_RANKS
 */
void mpi_group_translate_ranks_(int *group1,
				int *n,
				int *ranks1,
				int *group2,
				int *ranks2,
				int *ierr) {
  *ierr = MPI_Group_translate_ranks(*group1, *n, ranks1, *group2,
				    ranks2);
}

/** Fortran version for MPI_Group_incl */
void mpi_group_incl_(int*group, int*n, int*ranks, int*newgroup, int*ierr)
{
  MPI_Group _newgroup;
  int err = MPI_Group_incl(*group, *n, ranks, &_newgroup);
  *newgroup = _newgroup;
  *ierr = err;
}

/**
 * Fortran version for MPI_TYPE_CREATE_HINDEXED
 */
void mpi_type_create_hindexed_(int *count,
                               int *array_of_blocklengths,
                               uintptr_t *array_of_displacements,
                               int *oldtype,
                               int *newtype,
                               int *ierr)
{
  mpi_type_hindexed_(count,array_of_blocklengths,array_of_displacements,oldtype,newtype,ierr);
}

/**
 * Fortran version for MPI_TYPE_CREATE_INDEXED_BLOCK
 */
void mpi_type_create_indexed_block_(int *count,
                                    int *blocklength,
                                    int *array_of_displacements,
                                    int *oldtype,
                                    int *newtype,
                                    int *ierr)
{
  MPI_Datatype _newtype;
  *ierr = MPI_Type_create_indexed_block(*count, *blocklength, array_of_displacements,
          *oldtype, &_newtype);
  *newtype = _newtype;
}

/**
 * Fortran version for MPI_TYPE_CREATE_STRUCT
 */
void mpi_type_create_struct_(int *count,
                             int *array_of_blocklengths,
                             int *array_of_displacements,
                             int *array_of_types,
                             int *newtype,
                             int *ierr)
{
  mpi_type_struct_(count,array_of_blocklengths,array_of_displacements,array_of_types,newtype,ierr);
}

#endif /* NMAD_FORTRAN_TARGET_NONE */
