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
static struct nm_so_interface *p_so_sr_if;
static nm_so_pack_interface    p_so_pack_if;
static long                   *out_gate_id	= NULL;
static long                   *in_gate_id	= NULL;
static int                    *out_dest	 	= NULL;
static int                    *in_dest		= NULL;

#define MAX_ARG_LEN 64

#if defined PM2_FORTRAN_TARGET_GFORTRAN
/* GFortran iargc/getargc bindings
 */
void _gfortran_getarg_i4(int32_t *num, char *value, int val_len)	__attribute__ ((weak));

void _gfortran_getarg_i8(int64_t *num, char *value, int val_len)	__attribute__ ((weak));

int32_t _gfortran_iargc(void)	__attribute__ ((weak));
/** Initialisation by Fortran code.
 */
int mpi_init_() {
  int argc;
  int i;
  char **argv;

  argc = 1+_gfortran_iargc();
  MPI_NMAD_TRACE("argc = %d\n", argc);
  argv = malloc(argc * sizeof(char *));
  for (i = 0; i < argc; i++) {
    int j;

    argv[i] = malloc(MAX_ARG_LEN+1);
    if (sizeof(char*) == 4) {
      _gfortran_getarg_i4((int32_t *)&i, argv[i], MAX_ARG_LEN);
    }
    else {
      _gfortran_getarg_i8((int64_t *)&i, argv[i], MAX_ARG_LEN);
    }
    j = MAX_ARG_LEN;
    while (j > 1 && (argv[i])[j-1] == ' ') {
      j--;
    }
    (argv[i])[j] = '\0';
  }
  for (i = 0; i < argc; i++) {
    MPI_NMAD_TRACE("argv[%d] = [%s]\n", i, argv[i]);
  }
  return MPI_Init(&argc, &argv);
}
#elif defined PM2_FORTRAN_TARGET_IFORT
/* Ifort iargc/getargc bindings
 */
/** Initialisation by Fortran code.
 */
int  iargc_(); __attribute__ ((weak));;
void getarg_(int*, char*, int); __attribute__ ((weak));

