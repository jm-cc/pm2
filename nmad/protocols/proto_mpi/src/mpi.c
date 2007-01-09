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
  free(out_gate_id);
  free(in_gate_id);
  free(out_dest);
  free(in_dest);
  internal_exit();

  return MPI_SUCCESS;
}

int MPI_Abort(MPI_Comm comm,
              int errorcode) {
  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }

  mad_exit(madeleine);
  free(out_gate_id);
  free(in_gate_id);
  free(out_dest);
  free(in_dest);
  internal_exit();

  return errorcode;
}


int MPI_Comm_size(MPI_Comm comm,
                  int *size) {
  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }

  *size = global_size;
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) {
  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }

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
  int                   nmad_tag;
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;

  gate_id = out_gate_id[dest];

  mpir_datatype = get_datatype(datatype);
  nmad_tag = mpir_project_comm_and_tag(comm, tag);
  _request->request_ptr = NULL;
  MPI_NMAD_TRACE("Sending to %d with tag %d (%d, %d)\n", dest, nmad_tag, comm, tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Sending data of type %d at address %p with len %lu (%d*%lu)\n", datatype, buffer, count*sizeof_datatype(datatype), count, sizeof_datatype(datatype));
    err = nm_so_sr_isend(p_so_sr_if, gate_id, nmad_tag, buffer, count * sizeof_datatype(datatype), &(_request->request_id));
    MPI_NMAD_TRANSFER("Sent --> %ld: %lu bytes\n", gate_id, count * sizeof_datatype(datatype));
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_SEND;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j;
    void             *ptr = buffer;

    MPI_NMAD_TRACE("Sending (h)vector type: stride %d - blocklen %d - count %d - size %lu\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, mpir_datatype->size);
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

    MPI_NMAD_TRACE("Sending (h)indexed type: count %d - size %lu\n", mpir_datatype->elements, mpir_datatype->size);
    nm_so_begin_packing(p_so_pack_if, gate_id, nmad_tag, connection);
    for(i=0 ; i<count ; i++) {
      ptr = buffer + i * mpir_datatype->size;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, i*mpir_datatype->size);
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

      MPI_NMAD_TRACE("Sending struct type: size %lu\n", mpir_datatype->size);
      nm_so_begin_packing(p_so_pack_if, gate_id, nmad_tag, connection);
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i * mpir_datatype->size;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, i*mpir_datatype->size);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("packing data at %p (+%lu) with a size %d*%lu\n", ptr, mpir_datatype->indices[j], mpir_datatype->blocklens[j], mpir_datatype->old_sizes[j]);
          nm_so_pack(connection, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          ptr -= mpir_datatype->indices[j];
        }
      }
      if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_PACK_SEND;
    }
    else {
      void *newbuffer, *newptr, *ptr;
      size_t len;
      int i, j;

      MPI_NMAD_TRACE("Sending struct datatype in a contiguous buffer\n");
      len = count * sizeof_datatype(datatype);
      newbuffer = malloc(len);
      if (newbuffer == NULL) {
        ERROR("Cannot allocate memory with size %lu to send struct datatype\n", len);
        return  -1;
      }

      newptr = newbuffer;
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i * mpir_datatype->size;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, i*mpir_datatype->size);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("packing to %p data from %p (+%ld) with a size %d*%lu\n", newptr, ptr, mpir_datatype->indices[j], mpir_datatype->blocklens[j], mpir_datatype->old_sizes[j]);
          memcpy(newptr, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          newptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j];
          ptr -= mpir_datatype->indices[j];
        }
      }
      MPI_NMAD_TRACE("Sending data of struct type at address %p with len %lu (%d*%lu)\n", newbuffer, len, count, sizeof_datatype(datatype));
      err = nm_so_sr_isend(p_so_sr_if, gate_id, nmad_tag, newbuffer, len, &(_request->request_id));
      MPI_NMAD_TRANSFER("Sent --> %ld: %lu bytes\n", gate_id, count * sizeof_datatype(datatype));
      err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
      free(newbuffer);
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

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  _request->request_type = MPI_REQUEST_SEND;
  _request->request_ptr = NULL;
  mpi_inline_isend(buffer, count, datatype, dest, tag, comm, &request);

  if (_request->request_type == MPI_REQUEST_SEND) {
    MPI_NMAD_TRACE("Waiting for completion swait\n");
    err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
  }
  else if (_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    MPI_NMAD_TRACE("Waiting for completion end_packing\n");
    err = nm_so_end_packing(connection);
  }

  if (_request->request_ptr != NULL) {
    free(_request->request_ptr);
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
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  if (tbx_unlikely(dest >= global_size || out_gate_id[dest] == -1)) {
    fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    return 1;
  }

  _request->request_type = MPI_REQUEST_SEND;
  _request->request_ptr = NULL;
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
  int                   nmad_tag;
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
  nmad_tag = mpir_project_comm_and_tag(comm, tag);
  _request->request_ptr = NULL;

  MPI_NMAD_TRACE("Receiving from %d at address %p with tag %d (%d, %d)\n", source, buffer, nmad_tag, comm, tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Receiving data of type %d at address %p with len %lu (%d*%lu)\n", datatype, buffer, count*sizeof_datatype(datatype), count, sizeof_datatype(datatype));
    err = nm_so_sr_irecv(p_so_sr_if, gate_id, nmad_tag, buffer, count * sizeof_datatype(datatype), &(_request->request_id));
    MPI_NMAD_TRANSFER("Recv --< %ld: %lu bytes\n", gate_id, count * sizeof_datatype(datatype));
    if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR || mpir_datatype->dte_type == MPIR_HVECTOR) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    int               i, j, k=0;

    MPI_NMAD_TRACE("Receiving vector type: stride %d - blocklen %d - count %d - size %lu\n", mpir_datatype->stride, mpir_datatype->blocklen, mpir_datatype->elements, mpir_datatype->size);
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

    MPI_NMAD_TRACE("Receiving (h)indexed type: count %d - size %lu\n", mpir_datatype->elements, mpir_datatype->size);
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

      MPI_NMAD_TRACE("Receiving struct type: size %lu\n", mpir_datatype->size);
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
      MPI_NMAD_TRACE("Receiving struct type %d in a contiguous way at address %p with len %lu (%d*%lu)\n", datatype, recvbuffer, count*sizeof_datatype(datatype), count, sizeof_datatype(datatype));
      err = nm_so_sr_irecv(p_so_sr_if, gate_id, nmad_tag, recvbuffer, count * sizeof_datatype(datatype), &(_request->request_id));
      MPI_NMAD_TRANSFER("Recv --< %ld: %lu bytes\n", gate_id, count * sizeof_datatype(datatype));
      err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);

      recvptr = recvbuffer;
      for(i=0 ; i<count ; i++) {
        ptr = buffer + i*mpir_datatype->size;
        MPI_NMAD_TRACE("Element %d starts at %p (%p + %lu)\n", i, ptr, buffer, i*mpir_datatype->size);
        for(j=0 ; j<mpir_datatype->elements ; j++) {
          ptr += mpir_datatype->indices[j];
          MPI_NMAD_TRACE("Sub-element %d starts at %p (%p + %ld)\n", j, ptr, ptr-mpir_datatype->indices[j], mpir_datatype->indices[j]);
          memcpy(ptr, recvptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          MPI_NMAD_TRACE("Copying from %p and moving by %lu\n", recvptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
          recvptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j];
          ptr -= mpir_datatype->indices[j];
        }
      }
      free(recvbuffer);
      if (_request->request_type != MPI_REQUEST_ZERO) _request->request_type = MPI_REQUEST_RECV;
    }
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

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  _request->request_ptr = NULL;
  _request->request_type = MPI_REQUEST_RECV;
  mpi_inline_irecv(buffer, count, datatype, source, tag, comm, &request);

  if (_request->request_type == MPI_REQUEST_RECV) {
    MPI_NMAD_TRACE("Calling nm_so_sr_rwait...\n");
    err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);
  }
  else if (_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(_request->request_cnx);
    err = nm_so_end_unpacking(connection);
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
  return err;
}

