/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

/*
 * mpi.c
 * =====
 */

#include "mpi.h"
#include "mpi_nmad_private.h"
#include <nm_so_parameters.h>
#ifdef CONFIG_PROTO_MAD3
#include <nm_mad3_public.h>
#endif
#include <stdint.h>
#include <assert.h>

static puk_instance_t launcher_instance = NULL;
static mpir_internal_data_t mpir_internal_data;

#define MAX_ARG_LEN 64

#if defined PM2_FORTRAN_TARGET_GFORTRAN
/* GFortran iargc/getargc bindings
 */
void _gfortran_getarg_i4(int32_t *num, char *value, int val_len)	__attribute__ ((weak));

void _gfortran_getarg_i8(int64_t *num, char *value, int val_len)	__attribute__ ((weak));

int32_t _gfortran_iargc(void)	__attribute__ ((weak));
/**
 * Fortran version for MPI_INIT
 */
int mpi_init_(void) {
  int argc;
  char **argv;

  tbx_fortran_init(&argc, &argv);
  return MPI_Init(&argc, &argv);
}
#elif defined PM2_FORTRAN_TARGET_IFORT
/* Ifort iargc/getargc bindings
 */
/** Initialisation by Fortran code.
 */
extern int  iargc_();
extern void getarg_(int*, char*, int);

/**
 * Fortran version for MPI_INIT
 */
int mpi_init_() {
  int argc;
  char **argv;

  tbx_fortran_init(&argc, &argv);
  return MPI_Init(&argc, &argv);
}
#elif defined PM2_FORTRAN_TARGET_NONE
/* Nothing */
#else
#  error unknown FORTRAN TARGET for Mad MPI
#endif

#ifndef PM2_FORTRAN_TARGET_NONE
/* Alias Fortran
 */

/**
 * Fortran version for MPI_INIT_THREAD
 */
void mpi_init_thread_(int *argc,
                      char ***argv,
                      int required,
                      int *provided)	__attribute__ ((alias ("MPI_Init_thread")));

/**
 * Fortran version for MPI_INITIALIZED
 */
void mpi_initialized_(int *flag,
		      int *ierr) {
  *ierr = MPI_Initialized(flag);
}

/**
 * Fortran version for MPI_FINALIZE
 */
void mpi_finalize_(void)			__attribute__ ((alias ("MPI_Finalize")));

/**
 * Fortran version for MPI_ABORT
 */
void mpi_abort_(MPI_Comm comm,
                int errorcode)		__attribute__ ((alias ("MPI_Abort")));

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
 * Fortran version for MPI_GET_PROCESSOR_NAME
 */
void mpi_get_processor_name_(char *name,
			     int *resultlen) __attribute__ ((alias ("MPI_Get_processor_name")));

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
  *ierr = MPI_Send(buffer, *count, *datatype, *dest, *tag, *comm);
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
  MPI_Request *p_request = TBX_MALLOC(sizeof(MPI_Request));
  *ierr = MPI_Isend(buffer, *count, *datatype, *dest, *tag, *comm, p_request);
  *request = (intptr_t)p_request;
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
  *ierr = MPI_Recv(buffer, *count, *datatype, *source, *tag, *comm, &_status);
  if (*status) {
    status[0] = _status.MPI_SOURCE;
    status[1] = _status.MPI_TAG;
    status[2] = _status.MPI_ERROR;
  }
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
  MPI_Request *p_request = TBX_MALLOC(sizeof(MPI_Request));
  *ierr = MPI_Irecv(buffer, *count, *datatype, *source, *tag, *comm, p_request);
  *request = (intptr_t)p_request;
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
  if (*status) {
    status[0] = _status.MPI_SOURCE;
    status[1] = _status.MPI_TAG;
    status[2] = _status.MPI_ERROR;
  }
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
 * Fortran version for MPI_WAIT
 */
