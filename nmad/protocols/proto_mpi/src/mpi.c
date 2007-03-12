/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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
#include "nm_so_parameters.h"
#include <stdint.h>

static p_mad_madeleine_t       madeleine	= NULL;
static int                     global_size	= -1;
static int                     process_rank	= -1;
static struct nm_so_interface *p_so_sr_if;
static nm_so_pack_interface    p_so_pack_if;
static long                   *out_gate_id	= NULL;
static long                   *in_gate_id	= NULL;
static int                    *out_dest	 	= NULL;
static int                    *in_dest		= NULL;

/* GFortran iargc/getargc bindings
 */
void _gfortran_getarg_i4(int32_t *num, char *value, int val_len)	__attribute__ ((weak));

void _gfortran_getarg_i8(int64_t *num, char *value, int val_len)	__attribute__ ((weak));

int32_t _gfortran_iargc(void)	__attribute__ ((weak));


#define MAX_ARG_LEN 64
/** Initialisation by Fortran code.
 */
int mpi_init_() {
        int argc;
        int i;
        char **argv;

        argc = 1+_gfortran_iargc();
        fprintf(stderr, "argc = %d\n", argc);
        argv = malloc(argc * sizeof(char *));
        for (i = 0; i < argc; i++) {
                int j;

                argv[i] = malloc(MAX_ARG_LEN+1);
                if (sizeof(char*) == 4) {
                        _gfortran_getarg_i4((int32_t *)&i, argv[i], MAX_ARG_LEN);
                } else {
                        _gfortran_getarg_i8((int64_t *)&i, argv[i], MAX_ARG_LEN);
                }

                j = MAX_ARG_LEN;
                while (j > 1 && (argv[i])[j-1] == ' ') {
                        j--;
                }
                (argv[i])[j] = '\0';
        }
        for (i = 0; i < argc; i++) {
                fprintf(stderr, "argv[%d] = [%s]\n", i, argv[i]);
        }
        return MPI_Init(&argc, &argv);
}


/* Alias Fortran
 */

void mpi_init_thread_(int *argc,
                    char ***argv,
                    int required,
                    int *provided)	__attribute__ ((alias ("MPI_Init_thread")));

void mpi_finalize_(void)			__attribute__ ((alias ("MPI_Finalize")));

void mpi_abort_(MPI_Comm comm,
              int errorcode)		__attribute__ ((alias ("MPI_Abort")));

void mpi_comm_size_(int *comm,
                   int *size, int *ierr) {
        *ierr = MPI_Comm_size(*comm, size);
}



void mpi_comm_rank_(int *comm, int *rank, int *ierr) {
        *ierr = MPI_Comm_rank(*comm, rank);
}


void mpi_get_processor_name_(char *name, int *resultlen) __attribute__ ((alias ("MPI_Get_processor_name")));

/*
void mpi_esend_(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Extended is_completed,
              MPI_Comm comm,
               MPI_Request *request) {
}
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

void mpi_rsend_(void *buffer,
              int *count,
              int *datatype,
              int *dest,
              int *tag,
              int *comm,
              int *ierr) {
        *ierr = MPI_Rsend(buffer, *count, *datatype, *dest, *tag, *comm);
}

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
        memcpy(status, &_status, sizeof(_status));
}

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

void mpi_wait_(int *request,
               int *status,
               int *ierr) {
        MPI_Status _status;
        MPI_Request *p_request = (void *)*request;
        *ierr = MPI_Wait(p_request, &_status);
        memcpy(status, &_status, sizeof(_status));
        TBX_FREE((void *)*request);
}

void mpi_waitall_(int *count,
                  int *request,
                  int  status[][MPI_STATUS_SIZE],
                  int *ierr) {
  int err = NM_ESUCCESS;
  int i;

  if ((MPI_Status *)status == MPI_STATUSES_IGNORE) {
    for (i = 0; i < *count; i++) {
        MPI_Request *p_request = (void *)request[i];
        err =  MPI_Wait(p_request, MPI_STATUS_IGNORE);
        TBX_FREE(p_request);
        if (err != NM_ESUCCESS)
          goto out;
    }
  } else {
    for (i = 0; i < *count; i++) {
        MPI_Status _status;
        MPI_Request *p_request = (void *)request[i];
        err =  MPI_Wait(p_request, &_status);
        memcpy(status[i], &_status, sizeof(_status));
        TBX_FREE((void *)*request);
        if (err != NM_ESUCCESS)
          goto out;
    }
  }

 out:
  *ierr = err;
}

void mpi_test_(int *request,
               int *flag,
               int *status,
               int *ierr) {
        MPI_Status _status;
        MPI_Request *p_request = (void *)*request;
        *ierr = MPI_Test(p_request, flag, &_status);        
        if (*ierr != MPI_SUCCESS)
                return;
        memcpy(status, &_status, sizeof(_status));
        TBX_FREE((void *)*request);
}

void mpi_testany_(int count,
                MPI_Request *array_of_requests,
                int *index,
                int *flag,
                MPI_Status *status) {
        TBX_FAILURE("unimplemented");
}

void mpi_iprobe_(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status) {
        TBX_FAILURE("unimplemented");
}

void mpi_probe_(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status) {
        TBX_FAILURE("unimplemented");
}

void mpi_get_count_(int *status,
                    int *datatype,
                    int *count,
                    int *ierr) {
        MPI_Status _status;
        memcpy(&_status, status, sizeof(_status));
        *ierr = MPI_Get_count(&_status, *datatype, count);
}

void mpi_request_is_equal_(MPI_Request request1, MPI_Request request2) {
        TBX_FAILURE("unimplemented");
}

void mpi_barrier_(int *comm,
                  int *ierr) {
        *ierr = MPI_Barrier(*comm);
}

void mpi_bcast_(void *buffer,
                int *count,
                int *datatype,
                int *root,
                int *comm,
                int *ierr) {
        *ierr = MPI_Bcast(buffer, *count, *datatype, *root, *comm);
}

void mpi_alltoall_(void *sendbuf,
		   int *sendcount,
		   int *sendtype,
		   void *recvbuf,
		   int *recvcount,
		   int *recvtype,
		   int *comm,
		   int *ierr) {
 *ierr = MPI_Alltoall(sendbuf,*sendcount,*sendtype,
		      recvbuf,*recvcount,*recvtype,
		      *comm);
}

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
  *ierr = MPI_Alltoallv(sendbuf,sendcount,sdispls,*sendtype,
			recvbuf,recvcount,rdispls,*recvtype,
			*comm );
}

void mpi_op_create_(MPI_User_function *function,
                  int commute,
                  MPI_Op *op) {
        TBX_FAILURE("unimplemented");
}

void mpi_op_free_(MPI_Op *op) {
        TBX_FAILURE("unimplemented");
}

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

void mpi_allreduce_(void *sendbuf,
                    void *recvbuf,
                    int *count,
                    int *datatype,
                    int *op,
                    int *comm,
                    int *ierr) {
        *ierr = MPI_Allreduce(sendbuf, recvbuf, *count, *datatype, *op,
                              *comm);
}

double mpi_wtime_(void) {
        return MPI_Wtime();
}

double mpi_wtick_(void) {
        return MPI_Wtick();
}

void mpi_get_address_(void *location, MPI_Aint *address) {
        TBX_FAILURE("unimplemented");
}

void mpi_address_(void *location, MPI_Aint *address) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_size_(MPI_Datatype datatype, int *size) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_commit_(MPI_Datatype *datatype) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_free_(MPI_Datatype *datatype) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_contiguous_(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_vector_(int count,
                    int blocklength,
                    int stride,
                    MPI_Datatype oldtype,
                    MPI_Datatype *newtype) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_hvector_(int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_indexed_(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_hindexed_(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
        TBX_FAILURE("unimplemented");
}

void mpi_type_struct_(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype) {
        TBX_FAILURE("unimplemented");
}

void mpi_comm_dup_(MPI_Comm comm, MPI_Comm *newcomm) {
        TBX_FAILURE("unimplemented");
}

void mpi_comm_free_(MPI_Comm *comm) {
        TBX_FAILURE("unimplemented");
}

/**
 * This routine must be called before any other MPI routine. It must
 * be called at most once; subsequent calls are erroneous.
 */
