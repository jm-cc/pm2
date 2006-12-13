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
   * Check size of opaque type MPI_Request is the same as the size of
   * our internal request type
   */
  MPI_NMAD_TRACE("sizeof(struct MPI_Request_s) = %ld\n", sizeof(struct MPI_Request_s));
  MPI_NMAD_TRACE("sizeof(MPI_Request) = %ld\n", sizeof(MPI_Request));
  assert(sizeof(struct MPI_Request_s) == sizeof(MPI_Request));

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

  return MPI_SUCCESS;
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
  return MPI_SUCCESS;
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
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  *rank = process_rank;
  MPI_NMAD_TRACE("My comm rank is %d\n", *rank);
  return MPI_SUCCESS;
}

int MPI_Get_processor_name(char *name, int *resultlen) {
  int err;
  err = gethostname(name, MPI_MAX_PROCESSOR_NAME);
  if (!err) {
    *resultlen = strlen(name);
    return MPI_SUCCESS;
  }
  else {
    return errno;
  }
}

__inline__
int mpi_inline_isend(void *buffer,
                     int count,
                     MPI_Datatype datatype,
                     int dest,
                     int tag,
                     MPI_Comm comm,
                     MPI_Request *request) {
  long                  gate_id;
  mpir_datatype_t      *mpir_datatype = NULL;
  int                   err = MPI_SUCCESS;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;

  gate_id = out_gate_id[dest];

  mpir_datatype = get_datatype(datatype);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Sending data of type %d at address %p with len %d (%d*%d)\n", datatype, buffer, count*sizeof_datatype(datatype), count, sizeof_datatype(datatype));
    err = nm_so_sr_isend(p_so_sr_if, gate_id, tag, buffer, count * sizeof_datatype(datatype), &(_request->request_id));
    _request->request_type = MPI_REQUEST_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j;
    void             *ptr = buffer;

    MPI_NMAD_TRACE("Sending (h)vector type: stride %d - blocklen %d - count %d - size %d\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, mpir_datatype->size);
    nm_so_begin_packing(p_so_pack_if, gate_id, 0, connection);
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_pack(connection, ptr, mpir_datatype->block_size);
        ptr += mpir_datatype->stride;
      }
    }
    //nm_so_end_packing(connection);
    _request->request_type = MPI_REQUEST_PACK_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED || mpir_datatype->dte_type == MPIR_HINDEXED) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j;
    void             *ptr = buffer;

    MPI_NMAD_TRACE("Sending (h)indexed type: count %d - size %d\n", mpir_datatype->elements, mpir_datatype->size);
    nm_so_begin_packing(p_so_pack_if, gate_id, 0, connection);
    for(i=0 ; i<count ; i++) {
      ptr = buffer + i * mpir_datatype->size;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %d)\n", i, ptr, buffer, i*mpir_datatype->size);
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        ptr += mpir_datatype->indices[j];
        nm_so_pack(connection, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_type->size);
        ptr -= mpir_datatype->indices[j];
      }
    }
    //nm_so_end_packing(connection);
    _request->request_type = MPI_REQUEST_PACK_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j;
    void             *ptr = buffer;

    MPI_NMAD_TRACE("Sending struct type: size %d\n", mpir_datatype->size);
    connection = malloc(sizeof(struct nm_so_cnx));
    nm_so_begin_packing(p_so_pack_if, gate_id, 0, connection);
    for(i=0 ; i<count ; i++) {
      ptr = buffer + i * mpir_datatype->size;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %d)\n", i, ptr, buffer, i*mpir_datatype->size);
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        ptr += mpir_datatype->indices[j];
        MPI_NMAD_TRACE("packing data at %p (+%d) with a size %d*%d\n", ptr, mpir_datatype->indices[j], mpir_datatype->blocklens[j], mpir_datatype->old_types[j]->size);
        nm_so_pack(connection, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_types[j]->size);
        ptr -= mpir_datatype->indices[j];
      }
    }
    //nm_so_end_packing(connection);
    _request->request_type = MPI_REQUEST_PACK_SEND;
  }
  else {
    ERROR("Do not know how to send datatype %d\n", datatype);
    return -1;
  }

  inc_nb_outgoing_msg();
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
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request_ptr;
  int                   err = 0;

  if (tbx_unlikely(comm != MPI_COMM_WORLD)) return not_implemented("Not using MPI_COMM_WORLD");
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  mpi_inline_isend(buffer, count, datatype, dest, tag, comm, &request);

  if (_request->request_type == MPI_REQUEST_SEND) {
    err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
  }
  else {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_flush_packs(connection);
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
  if (tbx_unlikely(comm != MPI_COMM_WORLD)) return not_implemented("Not using MPI_COMM_WORLD");
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  if (tbx_unlikely(dest >= global_size || out_gate_id[dest] == -1)) {
    fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    return 1;
  }

  return mpi_inline_isend(buffer, count, datatype, dest, tag, comm, request);
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
  mpir_datatype_t      *mpir_datatype = NULL;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;

  if (tbx_unlikely(source == MPI_ANY_SOURCE)) {
    gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (tbx_unlikely(source >= global_size || in_gate_id[source] == -1)){
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
      return 1;
    }

    gate_id = in_gate_id[source];
  }

  mpir_datatype = get_datatype(datatype);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Receiving data of type %d at address %p with len %d (%d*%d)\n", datatype, buffer, count*sizeof_datatype(datatype), count, sizeof_datatype(datatype));
    err = nm_so_sr_irecv(p_so_sr_if, gate_id, tag, buffer, count * sizeof_datatype(datatype), &(_request->request_id));
    _request->request_type = MPI_REQUEST_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j, k=0;
    void            **ptr;

    MPI_NMAD_TRACE("Receiving vector type: stride %d - blocklen %d - count %d - size %d\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, mpir_datatype->size);
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
    //nm_so_end_unpacking(connection);
    _request->request_type = MPI_REQUEST_PACK_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED || mpir_datatype->dte_type == MPIR_HINDEXED) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j, k=0;
    void            **ptr;

    MPI_NMAD_TRACE("Receiving (h)indexed type: count %d - size %d\n", mpir_datatype->elements, mpir_datatype->size);
    nm_so_begin_unpacking(p_so_pack_if, gate_id, 0, connection);
    ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
    ptr[0] = buffer;
    for(i=0 ; i<count ; i++) {
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        nm_so_unpack(connection, ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_type->size);
        k++;
        ptr[k] = ptr[k-1] + mpir_datatype->blocklens[j] * mpir_datatype->old_type->size;
      }
    }
    //nm_so_end_unpacking(connection);
    _request->request_type = MPI_REQUEST_PACK_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j, k=0;
    void            **ptr;

    MPI_NMAD_TRACE("Receiving struct type: size %d\n", mpir_datatype->size);
    connection = malloc(sizeof(struct nm_so_cnx));
    nm_so_begin_unpacking(p_so_pack_if, gate_id, 0, connection);
    ptr = malloc((count*mpir_datatype->elements+1) * sizeof(float *));
    for(i=0 ; i<count ; i++) {
      ptr[k] = buffer + i*mpir_datatype->size;
      for(j=0 ; j<mpir_datatype->elements ; j++) {
        ptr[k] += mpir_datatype->indices[j];
        nm_so_unpack(connection, ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_types[j]->size);
        k++;
        ptr[k] = ptr[k-1] - mpir_datatype->indices[j];
      }
    }
    //nm_so_end_unpacking(connection);
    _request->request_type = MPI_REQUEST_PACK_RECV;
  }
  else {
    ERROR("Do not know how to receive datatype %d\n", datatype);
    return -1;
  }

  inc_nb_incoming_msg();
  MPI_NMAD_TRACE("Irecv completed\n");
  return err;
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
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request_ptr;
  int                  err = 0;

  if (tbx_unlikely(comm != MPI_COMM_WORLD)) return not_implemented("Not using MPI_COMM_WORLD");
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  mpi_inline_irecv(buffer, count, datatype, source, tag, comm, &request);

  if (_request->request_type == MPI_REQUEST_RECV) {
    MPI_NMAD_TRACE("Calling nm_so_sr_rwait...\n");
    err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);
  }
  else {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_flush_unpacks(connection);
  }

  MPI_NMAD_TRACE("Wait completed\n");

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
  return err;
}

