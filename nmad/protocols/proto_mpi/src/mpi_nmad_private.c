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
 * mpi_nmad_private.c
 * ==================
 */

#include <stdint.h>
#include "mpi.h"
#include "mpi_nmad_private.h"
#include "nm_so_parameters.h"

static mpir_datatype_t     **datatypes          = NULL;
static p_tbx_slist_t         available_datatypes;
static mpir_function_t     **functions          = NULL;
static p_tbx_slist_t         available_functions;
static mpir_communicator_t **communicators      = NULL;
static p_tbx_slist_t         available_communicators;
static int 		     nb_incoming_msg    = 0;
static int 		     nb_outgoing_msg    = 0;

static long                 *out_gate_id	= NULL;
static long                 *in_gate_id	        = NULL;
static int                  *out_dest	 	= NULL;
static int                  *in_dest		= NULL;

debug_type_t debug_mpi_nmad_trace=NEW_DEBUG_TYPE("MPI_NMAD: ", "mpi_nmad_trace");
debug_type_t debug_mpi_nmad_transfer=NEW_DEBUG_TYPE("MPI_NMAD_TRANSFER: ", "mpi_nmad_transfer");
debug_type_t debug_mpi_nmad_log=NEW_DEBUG_TYPE("MPI_NMAD_LOG: ", "mpi_nmad_log");

int mpir_not_implemented(char *s) {
  fprintf(stderr, "*************** ERROR: %s: Not implemented yet\n", s);
  fflush(stderr);
  return MPI_Abort(MPI_COMM_WORLD, 1);
}

