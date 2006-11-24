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

#include <stdint.h>
#include <unistd.h>

#include <madeleine.h>
#include <nm_public.h>
#include <nm_so_public.h>
#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>
#include <nm_mad3_private.h>

#include "mpi.h"
#include "mpi_nmad_private.h"

static p_mad_madeleine_t       madeleine	= NULL;
static int                     global_size	= -1;
static int                     process_rank	= -1;
static struct nm_so_interface *p_so_sr_if;
static nm_so_pack_interface    p_so_pack_if;
static long                   *out_gate_id	= NULL;
static long                   *in_gate_id	= NULL;
static int                    *out_dest	 	= NULL;
static int                    *in_dest		= NULL;

int MPI_Init(int *argc,
             char ***argv) {

  p_mad_session_t                  session    = NULL;
  struct nm_core                  *p_core     = NULL;
  int                              err;
  int                              dest;
  int                              source;
  p_mad_channel_t                  channel    = NULL;
  p_mad_connection_t               connection = NULL;
  p_mad_nmad_connection_specific_t cs	      = NULL;

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
  //printf("My global rank is %d\n", process_rank);

  /*
   * How to obtain the configuration size.
   */
  global_size = tbx_slist_get_length(madeleine->dir->process_slist);
  //printf("The configuration size is %d\n", global_size);

  /*
   * Reference to the NewMadeleine core object
   */
  p_core = mad_nmad_get_core();
  err = nm_so_sr_init(p_core, &p_so_sr_if);
  CHECK_RETURN_CODE(err, "nm_so_sr_interface_init");
  err =  nm_so_pack_interface_init(p_core, &p_so_pack_if);
  CHECK_RETURN_CODE(err, "nm_so_pack_interface_init");

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
  channel = tbx_htable_get(madeleine->channel_htable, "channel_comm_world");

  /* If that fails, it means that our process does not belong to the channel */
  if (!channel) {
    fprintf(stderr, "I don't belong to this channel");
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

  return 0;
}

int MPI_Init_thread(int *argc,
                    char ***argv,
                    int required,
                    int *provided) {
  int err;

  err = MPI_Init(argc, argv);
  *provided = MPI_THREAD_SINGLE;
  return err;
}

int MPI_Finalize(void) {
  mad_exit(madeleine);
  return 0;
}

int MPI_Abort(MPI_Comm comm,
              int errorcode) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  mad_exit(madeleine);
  return errorcode;
}


int MPI_Comm_size(MPI_Comm comm,
                  int *size) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  *size = global_size;
  return 0;
}

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  *rank = process_rank;
  return 0;
}

int MPI_Get_processor_name(char *name, int *resultlen) {
  int err;
  err = gethostname(name, MPI_MAX_PROCESSOR_NAME);
  if (!err) {
    *resultlen = strlen(name);
    return 0;
  }
  else {
    return errno;
  }
}

int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm) {
  MPI_Request request;
  int         err = 0;

  if (count == 0) return not_implemented("Sending 0 element");
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");
  if (tag == MPI_ANY_TAG) return not_implemented("Using MPI_ANY_TAG");

  MPI_Isend(buffer, count, datatype, dest, tag, comm, &request);

  if (request->request_type == MPI_REQUEST_SEND) {
    err = nm_so_sr_swait(p_so_sr_if, request->request_id);
  }
  else {
    err = nm_so_flush_packs(request->request_cnx);
    free(request->request_cnx);
  }

  return err;
}

int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status) {
  MPI_Request request;
  int         err = 0;

  if (count == 0) return not_implemented("Receiving 0 element");
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");
  if (tag == MPI_ANY_TAG) return not_implemented("Using MPI_ANY_TAG");

  MPI_Irecv(buffer, count, datatype, source, tag, comm, &request);

  if (request->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rwait(p_so_sr_if, request->request_id);
  }
  else {
    err = nm_so_flush_unpacks(request->request_cnx);
    free(request->request_cnx);
  }

  if (status != NULL) {
    status->count = count;
    status->MPI_TAG = tag;
    status->MPI_ERROR = err;

    if (source == MPI_ANY_SOURCE) {
      long gate_id;
      nm_so_sr_recv_source(p_so_sr_if, request->request_id, &gate_id);
      status->MPI_SOURCE = in_dest[gate_id];
    }
    else {
      status->MPI_SOURCE = source;
    }
  }
  return err;
}