int MPI_Irecv(void* buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  if (tbx_unlikely(comm != MPI_COMM_WORLD)) return not_implemented("Not using MPI_COMM_WORLD");
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  MPI_NMAD_TRACE("Receiving message from %d of datatype %d\n", source, datatype);
  return mpi_inline_irecv(buffer, count, datatype, source, tag, comm, request);
}

int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int                   err;

  MPI_NMAD_TRACE("Waiting for a request\n");
  if (_request->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);
  }
  else if (_request->request_type == MPI_REQUEST_SEND) {
    err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
  }
  else if (_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_flush_unpacks(connection);
  }
  else /* ((*request)->request_type == MPI_REQUEST_PACK_SEND) */ {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_flush_packs(connection);
  }
  _request->request_type = MPI_REQUEST_ZERO;

#warning Fill in the status object

  MPI_NMAD_TRACE("Request completed\n");
  return err;
}

int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int                   err;

  if (_request->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rtest(p_so_sr_if, _request->request_id);
  }
  else {
    err = nm_so_sr_stest(p_so_sr_if, _request->request_id);
  }

  if (err == NM_ESUCCESS) {
    *flag = 1;
    _request->request_type = MPI_REQUEST_ZERO;
#warning Fill in the status object
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }
  return MPI_SUCCESS;
}