int mpir_internal_init(int global_size, int process_rank, p_mad_madeleine_t madeleine) {
  int i;
  int                              dest;
  int                              source;
  p_mad_channel_t                  channel    = NULL;
  p_mad_connection_t               connection = NULL;
  p_mad_nmad_connection_specific_t cs	      = NULL;

  /*
   * Initialise the basic datatypes
   */
  datatypes = malloc((NUMBER_OF_DATATYPES+1) * sizeof(mpir_datatype_t *));

  for(i=MPI_DATATYPE_NULL ; i<=MPI_INTEGER ; i++) {
    datatypes[i] = malloc(sizeof(mpir_datatype_t));
  }
  for(i=MPI_DATATYPE_NULL ; i<=MPI_INTEGER ; i++) {
    datatypes[i]->basic = 1;
    datatypes[i]->committed = 1;
    datatypes[i]->is_contig = 1;
    datatypes[i]->dte_type = MPIR_BASIC;
    datatypes[i]->active_communications = 100;
    datatypes[i]->free_requested = 0;
  }
  datatypes[MPI_DATATYPE_NULL]->size = 0;
  datatypes[MPI_CHAR]->size = sizeof(signed char);
  datatypes[MPI_UNSIGNED_CHAR]->size = sizeof(unsigned char);
  datatypes[MPI_BYTE]->size = 1;
  datatypes[MPI_SHORT]->size = sizeof(signed short);
  datatypes[MPI_UNSIGNED_SHORT]->size = sizeof(unsigned short);
  datatypes[MPI_INT]->size = sizeof(signed int);
  datatypes[MPI_UNSIGNED]->size = sizeof(unsigned int);
  datatypes[MPI_LONG]->size = sizeof(signed long);
  datatypes[MPI_UNSIGNED_LONG]->size = sizeof(unsigned long);
  datatypes[MPI_FLOAT]->size = sizeof(float);
  datatypes[MPI_DOUBLE]->size = sizeof(double);
  datatypes[MPI_LONG_DOUBLE]->size = sizeof(long double);
  datatypes[MPI_LONG_LONG_INT]->size = sizeof(long long int);
  datatypes[MPI_LONG_LONG]->size = sizeof(long long);
  datatypes[MPI_COMPLEX]->size = 2*sizeof(float);
  datatypes[MPI_DOUBLE_COMPLEX]->size = 2*sizeof(double);
  datatypes[MPI_LOGICAL]->size = sizeof(float);
  datatypes[MPI_REAL]->size = sizeof(float);
  datatypes[MPI_DOUBLE_PRECISION]->size = sizeof(double);
  datatypes[MPI_INTEGER]->size = sizeof(float);

  for(i=MPI_DATATYPE_NULL ; i<=MPI_INTEGER ; i++) {
    datatypes[i]->extent = datatypes[i]->size;
  }

  available_datatypes = tbx_slist_nil();
  for(i=MPI_INTEGER+1 ; i<=NUMBER_OF_DATATYPES ; i++) {
    int *ptr;
    ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(available_datatypes, ptr);
  }

  /* Initialise data for communicators */
  communicators = malloc(NUMBER_OF_COMMUNICATORS * sizeof(mpir_communicator_t));
  communicators[0] = malloc(sizeof(mpir_communicator_t));
  communicators[0]->communicator_id = MPI_COMM_WORLD;
  communicators[0]->size = global_size;
  communicators[0]->rank = process_rank;
  communicators[0]->global_ranks = malloc(global_size * sizeof(int));
  for(i=0 ; i<global_size ; i++) {
    communicators[0]->global_ranks[i] = i;
  }

  communicators[1] = malloc(sizeof(mpir_communicator_t));
  communicators[1]->communicator_id = MPI_COMM_SELF;
  communicators[1]->size = 1;
  communicators[1]->rank = process_rank;
  communicators[1]->global_ranks = malloc(1 * sizeof(int));
  communicators[1]->global_ranks[0] = process_rank;

  available_communicators = tbx_slist_nil();
  for(i=MPI_COMM_SELF+1 ; i<=NUMBER_OF_COMMUNICATORS ; i++) {
    int *ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(available_communicators, ptr);
  }

  /* Initialise the collective functions */
  functions = malloc((NUMBER_OF_FUNCTIONS+1) * sizeof(mpir_function_t));
  for(i=MPI_MAX ; i<=MPI_MAXLOC ; i++) {
    functions[i] = malloc(sizeof(mpir_function_t));
    functions[i]->commute = 1;
  }
  functions[MPI_MAX]->function = &mpir_op_max;
  functions[MPI_MIN]->function = &mpir_op_min;
  functions[MPI_SUM]->function = &mpir_op_sum;
  functions[MPI_PROD]->function = &mpir_op_prod;

  available_functions = tbx_slist_nil();
  for(i=1 ; i<MPI_MAX ; i++) {
    int *ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(available_functions, ptr);
  }

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
    return MPI_ERR_INTERN;
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

  return MPI_SUCCESS;
}

void mpir_internal_exit() {
  int i;

  for(i=0 ; i<=MPI_INTEGER ; i++) {
    free(datatypes[i]);
  }
  free(datatypes);
  while (tbx_slist_is_nil(available_datatypes) == tbx_false) {
    int *ptr = tbx_slist_extract(available_datatypes);
    free(ptr);
  }
  tbx_slist_clear(available_datatypes);
  tbx_slist_free(available_datatypes);

  free(communicators[0]->global_ranks);
  free(communicators[0]);
  free(communicators[1]->global_ranks);
  free(communicators[1]);
  free(communicators);
  while (tbx_slist_is_nil(available_communicators) == tbx_false) {
    int *ptr = tbx_slist_extract(available_communicators);
    free(ptr);
  }
  tbx_slist_clear(available_communicators);
  tbx_slist_free(available_communicators);

  for(i=MPI_MAX ; i<=MPI_MAXLOC ; i++) {
    free(functions[i]);
  }
  free(functions);
  while (tbx_slist_is_nil(available_functions) == tbx_false) {
    int *ptr = tbx_slist_extract(available_functions);
    free(ptr);
  }
  tbx_slist_clear(available_functions);
  tbx_slist_free(available_functions);

  free(out_gate_id);
  free(in_gate_id);
  free(out_dest);
  free(in_dest);
}