int MPI_Init(int *argc,
             char ***argv) {

  p_mad_session_t                  session    = NULL;
  struct nm_core                  *p_core     = NULL;
  int                              dest;
  int                              source;
  p_mad_channel_t                  channel    = NULL;
  p_mad_connection_t               connection = NULL;
  p_mad_nmad_connection_specific_t cs	      = NULL;


  MPI_NMAD_LOG_IN();

  /*
   * Check size of opaque type MPI_Request is the same as the size of
   * our internal request type
   */
  MPI_NMAD_TRACE("sizeof(struct MPI_Request_s) = %ld\n", (unsigned long)sizeof(struct MPI_Request_s));
  MPI_NMAD_TRACE("sizeof(MPI_Request) = %ld\n", (unsigned long)sizeof(MPI_Request));
  assert(sizeof(struct MPI_Request_s) <= sizeof(MPI_Request));

  /*
   * Initialization of various libraries.
   * Reference to the Madeleine object.
   */
  madeleine    = mad_init(argc, *argv);

  /*
   * Reference to the session information object
   */
  session      = madeleine->session;

  /*
   * Globally unique process rank.
   */
  process_rank = session->process_rank;

  /*
   * How to obtain the configuration size.
   */
  global_size = tbx_slist_get_length(madeleine->dir->process_slist);
  //printf("The configuration size is %d\n", global_size);

  /*
   * Reference to the NewMadeleine core object
   */
  p_core = mad_nmad_get_core();
  p_so_sr_if = mad_nmad_get_sr_interface();
  p_so_pack_if = (nm_so_pack_interface)p_so_sr_if;

  /*
   * Internal initialisation
   */
  internal_init();

  /*
   * Store the gate id of all the other processes
   */
  out_gate_id = malloc(global_size * sizeof(long));
  in_gate_id = malloc(global_size * sizeof(long));
  out_dest = malloc(256 * sizeof(int));
  in_dest = malloc(256 * sizeof(int));

  /* Get a reference to the channel structure */
  channel = tbx_htable_get(madeleine->channel_htable, "pm2");

  /* If that fails, it means that our process does not belong to the channel */
  if (!channel) {
    fprintf(stderr, "I don't belong to this channel");
    MPI_NMAD_LOG_OUT();
    return 1;
  }

  for(dest=0 ; dest<global_size ; dest++) {
    out_gate_id[dest] = -1;
    if (dest == process_rank) continue;

    connection = tbx_darray_get(channel->out_connection_darray, dest);
    if (!connection) {
      fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    }
    else {
      MPI_NMAD_TRACE("Connection out: %p\n", connection);
      cs = connection->specific;
      out_gate_id[dest] = cs->gate_id;
      out_dest[cs->gate_id] = dest;
    }
  }

  for(source=0 ; source<global_size ; source++) {
    in_gate_id[source] = -1;
    if (source == process_rank) continue;

    connection =  tbx_darray_get(channel->in_connection_darray, source);
    if (!connection) {
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
    }
    else {
      MPI_NMAD_TRACE("Connection in: %p\n", connection);
      cs = connection->specific;
      in_gate_id[source] = cs->gate_id;
      in_dest[cs->gate_id] = source;
    }
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * The following function may be used to initialize MPI, and
 * initialize the MPI thread environment, instead of MPI_Init.
 *
 * @param argc a pointer to the process argc.
 * @param argv a pointer to the process argv.
 * @param required level of thread support (integer).
 * @param provided level of thread support (integer).
 * @return The MPI status.
 */
int MPI_Init_thread(int *argc,
                    char ***argv,
                    int required,
                    int *provided) {
  int err;

  MPI_NMAD_LOG_IN();

  err = MPI_Init(argc, argv);
  *provided = MPI_THREAD_SINGLE;

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * This routine must be called by each process before it exits. The
 * call cleans up all MPI state.
 */
int MPI_Finalize(void) {
  MPI_NMAD_LOG_IN();

  free(out_gate_id);
  free(in_gate_id);
  free(out_dest);
  free(in_dest);

  internal_exit();
  mad_exit(madeleine);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * This routine makes a ``best attempt'' to abort all tasks in the group of comm.
 */
int MPI_Abort(MPI_Comm comm,
              int errorcode) {
  MPI_NMAD_LOG_IN();
  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  mad_exit(madeleine);
  free(out_gate_id);
  free(in_gate_id);
  free(out_dest);
  free(in_dest);
  internal_exit();

  MPI_NMAD_LOG_OUT();
  return errorcode;
}

/**
 * This function indicates the number of processes involved in a an
 * intracommunicator.
 */
int MPI_Comm_size(MPI_Comm comm,
                  int *size) {
  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  *size = global_size;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * This function gives the rank of the process in the particular
 * communicator's group.
 */
int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) {
  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  *rank = process_rank;
  MPI_NMAD_TRACE("My comm rank is %d\n", *rank);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * This routine returns the name of the processor on which it was
 * called at the moment of the call.
 */
int MPI_Get_processor_name(char *name, int *resultlen) {
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

__inline__
int mpi_inline_isend(void *buffer,
                     int count,
                     MPI_Datatype datatype,
                     int dest,
                     int tag,
                     MPI_Extended is_completed,
                     MPI_Comm comm,
                     MPI_Request *request) {
  long                  gate_id;
  mpir_datatype_t      *mpir_datatype = NULL;
  int                   err = MPI_SUCCESS;
  int                   nmad_tag;
  int                   seq, probe;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;

  gate_id = out_gate_id[dest];

  mpir_datatype = get_datatype(datatype);
  nmad_tag = mpir_project_comm_and_tag(comm, tag);

  if (tbx_unlikely(nmad_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid sending tag %d (%d, %d). Maximum allowed tag: %d\n", nmad_tag, comm, tag, NM_SO_MAX_TAGS);
    return 1;
  }

  seq = nm_so_sr_get_current_send_seq(p_so_sr_if, gate_id, nmad_tag);
  probe = nm_so_sr_stest_range(p_so_sr_if, gate_id, nmad_tag, seq-1, 1);
  if ((seq == NM_SO_PENDING_PACKS_WINDOW-1) && (probe == -NM_EAGAIN)) {
    MPI_NMAD_TRACE("Reaching maximum sequence number in emission. Trigger automatic flushing");
    nm_so_sr_swait_range(p_so_sr_if, gate_id, nmad_tag, 0, seq-1);
    MPI_NMAD_TRACE("Automatic flushing over");
  }

  _request->request_ptr = NULL;
  _request->contig_buffer = NULL;
  MPI_NMAD_TRACE("Sending to %d with tag %d (%d, %d)\n", dest, nmad_tag, comm, tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Sending data of type %d at address %p with len %lu (%d*%lu)\n", datatype, buffer, (unsigned long)count*sizeof_datatype(datatype), count, (unsigned long)sizeof_datatype(datatype));
    MPI_NMAD_TRANSFER("[%s] Sent (contig) --> %d, %ld : %lu bytes\n", __TBX_FUNCTION__, dest, gate_id, count * sizeof_datatype(datatype));
    if (is_completed == MPI_DO_NOT_USE_EXTENDED) {
      err = nm_so_sr_isend(p_so_sr_if, gate_id, nmad_tag, buffer, count * sizeof_datatype(datatype), &(_request->request_id));
    }
    else {
      err = nm_so_sr_isend_extended(p_so_sr_if, gate_id, nmad_tag, buffer, count * sizeof_datatype(datatype), is_completed, &(_request->request_id));
    }
    MPI_NMAD_TRANSFER("[%s] Sent finished\n", __TBX_FUNCTION__);
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j;
    void             *ptr = buffer;

    MPI_NMAD_TRACE("Sending (h)vector type: stride %d - blocklen %d - count %d - size %lu\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, (unsigned long)mpir_datatype->size);
    nm_so_begin_packing(p_so_pack_if, gate_id, nmad_tag, connection);
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_pack(connection, ptr, mpir_datatype->block_size);
        ptr += mpir_datatype->stride;
      }
    }
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_PACK_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED || mpir_datatype->dte_type == MPIR_HINDEXED) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j;
    void             *ptr = buffer;

    MPI_NMAD_TRACE("Sending (h)indexed type: count %d - size %lu\n", mpir_datatype->elements, (unsigned long)mpir_datatype->size);
    nm_so_begin_packing(p_so_pack_if, gate_id, nmad_tag, connection);
    for(i=0 ; i<count ; i++) {
      ptr = buffer + i * mpir_datatype->size;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, (unsigned long)i*mpir_datatype->size);
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        ptr += mpir_datatype->indices[j];
        nm_so_pack(connection, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_size);
        ptr -= mpir_datatype->indices[j];
      }
    }
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_PACK_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    if (mpir_datatype->is_optimized) {
      struct nm_so_cnx *connection = &(_request->request_cnx);
      int               i, j;
      void             *ptr = buffer;

      MPI_NMAD_TRACE("Sending struct type: size %lu\n", (unsigned long)mpir_datatype->size);
      nm_so_begin_packing(p_so_pack_if, gate_id, nmad_tag, connection);
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i * mpir_datatype->extent;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, (unsigned long)i*mpir_datatype->extent);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("packing data at %p (+%lu) with a size %d*%lu\n", ptr, (unsigned long)mpir_datatype->indices[j], mpir_datatype->blocklens[j], (unsigned long)mpir_datatype->old_sizes[j]);
          nm_so_pack(connection, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          ptr -= mpir_datatype->indices[j];
        }
      }
      if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_PACK_SEND;
    }
    else {
      void *newptr, *ptr;
      size_t len;
      int i, j;

      MPI_NMAD_TRACE("Sending struct datatype in a contiguous buffer\n");
      len = count * sizeof_datatype(datatype);
      _request->contig_buffer = malloc(len);
      if (_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %lu to send struct datatype\n", (unsigned long)len);
        return  -1;
      }

      newptr = _request->contig_buffer;
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i * mpir_datatype->extent;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, (unsigned long)i*mpir_datatype->extent);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("packing to %p data from %p (+%ld) with a size %d*%lu\n", newptr, ptr, (unsigned long)mpir_datatype->indices[j], mpir_datatype->blocklens[j], (unsigned long)mpir_datatype->old_sizes[j]);
          memcpy(newptr, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          newptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j];
          ptr -= mpir_datatype->indices[j];
        }
      }
      MPI_NMAD_TRACE("Sending data of struct type at address %p with len %lu (%d*%lu)\n", _request->contig_buffer, (unsigned long)len, count, (unsigned long)sizeof_datatype(datatype));
      MPI_NMAD_TRANSFER("[%s] Sent (struct) --> %d, %ld: %lu bytes\n", __TBX_FUNCTION__, dest, gate_id, count * sizeof_datatype(datatype));
      if (is_completed == MPI_DO_NOT_USE_EXTENDED) {
        err = nm_so_sr_isend(p_so_sr_if, gate_id, nmad_tag, _request->contig_buffer, len, &(_request->request_id));
      }
      else {
        err = nm_so_sr_isend_extended(p_so_sr_if, gate_id, nmad_tag, _request->contig_buffer, len, is_completed, &(_request->request_id));
      }
      MPI_NMAD_TRANSFER("[%s] Sent (struct) finished\n", __TBX_FUNCTION__);
      if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_SEND;
    }
  }
  else {
    ERROR("Do not know how to send datatype %d\n", datatype);
    return -1;
  }

  inc_nb_outgoing_msg();
  return err;
}