int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *index,
                int *flag,
                MPI_Status *status) {
  int i, err = 0;
  for(i=0 ; i<count ; i++) {
    err = MPI_Test(&array_of_requests[i], flag, status);
    if (*flag == 1) {
      return MPI_SUCCESS;
    }
  }
  return err;
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
  return MPI_SUCCESS;
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
  return MPI_SUCCESS;
}

int MPI_Barrier(MPI_Comm comm) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  tbx_bool_t termination = test_termination(comm);
  MPI_NMAD_TRACE("Result %d\n", termination);
  while (termination == tbx_false) {
    sleep(1);
    termination = test_termination(comm);
  }
  return MPI_SUCCESS;
}

int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm) {
  int tag = 1;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  MPI_NMAD_TRACE("Entering a bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
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
    return MPI_SUCCESS;
  }
  else {
    return MPI_Recv(buffer, count, datatype, root, tag, comm, NULL);
  }
}

int MPI_Op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op) {
  return mpir_op_create(function, commute, op);
}

int MPI_Op_free(MPI_Op *op) {
  return mpir_op_free(op);
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

  mpir_function_t *function = mpir_get_function(op);
  if (function->function == NULL) {
    ERROR("Operation %d not implemented\n", op);
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
    return MPI_SUCCESS;
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

int MPI_Get_address(void *location, MPI_Aint *address) {
  /* This is the "portable" way to generate an address.
     The difference of two pointers is the number of elements
     between them, so this gives the number of chars between location
     and ptr.  As long as sizeof(char) == 1, this will be the number
     of bytes from 0 to location */
  *address = (MPI_Aint) ((char *)location - (char *)MPI_BOTTOM);
  return MPI_SUCCESS;
}

int MPI_Address(void *location, MPI_Aint *address) {
  return MPI_Get_address(location, address);
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
  int hstride = stride * sizeof_datatype(oldtype);
  return mpir_type_vector(count, blocklength, hstride, MPIR_VECTOR, oldtype, newtype);
}

int MPI_Type_hvector(int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  return mpir_type_vector(count, blocklength, stride, MPIR_HVECTOR, oldtype, newtype);
}

int MPI_Type_indexed(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  return mpir_type_indexed(count, array_of_blocklengths, array_of_displacements, MPIR_INDEXED, oldtype, newtype);
}

int MPI_Type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  return mpir_type_indexed(count, array_of_blocklengths, array_of_displacements, MPIR_HINDEXED, oldtype, newtype);
}

int MPI_Type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype) {
  return mpir_type_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype);
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  return mpir_comm_dup(comm, newcomm);
}

int MPI_Comm_free(MPI_Comm *comm) {
  return mpir_comm_free(comm);
}