long mpir_get_in_gate_id(int node) {
  return in_gate_id[node];
}

long mpir_get_out_gate_id(int node) {
  return out_gate_id[node];
}

int mpir_get_in_dest(long gate) {
  return in_dest[gate];
}

int mpir_get_out_dest(long gate) {
  return out_dest[gate];
}

__tbx_inline__
int mpir_inline_isend(void *buffer,
		      int count,
		      int dest,
		      int tag,
		      MPI_Communication_Mode communication_mode,
		      mpir_communicator_t *mpir_communicator,
		      mpir_request_t *mpir_request,
		      struct nm_so_interface *p_so_sr_if,
		      nm_so_pack_interface    p_so_pack_if) {
  long                  gate_id;
  mpir_datatype_t      *mpir_datatype = NULL;
  int                   err = MPI_SUCCESS;
  int                   seq, probe;

  if (tbx_unlikely(dest >= mpir_communicator->size || out_gate_id[mpir_communicator->global_ranks[dest]] == -1)) {
    NM_DISPF("Cannot find a connection between %d and %d, %d\n", mpir_communicator->rank, dest, mpir_communicator->global_ranks[dest]);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  gate_id = out_gate_id[mpir_communicator->global_ranks[dest]];

  mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  mpir_datatype->active_communications ++;
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_communicator, tag);
  mpir_request->user_tag = tag;

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid sending tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, tag, NM_SO_MAX_TAGS);
    return MPI_ERR_INTERN;
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
        return MPI_ERR_INTERN;
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
    return MPI_ERR_INTERN;
  }

  mpir_inc_nb_outgoing_msg();
  return err;
}

__tbx_inline__
void mpir_set_status(MPI_Request *request,
		     MPI_Status *status,
		     struct nm_so_interface *p_so_sr_if) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;

  status->MPI_TAG = mpir_request->user_tag;
  status->MPI_ERROR = mpir_request->request_error;

  status->count = mpir_sizeof_datatype(mpir_request->request_datatype);

  if (mpir_request->request_source == MPI_ANY_SOURCE) {
    long gate_id;
    nm_so_sr_recv_source(p_so_sr_if, mpir_request->request_nmad, &gate_id);
    status->MPI_SOURCE = in_dest[gate_id];
  }
  else {
    status->MPI_SOURCE = mpir_request->request_source;
  }
}

__tbx_inline__
int mpir_inline_irecv(void* buffer,
		      int count,
		      int source,
		      int tag,
		      mpir_communicator_t *mpir_communicator,
		      mpir_request_t *mpir_request,
		      struct nm_so_interface *p_so_sr_if,
		      nm_so_pack_interface    p_so_pack_if) {
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
      return MPI_ERR_INTERN;
    }
    gate_id = in_gate_id[mpir_communicator->global_ranks[source]];
  }

  mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  mpir_datatype->active_communications ++;
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_communicator, tag);
  mpir_request->user_tag = tag;
  mpir_request->request_source = source;

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid receiving tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, tag, NM_SO_MAX_TAGS);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
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
    return MPI_ERR_INTERN;
  }

  mpir_inc_nb_incoming_msg();
  MPI_NMAD_TRACE("Irecv completed\n");
  MPI_NMAD_LOG_OUT();
  return mpir_request->request_error;
}

int get_available_datatype() {
  if (tbx_slist_is_nil(available_datatypes) == tbx_true) {
    ERROR("Maximum number of datatypes created");
    return MPI_ERR_INTERN;
  }
  else {
    int datatype;
    int *ptr = tbx_slist_extract(available_datatypes);
    datatype = *ptr;
    free(ptr);
    return datatype;
  }
}

size_t mpir_sizeof_datatype(MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);
  return mpir_datatype->size;
}