int mpi_init_() {
  int argc;
  int i;
  char **argv;

  argc = 1+iargc_();
  MPI_NMAD_TRACE("argc = %d\n", argc);
  argv = malloc(argc * sizeof(char *));
  for (i = 0; i < argc; i++) {
    int j;

    argv[i] = malloc(MAX_ARG_LEN+1);
    getarg_((int32_t *)&i, argv[i], MAX_ARG_LEN);

    j = MAX_ARG_LEN;
    while (j > 1 && (argv[i])[j-1] == ' ') {
      j--;
    }
    (argv[i])[j] = '\0';
  }
  for (i = 0; i < argc; i++) {
    MPI_NMAD_TRACE("argv[%d] = [%s]\n", i, argv[i]);
  }
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
                MPI_Communication_Mode is_completed,
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
  //  TBX_FREE((void *)*request);
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
  }
  else {
    for (i = 0; i < *count; i++) {
      MPI_Status _status;
      MPI_Request *p_request = (void *)request[i];
      err =  MPI_Wait(p_request, &_status);
      memcpy(status[i], &_status, sizeof(_status));
      //TBX_FREE((void *)*request);
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
			*comm);
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

void mpi_comm_split_(int *comm, int *color, int *key, int *newcomm, int *ierr) {
  MPI_Comm _newcomm;
  *ierr = MPI_Comm_split(*comm, *color, *key, &_newcomm);
  *newcomm = _newcomm;
}

void mpi_comm_dup_(int *comm,
                   int *newcomm,
                   int *ierr) {
  MPI_Comm _newcomm;
  *ierr = MPI_Comm_dup(*comm, &_newcomm);
  *newcomm = _newcomm;
}

void mpi_comm_free_(int *comm, int *ierr) {
  MPI_Comm _newcomm = *comm;
  *ierr = MPI_Comm_free(&_newcomm);
  *comm = _newcomm;
}
#endif /* PM2_FORTRAN_TARGET_NONE */

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
  int                              global_size;
  int                              process_rank;

  MPI_NMAD_LOG_IN();

  /*
   * Initialization of various libraries.
   * Reference to the Madeleine object.
   */
  madeleine    = mad_init(argc, *argv);

  /*
   * Check size of opaque type MPI_Request is the same as the size of
   * our internal request type
   */
  MPI_NMAD_TRACE("sizeof(mpir_request_t) = %ld\n", (long)sizeof(mpir_request_t));
  MPI_NMAD_TRACE("sizeof(MPI_Request) = %ld\n", (long)sizeof(MPI_Request));
  assert(sizeof(mpir_request_t) <= sizeof(MPI_Request));

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
  internal_init(global_size, process_rank);

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
      NM_DISPF("Cannot find a connection between %d and %d", process_rank, dest);
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
      NM_DISPF("Cannot find a in connection between %d and %d", process_rank, source);
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

  free(out_gate_id);
  free(in_gate_id);
  free(out_dest);
  free(in_dest);

  internal_exit();
  mad_exit(madeleine);

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

  mpir_communicator_t *mpir_communicator = get_communicator(comm);
  *size = mpir_communicator->size;
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

  mpir_communicator_t *mpir_communicator = get_communicator(comm);
  *rank = mpir_communicator->rank;
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
                     int dest,
                     int tag,
                     MPI_Communication_Mode communication_mode,
                     mpir_communicator_t *mpir_communicator,
                     mpir_request_t *mpir_request) {
  long                  gate_id;
  mpir_datatype_t      *mpir_datatype = NULL;
  int                   err = MPI_SUCCESS;
  int                   seq, probe;

  if (tbx_unlikely(dest >= mpir_communicator->size || out_gate_id[mpir_communicator->global_ranks[dest]] == -1)) {
    NM_DISPF("Cannot find a connection between %d and %d, %d\n", mpir_communicator->rank, dest, mpir_communicator->global_ranks[dest]);
    MPI_NMAD_LOG_OUT();
    return 1;
  }

  gate_id = out_gate_id[mpir_communicator->global_ranks[dest]];

  mpir_datatype = get_datatype(mpir_request->request_datatype);
  mpir_datatype->active_communications ++;
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_communicator, tag);
  mpir_request->user_tag = tag;

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid sending tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, tag, NM_SO_MAX_TAGS);
    return 1;
  }

  seq = nm_so_sr_get_current_send_seq(p_so_sr_if, gate_id, mpir_request->request_tag);
  if (seq == NM_SO_PENDING_PACKS_WINDOW-1) {
    probe = nm_so_sr_stest_range(p_so_sr_if, gate_id, mpir_request->request_tag, seq-1, 1);
    if (probe == -NM_EAGAIN) {
      MPI_NMAD_TRACE("Reaching maximum sequence number in emission. Trigger automatic flushing");
      nm_so_sr_swait_range(p_so_sr_if, gate_id, mpir_request->request_tag, 0, seq-1);
      MPI_NMAD_TRACE("Automatic flushing over");
    }
  }

  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  MPI_NMAD_TRACE("Sending to %d with tag %d (%d, %d)\n", dest, mpir_request->request_tag, mpir_communicator->communicator_id, tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Sending data of type %d at address %p with len %ld (%d*%ld)\n", mpir_request->request_datatype, buffer, (long)count*mpir_datatype->size, count, (long)mpir_datatype->size);
    MPI_NMAD_TRANSFER("Sent (contig) --> %d, %ld : %ld bytes\n", dest, gate_id, (long)count * mpir_datatype->size);
    if (communication_mode == MPI_IMMEDIATE_MODE) {
      err = nm_so_sr_isend(p_so_sr_if, gate_id, mpir_request->request_tag, buffer, count * mpir_datatype->size, &(mpir_request->request_nmad));
    }
    else if (communication_mode == MPI_READY_MODE) {
      err = nm_so_sr_rsend(p_so_sr_if, gate_id, mpir_request->request_tag, buffer, count * mpir_datatype->size, &(mpir_request->request_nmad));
    }
    else {
      seq = nm_so_sr_get_current_send_seq(p_so_sr_if, gate_id, mpir_request->request_tag);
      if (seq == NM_SO_PENDING_PACKS_WINDOW-2) {
        MPI_NMAD_TRACE("Reaching critical maximum sequence number in emission. Force completed mode\n");
        err = nm_so_sr_isend_extended(p_so_sr_if, gate_id, mpir_request->request_tag, buffer, count * mpir_datatype->size, MPI_IS_COMPLETED, &(mpir_request->request_nmad));
      }
      else {
        err = nm_so_sr_isend_extended(p_so_sr_if, gate_id, mpir_request->request_tag, buffer, count * mpir_datatype->size, communication_mode, &(mpir_request->request_nmad));
      }
    }
    MPI_NMAD_TRANSFER("Sent finished\n");
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    int               i, j;
    void             *ptr = buffer;

    MPI_NMAD_TRACE("Sending (h)vector type: stride %d - blocklen %d - count %d - size %ld\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, (long)mpir_datatype->size);
    nm_so_begin_packing(p_so_pack_if, gate_id, mpir_request->request_tag, connection);
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_pack(connection, ptr, mpir_datatype->block_size);
        ptr += mpir_datatype->stride;
      }
    }
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED || mpir_datatype->dte_type == MPIR_HINDEXED) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    int               i, j;
    void             *ptr = buffer, *subptr;

    MPI_NMAD_TRACE("Sending (h)indexed type: count %d - size %ld - extent %ld\n", mpir_datatype->elements,
		   (long)mpir_datatype->size, (long)mpir_datatype->extent);
    nm_so_begin_packing(p_so_pack_if, gate_id, mpir_request->request_tag, connection);
    for(i=0 ; i<count ; i++) {
      ptr = buffer + i * mpir_datatype->extent;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*mpir_datatype->extent);
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        subptr = ptr + mpir_datatype->indices[j];
	MPI_NMAD_TRACE("Sub-element %d,%d starts at %p (%p + %ld) with size %ld\n", i, j, subptr, ptr,
		       (long) mpir_datatype->indices[j], (long) mpir_datatype->blocklens[j] * mpir_datatype->old_size);
        nm_so_pack(connection, subptr, mpir_datatype->blocklens[j] * mpir_datatype->old_size);
      }
    }
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    if (mpir_datatype->is_optimized) {
      struct nm_so_cnx *connection = &(mpir_request->request_cnx);
      int               i, j;
      void             *ptr = buffer;

      MPI_NMAD_TRACE("Sending struct type: size %ld\n", (long)mpir_datatype->size);
      nm_so_begin_packing(p_so_pack_if, gate_id, mpir_request->request_tag, connection);
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i * mpir_datatype->extent;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*mpir_datatype->extent);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("packing data at %p (+%ld) with a size %d*%ld\n", ptr, (long)mpir_datatype->indices[j], mpir_datatype->blocklens[j], (long)mpir_datatype->old_sizes[j]);
          nm_so_pack(connection, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          ptr -= mpir_datatype->indices[j];
        }
      }
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_SEND;
    }
    else {
      void *newptr, *ptr;
      size_t len;
      int i, j;

      MPI_NMAD_TRACE("Sending struct datatype in a contiguous buffer\n");
      len = count * mpir_datatype->size;
      mpir_request->contig_buffer = malloc(len);
      if (mpir_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to send struct datatype\n", (long)len);
        return  -1;
      }

      newptr = mpir_request->contig_buffer;
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i * mpir_datatype->extent;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*mpir_datatype->extent);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("packing to %p data from %p (+%ld) with a size %d*%ld\n", newptr, ptr, (long)mpir_datatype->indices[j], mpir_datatype->blocklens[j], (long)mpir_datatype->old_sizes[j]);
          memcpy(newptr, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          newptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j];
          ptr -= mpir_datatype->indices[j];
        }
      }
      MPI_NMAD_TRACE("Sending data of struct type at address %p with len %ld (%d*%ld)\n", mpir_request->contig_buffer, (long)len, count, (long)mpir_datatype->size);
      MPI_NMAD_TRANSFER("Sent (struct) --> %d, %ld: %ld bytes\n", dest, gate_id, (long)count * mpir_datatype->size);
      if (communication_mode == MPI_IMMEDIATE_MODE) {
        err = nm_so_sr_isend(p_so_sr_if, gate_id, mpir_request->request_tag, mpir_request->contig_buffer, len, &(mpir_request->request_nmad));
      }
      else if (communication_mode == MPI_READY_MODE) {
        err = nm_so_sr_rsend(p_so_sr_if, gate_id, mpir_request->request_tag, mpir_request->contig_buffer, len, &(mpir_request->request_nmad));
      }
      else {
        err = nm_so_sr_isend_extended(p_so_sr_if, gate_id, mpir_request->request_tag, mpir_request->contig_buffer, len, communication_mode, &(mpir_request->request_nmad));
      }
      MPI_NMAD_TRANSFER("Sent (struct) finished\n");
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_SEND;
    }
  }
  else {
    ERROR("Do not know how to send datatype %d\n", mpir_request->request_datatype);
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
              MPI_Communication_Mode is_completed,
              MPI_Comm comm,
              MPI_Request *request) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t  *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Esending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = get_communicator(comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  err = mpi_inline_isend(buffer, count, dest, tag, is_completed, mpir_communicator, mpir_request);

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
  mpir_request_t       *mpir_request = (mpir_request_t *)request_ptr;
  mpir_communicator_t  *mpir_communicator;
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Sending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = get_communicator(comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpi_inline_isend(buffer, count, dest, tag, MPI_IMMEDIATE_MODE, mpir_communicator, mpir_request);

  MPI_Wait(&request, NULL);

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
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t  *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = get_communicator(comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  err = mpi_inline_isend(buffer, count, dest, tag, MPI_IMMEDIATE_MODE, mpir_communicator, mpir_request);

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
  mpir_request_t       *mpir_request = (mpir_request_t *)request_ptr;
  mpir_communicator_t  *mpir_communicator;
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Rsending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  mpir_communicator = get_communicator(comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using MPI_ANY_TAG");
  }

  mpir_request->request_type = MPI_REQUEST_SEND;
  mpir_request->request_ptr = NULL;
  mpir_request->contig_buffer = NULL;
  mpir_request->request_datatype = datatype;
  mpi_inline_isend(buffer, count, dest, tag, MPI_READY_MODE, mpir_communicator, mpir_request);

  MPI_NMAD_LOG_OUT();
  return err;
}

__inline__
void mpi_set_status(MPI_Request *request, MPI_Status*status) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;

  status->MPI_TAG = mpir_request->user_tag;
  status->MPI_ERROR = mpir_request->request_error;

  status->count = sizeof_datatype(mpir_request->request_datatype);

  if (mpir_request->request_source == MPI_ANY_SOURCE) {
    long gate_id;
    nm_so_sr_recv_source(p_so_sr_if, mpir_request->request_nmad, &gate_id);
    status->MPI_SOURCE = in_dest[gate_id];
  }
  else {
    status->MPI_SOURCE = mpir_request->request_source;
  }
}

__inline__
int mpi_inline_irecv(void* buffer,
                     int count,
                     int source,
                     int tag,
                     mpir_communicator_t *mpir_communicator,
                     mpir_request_t *mpir_request) {
  long                  gate_id;
  int                   seq, probe;
  mpir_datatype_t      *mpir_datatype = NULL;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(source == MPI_ANY_SOURCE)) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (tbx_unlikely(source >= mpir_communicator->size || in_gate_id[mpir_communicator->global_ranks[source]] == -1)){
      NM_DISPF("Cannot find a connection between %d and %d, %d\n", mpir_communicator->rank, source, mpir_communicator->global_ranks[source]);
      MPI_NMAD_LOG_OUT();
      return 1;
    }
    gate_id = in_gate_id[mpir_communicator->global_ranks[source]];
  }

  mpir_datatype = get_datatype(mpir_request->request_datatype);
  mpir_datatype->active_communications ++;
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_communicator, tag);
  mpir_request->user_tag = tag;
  mpir_request->request_source = source;

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid receiving tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, tag, NM_SO_MAX_TAGS);
    MPI_NMAD_LOG_OUT();
    return 1;
  }

  if (source != MPI_ANY_SOURCE) {
    seq = nm_so_sr_get_current_recv_seq(p_so_sr_if, gate_id, mpir_request->request_tag);
    if (seq == NM_SO_PENDING_PACKS_WINDOW-1) {
      probe = nm_so_sr_rtest_range(p_so_sr_if, gate_id, mpir_request->request_tag, seq-1, 1);
      if (probe == -NM_EAGAIN) {
	MPI_NMAD_TRACE("Reaching maximum sequence number in reception. Trigger automatic flushing");
	nm_so_sr_rwait_range(p_so_sr_if, gate_id, mpir_request->request_tag, 0, seq-1);
	MPI_NMAD_TRACE("Automatic flushing over");
      }
    }
  }

  mpir_request->request_ptr = NULL;

  MPI_NMAD_TRACE("Receiving from %d at address %p with tag %d (%d, %d)\n", source, buffer, mpir_request->request_tag, mpir_communicator->communicator_id, tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Receiving data of type %d at address %p with len %ld (%d*%ld)\n", mpir_request->request_datatype, buffer, (long)count*mpir_datatype->size, count, (long)mpir_datatype->size);
    MPI_NMAD_TRANSFER("Recv (contig) --< %ld: %ld bytes\n", gate_id, (long)count * mpir_datatype->size);
    mpir_request->request_error = nm_so_sr_irecv(p_so_sr_if, gate_id, mpir_request->request_tag, buffer, count * mpir_datatype->size, &(mpir_request->request_nmad));
    MPI_NMAD_TRANSFER("Recv (contig) finished, request = %p\n", &(mpir_request->request_nmad));
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    int               i, j, k=0;

    MPI_NMAD_TRACE("Receiving vector type: stride %d - blocklen %d - count %d - size %ld\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, (long)mpir_datatype->size);
    nm_so_begin_unpacking(p_so_pack_if, gate_id, mpir_request->request_tag, connection);
    mpir_request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
    mpir_request->request_ptr[0] = buffer;
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_unpack(connection, mpir_request->request_ptr[k], mpir_datatype->block_size);
        k++;
        mpir_request->request_ptr[k] = mpir_request->request_ptr[k-1] + mpir_datatype->block_size;
      }
    }
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED || mpir_datatype->dte_type == MPIR_HINDEXED) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    int               i, j, k=0;

    MPI_NMAD_TRACE("Receiving (h)indexed type: count %d - size %ld\n", mpir_datatype->elements, (long)mpir_datatype->size);
    nm_so_begin_unpacking(p_so_pack_if, gate_id, mpir_request->request_tag, connection);
    mpir_request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
    mpir_request->request_ptr[0] = buffer;
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_unpack(connection, mpir_request->request_ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_size);
        k++;
        mpir_request->request_ptr[k] = mpir_request->request_ptr[k-1] + mpir_datatype->blocklens[j] * mpir_datatype->old_size;
      }
    }
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    if (mpir_datatype->is_optimized) {
      struct nm_so_cnx *connection = &(mpir_request->request_cnx);
      int               i, j, k=0;

      MPI_NMAD_TRACE("Receiving struct type: size %ld\n", (long)mpir_datatype->size);
      nm_so_begin_unpacking(p_so_pack_if, gate_id, mpir_request->request_tag, connection);
      mpir_request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
      for(i=0 ; i<count ; i++) {
        mpir_request->request_ptr[k] = buffer + i*mpir_datatype->size;
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          mpir_request->request_ptr[k] += mpir_datatype->indices[j];
          nm_so_unpack(connection, mpir_request->request_ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          k++;
          mpir_request->request_ptr[k] = mpir_request->request_ptr[k-1] - mpir_datatype->indices[j];
        }
      }
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
    }
    else {
      void *recvbuffer = NULL, *recvptr, *ptr;
      int   i, j;

      recvbuffer = malloc(count * mpir_datatype->size);
      MPI_NMAD_TRACE("Receiving struct type %d in a contiguous way at address %p with len %ld (%d*%ld)\n", mpir_request->request_datatype, recvbuffer, (long)count*mpir_datatype->size, count, (long)mpir_datatype->size);
      MPI_NMAD_TRANSFER("Recv (struct) --< %ld: %ld bytes\n", gate_id, (long)count * mpir_datatype->size);
      mpir_request->request_error = nm_so_sr_irecv(p_so_sr_if, gate_id, mpir_request->request_tag, recvbuffer, count * mpir_datatype->size, &(mpir_request->request_nmad));
      MPI_NMAD_TRANSFER("Recv (struct) finished\n");
      MPI_NMAD_TRACE("Calling nm_so_sr_rwait\n");
      MPI_NMAD_TRANSFER("Calling nm_so_sr_rwait (struct)\n");
      nm_so_sr_rwait(p_so_sr_if, mpir_request->request_nmad);
      MPI_NMAD_TRANSFER("Returning from nm_so_sr_rwait\n");

      recvptr = recvbuffer;
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i*mpir_datatype->extent;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*mpir_datatype->extent);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("Sub-element %d starts at %p (%p + %ld)\n", j, ptr, ptr-mpir_datatype->indices[j], (long)mpir_datatype->indices[j]);
          memcpy(ptr, recvptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          MPI_NMAD_TRACE("Copying from %p and moving by %ld\n", recvptr, (long)mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          recvptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j];
          ptr -= mpir_datatype->indices[j];
        }
      }
      free(recvbuffer);
      mpir_request->request_type = MPI_REQUEST_ZERO;
    }
  }
  else {
    ERROR("Do not know how to receive datatype %d\n", mpir_request->request_datatype);
    MPI_NMAD_LOG_OUT();
    return -1;
  }

  inc_nb_incoming_msg();
  MPI_NMAD_TRACE("Irecv completed\n");
  MPI_NMAD_LOG_OUT();
  return mpir_request->request_error;
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
  mpir_request_t       *mpir_request = (mpir_request_t *)request_ptr;
  mpir_communicator_t  *mpir_communicator;
  int                  err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Receiving message from %d of datatype %d with tag %d, count %d\n", source, datatype, tag, count);
  MPI_NMAD_TIMER_IN();

  mpir_communicator = get_communicator(comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
      MPI_NMAD_TIMER_OUT();
      MPI_NMAD_LOG_OUT();
      return not_implemented("Using MPI_ANY_TAG");
  }

  mpir_request->request_ptr = NULL;
  mpir_request->request_type = MPI_REQUEST_RECV;
  mpir_request->request_datatype = datatype;
  mpi_inline_irecv(buffer, count, source, tag, mpir_communicator, mpir_request);

  MPI_Wait(&request, status);

  MPI_NMAD_TIMER_OUT();
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
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  mpir_communicator_t *mpir_communicator;
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Ireceiving message from %d of datatype %d with tag %d\n", source, datatype, tag);

  mpir_communicator = get_communicator(comm);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
      MPI_NMAD_LOG_OUT();
      return not_implemented("Using MPI_ANY_TAG");
  }

  mpir_request->request_ptr = NULL;
  mpir_request->request_type = MPI_REQUEST_RECV;
  mpir_request->request_datatype = datatype;
  err = mpi_inline_irecv(buffer, count, source, tag, mpir_communicator, mpir_request);

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Executes a blocking send and receive operation.
 */
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

  MPI_Wait(&srequest, NULL);
  MPI_Wait(&rrequest, status);

  MPI_NMAD_LOG_OUT();
  return err;
}