int MPI_Isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  int  err     = 0;
  long gate_id;
  mpir_datatype_t *mpir_datatype = NULL;

  if (count == 0) return not_implemented("Sending 0 element");
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");
  if (tag == MPI_ANY_TAG) return not_implemented("Using MPI_ANY_TAG");

  if (dest >= global_size || out_gate_id[dest] == -1) {
    fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    return 1;
  }

  gate_id = out_gate_id[dest];

  *request = malloc(sizeof(MPI_Request_t));

  mpir_datatype = get_datatype(datatype);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Sending data of type %d at address %p with len %d (%d*%d)\n", datatype, buffer, count*sizeof_datatype(datatype), count, sizeof_datatype(datatype));
    err = nm_so_sr_isend(p_so_sr_if, gate_id, tag, buffer, count * sizeof_datatype(datatype), &((*request)->request_id));
    (*request)->request_type = MPI_REQUEST_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR) {
    struct nm_so_cnx *connection;
    int              i, j;
    void            *ptr = buffer;

    MPI_NMAD_TRACE("Sending vector type: stride %d - blocklen %d - count %d - size %d\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, mpir_datatype->size);
    connection = malloc(sizeof(struct nm_so_cnx));
    nm_so_begin_packing(p_so_pack_if, gate_id, 0, connection);
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_pack(connection, ptr, mpir_datatype->block_size);
        ptr += mpir_datatype->stride;
      }
    }
    nm_so_end_packing(connection);
    (*request)->request_type = MPI_REQUEST_PACK_SEND;
    (*request)->request_cnx = connection;
  }
  else {
    ERROR("Do not know how to send datatype %d\n", datatype);
    return 0;
  }

  inc_nb_outgoing_msg();
  return err;
}

int MPI_Irecv(void* buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  int err      = 0;
  long gate_id;
  mpir_datatype_t *mpir_datatype = NULL;

  if (count == 0) return not_implemented("Receiving 0 element");
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");
  if (tag == MPI_ANY_TAG) return not_implemented("Using MPI_ANY_TAG");

  if (source == MPI_ANY_SOURCE) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (source >= global_size || in_gate_id[source] == -1) {
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
      return 1;
    }

    gate_id = in_gate_id[source];
  }

  *request = malloc(sizeof(MPI_Request_t));

  mpir_datatype = get_datatype(datatype);
  if (mpir_datatype->is_contig == 1) {
    err = nm_so_sr_irecv(p_so_sr_if, gate_id, tag, buffer, count * sizeof_datatype(datatype), &((*request)->request_id));
    (*request)->request_type = MPI_REQUEST_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR) {
    struct nm_so_cnx *connection;
    int              i, j, k=0;
    void            **ptr;

    MPI_NMAD_TRACE("Receiving vector type: stride %d - blocklen %d - count %d - size %d\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, mpir_datatype->size);
    connection = malloc(sizeof(struct nm_so_cnx));
    nm_so_begin_unpacking(p_so_pack_if, gate_id, 0, connection);
    ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
    ptr[0] = buffer;
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_unpack(connection, ptr[k], mpir_datatype->block_size);
        k++;
        ptr[k] = ptr[k-1] + mpir_datatype->block_size;
      }
    }
    nm_so_end_unpacking(connection);
    (*request)->request_type = MPI_REQUEST_PACK_RECV;
    (*request)->request_cnx = connection;
  }
  else {
    ERROR("Do not know how to receive datatype %d\n", datatype);
    return 0;
  }

  inc_nb_incoming_msg();
  return err;
}