int MPI_Irecv(void* buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }
  if (tbx_unlikely(tag == MPI_ANY_TAG)) return not_implemented("Using MPI_ANY_TAG");

  MPI_NMAD_TRACE("Receiving message from %d of datatype %d\n", source, datatype);
  _request->request_ptr = NULL;
  _request->request_type = MPI_REQUEST_RECV;
  return mpi_inline_irecv(buffer, count, datatype, source, tag, comm, request);
}

int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) {
  struct MPI_Request_s *_request = (struct MPI_Request_s *)request;
  int                   err;

  MPI_NMAD_TRACE("Waiting for a request %d\n", _request->request_type);
  if (_request->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rwait(p_so_sr_if, _request->request_id);
  }
  else if (_request->request_type == MPI_REQUEST_SEND) {
    err = nm_so_sr_swait(p_so_sr_if, _request->request_id);
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
      *index = i;
      return MPI_SUCCESS;
    }
  }
  *index = MPI_UNDEFINED;
  return err;
}

int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status) {
  int err      = 0;
  long gate_id;

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }
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

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }
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

int MPI_Barrier(MPI_Comm comm) {
  tbx_bool_t termination;

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }

  termination = test_termination(comm);
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
  int err;

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
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
      if (err != 0) return err;
    }
    for(i=0 ; i<global_size ; i++) {
      if (i==root) continue;
      err = MPI_Wait(&requests[i], NULL);
      if (err != 0) return err;
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
  return err;
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
  mpir_function_t *function;

  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }

  function = mpir_get_function(op);
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

    free(remote_sendbufs);
    free(requests);
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
  if (tbx_unlikely(!(mpir_is_comm_valid(comm)))) {
    ERROR("Communicator %d not valid (does not exist or is not global)\n", comm);
    return -1;
  }

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
                     MPI_Aint *array_of_displacements,
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