/**
 * Returns when the operation identified by request is complete.
 */
int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  int                   err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Waiting for a request %d\n", mpir_request->request_type);
  if (mpir_request->request_type == MPI_REQUEST_RECV) {
    MPI_NMAD_TRACE("Calling nm_so_sr_rwait\n");
    MPI_NMAD_TRANSFER("Calling nm_so_sr_rwait for request=%p\n", &(mpir_request->request_nmad));
    err = nm_so_sr_rwait(p_so_sr_if, mpir_request->request_nmad);
    MPI_NMAD_TRANSFER("Returning from nm_so_sr_rwait\n");
  }
  else if (mpir_request->request_type == MPI_REQUEST_SEND) {
    MPI_NMAD_TRACE("Calling nm_so_sr_swait\n");
    MPI_NMAD_TRANSFER("Calling nm_so_sr_swait\n");
    err = nm_so_sr_swait(p_so_sr_if, mpir_request->request_nmad);
    MPI_NMAD_TRANSFER("Returning from nm_so_sr_swait\n");
    if (mpir_request->contig_buffer != NULL) {
      free(mpir_request->contig_buffer);
    }
  }
  else if (mpir_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    MPI_NMAD_TRANSFER("Calling nm_so_end_unpacking\n");
    err = nm_so_end_unpacking(connection);
    MPI_NMAD_TRANSFER("Returning from nm_so_end_unpacking\n");
  }
  else if (mpir_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    MPI_NMAD_TRACE("Waiting for completion end_packing\n");
    err = nm_so_end_packing(connection);
  }
  else {
    MPI_NMAD_TRACE("Waiting operation invalid for request type %d\n", mpir_request->request_type);
  }
  MPI_NMAD_TRACE("Wait completed\n");

  mpir_request->request_type = MPI_REQUEST_ZERO;
  if (mpir_request->request_ptr != NULL) {
    free(mpir_request->request_ptr);
  }

  if (status != NULL) {
    mpi_set_status(request, status);
  }

  // Release one active communication for that type
  if (mpir_request->request_datatype > MPI_INTEGER) {
    mpir_type_unlock(mpir_request->request_datatype);
  }

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
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  int             err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  if (mpir_request->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rtest(p_so_sr_if, mpir_request->request_nmad);
  }
  else if (mpir_request->request_type == MPI_REQUEST_SEND) {
    err = nm_so_sr_stest(p_so_sr_if, mpir_request->request_nmad);
  }
  else if (mpir_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    err = nm_so_test_end_unpacking(connection);
  }
  else if (mpir_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    err = nm_so_test_end_packing(connection);
  }

  if (err == NM_ESUCCESS) {
    *flag = 1;
    mpir_request->request_type = MPI_REQUEST_ZERO;

    if (status != NULL) {
      mpi_set_status(request, status);
    }
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
  long gate_id, out_gate_id;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);
  if (tag == MPI_ANY_TAG) {
      MPI_NMAD_LOG_OUT();
      return not_implemented("Using MPI_ANY_TAG");
  }

  if (source == MPI_ANY_SOURCE) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (source >= mpir_communicator->size || in_gate_id[source] == -1) {
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", mpir_communicator->rank, source);
      MPI_NMAD_LOG_OUT();
      return 1;
    }

    gate_id = in_gate_id[source];
  }

  err = nm_so_sr_probe(p_so_sr_if, gate_id, &out_gate_id, tag);
  if (err == NM_ESUCCESS) {
    *flag = 1;
    if (status != NULL) {
      status->MPI_TAG = tag;
      status->MPI_SOURCE = in_dest[out_gate_id];
    }
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
  mpir_request_t *mpir_request1 = (mpir_request_t *)(&request1);
  mpir_request_t *mpir_request2 = (mpir_request_t *)(&request2);
  if (mpir_request1->request_type == MPI_REQUEST_ZERO) {
    return (mpir_request1->request_type == mpir_request2->request_type);
  }
  else if (mpir_request2->request_type == MPI_REQUEST_ZERO) {
    return (mpir_request1->request_type == mpir_request2->request_type);
  }
  else {
    return (mpir_request1->request_nmad == mpir_request2->request_nmad);
  }
}