mpir_datatype_t* mpir_get_datatype(MPI_Datatype datatype) {
  if (tbx_unlikely(datatype <= NUMBER_OF_DATATYPES)) {
    if (tbx_unlikely(datatypes[datatype] == NULL)) {
      ERROR("Datatype %d invalid", datatype);
      return NULL;
    }
    else {
      return datatypes[datatype];
    }
  }
  else {
    ERROR("Datatype %d unknown", datatype);
    return NULL;
  }
}

int mpir_type_size(MPI_Datatype datatype, int *size) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);
  *size = mpir_datatype->size;
  return MPI_SUCCESS;
}

int mpir_type_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);
  *lb = 0;
  *extent = mpir_datatype->extent;
  return MPI_SUCCESS;
}

int mpir_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype) {
  int i;
  mpir_datatype_t *mpir_old_datatype = mpir_get_datatype(oldtype);

  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type  = mpir_old_datatype->dte_type;
  datatypes[*newtype]->basic     = mpir_old_datatype->basic;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = mpir_old_datatype->is_contig;
  datatypes[*newtype]->size      = mpir_old_datatype->size;
  datatypes[*newtype]->extent    = extent;

  if(datatypes[*newtype]->dte_type == MPIR_CONTIG) {
    datatypes[*newtype]->old_size  = mpir_old_datatype->old_size;
    datatypes[*newtype]->elements  = mpir_old_datatype->elements;
  }
  else if (datatypes[*newtype]->dte_type == MPIR_VECTOR ||
	   datatypes[*newtype]->dte_type == MPIR_HVECTOR) {
    datatypes[*newtype]->old_size   = mpir_old_datatype->old_size;
    datatypes[*newtype]->old_sizes  = mpir_old_datatype->old_sizes;
    datatypes[*newtype]->elements   = mpir_old_datatype->elements;
    datatypes[*newtype]->blocklen   = mpir_old_datatype->blocklen;
    datatypes[*newtype]->block_size = mpir_old_datatype->block_size;
    datatypes[*newtype]->stride     = mpir_old_datatype->stride;
  }
  else if (datatypes[*newtype]->dte_type == MPIR_INDEXED ||
	   datatypes[*newtype]->dte_type == MPIR_HINDEXED) {
    datatypes[*newtype]->old_size  = mpir_old_datatype->old_size;
    datatypes[*newtype]->old_sizes = mpir_old_datatype->old_sizes;
    datatypes[*newtype]->elements  = mpir_old_datatype->elements;
    datatypes[*newtype]->blocklens = malloc(datatypes[*newtype]->elements * sizeof(int));
    datatypes[*newtype]->indices   = malloc(datatypes[*newtype]->elements * sizeof(MPI_Aint));
    for(i=0 ; i<datatypes[*newtype]->elements ; i++) {
      datatypes[*newtype]->blocklens[i] = mpir_old_datatype->blocklens[i];
      datatypes[*newtype]->indices[i]   = mpir_old_datatype->indices[i];
    }
  }
  else if (datatypes[*newtype]->dte_type == MPIR_STRUCT) {
    datatypes[*newtype]->elements  = mpir_old_datatype->elements;
    datatypes[*newtype]->blocklens = malloc(datatypes[*newtype]->elements * sizeof(int));
    datatypes[*newtype]->indices   = malloc(datatypes[*newtype]->elements * sizeof(MPI_Aint));
    datatypes[*newtype]->old_sizes = malloc(datatypes[*newtype]->elements * sizeof(size_t));
    for(i=0 ; i<datatypes[*newtype]->elements ; i++) {
      datatypes[*newtype]->blocklens[i] = mpir_old_datatype->blocklens[i];
      datatypes[*newtype]->indices[i]   = mpir_old_datatype->indices[i];
      datatypes[*newtype]->old_sizes[i] = mpir_old_datatype->old_sizes[i];
    }
  }
  else if (datatypes[*newtype]->dte_type != MPIR_BASIC) {
    ERROR("Datatype %d unknown", oldtype);
    return MPI_ERR_OTHER;
  }

  MPI_NMAD_TRACE_LEVEL(3, "Creating resized type (%d) with extent=%ld, based on type %d with a extent %ld\n",
		       *newtype, (long)datatypes[*newtype]->extent, oldtype, (long)mpir_old_datatype->extent);
  return MPI_SUCCESS;
}