/**
 * Performs a extended send
 */
int MPI_Esend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Extended is_completed,
              MPI_Comm comm,
              MPI_Request *request) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  if (tbx_unlikely(dest >= global_size || out_gate_id[dest] == -1)) {
    fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    MPI_NMAD_LOG_OUT();
    return 1;
  }

  _request->request_type = MPI_REQUEST_SEND;
  _request->request_ptr = NULL;
  _request->contig_buffer = NULL;
  err = mpi_inline_isend(buffer, count, datatype, dest, tag, is_completed, comm, request);

  MPI_NMAD_LOG_OUT();
  return err;
}


/**
 * Performs a standard-mode, blocking send.
 */
int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm) {
  MPI_Request           request;
  MPI_Request          *request_ptr = &request;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request_ptr;
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Sending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  _request->request_type = MPI_REQUEST_SEND;
  _request->request_ptr = NULL;
  _request->contig_buffer = NULL;
  mpi_inline_isend(buffer, count, datatype, dest, tag, MPI_DO_NOT_USE_EXTENDED, comm, &request);

  if (_request->request_type == MPI_REQUEST_SEND) {
    MPI_NMAD_TRACE("Calling nm_so_sr_swait\n");
    MPI_NMAD_TRANSFER("[%s] Calling nm_so_sr_swait\n", __TBX_FUNCTION__);
    err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
    MPI_NMAD_TRANSFER("[%s] Returning from nm_so_sr_swait\n", __TBX_FUNCTION__);
    if (_request->contig_buffer != NULL) {
      free(_request->contig_buffer);
    }
  }
  else if (_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    MPI_NMAD_TRACE("Waiting for completion end_packing\n");
    err = nm_so_end_packing(connection);
  }

  if (_request->request_ptr != NULL) {
    free(_request->request_ptr);
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Posts a standard-mode, non blocking send.
 */
int MPI_Isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  if (tbx_unlikely(dest >= global_size || out_gate_id[dest] == -1)) {
    fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    MPI_NMAD_LOG_OUT();
    return 1;
  }

  _request->request_type = MPI_REQUEST_SEND;
  _request->request_ptr = NULL;
  _request->contig_buffer = NULL;
  err = mpi_inline_isend(buffer, count, datatype, dest, tag, MPI_DO_NOT_USE_EXTENDED, comm, request);

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Performs a ready-mode, blocking send.
 */
int MPI_Rsend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm) {
  MPI_Request           request;
  MPI_Request          *request_ptr = &request;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request_ptr;
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Rsending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  _request->request_type = MPI_REQUEST_SEND;
  _request->request_ptr = NULL;
  _request->contig_buffer = NULL;
  mpi_inline_isend(buffer, count, datatype, dest, tag, MPI_DO_NOT_USE_EXTENDED, comm, &request);

  if (_request->request_type == MPI_REQUEST_SEND) {
    MPI_NMAD_TRACE("Calling nm_so_sr_swait\n");
    MPI_NMAD_TRANSFER("Calling nm_so_sr_swait\n");
    err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
    if (_request->contig_buffer != NULL) {
      free(_request->contig_buffer);
    }
  }
  else if (_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    MPI_NMAD_TRACE("Waiting for completion end_packing\n");
    err = nm_so_end_packing(connection);
  }

  if (_request->request_ptr != NULL) {
    free(_request->request_ptr);
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

__inline__
int mpi_inline_irecv(void* buffer,
                     int count,
                     MPI_Datatype datatype,
                     int source,
                     int tag,
                     MPI_Comm comm,
                     MPI_Request *request) {
  int                   err      = 0;
  long                  gate_id;
  int                   nmad_tag;
  int                   seq, probe;
  mpir_datatype_t      *mpir_datatype = NULL;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(source == MPI_ANY_SOURCE)) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (tbx_unlikely(source >= global_size || in_gate_id[source] == -1)){
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
      MPI_NMAD_LOG_OUT();
      return 1;
    }

    gate_id = in_gate_id[source];
  }

  mpir_datatype = get_datatype(datatype);
  nmad_tag = mpir_project_comm_and_tag(comm, tag);

  if (tbx_unlikely(nmad_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid receiving tag %d (%d, %d). Maximum allowed tag: %d\n", nmad_tag, comm, tag, NM_SO_MAX_TAGS);
    MPI_NMAD_LOG_OUT();
    return 1;
  }

  if (source != MPI_ANY_SOURCE) {
    seq = nm_so_sr_get_current_recv_seq(p_so_sr_if, gate_id, nmad_tag);
    probe = nm_so_sr_rtest_range(p_so_sr_if, gate_id, nmad_tag, seq-1, 1);
    if ((seq == NM_SO_PENDING_PACKS_WINDOW-1) && (probe == -NM_EAGAIN)) {
      MPI_NMAD_TRACE("Reaching maximum sequence number in reception. Trigger automatic flushing");
      nm_so_sr_rwait_range(p_so_sr_if, gate_id, nmad_tag, 0, seq-1);
      MPI_NMAD_TRACE("Automatic flushing over");
    }
  }

  _request->request_ptr = NULL;

  MPI_NMAD_TRACE("Receiving from %d at address %p with tag %d (%d, %d)\n", source, buffer, nmad_tag, comm, tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Receiving data of type %d at address %p with len %lu (%d*%lu)\n", datatype, buffer, (unsigned long)count*sizeof_datatype(datatype), count, (unsigned long)sizeof_datatype(datatype));
    MPI_NMAD_TRANSFER("[%s] Recv (contig) --< %ld: %lu bytes\n", __TBX_FUNCTION__, gate_id, count * sizeof_datatype(datatype));
    err = nm_so_sr_irecv(p_so_sr_if, gate_id, nmad_tag, buffer, count * sizeof_datatype(datatype), &(_request->request_id));
    MPI_NMAD_TRANSFER("[%s] Recv (contig) finished, request = %p\n", __TBX_FUNCTION__, &(_request->request_id));
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j, k=0;

    MPI_NMAD_TRACE("Receiving vector type: stride %d - blocklen %d - count %d - size %lu\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, (unsigned long)mpir_datatype->size);
    nm_so_begin_unpacking(p_so_pack_if, gate_id, nmad_tag, connection);
    _request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
    _request->request_ptr[0] = buffer;
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_unpack(connection, _request->request_ptr[k], mpir_datatype->block_size);
        k++;
        _request->request_ptr[k] = _request->request_ptr[k-1] + mpir_datatype->block_size;
      }
    }
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_PACK_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED || mpir_datatype->dte_type == MPIR_HINDEXED) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j, k=0;

    MPI_NMAD_TRACE("Receiving (h)indexed type: count %d - size %lu\n", mpir_datatype->elements, (unsigned long)mpir_datatype->size);
    nm_so_begin_unpacking(p_so_pack_if, gate_id, nmad_tag, connection);
    _request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
    _request->request_ptr[0] = buffer;
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_unpack(connection, _request->request_ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_size);
        k++;
        _request->request_ptr[k] = _request->request_ptr[k-1] + mpir_datatype->blocklens[j] * mpir_datatype->old_size;
      }
    }
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_PACK_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    if (mpir_datatype->is_optimized) {
      struct nm_so_cnx *connection = &(_request->request_cnx);
      int               i, j, k=0;

      MPI_NMAD_TRACE("Receiving struct type: size %lu\n", (unsigned long)mpir_datatype->size);
      nm_so_begin_unpacking(p_so_pack_if, gate_id, nmad_tag, connection);
      _request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
      for(i=0 ; i<count ; i++) {
        _request->request_ptr[k] = buffer + i*mpir_datatype->size;
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          _request->request_ptr[k] += mpir_datatype->indices[j];
          nm_so_unpack(connection, _request->request_ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          k++;
          _request->request_ptr[k] = _request->request_ptr[k-1] - mpir_datatype->indices[j];
        }
      }
      if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_PACK_RECV;
    }
    else {
      void *recvbuffer = NULL, *recvptr, *ptr;
      int   i, j;

      recvbuffer = malloc(count * sizeof_datatype(datatype));
      MPI_NMAD_TRACE("Receiving struct type %d in a contiguous way at address %p with len %lu (%d*%lu)\n", datatype, recvbuffer, (unsigned long)count*sizeof_datatype(datatype), count, (unsigned long)sizeof_datatype(datatype));
      MPI_NMAD_TRANSFER("[%s] Recv (struct) --< %ld: %lu bytes\n", __TBX_FUNCTION__, gate_id, count * sizeof_datatype(datatype));
      err = nm_so_sr_irecv(p_so_sr_if, gate_id, nmad_tag, recvbuffer, count * sizeof_datatype(datatype), &(_request->request_id));
      MPI_NMAD_TRANSFER("[%s] Recv (struct) finished\n", __TBX_FUNCTION__);
      MPI_NMAD_TRACE("Calling nm_so_sr_rwait\n");
      MPI_NMAD_TRANSFER("[%s] Calling nm_so_sr_rwait (struct)\n", __TBX_FUNCTION__);
      err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);
      MPI_NMAD_TRANSFER("[%s] Returning from nm_so_sr_rwait\n", __TBX_FUNCTION__);

      recvptr = recvbuffer;
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i*mpir_datatype->extent;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, (unsigned long)i*mpir_datatype->extent);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("Sub-element %d starts at %p (%p + %ld)\n", j, ptr, ptr-mpir_datatype->indices[j], (unsigned long)mpir_datatype->indices[j]);
          memcpy(ptr, recvptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          MPI_NMAD_TRACE("Copying from %p and moving by %lu\n", recvptr, (unsigned long)mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          recvptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j];
          ptr -= mpir_datatype->indices[j];
        }
      }
      free(recvbuffer);
      _request->request_type = MPI_REQUEST_ZERO;
    }
  }
  else {
    ERROR("Do not know how to receive datatype %d\n", datatype);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  inc_nb_incoming_msg();
  MPI_NMAD_TRACE("Irecv completed\n");
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Performs a standard-mode, blocking receive.
 */
int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status) {
  MPI_Request           request;
  MPI_Request          *request_ptr = &request;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request_ptr;
  int                  err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Receiving message from %d of datatype %d with tag %d\n", source, datatype, tag);

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
      MPI_NMAD_LOG_OUT();
      return not_implemented("Using MPI_ANY_TAG");
  }

  _request->request_ptr = NULL;
  _request->request_type = MPI_REQUEST_RECV;
  mpi_inline_irecv(buffer, count, datatype, source, tag, comm, &request);

  if (_request->request_type == MPI_REQUEST_RECV) {
    MPI_NMAD_TRACE("Calling nm_so_sr_rwait\n");
    MPI_NMAD_TRANSFER("[%s] Calling nm_so_sr_rwait for request = %p\n", __TBX_FUNCTION__, &(_request->request_id));
    err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);
    MPI_NMAD_TRANSFER("[%s] Returning from nm_so_sr_rwait\n", __TBX_FUNCTION__);
  }
  else if (_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    MPI_NMAD_TRANSFER("[%s] Calling nm_so_end_unpacking\n", __TBX_FUNCTION__);
    err = nm_so_end_unpacking(connection);
    MPI_NMAD_TRANSFER("[%s] Returning from nm_so_end_unpacking\n", __TBX_FUNCTION__);
  }
  MPI_NMAD_TRACE("Wait completed\n");

  if (_request->request_ptr != NULL) {
    free(_request->request_ptr);
  }

  if (status != NULL) {
    status->count = count;
    status->MPI_TAG = tag;
    status->MPI_ERROR = err;

    if (source == MPI_ANY_SOURCE) {
      long gate_id;
      nm_so_sr_recv_source(p_so_sr_if, _request->request_id, &gate_id);
      status->MPI_SOURCE = in_dest[gate_id];
    }
    else {
      status->MPI_SOURCE = source;
    }
  }
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Posts a nonblocking receive.
 */