/**
 * Blocks the caller until all group members have called the routine.
 */
int MPI_Barrier(MPI_Comm comm) {
  tbx_bool_t termination;

  MPI_NMAD_LOG_IN();

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
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);

  MPI_NMAD_TRACE("Entering a bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i, err;
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
 * Each process sends the contents of its send buffer to the root
 * process.
 */
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

  mpir_communicator = get_communicator(comm);

  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i, err;
    mpir_datatype_t *mpir_recv_datatype, *mpir_send_datatype;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    mpir_recv_datatype = get_datatype(recvtype);
    mpir_send_datatype = get_datatype(sendtype);

    // receive data from other processes
    for(i=0 ; i<mpir_communicator->size; i++) {
      if(i == root) continue;
      MPI_Irecv(recvbuf + (i * recvcount * mpir_recv_datatype->extent),
                recvcount, recvtype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], NULL);
    }

    // copy local data for itself
    memcpy(recvbuf + (mpir_communicator->rank * mpir_recv_datatype->extent),
           sendbuf, sendcount * mpir_send_datatype->extent);

    // free memory
    free(requests);
  }
  else {
    MPI_Send(sendbuf, sendcount, sendtype, root, tag, comm);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * MPI_GATHERV extends the functionality of MPI_GATHER by allowing a
 * varying count of data.
 */
int MPI_Gatherv(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *displs,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm) {
  int tag = 4;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);

  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i, err;
    mpir_datatype_t *mpir_recv_datatype, *mpir_send_datatype;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    mpir_recv_datatype = get_datatype(recvtype);
    mpir_send_datatype = get_datatype(sendtype);

    // receive data from other processes
    for(i=0 ; i<mpir_communicator->size; i++) {
      if(i == root) continue;
      MPI_Irecv(recvbuf + (displs[i] * mpir_recv_datatype->extent),
                recvcounts[i], recvtype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], NULL);
    }

    // copy local data for itself
    MPI_NMAD_TRACE("Copying local data from %p to %p with len %ld\n", sendbuf,
                   recvbuf + (displs[mpir_communicator->rank] * mpir_recv_datatype->extent),
                   (long) sendcount * mpir_send_datatype->extent);
    memcpy(recvbuf + (displs[mpir_communicator->rank] * mpir_recv_datatype->extent),
           sendbuf, sendcount * mpir_send_datatype->extent);

    // free memory
    free(requests);
  }
  else {
    MPI_Send(sendbuf, sendcount, sendtype, root, tag, comm);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/*
 * MPI_ALLGATHER can be thought of as MPI_GATHER, except all processes
 * receive the result.
 */
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

  mpir_communicator = get_communicator(comm);

  MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm);

  // Broadcast the result to all processes
  err = MPI_Bcast(recvbuf, mpir_communicator->size*recvcount, recvtype, 0, comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * MPI_ALLGATHERV can be thought of as MPI_GATHERV, except all processes
 * receive the result.
 */
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

  mpir_communicator = get_communicator(comm);

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


/**
 * Inverse operation of MPI_GATHER
 */
int MPI_Scatter(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm) {
  int tag = 3;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);

  if (mpir_communicator->rank == root) {
    MPI_Request *requests;
    int i, err;
    mpir_datatype_t *mpir_recv_datatype, *mpir_send_datatype;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
    mpir_recv_datatype = get_datatype(recvtype);
    mpir_send_datatype = get_datatype(sendtype);

    // send data to other processes
    for(i=0 ; i<mpir_communicator->size; i++) {
      if(i == root) continue;
      MPI_Isend(sendbuf + (i * sendcount * mpir_send_datatype->extent),
                sendcount, sendtype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], NULL);
    }

    // copy local data for itself
    memcpy(recvbuf + (mpir_communicator->rank * mpir_recv_datatype->extent),
           sendbuf, sendcount * mpir_send_datatype->extent);

    // free memory
    free(requests);
  }
  else {
    MPI_Recv(recvbuf, recvcount, recvtype, root, tag, comm, NULL);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
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
  MPI_Request *send_requests, *recv_requests;
  mpir_datatype_t *mpir_send_datatype,*mpir_recv_datatype;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);
  send_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
  recv_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));

  mpir_send_datatype = get_datatype(sendtype);
  mpir_recv_datatype = get_datatype(recvtype);

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
    MPI_Wait(&recv_requests[i], NULL);
    MPI_Wait(&send_requests[i], NULL);
  }

  free(send_requests);
  free(recv_requests);

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
  MPI_Request *send_requests, *recv_requests;
  mpir_datatype_t *mpir_send_datatype,*mpir_recv_datatype;
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);
  send_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));
  recv_requests = malloc(mpir_communicator->size * sizeof(MPI_Request));

  mpir_send_datatype = get_datatype(sendtype);
  mpir_recv_datatype = get_datatype(recvtype);

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
    MPI_Wait(&recv_requests[i], NULL);
    MPI_Wait(&send_requests[i], NULL);
  }

  free(send_requests);
  free(recv_requests);

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
  mpir_communicator_t *mpir_communicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);
  function = mpir_get_function(op);
  if (function->function == NULL) {
    ERROR("Operation %d not implemented\n", op);
    MPI_NMAD_LOG_OUT();
    return -1;
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
      remote_sendbufs[i] = malloc(count * sizeof_datatype(datatype));
      MPI_Irecv(remote_sendbufs[i], count, datatype, i, tag, comm, &requests[i]);
    }
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i == root) continue;
      MPI_Wait(&requests[i], NULL);
    }

    // Do the reduction operation
    memcpy(recvbuf, sendbuf, count*sizeof_datatype(datatype));
    for(i=0 ; i<mpir_communicator->size ; i++) {
      if (i == root) continue;
      function->function(remote_sendbufs[i], recvbuf, &count, &datatype);
    }

    // Free memory
    for(i=0 ; i<mpir_communicator->size ; i++) {
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

  MPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);

  // Broadcast the result to all processes
  err = MPI_Bcast(recvbuf, count, datatype, 0, comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 *
 */
int MPI_Reduce_scatter(void *sendbuf,
                       void *recvbuf,
                       int *recvcounts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm) {
  int err=MPI_SUCCESS, count=0, i;
  mpir_communicator_t *mpir_communicator;
  mpir_datatype_t *mpir_datatype;
  int tag = 4;
  void *reducebuf = NULL;

  MPI_NMAD_LOG_IN();

  mpir_datatype = get_datatype(datatype);
  mpir_communicator = get_communicator(comm);
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
    int i;

    requests = malloc(mpir_communicator->size * sizeof(MPI_Request));

    // send data to other processes
    for(i=1 ; i<mpir_communicator->size; i++) {
      MPI_Isend(reducebuf + (i * recvcounts[i] * mpir_datatype->extent),
                recvcounts[i], datatype, i, tag, comm, &requests[i]);
    }
    for(i=1 ; i<mpir_communicator->size ; i++) {
      err = MPI_Wait(&requests[i], NULL);
    }

    // copy local data for itself
    memcpy(recvbuf, reducebuf, recvcounts[0] * mpir_datatype->extent);

    // free memory
    free(requests);
  }
  else {
    MPI_Recv(recvbuf, recvcounts[mpir_communicator->rank], datatype, 0, tag, comm, NULL);
  }

  if (mpir_communicator->rank == 0) {
    free(reducebuf);
  }

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

int MPI_Error_string(int errorcode, char *string, int *resultlen) {
  *resultlen = 100;
  string = malloc(*resultlen * sizeof(char));
  switch (errorcode) {
    case MPI_SUCCESS : {
      snprintf(string, *resultlen, "Error MPI_SUCCESS\n");
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
    default : {
      snprintf(string, *resultlen, "Error %d unknown\n", errorcode);
    }
  }
  return MPI_SUCCESS;
}

/**
 * Returns the version
 */
int MPI_Get_version(int *version, int *subversion) {
  *version = MADMPI_VERSION;
  *subversion = MADMPI_SUBVERSION;
  return MPI_SUCCESS;
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
 * Returns the lower bound and the extent of datatype
 */
int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent) {
  return mpir_type_extent(datatype, lb, extent);
}

/**
 * Returns in newtype a new datatype that is identical to oldtype,
 * except that the lower bound of this new datatype is set to be lb,
 * and its upper bound is set to lb + extent.
 */
int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype) {
  int err;

  MPI_NMAD_LOG_IN();
  if (lb != 0) {
    MPI_NMAD_LOG_OUT();
    return not_implemented("Using lb not equals to 0");
  }
  err = mpir_type_create_resized(oldtype, lb, extent, newtype);

  MPI_NMAD_LOG_OUT();
  return err;
}

/**
 * Commits the datatype.
 */
int MPI_Type_commit(MPI_Datatype *datatype) {
  return mpir_type_commit(*datatype);
}

/**
 * Marks the datatype object associated with datatype for
 * deallocation.
 */
int MPI_Type_free(MPI_Datatype *datatype) {
  int err = mpir_type_free(*datatype);
  if (err == MPI_SUCCESS) {
    *datatype = MPI_DATATYPE_NULL;
  }
  return MPI_SUCCESS;
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
  int err, i;

  MPI_NMAD_LOG_IN();

  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  for(i=0 ; i<count ; i++) {
    displacements[i] = array_of_displacements[i];
  }

  err = mpir_type_indexed(count, array_of_blocklengths, displacements, MPIR_INDEXED, oldtype, newtype);

  free(displacements);
  MPI_NMAD_LOG_OUT();
  return err;
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
 * Returns a handle to the group of the given communicator.
 */
int MPI_Comm_group(MPI_Comm comm, MPI_Group *group) {
  MPI_NMAD_LOG_IN();

  *group = comm;

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * Partitions the group associated to the communicator into disjoint
 * subgroups, one for each value of color.
 */
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) {
  int *sendbuf, *recvbuf;
  int i, j, nb_conodes, **conodes;
  mpir_communicator_t *mpir_communicator;
  mpir_communicator_t *mpir_newcommunicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(comm);

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
    mpir_newcommunicator = get_communicator(*newcomm);
    mpir_newcommunicator->size = nb_conodes;
    free(mpir_newcommunicator->global_ranks);
    mpir_newcommunicator->global_ranks = malloc(nb_conodes*sizeof(int));
    for(i=0 ; i<nb_conodes ; i++) {
      int *ptr = conodes[i];
      if ((*(ptr+1) == key) && (*(ptr+2) == mpir_communicator->rank)) {
        mpir_newcommunicator->rank = i;
      }
      if (*ptr == color) {
        mpir_newcommunicator->global_ranks[i] = *(ptr+2);
      }
    }
  }

  // free the allocated area
  free(conodes);
  free(sendbuf);
  free(recvbuf);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
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

/*
 * Maps the rank of a set of processes in group1 to their rank in
 * group2.
 */
int MPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2) {
  int i, j, x;
  mpir_communicator_t *mpir_communicator;
  mpir_communicator_t *mpir_communicator2;

  MPI_NMAD_LOG_IN();

  mpir_communicator = get_communicator(group1);
  mpir_communicator2 = get_communicator(group2);
  for(i=0 ; i<n ; i++) {
    ranks2[i] = MPI_UNDEFINED;
    x = mpir_communicator->global_ranks[ranks1[i]];
    for(j=0 ; j<mpir_communicator2->size ; j++) {
      if (mpir_communicator2->global_ranks[j] == x) {
        ranks2[i] = j;
        break;
      }
    }
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}