int mpir_type_commit(MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);
  mpir_datatype->committed = 1;
  mpir_datatype->is_optimized = 0;
  mpir_datatype->active_communications = 0;
  mpir_datatype->free_requested = 0;
  return MPI_SUCCESS;
}

int mpir_type_unlock(MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);

  MPI_NMAD_TRACE_LEVEL(3, "Unlocking datatype %d\n", datatype);
  mpir_datatype->active_communications --;
  if (mpir_datatype->active_communications == 0 && mpir_datatype->free_requested == 1) {
    mpir_type_free(datatype);
  }
  return MPI_SUCCESS;
}

int mpir_type_free(MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);

  if (mpir_datatype->active_communications != 0) {
    mpir_datatype->free_requested = 1;
    MPI_NMAD_TRACE_LEVEL(3, "Datatype %d still in use. Cannot be released\n", datatype);
    return MPI_DATATYPE_ACTIVE;
  }
  else {
    MPI_NMAD_TRACE_LEVEL(3, "Releasing datatype %d\n", datatype);
    if (datatypes[datatype]->dte_type == MPIR_INDEXED ||
        datatypes[datatype]->dte_type == MPIR_HINDEXED ||
        datatypes[datatype]->dte_type == MPIR_STRUCT) {
      free(datatypes[datatype]->blocklens);
      free(datatypes[datatype]->indices);
      if (datatypes[datatype]->dte_type == MPIR_STRUCT) {
        free(datatypes[datatype]->old_sizes);
      }
    }
    int *ptr;
    ptr = malloc(sizeof(int));
    *ptr = datatype;
    tbx_slist_enqueue(available_datatypes, ptr);

    free(datatypes[datatype]);
    datatypes[datatype] = NULL;
    return MPI_SUCCESS;
  }
}