int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) {
  int err;

  if ((*request)->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rwait(p_so_sr_if, (*request)->request_id);
  }
  else {
    err = nm_so_sr_swait(p_so_sr_if, (*request)->request_id);
  }
  free(*request);
  *request = MPI_REQUEST_NULL;

#warning Fill in the status object

  return err;
}

int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status) {
  int err;

  if ((*request)->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rtest(p_so_sr_if, (*request)->request_id);
  }
  else {
    err = nm_so_sr_stest(p_so_sr_if, (*request)->request_id);
  }

  if (err == NM_ESUCCESS) {
    *flag = 1;
    free(*request);
    *request = MPI_REQUEST_NULL;
#warning Fill in the status object
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }
  return 0;
}

int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status) {
  int err      = 0;
  long gate_id;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");
  if (tag == MPI_ANY_TAG) return not_implemented("Using MPI_ANY_TAG");

  if (source == MPI_ANY_SOURCE) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (source >= global_size || in_gate_id[source] == -1) {
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
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
  return 0;
}

int MPI_Probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status) {
  int flag = 0;
  int err;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");
  if (tag == MPI_ANY_TAG) return not_implemented("Using MPI_ANY_TAG");

  err = MPI_Iprobe(source, tag, comm, &flag, status);
  while (flag != 1) {
    err = MPI_Iprobe(source, tag, comm, &flag, status);
  }
  return err;
}


int MPI_Get_count(MPI_Status *status,
                  MPI_Datatype datatype,
                  int *count) {
  *count = status->count;
  return 0;
}

int MPI_Barrier(MPI_Comm comm) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  tbx_bool_t termination = test_termination(comm);
  MPI_NMAD_TRACE("Result %d\n", termination);
  while (termination == tbx_false) {
    sleep(1);
    termination = test_termination(comm);
  }
  return 0;
}

int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm) {
  int tag = 1;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  if (process_rank == root) {
    MPI_Request *requests;
    int i, err;
    requests = malloc(global_size * sizeof(MPI_Request));
    for(i=0 ; i<global_size ; i++) {
      if (i==root) continue;
      err = MPI_Isend(buffer, count, datatype, i, tag, comm, &requests[i]);
      if (err != 0) return err;
    }
    for(i=0 ; i<global_size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], NULL);
      if (err != 0) return err;
    }
    return 0;
  }
  else {
    return MPI_Recv(buffer, count, datatype, root, tag, comm, NULL);
  }
}