int MPI_Irecv(void* buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Ireceiving message from %d of datatype %d with tag %d\n", source, datatype, tag);

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
      MPI_NMAD_LOG_OUT();
      return not_implemented("Using MPI_ANY_TAG");
  }

  _request->request_ptr = NULL;
  _request->request_type = MPI_REQUEST_RECV;
  err = mpi_inline_irecv(buffer, count, datatype, source, tag, comm, request);

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Returns when the operation identified by request is complete.
 */
int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int                   err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Waiting for a request %d\n", _request->request_type);
  if (_request->request_type == MPI_REQUEST_RECV) {
    MPI_NMAD_TRACE("Calling nm_so_sr_rwait\n");
    MPI_NMAD_TRANSFER("[%s] Calling nm_so_sr_rwait for request=%p\n", __TBX_FUNCTION__, &(_request->request_id));
    err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);
    MPI_NMAD_TRANSFER("[%s] Returning from nm_so_sr_rwait\n", __TBX_FUNCTION__);
  }
  else if (_request->request_type == MPI_REQUEST_SEND) {
    MPI_NMAD_TRACE("Calling nm_so_sr_swait\n");
    MPI_NMAD_TRANSFER("[%s] Calling nm_so_sr_swait\n", __TBX_FUNCTION__);
    err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
    MPI_NMAD_TRANSFER("[%s] Returning from nm_so_sr_swait\n", __TBX_FUNCTION__);
    if (_request->contig_buffer != NULL) {
      free(_request->contig_buffer);
    }
  }
  else if (_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_end_unpacking(connection);
  }
  else if (_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_end_packing(connection);
  }
  else {
    MPI_NMAD_TRACE("Waiting operation invalid for request type %d\n", _request->request_type);
  }
  _request->request_type = MPI_REQUEST_ZERO;
  if (_request->request_ptr != NULL) {
    free(_request->request_ptr);
  }