int mpir_type_contiguous(int count,
                         MPI_Datatype oldtype,
                         MPI_Datatype *newtype) {
  mpir_datatype_t *mpir_old_datatype;

  mpir_old_datatype = mpir_get_datatype(oldtype);
  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = MPIR_CONTIG;
  datatypes[*newtype]->basic = 0;
  datatypes[*newtype]->old_size = mpir_old_datatype->extent;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 1;
  datatypes[*newtype]->size = datatypes[*newtype]->old_size * count;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->extent = datatypes[*newtype]->old_size * count;

  MPI_NMAD_TRACE_LEVEL(3, "Creating new contiguous type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		       (long)datatypes[*newtype]->size, (long)datatypes[*newtype]->extent, oldtype, (long)datatypes[*newtype]->old_size);
  return MPI_SUCCESS;
}

int mpir_type_vector(int count,
                     int blocklength,
                     int stride,
                     mpir_nodetype_t type,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  mpir_datatype_t *mpir_old_datatype;

  mpir_old_datatype = mpir_get_datatype(oldtype);
  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = type;
  datatypes[*newtype]->basic = 0;
  datatypes[*newtype]->old_size = mpir_old_datatype->extent;
  datatypes[*newtype]->old_sizes = NULL;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->size = datatypes[*newtype]->old_size * count * blocklength;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->blocklen = blocklength;
  datatypes[*newtype]->block_size = blocklength * datatypes[*newtype]->old_size;
  datatypes[*newtype]->stride = stride;
  datatypes[*newtype]->extent = datatypes[*newtype]->old_size * count * blocklength;

  MPI_NMAD_TRACE_LEVEL(3, "Creating new (h)vector type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		       (long)datatypes[*newtype]->size, (long)datatypes[*newtype]->extent, oldtype, (long)datatypes[*newtype]->old_size);
  return MPI_SUCCESS;
}

int mpir_type_indexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      mpir_nodetype_t type,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  int i;
  mpir_datatype_t *mpir_old_datatype;

  MPI_NMAD_TRACE("Creating indexed derived datatype based on %d elements\n", count);
  mpir_old_datatype = mpir_get_datatype(oldtype);

  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = type;
  datatypes[*newtype]->basic = 0;
  datatypes[*newtype]->old_size = mpir_old_datatype->extent;
  datatypes[*newtype]->old_sizes = NULL;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->indices[i] = array_of_displacements[i];
    MPI_NMAD_TRACE("Element %d: length %d, indice %ld\n", i, datatypes[*newtype]->blocklens[i], (long)datatypes[*newtype]->indices[i]);
  }
  if (type == MPIR_HINDEXED) {
    for(i=0 ; i<count ; i++) {
      datatypes[*newtype]->indices[i] *= datatypes[*newtype]->old_size;
      MPI_NMAD_TRACE("Element %d: indice %ld\n", i, (long)datatypes[*newtype]->indices[i]);
    }
  }
  datatypes[*newtype]->size = (datatypes[*newtype]->indices[count-1] + datatypes[*newtype]->old_size * datatypes[*newtype]->blocklens[count-1]);
  datatypes[*newtype]->extent = (datatypes[*newtype]->indices[count-1] + mpir_old_datatype->extent * datatypes[*newtype]->blocklens[count-1]);

  MPI_NMAD_TRACE_LEVEL(3, "Creating new index type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		       (long)datatypes[*newtype]->size, (long)datatypes[*newtype]->extent, oldtype, (long)datatypes[*newtype]->old_size);
  return MPI_SUCCESS;
}

int mpir_type_struct(int count,
                     int *array_of_blocklengths,
                     MPI_Aint *array_of_displacements,
                     MPI_Datatype *array_of_types,
                     MPI_Datatype *newtype) {
  int i;

  MPI_NMAD_TRACE("Creating struct derived datatype based on %d elements\n", count);
  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = MPIR_STRUCT;
  datatypes[*newtype]->basic = 0;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->size = 0;

  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  datatypes[*newtype]->old_sizes = malloc(count * sizeof(size_t));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->old_sizes[i] = mpir_get_datatype(array_of_types[i])->size;
    datatypes[*newtype]->indices[i] = array_of_displacements[i];

    MPI_NMAD_TRACE("Element %d: length %d, old_type size %ld, indice %ld\n", i, datatypes[*newtype]->blocklens[i], (long)datatypes[*newtype]->old_sizes[i],
                   (long)datatypes[*newtype]->indices[i]);
    datatypes[*newtype]->size +=  datatypes[*newtype]->blocklens[i] * datatypes[*newtype]->old_sizes[i];
  }
  /* We suppose here that the last field of the struct does not need
     an alignment. In case, one sends an array of struct, the 1st
     field of the 2nd struct immediatly follows the last field of the
     previous struct.
  */
  datatypes[*newtype]->extent = datatypes[*newtype]->indices[count-1] + datatypes[*newtype]->blocklens[count-1] * datatypes[*newtype]->old_sizes[count-1];
  MPI_NMAD_TRACE_LEVEL(3, "Creating new struct type (%d) with size=%ld and extent=%ld\n", *newtype, (long)datatypes[*newtype]->size, (long)datatypes[*newtype]->extent);
  return MPI_SUCCESS;
}

int mpir_op_create(MPI_User_function *function,
                   int commute,
                   MPI_Op *op) {
  if (tbx_slist_is_nil(available_functions) == tbx_true) {
    ERROR("Maximum number of operations created");
    return MPI_ERR_INTERN;
  }
  else {
    int *ptr = tbx_slist_extract(available_functions);
    *op = *ptr;
    free(ptr);

    functions[*op] = malloc(sizeof(mpir_function_t));
    functions[*op]->function = function;
    functions[*op]->commute = commute;
    return MPI_SUCCESS;
  }
}