int reduce(void *sendbuf, void *remote_sendbufs, int size, MPI_Datatype datatype, int count, MPI_Op op, void *recvbuf) {
  int i, j;

  switch (op) {
    case MPI_MIN : {
      switch (datatype) {
        case MPI_INT : {
          int *in_int = (int *) sendbuf;
          int *out_int = (int *) recvbuf;
          for(i=0 ; i<count ; i++) {
            out_int[i] = in_int[i];
          }
          in_int = (int *) remote_sendbufs;
          for(j=1 ; j<size ; j++) {
            for(i=0 ; i<count ; i++) {
              if (*in_int < out_int[i]) out_int[i] = *in_int;
              in_int ++;
            }
          }
          break;
        } /* END MPI_INT FOR MPI_MIN */
        default : {
          return not_implemented("Reduce Datatype");
          break;
        }
      }
      break;
    } /* END MPI_MIN */
    case MPI_SUM : {
      switch (datatype) {
        case MPI_INT : {
          int *in_int = (int *) sendbuf;
          int *out_int = (int *) recvbuf;
          for(i=0 ; i<count ; i++) {
            out_int[i] = in_int[i];
	    MPI_NMAD_TRACE("Setting value %d for count %d\n", out_int[i], i);
          }
          in_int = (int *) remote_sendbufs;
          for(j=1 ; j<size ; j++) {
            for(i=0 ; i<count ; i++) {
              out_int[i] += *in_int;
	      MPI_NMAD_TRACE("Adding value %d from process %d for count %d\n", *in_int, j, i);
              in_int ++;
            }
          }
          break;
        } /* END MPI_INT FOR MPI_SUM */
        case MPI_DOUBLE : {
          double *in_int = (double *) sendbuf;
          double *out_int = (double *) recvbuf;
          for(i=0 ; i<count ; i++) {
            out_int[i] = in_int[i];
          }
          in_int = (double *) remote_sendbufs;
          for(j=1 ; j<size ; j++) {
            for(i=0 ; i<count ; i++) {
              out_int[i] += *in_int;
              in_int ++;
            }
          }
          break;
        } /* END MPI_DOUBLE FOR MPI_SUM */
        default : {
          return not_implemented("Reduce Datatype");
          break;
        }
      }
      break;
    }
    case MPI_PROD : {
      switch (datatype) {
        case MPI_INT : {
          int *in_int = (int *) sendbuf;
          int *out_int = (int *) recvbuf;
          for(i=0 ; i<count ; i++) {
            out_int[i] = in_int[i];
          }
          in_int = (int *) remote_sendbufs;
          for(j=1 ; j<size ; j++) {
            for(i=0 ; i<count ; i++) {
              out_int[i] *= *in_int;
              in_int ++;
            }
          }
          break;
        } /* END MPI_INT FOR MPI_PROD */
        case MPI_DOUBLE : {
          double *in_int = (double *) sendbuf;
          double *out_int = (double *) recvbuf;
          for(i=0 ; i<count ; i++) {
            out_int[i] = in_int[i];
          }
          in_int = (double *) remote_sendbufs;
          for(j=1 ; j<size ; j++) {
            for(i=0 ; i<count ; i++) {
              out_int[i] *= *in_int;
              in_int ++;
            }
          }
          break;
        } /* END MPI_DOUBLE FOR MPI_PROD */
        default : {
          return not_implemented("Reduce Datatype");
          break;
        }
      }
      break;
    }
    default : {
      return not_implemented("Reduce Operation");
      break;
    }
  }
  return 0;
}

int MPI_Reduce(void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm) {
  int tag = 2;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  if (process_rank == root) {
    // Get the input buffers of all the processes
    void *remote_sendbufs;
    void **ptr;
    MPI_Request *requests;
    int i, j=0;
    remote_sendbufs = malloc(global_size * count * sizeof_datatype(datatype));
    ptr = malloc(global_size * sizeof(void *));
    ptr[0] = remote_sendbufs;
    requests = malloc(global_size * sizeof(MPI_Request));
    for(i=0 ; i<global_size ; i++) {
      if (i == root) continue;
      MPI_Irecv(ptr[j], count, datatype, i, tag, comm, &requests[j]);
      j++;
      ptr[j] = ptr[j-1] + count * sizeof_datatype(datatype);
    }
    for(i=0 ; i<global_size-1 ; i++) {
      MPI_Wait(&requests[i], NULL);
    }

    // Do the reduction operation
    return reduce(sendbuf, remote_sendbufs, global_size, datatype, count, op, recvbuf);
  }
  else {
    return MPI_Send(sendbuf, count, datatype, root, tag, comm);
  }
}

int MPI_Allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  MPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);

  // Broadcast the result to all processes
  return MPI_Bcast(recvbuf, count, datatype, 0, comm);
}

double MPI_Wtime(void) {
  tbx_tick_t time;
  TBX_GET_TICK(time);
  double usec = tbx_tick2usec(time);
  return usec / 1000000;
}

double MPI_Wtick(void) {
  return 1e-7;
}

int MPI_Type_commit(MPI_Datatype *datatype) {
  return mpir_type_commit(datatype);
}

int MPI_Type_free(MPI_Datatype *datatype) {
  return mpir_type_free(datatype);
}

int MPI_Type_contiguous(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype) {
  return mpir_type_contiguous(count, oldtype, newtype);
}

int MPI_Type_vector(int count,
                    int blocklength,
                    int stride,
                    MPI_Datatype oldtype,
                    MPI_Datatype *newtype) {
  return mpir_type_vector(count, blocklength, stride, oldtype, newtype);
}