#warning Fill in the status object

  MPI_NMAD_TRACE("Request completed\n");
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Returns when all the operations identified by requests are complete.
 */
int
MPI_Waitall(int count,
            MPI_Request *requests,
            MPI_Status *statuses) {
  int err = NM_ESUCCESS;
  int i;

  if (statuses == MPI_STATUSES_IGNORE) {
    for (i = 0; i < count; i++) {
      err =  MPI_Wait(&(requests[i]), MPI_STATUS_IGNORE);
      if (err != NM_ESUCCESS)
        goto out;
    }
  } else {
    for (i = 0; i < count; i++) {
      err =  MPI_Wait(&(requests[i]), &(statuses[i]));
      if (err != NM_ESUCCESS)
        goto out;
    }
  }

 out:
  return err;
}

/**
 * Returns flag = true if the operation identified by request is
 * complete.
 */
int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int                   err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  if (_request->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rtest(p_so_sr_if, _request->request_id);
  }
  else if (_request->request_type == MPI_REQUEST_SEND) {
    err = nm_so_sr_stest(p_so_sr_if, _request->request_id);
  }
  else if (_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_test_end_unpacking(connection);
  }
  else if (_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_test_end_packing(connection);
  }

  if (err == NM_ESUCCESS) {
    *flag = 1;
    _request->request_type = MPI_REQUEST_ZERO;
#warning Fill in the status object
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * Tests for completion of the communication operations associated
 * with requests in the array.
 */
int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *index,
                int *flag,
                MPI_Status *status) {
  int i, err = 0;

  MPI_NMAD_LOG_IN();

  for(i=0 ; i<count ; i++) {
    err = MPI_Test(&array_of_requests[i], flag, status);
    if (*flag == 1) {
      *index = i;
      return MPI_SUCCESS;
    }
  }
  *index = MPI_UNDEFINED;

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Nonblocking operation that returns flag = true if there is a
 * message that can be received and that matches the message envelope
 * specified by source, tag and comm.
 */
int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status) {
  int err      = 0;
  long gate_id;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tag == MPI_ANY_TAG) {
      MPI_NMAD_LOG_OUT();
      return not_implemented("Using MPI_ANY_TAG");
  }

  if (source == MPI_ANY_SOURCE) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (source >= global_size || in_gate_id[source] == -1) {
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
      MPI_NMAD_LOG_OUT();
      return 1;
    }

    gate_id = in_gate_id[source];
  }

  err = nm_so_sr_probe(p_so_sr_if, gate_id, tag);
  if (err == NM_ESUCCESS) {
    *flag = 1;
#warning Fill in the status object
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * Blocks and returns only after a message that matches the message
 * envelope specified by source, tag and comm can be received.
 */
int MPI_Probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status) {
  int flag = 0;
  int err;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }
  if (tag == MPI_ANY_TAG) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  err = MPI_Iprobe(source, tag, comm, &flag, status);
  while (flag != 1) {
    err = MPI_Iprobe(source, tag, comm, &flag, status);
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Computes the number of entries received.
 */
int MPI_Get_count(MPI_Status *status,
                  MPI_Datatype datatype,
                  int *count) {
  MPI_NMAD_LOG_IN();
  *count = status->count;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int MPI_Request_is_equal(MPI_Request request1, MPI_Request request2) {
  struct MPI_Request_s *_request1 = (struct MPI_Request_s *)(&request1);
  struct MPI_Request_s *_request2 = (struct MPI_Request_s *)(&request2);
  if (_request1->request_type == MPI_REQUEST_ZERO) {
    return (_request1->request_type == _request2->request_type);
  }
  else if (_request2->request_type == MPI_REQUEST_ZERO) {
    return (_request1->request_type == _request2->request_type);
  }
  else {
    return (_request1->request_id == _request2->request_id);
  }
}

/**
 * Blocks the caller until all group members have called the routine.
 */
int MPI_Barrier(MPI_Comm comm) {
  tbx_bool_t termination;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  termination = test_termination(comm);
  MPI_NMAD_TRACE("Result %d\n", termination);
  while (termination == tbx_false) {
    sleep(1);
    termination = test_termination(comm);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * Broadcasts a message from the process with rank root to all
 * processes of the group, itself included.
 */
int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm) {
  int tag = 1;
  int err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  MPI_NMAD_TRACE("Entering a bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
  if (process_rank == root) {
    MPI_Request *requests;
    int i, err;
    requests = malloc(global_size * sizeof(MPI_Request));
    for(i=0 ; i<global_size ; i++) {
      if (i==root) continue;
      err = MPI_Isend(buffer, count, datatype, i, tag, comm, &requests[i]);
      if (err != 0) {
        MPI_NMAD_LOG_OUT();
        return err;
      }
    }
    for(i=0 ; i<global_size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], NULL);
      if (err != 0) {
          MPI_NMAD_LOG_OUT();
          return err;
      }
    }
    free(requests);
    err = MPI_SUCCESS;
  }
  else {
    MPI_Request request;
    MPI_Irecv(buffer, count, datatype, root, tag, comm, &request);
    err = MPI_Wait(&request, NULL);
  }
  MPI_NMAD_TRACE("End of bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 *
 */
int MPI_Alltoall(void* sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 int recvcount,
		 MPI_Datatype recvtype,
		 MPI_Comm comm) {
  int tag = 3;
  int err;
  int i;
  MPI_Request *requests;
      
  requests = malloc(global_size * sizeof(MPI_Request));
  
  MPI_NMAD_LOG_IN();
  
  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  for(i=0 ; i<global_size; i++) {
    if(i == process_rank)
      memcpy(recvbuf + (i * sizeof_datatype(recvtype)), sendbuf + (i * sizeof_datatype(sendtype)), sendcount * sizeof_datatype(sendtype));
    else
      {
	MPI_Irecv(recvbuf + (i * sizeof_datatype(recvtype)), recvcount, recvtype, i, tag, comm, &requests[i]);
	
	err = MPI_Send(sendbuf + (i * sizeof_datatype(sendtype)), sendcount, sendtype, i, tag, comm);
	
	if (err != 0) {
	  MPI_NMAD_LOG_OUT();
	  return err;
	}
      }
  }
  
  for(i=0 ; i<global_size ; i++) {
    if (i == process_rank) continue;
    MPI_Wait(&requests[i], NULL);
  }
  
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS; 
}

/**
 *
 */
int MPI_Alltoallv(void* sendbuf,
		  int *sendcounts,
		  int *sdispls,
		  MPI_Datatype sendtype,
		  void *recvbuf,
		  int *recvcounts,
		  int *rdispls,
		  MPI_Datatype recvtype,
		  MPI_Comm comm) {
  int tag = 4;
  int err;
  int i;
  MPI_Request *requests;
      
  requests = malloc(global_size * sizeof(MPI_Request));
  
  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  for(i=0 ; i<global_size; i++) {
    if(i == process_rank)
      memcpy(recvbuf + (rdispls[i] * sizeof_datatype(recvtype)), sendbuf + (sdispls[i] * sizeof_datatype(sendtype)), sendcounts[i] * sizeof_datatype(sendtype));
    else
      {
	MPI_Irecv(recvbuf + (rdispls[i] * sizeof_datatype(recvtype)), recvcounts[i], recvtype, i, tag, comm, &requests[i]);
	
	err = MPI_Send(sendbuf + (sdispls[i] * sizeof_datatype(sendtype)), sendcounts[i], sendtype, i, tag, comm);
	
	if (err != 0) {
	  MPI_NMAD_LOG_OUT();
	  return err;
	}
      }
  }
  
  for(i=0 ; i<global_size ; i++) {
    if (i == process_rank) continue;
    MPI_Wait(&requests[i], NULL);
  }
  
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS; 
}


/**
 * Binds a user-defined global operation to an op handle that can
 * subsequently used in a global reduction operation.
 */
int MPI_Op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op) {
  int err;

  MPI_NMAD_LOG_IN();
  err = mpir_op_create(function, commute, op);
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Marks a user-defined reduction operation for deallocation.
 */
int MPI_Op_free(MPI_Op *op) {
  int err;

  MPI_NMAD_LOG_IN();
  err = mpir_op_free(op);
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Combines the elements provided in the input buffer of each process
 * in the group, using the operation op, and returns the combined
 * value in the output buffer of the process with rank root.
 */
int MPI_Reduce(void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm) {
  int tag = 2;
  mpir_function_t *function;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  function = mpir_get_function(op);
  if (function->function == NULL) {
    ERROR("Operation %d not implemented\n", op);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  if (process_rank == root) {
    // Get the input buffers of all the processes
    void **remote_sendbufs;
    MPI_Request *requests;
    int i;
    remote_sendbufs = malloc(global_size * sizeof(void *));
    requests = malloc(global_size * sizeof(MPI_Request));
    for(i=0 ; i<global_size ; i++) {
      if (i == root) continue;
      remote_sendbufs[i] = malloc(count * sizeof_datatype(datatype));
      MPI_Irecv(remote_sendbufs[i], count, datatype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<global_size ; i++) {
      if (i == root) continue;
      MPI_Wait(&requests[i], NULL);
    }

    // Do the reduction operation
    memcpy(recvbuf, sendbuf, count*sizeof_datatype(datatype));
    for(i=0 ; i<global_size ; i++) {
      if (i == root) continue;
      function->function(remote_sendbufs[i], recvbuf, &count, &datatype);
    }

    // Free memory
    for(i=0 ; i<global_size ; i++) {
      if (i == root) continue;
      free(remote_sendbufs[i]);
    }
    free(remote_sendbufs);
    free(requests);

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

/**
 * Combines the elements provided in the input buffer of each process
 * in the group, using the operation op, and returns the combined
 * value in the output buffer of each process in the group.
 */
int MPI_Allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm) {
  int err;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  MPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);

  // Broadcast the result to all processes
  err = MPI_Bcast(recvbuf, count, datatype, 0, comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Returns a floating-point number of seconds, representing elapsed
 * wall-clock time since some time in the past.
 */
double MPI_Wtime(void) {
  tbx_tick_t time;

  MPI_NMAD_LOG_IN();

  TBX_GET_TICK(time);
  double usec = tbx_tick2usec(time);

  MPI_NMAD_LOG_OUT();
  return usec / 1000000;
}

/**
 * Returns the resolution of MPI_WTIME in seconds.
 */
double MPI_Wtick(void) {
  return 1e-7;
}

/**
 * Returns the byte address of location.
 */
int MPI_Get_address(void *location, MPI_Aint *address) {
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

/**
 * Returns the byte address of location.
 */
int MPI_Address(void *location, MPI_Aint *address) {
  return MPI_Get_address(location, address);
}

/**
 * Returns the total size, in bytes, of the entries in the type
 * signature associated with datatype.
 */
int MPI_Type_size(MPI_Datatype datatype, int *size) {
  return mpir_type_size(datatype, size);
}

/**
 * Commits the datatype.
 */
int MPI_Type_commit(MPI_Datatype *datatype) {
  return mpir_type_commit(datatype);
}

/**
 * Marks the datatype object associated with datatype for
 * deallocation.
 */
int MPI_Type_free(MPI_Datatype *datatype) {
  return mpir_type_free(datatype);
}

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into contiguous locations.
 */
int MPI_Type_contiguous(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype) {
  return mpir_type_contiguous(count, oldtype, newtype);
}

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into location that consist of equally spaced blocks.
 */
int MPI_Type_vector(int count,
                    int blocklength,
                    int stride,
                    MPI_Datatype oldtype,
                    MPI_Datatype *newtype) {
  int hstride = stride * sizeof_datatype(oldtype);
  return mpir_type_vector(count, blocklength, hstride, MPIR_VECTOR, oldtype, newtype);
}

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into location that consist of equally spaced blocks, assumes that
 * the stride between successive blocks is a multiple of the oldtype
 * extent.
 */
int MPI_Type_hvector(int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  return mpir_type_vector(count, blocklength, stride, MPIR_HVECTOR, oldtype, newtype);
}

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into a sequence of blocks, each block is a concatenation of the old
 * datatype.
 */
int MPI_Type_indexed(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  return mpir_type_indexed(count, array_of_blocklengths, (MPI_Aint *)array_of_displacements, MPIR_INDEXED, oldtype, newtype);
}

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into a sequence of blocks, each block is a concatenation of the old
 * datatype; block displacements are specified in bytes, rather than
 * in multiples of the old datatype extent.
 */
int MPI_Type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  return mpir_type_indexed(count, array_of_blocklengths, array_of_displacements, MPIR_HINDEXED, oldtype, newtype);
}

/**
 * Constructs a typemap consisting of the replication of different
 * datatypes, with different block sizes.
 */
int MPI_Type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype) {
  return mpir_type_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype);
}

/**
 * Creates a new intracommunicator with the same fixed attributes as
 * the input intracommunicator.
 */
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  return mpir_comm_dup(comm, newcomm);
}

/**
 * Marks the communication object for deallocation.
 */
int MPI_Comm_free(MPI_Comm *comm) {
  return mpir_comm_free(comm);
}