void mpi_wait_(int *request,
               int *status,
               int *ierr) {
  MPI_Status _status;
  MPI_Request *p_request = (void *)*request;
  *ierr = MPI_Wait(p_request, &_status);
  if (*status) {
    status[0] = _status.MPI_SOURCE;
    status[1] = _status.MPI_TAG;
    status[2] = _status.MPI_ERROR;
  }
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

  if ((MPI_Status *)array_of_statuses == MPI_STATUSES_IGNORE) {
    for (i = 0; i < *count; i++) {
      MPI_Request *p_request = (void *)array_of_requests[i];
        err =  MPI_Wait(p_request, MPI_STATUS_IGNORE);
        TBX_FREE(p_request);
        if (err != NM_ESUCCESS)
          goto out;
    }
  }
  else {
    for (i = 0; i < *count; i++) {
      MPI_Status _status;
      MPI_Request *p_request = (void *)array_of_requests[i];
      err =  MPI_Wait(p_request, &_status);
      if (*(array_of_statuses[i])) {
	array_of_statuses[i][0] = _status.MPI_SOURCE;
	array_of_statuses[i][1] = _status.MPI_TAG;
	array_of_statuses[i][2] = _status.MPI_ERROR;
      }
      if (err != NM_ESUCCESS)
        goto out;
    }
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
  int i, flag;
  MPI_Status _status;

  while(1) {
    for (i = 0; i < *count; i++) {
      MPI_Request *p_request = (void *)request[i];
      MPI_Test(p_request, &flag, &_status);
      if (flag) {
	mpir_request_t *mpir_request = (mpir_request_t *)(&request);

	mpir_request->request_type = MPI_REQUEST_ZERO;
	*rqindex = i;

	if (*status) {
	  status[0] = _status.MPI_SOURCE;
	  status[1] = _status.MPI_TAG;
	  status[2] = _status.MPI_ERROR;
	}

	*ierr = err;
	return;
      }
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
  MPI_Request *p_request = (void *)*request;
  *ierr = MPI_Test(p_request, flag, &_status);
  if (*ierr != MPI_SUCCESS)
    return;
  if (*status) {
    status[0] = _status.MPI_SOURCE;
    status[1] = _status.MPI_TAG;
    status[2] = _status.MPI_ERROR;
  }
  TBX_FREE((void *)*request);
}

/**
 * Fortran version for MPI_TESTANY
 */
/* TODO : il est fait comme l'ancienne version C */
void mpi_testany_(int *count,
                  int  array_of_requests[][MPI_REQUEST_SIZE],
                  int *rqindex,
                  int *flag,
                  int *status,
		  int *ierr) {
  int err = NM_ESUCCESS;
  int i, _flag;
  MPI_Status _status;

  for (i = 0; i < *count; i++) {
    MPI_Request *p_request = (void *)array_of_requests[i];
    err = MPI_Test(p_request, &_flag, &_status);
    if (_flag) {
      *rqindex = i;
      *flag = _flag;
      *ierr = MPI_SUCCESS;

      if (*status) {
	status[0] = _status.MPI_SOURCE;
	status[1] = _status.MPI_TAG;
	status[2] = _status.MPI_ERROR;
      }

      return;
    }
  }
  *rqindex = MPI_UNDEFINED;
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

  if (*status) {
    status[0] = _status.MPI_SOURCE;
    status[1] = _status.MPI_TAG;
    status[2] = _status.MPI_ERROR;
  }
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

  if (*status) {
    status[0] = _status.MPI_SOURCE;
    status[1] = _status.MPI_TAG;
    status[2] = _status.MPI_ERROR;
  }
}

/**
 * Fortran version for MPI_CANCEL
 */
void mpi_cancel_(int *request,
		 int *ierr) {
  MPI_Request *p_request = (void *)request[0];
  *ierr = MPI_Cancel(p_request);
}

/**
 * Fortran version for MPI_REQUEST_FREE
 */
void mpi_request_free_(int *request,
		       int *ierr) {
  MPI_Request *p_request = (void *)*request;
  *ierr = MPI_Request_free(p_request);
}

/**
 * Fortran version for MPI_GET_COUNT
 */
void mpi_get_count_(int *status,
                    int *datatype,
                    int *count,
                    int *ierr) {
  MPI_Status _status;
  _status.MPI_SOURCE = status[0];
  _status.MPI_TAG = status[1];
  _status.MPI_ERROR = status[2];
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
  MPI_Request *p_request = TBX_MALLOC(sizeof(MPI_Request));
  *ierr = MPI_Send_init(buf, *count, *datatype, *dest, *tag, *comm, p_request);
  *request = (intptr_t)p_request;
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
  MPI_Request *p_request = TBX_MALLOC(sizeof(MPI_Request));
  *ierr = MPI_Recv_init(buf, *count, *datatype, *source, *tag, *comm, p_request);
  *request = (intptr_t)p_request;
}

/**
 * Fortran version for MPI_START
 */
void mpi_start_(int *request,
		int *ierr) {
  MPI_Request *p_request = (void *)*request;
  *ierr = MPI_Start(p_request);
}

/**
 * Fortran version for MPI_START_ALL
 */
void mpi_startall_(int *count,
		   int  array_of_requests[][MPI_REQUEST_SIZE],
		   int *ierr) {
  int i;
  for (i = 0; i < *count; i++) {
    MPI_Request *p_request = (void *)array_of_requests[i];
    *ierr = MPI_Start(p_request);
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
                    MPI_Op *op TBX_UNUSED) {
  TBX_FAILURE("unimplemented");
}

/**
 * Fortran version for MPI_OP_FREE
 */
void mpi_op_free_(MPI_Op *op) {
  TBX_FAILURE("unimplemented");
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
		      int *address,
		      int *ierr) {
  MPI_Aint _address;
  *ierr = MPI_Get_address(location, &_address);
  *address = _address;
}

/**
 * Fortran version for MPI_ADDRESS
 */
void mpi_address_(void *location,
		  int *address,
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
                        int *array_of_displacements,
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

#endif /* PM2_FORTRAN_TARGET_NONE */

mpir_internal_data_t *get_mpir_internal_data() {
  return &mpir_internal_data;
}

int MPI_Init(int *argc,
             char ***argv) {
  int ret;

  MPI_NMAD_LOG_IN();

  /*
   * Check size of opaque type MPI_Request is the same as the size of
   * our internal request type
   */
  MPI_NMAD_TRACE("sizeof(mpir_request_t) = %ld\n", (long)sizeof(mpir_request_t));
  MPI_NMAD_TRACE("sizeof(MPI_Request) = %ld\n", (long)sizeof(MPI_Request));
  assert(sizeof(mpir_request_t) <= sizeof(MPI_Request));

  /*
   * Lazy Puk initialization (it may already have been initialized in PadicoTM)
   */
  if(!padico_puk_initialized())
    {
      padico_puk_init(*argc, *argv);
    }

#ifdef CONFIG_PROTO_MAD3
  /*
   * Explicitely load NewMad_Launcher if proto_mad3 is selected
   */
  nm_mad3_launcher_init();
#endif

  /*
   * NewMad initialization is performed by component 'NewMad_Launcher'
   * (from proto_mad3 or from PadicoTM)
   */
  puk_adapter_t launcher = puk_adapter_resolve("NewMad_Launcher");
  launcher_instance = puk_adapter_instanciate(launcher);
  struct puk_receptacle_NewMad_Launcher_s r;
  puk_instance_indirect_NewMad_Launcher(launcher_instance, NULL, &r);
  (*r.driver->init)(r._status, argc, *argv, "Mad-MPI", &nm_so_load);

  /*
   * Internal initialisation
   */
  ret = mpir_internal_init(&mpir_internal_data, &r);

  MPI_NMAD_LOG_OUT();
  return ret;
}

int MPI_Init_thread(int *argc,
                    char ***argv,
                    int required TBX_UNUSED,
                    int *provided) {
  int err;

  MPI_NMAD_LOG_IN();

  err = MPI_Init(argc, argv);
#ifndef PIOMAN
  *provided = MPI_THREAD_SINGLE;
#else
  *provided = MPI_THREAD_MULTIPLE;
#endif

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Initialized(int *flag) {
  *flag = (launcher_instance != NULL);
  return MPI_SUCCESS;
}

int MPI_Finalize(void) {
  int err;

  MPI_NMAD_LOG_IN();

  err = mpir_internal_exit(&mpir_internal_data);

  puk_instance_destroy(launcher_instance);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Abort(MPI_Comm comm TBX_UNUSED,
              int errorcode) {
  int err;
  MPI_NMAD_LOG_IN();

  err = mpir_internal_exit(&mpir_internal_data);

  puk_instance_destroy(launcher_instance);

  MPI_NMAD_LOG_OUT();
  return errorcode;
}

int MPI_Comm_size(MPI_Comm comm,
                  int *size) {
  MPI_NMAD_LOG_IN();

  mpir_communicator_t *mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  *size = mpir_communicator->size;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) {
  MPI_NMAD_LOG_IN();

  mpir_communicator_t *mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  *rank = mpir_communicator->rank;
  MPI_NMAD_TRACE("My comm rank is %d\n", *rank);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Get_processor_name(char *name,
			   int *resultlen) {
  int err;

  MPI_NMAD_LOG_IN();

  err = gethostname(name, MPI_MAX_PROCESSOR_NAME);
  if (!err) {
    *resultlen = strlen(name);
    MPI_NMAD_LOG_OUT();
    return MPI_SUCCESS;
  }
  else {
    MPI_NMAD_LOG_OUT();
    return errno;
  }
}

double MPI_Wtime(void) {
  tbx_tick_t time;

  MPI_NMAD_LOG_IN();

  TBX_GET_TICK(time);
  double usec = tbx_tick2usec(time);

  MPI_NMAD_LOG_OUT();
  return usec / 1000000;
}

double MPI_Wtick(void) {
  return 1e-7;
}

int MPI_Error_string(int errorcode,
		     char *string,
		     int *resultlen) {
  *resultlen = 100;
  string = malloc(*resultlen * sizeof(char));
  switch (errorcode) {
    case MPI_SUCCESS : {
      snprintf(string, *resultlen, "Error MPI_SUCCESS\n");
      break;
    }
    case MPI_ERR_BUFFER : {
      snprintf(string, *resultlen, "Error MPI_ERR_BUFFER\n");
      break;
    }
    case MPI_ERR_COUNT : {
      snprintf(string, *resultlen, "Error MPI_ERR_COUNT\n");
      break;
    }
    case MPI_ERR_TYPE : {
      snprintf(string, *resultlen, "Error MPI_ERR_TYPE\n");
      break;
    }
    case MPI_ERR_TAG : {
      snprintf(string, *resultlen, "Error MPI_ERR_TAG\n");
      break;
    }
    case MPI_ERR_COMM : {
      snprintf(string, *resultlen, "Error MPI_ERR_COMM\n");
      break;
    }
    case MPI_ERR_RANK : {
      snprintf(string, *resultlen, "Error MPI_ERR_RANK\n");
      break;
    }
    case MPI_ERR_ROOT : {
      snprintf(string, *resultlen, "Error MPI_ERR_ROOT\n");
      break;
    }
    case MPI_ERR_GROUP : {
      snprintf(string, *resultlen, "Error MPI_ERR_GROUP\n");
      break;
    }
    case MPI_ERR_OP : {
      snprintf(string, *resultlen, "Error MPI_ERR_OP\n");
      break;
    }
    case MPI_ERR_TOPOLOGY : {
      snprintf(string, *resultlen, "Error MPI_ERR_TOPOLOGY\n");
      break;
    }
    case MPI_ERR_DIMS : {
      snprintf(string, *resultlen, "Error MPI_ERR_DIMS\n");
      break;
    }
    case MPI_ERR_ARG : {
      snprintf(string, *resultlen, "Error MPI_ERR_ARG\n");
      break;
    }
    case MPI_ERR_UNKNOWN : {
      snprintf(string, *resultlen, "Error MPI_ERR_UNKNOWN\n");
      break;
    }
    case MPI_ERR_TRUNCATE : {
      snprintf(string, *resultlen, "Error MPI_ERR_TRUNCATE\n");
      break;
    }
    case MPI_ERR_OTHER : {
      snprintf(string, *resultlen, "Error MPI_ERR_OTHER\n");
      break;
    }
    case MPI_ERR_INTERN : {
      snprintf(string, *resultlen, "Error MPI_ERR_INTERN\n");
      break;
    }
    case MPI_ERR_IN_STATUS : {
      snprintf(string, *resultlen, "Error MPI_ERR_IN_STATUS\n");
      break;
    }
    case MPI_ERR_PENDING : {
      snprintf(string, *resultlen, "Error MPI_ERR_PENDING\n");
      break;
    }
    case MPI_ERR_REQUEST : {
      snprintf(string, *resultlen, "Error MPI_ERR_REQUEST\n");
      break;
    }
    case MPI_ERR_LASTCODE : {
      snprintf(string, *resultlen, "Error MPI_ERR_LASTCODE\n");
      break;
    }
    default : {
      snprintf(string, *resultlen, "Error %d unknown\n", errorcode);
    }
  }
  return MPI_SUCCESS;
}

int MPI_Get_version(int *version,
		    int *subversion) {
  *version = MADMPI_VERSION;
  *subversion = MADMPI_SUBVERSION;
  return MPI_SUCCESS;
}

int MPI_Esend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Communication_Mode is_completed,
              MPI_Comm comm,
              MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t  *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Esending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buffer;
  mpir_request->count = count;
  mpir_request->user_tag = tag;
  mpir_request->communication_mode = is_completed;

  err = mpir_isend(&mpir_internal_data, mpir_request, dest, mpir_communicator);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm) {
  MPI_Request           request;
  MPI_Request          *request_ptr = &request;
  mpir_request_t       *mpir_request = (mpir_request_t *)request_ptr;
  mpir_communicator_t  *mpir_communicator;
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Sending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buffer;
  mpir_request->count = count;
  mpir_request->user_tag = tag;
  mpir_request->communication_mode = MPI_IMMEDIATE_MODE;

  err = mpir_isend(&mpir_internal_data, mpir_request, dest, mpir_communicator);

  MPI_Wait(&request, MPI_STATUS_IGNORE);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t  *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buffer;
  mpir_request->count = count;
  mpir_request->user_tag = tag;
  mpir_request->communication_mode = MPI_IMMEDIATE_MODE;

  err = mpir_isend(&mpir_internal_data, mpir_request, dest, mpir_communicator);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Rsend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm) {
  MPI_Request           request;
  MPI_Request          *request_ptr = &request;
  mpir_request_t       *mpir_request = (mpir_request_t *)request_ptr;
  mpir_communicator_t  *mpir_communicator;
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Rsending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buffer;
  mpir_request->count = count;
  mpir_request->user_tag = tag;
  mpir_request->communication_mode = MPI_READY_MODE;

  err = mpir_isend(&mpir_internal_data, mpir_request, dest, mpir_communicator);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Ssend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm) {
  MPI_Request           request;
  MPI_Request          *request_ptr = &request;
  mpir_request_t       *mpir_request = (mpir_request_t *)request_ptr;
  mpir_communicator_t  *mpir_communicator;
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Ssending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buffer;
  mpir_request->count = count;
  mpir_request->user_tag = tag;
  mpir_request->communication_mode = MPI_READY_MODE;

  err = mpir_isend(&mpir_internal_data, mpir_request, dest, mpir_communicator);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Pack(void* inbuf,
             int incount,
             MPI_Datatype datatype,
             void *outbuf,
             int outsize,
             int *position,
             MPI_Comm comm) {
  size_t size_datatype;
  void *ptr = outbuf;

  MPI_NMAD_LOG_IN();

  MPI_NMAD_TRACE("Packing %d data of datatype %d at position %d\n", incount, datatype, *position);

  size_datatype = mpir_sizeof_datatype(&mpir_internal_data, datatype);

  ptr += *position;
  memcpy(ptr, inbuf, incount*size_datatype);
  *position += incount*size_datatype;

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status) {
  MPI_Request           request;
  MPI_Request          *request_ptr = &request;
  mpir_request_t       *mpir_request = (mpir_request_t *)request_ptr;
  mpir_communicator_t  *mpir_communicator;
  int                  err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Receiving message from %d of datatype %d with tag %d, count %d\n", source, datatype, tag, count);
  MPI_NMAD_TIMER_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_ptr = NULL;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  mpir_request->request_type = MPI_REQUEST_RECV;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buffer;
  mpir_request->count = count;
  mpir_request->user_tag = tag;

  err = mpir_irecv(&mpir_internal_data, mpir_request, source, mpir_communicator);

  err = MPI_Wait(&request, status);

  MPI_NMAD_TIMER_OUT();
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Irecv(void *buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Ireceiving message from %d of datatype %d with tag %d\n", source, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_ptr = NULL;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  mpir_request->request_type = MPI_REQUEST_RECV;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buffer;
  mpir_request->count = count;
  mpir_request->user_tag = tag;

  err = mpir_irecv(&mpir_internal_data, mpir_request, source, mpir_communicator);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Sendrecv(void *sendbuf,
                 int sendcount,
                 MPI_Datatype sendtype,
                 int dest,
                 int sendtag,
                 void *recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int source,
                 int recvtag,
                 MPI_Comm comm,
                 MPI_Status *status) {
  int err;
  MPI_Request srequest, rrequest;

  MPI_NMAD_LOG_IN();

  err = MPI_Isend(sendbuf, sendcount, sendtype, dest, sendtag, comm, &srequest);
  err = MPI_Irecv(recvbuf, recvcount, recvtype, source, recvtag, comm, &rrequest);

  err = MPI_Wait(&srequest, MPI_STATUS_IGNORE);
  err = MPI_Wait(&rrequest, status);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Unpack(void* inbuf,
               int insize,
               int *position,
               void *outbuf,
               int outcount,
               MPI_Datatype datatype,
               MPI_Comm comm) {
  size_t size_datatype;
  void *ptr = inbuf;

  MPI_NMAD_LOG_IN();

  MPI_NMAD_TRACE("Unpacking %d data of datatype %d at position %d\n", outcount, datatype, *position);

  size_datatype = mpir_sizeof_datatype(&mpir_internal_data, datatype);

  ptr += *position;
  memcpy(outbuf, ptr, outcount*size_datatype);
  *position += outcount*size_datatype;

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) {
  mpir_request_t  *mpir_request = (mpir_request_t *)request;
  int                       err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  err = mpir_wait(&mpir_internal_data, mpir_request);

  if (status != MPI_STATUS_IGNORE) {
      err = mpir_set_status(&mpir_internal_data, request, status);
  }

  if (mpir_request->request_persistent_type == MPI_REQUEST_ZERO) {
    mpir_request->request_type = MPI_REQUEST_ZERO;
  }
  if (mpir_request->request_ptr != NULL) {
    FREE_AND_SET_NULL(mpir_request->request_ptr);
  }

  // Release one active communication for that type
  if (mpir_request->request_datatype > MPI_PACKED) {
    err = mpir_type_unlock(&mpir_internal_data, mpir_request->request_datatype);
  }

  MPI_NMAD_TRACE("Request completed\n");
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Waitall(int count,
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

int MPI_Waitany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                MPI_Status *status) {
  int flag, i, err;
  int count_null;

  MPI_NMAD_LOG_IN();

  while (1) {
    count_null = 0;
    for(i=0 ; i<count ; i++) {
            /*      MPI_Request request = array_of_requests[i];*/
      mpir_request_t *mpir_request = (mpir_request_t *)(&(array_of_requests[i]));
      if (mpir_request->request_type == MPI_REQUEST_ZERO) {
        count_null++;
        continue;
      }
      err = MPI_Test(&array_of_requests[i], &flag, status);
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

int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  int             err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  err = mpir_test(&mpir_internal_data, mpir_request);

  if (err == NM_ESUCCESS) {
    *flag = 1;

    if (status != MPI_STATUS_IGNORE) {
      err = mpir_set_status(&mpir_internal_data, request, status);
    }

    mpir_request->request_type = MPI_REQUEST_ZERO;
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                int *flag,
                MPI_Status *status) {
  int i, err = 0;
  mpir_request_t *mpir_request;

  MPI_NMAD_LOG_IN();

  for(i=0 ; i<count ; i++) {
    mpir_request = (mpir_request_t *)&(array_of_requests[i]);
    if (mpir_request->request_type == MPI_REQUEST_ZERO) {
      *flag = 1;
      goto out;
    }
    err = MPI_Test(&(array_of_requests[i]), flag, status);
    if (*flag == 1) {
      *rqindex = i;
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }
 out:
  *rqindex = MPI_UNDEFINED;

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status) {
  int err      = 0;
  nm_gate_id_t gate_id, out_gate_id;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tag == MPI_ANY_TAG) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  if (source == MPI_ANY_SOURCE) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    gate_id = mpir_get_in_gate_id(&mpir_internal_data, source);

    if (source >= mpir_communicator->size || gate_id == NM_SO_ANY_SRC) {
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", mpir_communicator->rank, source);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  }

  err = mpir_probe(&mpir_internal_data, gate_id, &out_gate_id, tag);

  if (err == NM_ESUCCESS) {
    *flag = 1;
    if (status != NULL) {
      status->MPI_TAG = tag;
      status->MPI_SOURCE = mpir_get_in_dest(&mpir_internal_data, out_gate_id);
    }
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status) {
  int flag = 0;
  int err;

  MPI_NMAD_LOG_IN();

  err = MPI_Iprobe(source, tag, comm, &flag, status);
  while (flag != 1) {
    err = MPI_Iprobe(source, tag, comm, &flag, status);
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Cancel(MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  int             err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  MPI_NMAD_TRACE("Request is cancelled\n");

  mpir_request->request_type = MPI_REQUEST_CANCELLED;
  if (mpir_request->contig_buffer != NULL) {
    FREE_AND_SET_NULL(mpir_request->contig_buffer);
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Request_free(MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;

  MPI_NMAD_LOG_IN();
  mpir_request->request_type = MPI_REQUEST_ZERO;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Get_count(MPI_Status *status,
                  MPI_Datatype datatype TBX_UNUSED,
                  int *count) {
  MPI_NMAD_LOG_IN();
  *count = status->count;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Request_is_equal(MPI_Request request1,
			 MPI_Request request2) {
  mpir_request_t *mpir_request1 = (mpir_request_t *)(&request1);
  mpir_request_t *mpir_request2 = (mpir_request_t *)(&request2);
  if (mpir_request1->request_type == MPI_REQUEST_ZERO) {
    return (mpir_request1->request_type == mpir_request2->request_type);
  }
  else if (mpir_request2->request_type == MPI_REQUEST_ZERO) {
    return (mpir_request1->request_type == mpir_request2->request_type);
  }
  else {
    return (mpir_request1->request_nmad.status == mpir_request2->request_nmad.status);
  }
}

int MPI_Send_init(void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int dest,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t  *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Init Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_persistent_type = MPI_REQUEST_SEND;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buf;
  mpir_request->count = count;
  mpir_request->user_tag = tag;
  mpir_request->communication_mode = MPI_IMMEDIATE_MODE;

  err = mpir_isend_init(&mpir_internal_data, mpir_request, dest, mpir_communicator);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Recv_init(void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int source,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t  *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Init Irecv message from %d of datatype %d with tag %d\n", source, datatype, tag);

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->request_ptr = NULL;
  mpir_request->request_type = MPI_REQUEST_RECV;
  mpir_request->request_persistent_type = MPI_REQUEST_RECV;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpir_request->buffer = buf;
  mpir_request->count = count;
  mpir_request->user_tag = tag;

  err = mpir_irecv_init(&mpir_internal_data, mpir_request, source, mpir_communicator);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Start(MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Start request\n");

  err =  mpir_start(&mpir_internal_data, mpir_request);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Startall(int count,
                 MPI_Request *array_of_requests) {
  int i;
  int err = MPI_SUCCESS;

  MPI_NMAD_LOG_IN();
  for(i=0 ; i<count ; i++) {
    MPI_Request request = array_of_requests[i];
    err = MPI_Start(&request);
    if (err != MPI_SUCCESS) {
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Barrier(MPI_Comm comm) {
  tbx_bool_t termination;

  MPI_NMAD_LOG_IN();

  termination = mpir_test_termination(&mpir_internal_data, comm);
  MPI_NMAD_TRACE("Result %d\n", termination);
  while (termination == tbx_false) {
    sleep(1);
    termination = mpir_test_termination(&mpir_internal_data, comm);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm) {
  int tag = 1;
  int err = NM_ESUCCESS;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);

  MPI_NMAD_TRACE("Entering a bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i;
    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Isend(buffer, count, datatype, i, tag, comm, &requests[i]);
      if (err != 0) {
        MPI_NMAD_LOG_OUT();
        return err;
      }
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
      if (err != 0) {
          MPI_NMAD_LOG_OUT();
          return err;
      }
    }
    FREE_AND_SET_NULL(requests);
    err = MPI_SUCCESS;
  }
  else {
    MPI_Request request;
    MPI_Irecv(buffer, count, datatype, root, tag, comm, &request);
    err = MPI_Wait(&request, MPI_STATUS_IGNORE);
  }
  MPI_NMAD_TRACE("End of bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Gather(void *sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               int root,
               MPI_Comm comm) {
  int tag = 2;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);

  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i, err;
    mpir_datatype_t *mpir_recv_datatype, *mpir_send_datatype;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    mpir_recv_datatype = mpir_get_datatype(&mpir_internal_data, recvtype);
    mpir_send_datatype = mpir_get_datatype(&mpir_internal_data, sendtype);

    // receive data from other processes
    for(i=0 ; i<mpir_communicator->size; i++) {
      if(i == root) continue;
      MPI_Irecv(recvbuf + (i * recvcount * mpir_recv_datatype->extent),
                recvcount, recvtype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }

    // copy local data for itself
    memcpy(recvbuf + (mpir_communicator->rank * mpir_recv_datatype->extent),
           sendbuf, sendcount * mpir_send_datatype->extent);

    // free memory
    FREE_AND_SET_NULL(requests);
  }
  else {
    MPI_Send(sendbuf, sendcount, sendtype, root, tag, comm);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Gatherv(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *displs,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm) {
  int tag = 3;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);

  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i, err;
    mpir_datatype_t *mpir_recv_datatype, *mpir_send_datatype;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    mpir_recv_datatype = mpir_get_datatype(&mpir_internal_data, recvtype);
    mpir_send_datatype = mpir_get_datatype(&mpir_internal_data, sendtype);

    // receive data from other processes
    for(i=0 ; i<mpir_communicator->size; i++) {
      if(i == root) continue;
      MPI_Irecv(recvbuf + (displs[i] * mpir_recv_datatype->extent),
                recvcounts[i], recvtype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }

    // copy local data for itself
    MPI_NMAD_TRACE("Copying local data from %p to %p with len %ld\n", sendbuf,
                   recvbuf + (displs[mpir_communicator->rank] * mpir_recv_datatype->extent),
                   (long) sendcount * mpir_send_datatype->extent);
    memcpy(recvbuf + (displs[mpir_communicator->rank] * mpir_recv_datatype->extent),
           sendbuf, sendcount * mpir_send_datatype->extent);

    // free memory
    FREE_AND_SET_NULL(requests);
  }
  else {
    MPI_Send(sendbuf, sendcount, sendtype, root, tag, comm);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Allgather(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  MPI_Comm comm) {
  int err;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);

  MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm);

  // Broadcast the result to all processes
  err = MPI_Bcast(recvbuf, mpir_communicator->size*recvcount, recvtype, 0, comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Allgatherv(void *sendbuf,
                   int sendcount,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int *recvcounts,
                   int *displs,
                   MPI_Datatype recvtype,
                   MPI_Comm comm) {
  int err, i, recvcount=0;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);

  // Broadcast the total receive count to all processes
  if (mpir_communicator->rank == 0) {
    for(i=0 ; i<mpir_communicator->size ; i++) {
      MPI_NMAD_TRACE("recvcounts[%d] = %d\n", i, recvcounts[i]);
      recvcount += recvcounts[i];
    }
    MPI_NMAD_TRACE("recvcount = %d\n", recvcount);
  }
  err = MPI_Bcast(&recvcount, 1, MPI_INT, 0, comm);

  // Gather on process 0
  MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, 0, comm);

  // Broadcast the result to all processes
  err = MPI_Bcast(recvbuf, recvcount, recvtype, 0, comm);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Scatter(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm) {
  int tag = 4;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);

  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i, err;
    mpir_datatype_t *mpir_recv_datatype, *mpir_send_datatype;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    mpir_recv_datatype = mpir_get_datatype(&mpir_internal_data, recvtype);
    mpir_send_datatype = mpir_get_datatype(&mpir_internal_data, sendtype);

    // send data to other processes
    for(i=0 ; i<mpir_communicator->size; i++) {
      if(i == root) continue;
      MPI_Isend(sendbuf + (i * sendcount * mpir_send_datatype->extent),
                sendcount, sendtype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }

    // copy local data for itself
    memcpy(recvbuf + (mpir_communicator->rank * mpir_recv_datatype->extent),
           sendbuf, sendcount * mpir_send_datatype->extent);

    // free memory
    FREE_AND_SET_NULL(requests);
  }
  else {
    MPI_Recv(recvbuf, recvcount, recvtype, root, tag, comm, MPI_STATUS_IGNORE);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Alltoall(void* sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 int recvcount,
		 MPI_Datatype recvtype,
		 MPI_Comm comm) {
  int tag = 5;
  int err;
  int i;
  MPI_Request *send_requests, *recv_requests;
  mpir_datatype_t *mpir_send_datatype,*mpir_recv_datatype;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  send_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
  recv_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));

  mpir_send_datatype = mpir_get_datatype(&mpir_internal_data, sendtype);
  mpir_recv_datatype = mpir_get_datatype(&mpir_internal_data, recvtype);

  for(i=0 ; i<mpir_communicator->size; i++) {
    if(i == mpir_communicator->rank)
      memcpy(recvbuf + (i * recvcount * mpir_recv_datatype->extent),
	     sendbuf + (i * sendcount * mpir_send_datatype->extent),
	     sendcount * mpir_send_datatype->extent);
    else
      {
	MPI_Irecv(recvbuf + (i * recvcount * mpir_recv_datatype->extent),
		  recvcount, recvtype, i, tag, comm, &recv_requests[i]);

	err = MPI_Isend(sendbuf + (i * sendcount * mpir_send_datatype->extent),
		       sendcount, sendtype, i, tag, comm, &send_requests[i]);

	if (err != 0) {
	  MPI_NMAD_LOG_OUT();
	  return err;
	}
      }
  }

  for(i=0 ; i<mpir_communicator->size ; i++) {
    if (i == mpir_communicator->rank) continue;
    MPI_Wait(&recv_requests[i], MPI_STATUS_IGNORE);
    MPI_Wait(&send_requests[i], MPI_STATUS_IGNORE);
  }

  FREE_AND_SET_NULL(send_requests);
  FREE_AND_SET_NULL(recv_requests);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Alltoallv(void* sendbuf,
		  int *sendcounts,
		  int *sdispls,
		  MPI_Datatype sendtype,
		  void *recvbuf,
		  int *recvcounts,
		  int *rdispls,
		  MPI_Datatype recvtype,
		  MPI_Comm comm) {
  int tag = 6;
  int err;
  int i;
  MPI_Request *send_requests, *recv_requests;
  mpir_datatype_t *mpir_send_datatype,*mpir_recv_datatype;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  send_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
  recv_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));

  mpir_send_datatype = mpir_get_datatype(&mpir_internal_data, sendtype);
  mpir_recv_datatype = mpir_get_datatype(&mpir_internal_data, recvtype);

  for(i=0 ; i<mpir_communicator->size; i++) {
    if(i == mpir_communicator->rank)
      memcpy(recvbuf + (rdispls[i] * mpir_recv_datatype->extent),
	     sendbuf + (sdispls[i] * mpir_send_datatype->extent),
	     sendcounts[i] * mpir_send_datatype->extent);
    else
      {
	MPI_Irecv(recvbuf + (rdispls[i] * mpir_recv_datatype->extent),
		  recvcounts[i], recvtype, i, tag, comm, &recv_requests[i]);

	err = MPI_Isend(sendbuf + (sdispls[i] * mpir_send_datatype->extent),
		       sendcounts[i], sendtype, i, tag, comm, &send_requests[i]);

	if (err != 0) {
	  MPI_NMAD_LOG_OUT();
	  return err;
	}
      }
  }

  for(i=0 ; i<mpir_communicator->size ; i++) {
    if (i == mpir_communicator->rank) continue;
    MPI_Wait(&recv_requests[i], MPI_STATUS_IGNORE);
    MPI_Wait(&send_requests[i], MPI_STATUS_IGNORE);
  }

  FREE_AND_SET_NULL(send_requests);
  FREE_AND_SET_NULL(recv_requests);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op) {
  int err;

  MPI_NMAD_LOG_IN();
  err = mpir_op_create(&mpir_internal_data, function, commute, op);
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Op_free(MPI_Op *op) {
  int err;

  MPI_NMAD_LOG_IN();
  err = mpir_op_free(&mpir_internal_data, op);
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Reduce(void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm) {
  int tag = 7;
  mpir_operator_t *operator;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  operator = mpir_get_operator(&mpir_internal_data, op);
  if (operator->function == NULL) {
    ERROR("Operation %d not implemented\n", op);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  if (mpir_communicator->rank == root) {
    // Get the input buffers of all the processes
    void **remote_sendbufs;
    MPI_Request *requests;
    int i;
    remote_sendbufs = malloc(mpir_communicator->size * sizeof(void *));
    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i == root) continue;
      remote_sendbufs[i] = malloc(count * mpir_sizeof_datatype(&mpir_internal_data, datatype));
      MPI_Irecv(remote_sendbufs[i], count, datatype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i == root) continue;
      MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }

    // Do the reduction operation
    memcpy(recvbuf, sendbuf, count*mpir_sizeof_datatype(&mpir_internal_data, datatype));
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i == root) continue;
      operator->function(remote_sendbufs[i], recvbuf, &count, &datatype);
    }

    // Free memory
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i == root) continue;
      FREE_AND_SET_NULL(remote_sendbufs[i]);
    }
    FREE_AND_SET_NULL(remote_sendbufs);
    FREE_AND_SET_NULL(requests);

    MPI_NMAD_LOG_OUT();
    return MPI_SUCCESS;
  }
  else {
    int err;
    err = MPI_Send(sendbuf, count, datatype, root, tag, comm);
    MPI_NMAD_LOG_OUT();
    return err;
  }
}

int MPI_Allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm) {
  int err;

  MPI_NMAD_LOG_IN();

  MPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);

  // Broadcast the result to all processes
  err = MPI_Bcast(recvbuf, count, datatype, 0, comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Reduce_scatter(void *sendbuf,
                       void *recvbuf,
                       int *recvcounts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm) {
  int err=MPI_SUCCESS, count=0, i;
  mpir_communicator_t *mpir_communicator;
  mpir_datatype_t *mpir_datatype;
  int tag = 8;
  void *reducebuf = NULL;

  MPI_NMAD_LOG_IN();

  mpir_datatype = mpir_get_datatype(&mpir_internal_data, datatype);
  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);
  for(i=0 ; i<mpir_communicator->size ; i++) {
    count += recvcounts[i];
  }

  if (mpir_communicator->rank == 0) {
    reducebuf = malloc(count * mpir_datatype->size);
  }
  MPI_Reduce(sendbuf, reducebuf, count, datatype, op, 0, comm);

  // Scatter the result
  if (mpir_communicator->rank == 0) {
    MPI_Request *requests;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));

    // send data to other processes
    for(i=1 ; i<mpir_communicator->size; i++) {
      MPI_Isend(reducebuf + (i * recvcounts[i] * mpir_datatype->extent),
                recvcounts[i], datatype, i, tag, comm, &requests[i]);
    }
    for(i=1 ; i<mpir_communicator->size ; i++) {
      err = MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }

    // copy local data for itself
    memcpy(recvbuf, reducebuf, recvcounts[0] * mpir_datatype->extent);

    // free memory
    FREE_AND_SET_NULL(requests);
  }
  else {
    err = MPI_Recv(recvbuf, recvcounts[mpir_communicator->rank], datatype, 0, tag, comm, MPI_STATUS_IGNORE);
  }

  if (mpir_communicator->rank == 0) {
    FREE_AND_SET_NULL(reducebuf);
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Get_address(void *location,
		    MPI_Aint *address) {
  /* This is the "portable" way to generate an address.
     The difference of two pointers is the number of elements
     between them, so this gives the number of chars between location
     and ptr.  As long as sizeof(char) == 1, this will be the number
     of bytes from 0 to location */
  //MPI_NMAD_LOG_IN();
  *address = (MPI_Aint) ((char *)location - (char *)MPI_BOTTOM);
  //MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Address(void *location,
		MPI_Aint *address) {
  return MPI_Get_address(location, address);
}

int MPI_Type_size(MPI_Datatype datatype,
		  int *size) {
  return mpir_type_size(&mpir_internal_data, datatype, size);
}

int MPI_Type_get_extent(MPI_Datatype datatype,
			MPI_Aint *lb,
			MPI_Aint *extent) {
  return mpir_type_get_lb_and_extent(&mpir_internal_data, datatype, lb, extent);
}

int MPI_Type_extent(MPI_Datatype datatype,
		    MPI_Aint *extent) {
  return mpir_type_get_lb_and_extent(&mpir_internal_data, datatype, NULL, extent);
}

int MPI_Type_lb(MPI_Datatype datatype,
		MPI_Aint *lb) {
  return mpir_type_get_lb_and_extent(&mpir_internal_data, datatype, lb, NULL);
}

int MPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype) {
  int err;

  MPI_NMAD_LOG_IN();
  if (lb != 0) {
    ERROR("<Using lb not equals to 0> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }
  err = mpir_type_create_resized(&mpir_internal_data, oldtype, lb, extent, newtype);

  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Type_commit(MPI_Datatype *datatype) {
  return mpir_type_commit(&mpir_internal_data, *datatype);
}

int MPI_Type_free(MPI_Datatype *datatype) {
  int err = mpir_type_free(&mpir_internal_data, *datatype);
  if (err == MPI_SUCCESS) {
    *datatype = MPI_DATATYPE_NULL;
  }
  return err;
}

int MPI_Type_optimized(MPI_Datatype *datatype,
                       int optimized) {
  return mpir_type_optimized(&mpir_internal_data, *datatype, optimized);
}

int MPI_Type_contiguous(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype) {
  return mpir_type_contiguous(&mpir_internal_data, count, oldtype, newtype);
}

int MPI_Type_vector(int count,
                    int blocklength,
                    int stride,
                    MPI_Datatype oldtype,
                    MPI_Datatype *newtype) {
  int hstride = stride * mpir_sizeof_datatype(&mpir_internal_data, oldtype);
  return mpir_type_vector(&mpir_internal_data, count, blocklength, hstride, oldtype, newtype);
}

int MPI_Type_hvector(int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  return mpir_type_vector(&mpir_internal_data, count, blocklength, stride, oldtype, newtype);
}

int MPI_Type_indexed(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  int err, i;

  MPI_NMAD_LOG_IN();

  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  for(i=0 ; i<count ; i++) {
    displacements[i] = array_of_displacements[i];
  }

  err = mpir_type_indexed(&mpir_internal_data, count, array_of_blocklengths, displacements, oldtype, newtype);

  FREE_AND_SET_NULL(displacements);
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  int err, i;
  size_t old_size;

  MPI_NMAD_LOG_IN();

  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  old_size = mpir_sizeof_datatype(&mpir_internal_data, oldtype);
  for(i=0 ; i<count ; i++) {
    displacements[i] = array_of_displacements[i] * old_size;
  }

  err = mpir_type_indexed(&mpir_internal_data, count, array_of_blocklengths, displacements, oldtype, newtype);

  FREE_AND_SET_NULL(displacements);
  MPI_NMAD_LOG_OUT();
  return err;
}

int MPI_Type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype) {
  return mpir_type_struct(&mpir_internal_data, count, array_of_blocklengths, array_of_displacements, array_of_types, newtype);
}

int MPI_Comm_group(MPI_Comm comm,
		   MPI_Group *group) {
  MPI_NMAD_LOG_IN();

  *group = comm;

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Comm_split(MPI_Comm comm,
		   int color,
		   int key,
		   MPI_Comm *newcomm) {
  int *sendbuf, *recvbuf;
  int i, j, nb_conodes;
  int **conodes;
  mpir_communicator_t *mpir_communicator;
  mpir_communicator_t *mpir_newcommunicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, comm);

  sendbuf = malloc(3*mpir_communicator->size*sizeof(int));
  for(i=0 ; i<mpir_communicator->size*3 ; i+=3) {
    sendbuf[i] = color;
    sendbuf[i+1] = key;
    sendbuf[i+2] = mpir_communicator->global_ranks[mpir_communicator->rank];
  }
  recvbuf = malloc(3*mpir_communicator->size*sizeof(int));

#ifdef NMAD_DEBUG
  MPI_NMAD_TRACE_LEVEL(3, "[%d] Sending: ", mpir_communicator->rank);
  for(i=0 ; i<mpir_communicator->size*3 ; i++) {
    MPI_NMAD_TRACE_NOF_LEVEL(3, "%d ", sendbuf[i]);
  }
  MPI_NMAD_TRACE_NOF_LEVEL(3, "\n");
#endif /* NMAD_DEBUG */

  MPI_Alltoall(sendbuf, 3, MPI_INT, recvbuf, 3, MPI_INT, comm);

#ifdef NMAD_DEBUG
  MPI_NMAD_TRACE_LEVEL(3, "[%d] Received: ", mpir_communicator->rank);
  for(i=0 ; i<mpir_communicator->size*3 ; i++) {
    MPI_NMAD_TRACE_NOF_LEVEL(3, "%d ", recvbuf[i]);
  }
  MPI_NMAD_TRACE_NOF_LEVEL(3, "\n");
#endif /* NMAD_DEBUG */

  // Counting how many nodes have the same color
  nb_conodes=0;
  for(i=0 ; i<mpir_communicator->size*3 ; i+=3) {
    if (recvbuf[i] == color) {
      nb_conodes++;
    }
  }

  // Accumulating the nodes with the same color into an array
  conodes = malloc(nb_conodes*sizeof(int*));
  j=0;
  for(i=0 ; i<mpir_communicator->size*3 ; i+=3) {
    if (recvbuf[i] == color) {
      conodes[j] = &recvbuf[i];
      j++;
    }
  }

  // Sorting the nodes with the same color according to their key
  {
    int nodecmp(const void *v1, const void *v2) {
      int **node1 = (int **)v1;
      int **node2 = (int **)v2;
      return ((*node1)[1] - (*node2)[1]);
    }
    qsort(conodes, nb_conodes, sizeof(int *), nodecmp);
  }

#ifdef NMAD_DEBUG
  MPI_NMAD_TRACE_LEVEL(3, "[%d] Conodes: ", mpir_communicator->rank);
  for(i=0 ; i<nb_conodes ; i++) {
    int *ptr = conodes[i];
    MPI_NMAD_TRACE_NOF_LEVEL(3, "[%d %d %d] ", *ptr, *(ptr+1), *(ptr+2));
  }
  MPI_NMAD_TRACE_NOF_LEVEL(3, "\n");
#endif /* NMAD_DEBUG */

  // Create the new communicator
  if (color == MPI_UNDEFINED) {
    *newcomm = MPI_COMM_NULL;
  }
  else {
    MPI_Comm_dup(comm, newcomm);
    mpir_newcommunicator = mpir_get_communicator(&mpir_internal_data, *newcomm);
    mpir_newcommunicator->size = nb_conodes;
    FREE_AND_SET_NULL(mpir_newcommunicator->global_ranks);
    mpir_newcommunicator->global_ranks = malloc(nb_conodes*sizeof(int));
    for(i=0 ; i<nb_conodes ; i++) {
      int *ptr = conodes[i];
      if ((*(ptr+1) == key) && (*(ptr+2) == mpir_communicator->global_ranks[mpir_communicator->rank])) {
        mpir_newcommunicator->rank = i;
      }
      if (*ptr == color) {
        mpir_newcommunicator->global_ranks[i] = *(ptr+2);
      }
    }
  }

  // free the allocated area
  FREE_AND_SET_NULL(conodes);
  FREE_AND_SET_NULL(sendbuf);
  FREE_AND_SET_NULL(recvbuf);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Comm_dup(MPI_Comm comm,
		 MPI_Comm *newcomm) {
  return mpir_comm_dup(&mpir_internal_data, comm, newcomm);
}

int MPI_Comm_free(MPI_Comm *comm) {
  return mpir_comm_free(&mpir_internal_data, comm);
}

int MPI_Group_translate_ranks(MPI_Group group1,
			      int n,
			      int *ranks1,
			      MPI_Group group2,
			      int *ranks2) {
  int i, j, x;
  mpir_communicator_t *mpir_communicator;
  mpir_communicator_t *mpir_communicator2;

  MPI_NMAD_LOG_IN();

  mpir_communicator = mpir_get_communicator(&mpir_internal_data, group1);
  mpir_communicator2 = mpir_get_communicator(&mpir_internal_data, group2);
  for(i=0 ; i<n ; i++) {
    ranks2[i] = MPI_UNDEFINED;
    if (ranks1[i] < mpir_communicator->size) {
      x = mpir_communicator->global_ranks[ranks1[i]];
      for(j=0 ; j<mpir_communicator2->size ; j++) {
        if (mpir_communicator2->global_ranks[j] == x) {
          ranks2[i] = j;
          break;
        }
      }
    }
    else {
      return MPI_ERR_RANK;
    }
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}