int mpir_op_free(MPI_Op *op) {
  if (*op > NUMBER_OF_FUNCTIONS || functions[*op] == NULL) {
    ERROR("Operator %d unknown\n", *op);
    return MPI_ERR_OTHER;
  }
  else {
    int *ptr;
    free(functions[*op]);
    ptr = malloc(sizeof(int));
    *ptr = *op;
    tbx_slist_enqueue(available_functions, ptr);
    *op = MPI_OP_NULL;
    return MPI_SUCCESS;
  }
}

mpir_function_t *mpir_get_function(MPI_Op op) {
  if (functions[op] != NULL) {
    return functions[op];
  }
  else {
    ERROR("Operation %d unknown", op);
    return NULL;
  }
}

void mpir_op_max(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MAX */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_MAX */
    default : {
      ERROR("Datatype %d for MAX Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_min(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
          if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MIN */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_MAX */
    default : {
      ERROR("Datatype %d for MIN Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_sum(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        MPI_NMAD_TRACE("Summing %d and %d\n", i_inoutvec[i], i_invec[i]);
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_SUM */
    case MPI_FLOAT : {
      float *i_invec = (float *) invec;
      float *i_inoutvec = (float *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        MPI_NMAD_TRACE("Summing %f and %f\n", i_inoutvec[i], i_invec[i]);
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_FLOAT FOR MPI_SUM */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_SUM */
    case MPI_DOUBLE_COMPLEX : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len*2 ; i++) {
        i_inoutvec[i] += i_invec[i];
      }
      break;
    }
    default : {
      ERROR("Datatype %d for SUM Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_prod(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_PROD */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_PROD */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_PROD */
    default : {
      ERROR("Datatype %d for PROD Reduce operation", *type);
      break;
    }
  }
}

mpir_communicator_t *mpir_get_communicator(MPI_Comm comm) {
  if (tbx_unlikely(comm <= NUMBER_OF_COMMUNICATORS)) {
    if (communicators[comm-MPI_COMM_WORLD] == NULL) {
      ERROR("Communicator %d invalid", comm);
      return NULL;
    }
    else {
      return communicators[comm-MPI_COMM_WORLD];
    }
  }
  else {
    ERROR("Communicator %d unknown", comm);
    return NULL;
  }
}

int mpir_comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  if (tbx_slist_is_nil(available_communicators) == tbx_true) {
    ERROR("Maximum number of communicators created");
    return MPI_ERR_INTERN;
  }
  else if (communicators[comm-MPI_COMM_WORLD] == NULL) {
    ERROR("Communicator %d is not valid", comm);
    return MPI_ERR_OTHER;
  }
  else {
    int i;
    int *ptr = tbx_slist_extract(available_communicators);
    *newcomm = *ptr;
    free(ptr);

    communicators[*newcomm - MPI_COMM_WORLD] = malloc(sizeof(mpir_communicator_t));
    communicators[*newcomm - MPI_COMM_WORLD]->communicator_id = *newcomm;
    communicators[*newcomm - MPI_COMM_WORLD]->size = communicators[comm - MPI_COMM_WORLD]->size;
    communicators[*newcomm - MPI_COMM_WORLD]->global_ranks = malloc(communicators[*newcomm - MPI_COMM_WORLD]->size * sizeof(int));
    for(i=0 ; i<communicators[*newcomm - MPI_COMM_WORLD]->size ; i++) {
      communicators[*newcomm - MPI_COMM_WORLD]->global_ranks[i] = communicators[comm - MPI_COMM_WORLD]->global_ranks[i];
    }

    return MPI_SUCCESS;
  }
}

int mpir_comm_free(MPI_Comm *comm) {
  if (*comm == MPI_COMM_WORLD) {
    ERROR("Cannot free communicator MPI_COMM_WORLD");
    return MPI_ERR_OTHER;
  }
  else if (*comm == MPI_COMM_SELF) {
    ERROR("Cannot free communicator MPI_COMM_SELF");
    return MPI_ERR_OTHER;
  }
  else if (*comm > NUMBER_OF_COMMUNICATORS || communicators[*comm-MPI_COMM_WORLD] == NULL) {
    ERROR("Communicator %d unknown\n", *comm);
    return MPI_ERR_OTHER;
  }
  else {
    int *ptr;
    free(communicators[*comm-MPI_COMM_WORLD]->global_ranks);
    free(communicators[*comm-MPI_COMM_WORLD]);
    communicators[*comm-MPI_COMM_WORLD] = NULL;
    ptr = malloc(sizeof(int));
    *ptr = *comm;
    tbx_slist_enqueue(available_communicators, ptr);
    *comm = MPI_COMM_NULL;
    return MPI_SUCCESS;
  }
}

__inline__
int mpir_project_comm_and_tag(mpir_communicator_t *mpir_communicator, int tag) {
  /*
   * NewMadeleine only allows us 7 bits!
   * We suppose that comm is represented on 3 bits and tag on 4 bits.
   * We stick both of them into a new 7-bits representation
   */
  int newtag = (mpir_communicator->communicator_id-MPI_COMM_WORLD) << 4;
  newtag += tag;
  return newtag;
}

/*
 * Using Friedmann  Mattern's Four Counter Method to detect
 * termination detection.
 * "Algorithms for Distributed Termination Detection." Distributed
 * Computing, vol 2, pp 161-175, 1987.
 */
tbx_bool_t mpir_test_termination(MPI_Comm comm) {
  int process_rank, global_size;
  MPI_Comm_rank(comm, &process_rank);
  MPI_Comm_size(comm, &global_size);
  int tag = 49;

  if (process_rank == 0) {
    // 1st phase
    int global_nb_incoming_msg = 0;
    int global_nb_outgoing_msg = 0;
    int i, remote_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    global_nb_incoming_msg = nb_incoming_msg;
    global_nb_outgoing_msg = nb_outgoing_msg;
    for(i=1 ; i<global_size ; i++) {
      MPI_Recv(remote_counters, 2, MPI_INT, i, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      global_nb_incoming_msg += remote_counters[0];
      global_nb_outgoing_msg += remote_counters[1];
    }
    MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);

    tbx_bool_t answer = tbx_true;
    if (global_nb_incoming_msg != global_nb_outgoing_msg) {
      answer = tbx_false;
    }

    for(i=1 ; i<global_size ; i++) {
      MPI_Send(&answer, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
    }

    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      global_nb_incoming_msg = nb_incoming_msg;
      global_nb_outgoing_msg = nb_outgoing_msg;
      for(i=1 ; i<global_size ; i++) {
        MPI_Recv(remote_counters, 2, MPI_INT, i, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        global_nb_incoming_msg += remote_counters[0];
        global_nb_outgoing_msg += remote_counters[1];
      }

      MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);
      tbx_bool_t answer = tbx_true;
      if (global_nb_incoming_msg != global_nb_outgoing_msg) {
        answer = tbx_false;
      }

      for(i=1 ; i<global_size ; i++) {
        MPI_Send(&answer, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
      }

      return answer;
    }
  }
  else {
    int answer, local_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    local_counters[0] = nb_incoming_msg;
    local_counters[1] = nb_outgoing_msg;
    MPI_Send(local_counters, 2, MPI_INT, 0, tag, MPI_COMM_WORLD);

    MPI_Recv(&answer, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      local_counters[0] = nb_incoming_msg;
      local_counters[1] = nb_outgoing_msg;
      MPI_Send(local_counters, 2, MPI_INT, 0, tag, MPI_COMM_WORLD);

      MPI_Recv(&answer, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      return answer;
    }
  }
}

void mpir_inc_nb_incoming_msg(void) {
  nb_incoming_msg ++;
}

void mpir_inc_nb_outgoing_msg(void) {
  nb_outgoing_msg ++;
}
