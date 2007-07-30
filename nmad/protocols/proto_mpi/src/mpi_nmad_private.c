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
 * mpi_nmad_private.c
 * ==================
 */

#include <stdint.h>
#include "madeleine.h"
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

static struct nm_so_interface *p_so_sr_if;
static nm_so_pack_interface    p_so_pack_if;

debug_type_t debug_mpi_nmad_trace=NEW_DEBUG_TYPE("MPI_NMAD: ", "mpi_nmad_trace");
debug_type_t debug_mpi_nmad_transfer=NEW_DEBUG_TYPE("MPI_NMAD_TRANSFER: ", "mpi_nmad_transfer");
debug_type_t debug_mpi_nmad_log=NEW_DEBUG_TYPE("MPI_NMAD_LOG: ", "mpi_nmad_log");

int mpir_internal_init(int global_size,
		       int process_rank,
		       p_mad_madeleine_t madeleine,
                       struct nm_so_interface *_p_so_sr_if,
		       nm_so_pack_interface _p_so_pack_if) {
  int i;
  int                              dest;
  int                              source;
  p_mad_channel_t                  channel    = NULL;
  p_mad_connection_t               connection = NULL;
  p_mad_nmad_connection_specific_t cs	      = NULL;

  /*
    Set the NewMadeleine interfaces
  */
  p_so_sr_if = _p_so_sr_if;
  p_so_pack_if = _p_so_pack_if;

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
    datatypes[i]->lb = 0;
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
  functions[MPI_LAND]->function = &mpir_op_land;
  functions[MPI_BAND]->function = &mpir_op_band;
  functions[MPI_LOR]->function = &mpir_op_lor;
  functions[MPI_BOR]->function = &mpir_op_bor;
  functions[MPI_LXOR]->function = &mpir_op_lxor;
  functions[MPI_BXOR]->function = &mpir_op_bxor;
  functions[MPI_MINLOC]->function = &mpir_op_minloc;
  functions[MPI_MAXLOC]->function = &mpir_op_maxloc;

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

int mpir_internal_exit() {
  int i;

  for(i=0 ; i<=MPI_INTEGER ; i++) {
    FREE_AND_SET_NULL(datatypes[i]);
  }
  FREE_AND_SET_NULL(datatypes);
  while (tbx_slist_is_nil(available_datatypes) == tbx_false) {
    int *ptr = tbx_slist_extract(available_datatypes);
    FREE_AND_SET_NULL(ptr);
  }
  tbx_slist_clear(available_datatypes);
  tbx_slist_free(available_datatypes);

  FREE_AND_SET_NULL(communicators[0]->global_ranks);
  FREE_AND_SET_NULL(communicators[0]);
  FREE_AND_SET_NULL(communicators[1]->global_ranks);
  FREE_AND_SET_NULL(communicators[1]);
  FREE_AND_SET_NULL(communicators);
  while (tbx_slist_is_nil(available_communicators) == tbx_false) {
    int *ptr = tbx_slist_extract(available_communicators);
    FREE_AND_SET_NULL(ptr);
  }
  tbx_slist_clear(available_communicators);
  tbx_slist_free(available_communicators);

  for(i=MPI_MAX ; i<=MPI_MAXLOC ; i++) {
    FREE_AND_SET_NULL(functions[i]);
  }
  FREE_AND_SET_NULL(functions);
  while (tbx_slist_is_nil(available_functions) == tbx_false) {
    int *ptr = tbx_slist_extract(available_functions);
    FREE_AND_SET_NULL(ptr);
  }
  tbx_slist_clear(available_functions);
  tbx_slist_free(available_functions);

  FREE_AND_SET_NULL(out_gate_id);
  FREE_AND_SET_NULL(in_gate_id);
  FREE_AND_SET_NULL(out_dest);
  FREE_AND_SET_NULL(in_dest);

  return MPI_SUCCESS;
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

/**
 * Aggregates data represented by a vector datatype in a contiguous
 * buffer.
 */
static inline int mpir_datatype_vector_aggregate(void *newptr,
						 void *buffer,
						 mpir_datatype_t *mpir_datatype,
						 int count) {
#ifdef NMAD_DEBUG
  void * const orig = buffer;
  void * const dest = newptr;
#endif
  int i, j;
  for(i=0 ; i<count ; i++) {
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      MPI_NMAD_TRACE("Copy element %d, %d (size %ld) from %p (+%d) to %p (+%d)\n",
                     i, j, (long) mpir_datatype->block_size, buffer, (int)(buffer-orig),
                     newptr, (int)(newptr-dest));
      memcpy(newptr, buffer, mpir_datatype->block_size);
      newptr += mpir_datatype->block_size;
      buffer += mpir_datatype->stride;
    }
  }
  return MPI_SUCCESS;
}

/**
 * Packs into a NM connection data represented by a vector datatype.
 */
static inline int mpir_datatype_vector_pack(struct nm_so_cnx *connection,
					    void *buffer,
					    mpir_datatype_t *mpir_datatype,
					    int count) {
#ifdef NMAD_DEBUG
  void *const orig = buffer;
#endif
  int               i, j, err;
  for(i=0 ; i<count ; i++) {
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      MPI_NMAD_TRACE("Element %d, %d with size %ld starts at %p (+ %d)\n", i, j, (long) mpir_datatype->block_size, buffer, (int)(buffer-orig));
      err = nm_so_pack(connection, buffer, mpir_datatype->block_size);
      buffer += mpir_datatype->stride;
    }
  }
  return err;
}

/**
 * Unpacks from a NM connection data represented by a vector datatype.
 */
static inline int mpir_datatype_vector_unpack(struct nm_so_cnx *connection,
					      mpir_request_t *mpir_request,
					      void *buffer,
					      mpir_datatype_t *mpir_datatype,
					      int count) {
  int i, k=0, err;
  mpir_request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(void *));
  mpir_request->request_ptr[0] = buffer;
  for(i=0 ; i<count ; i++) {
    int j;
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      err = nm_so_unpack(connection, mpir_request->request_ptr[k], mpir_datatype->block_size);
      k++;
      mpir_request->request_ptr[k] = mpir_request->request_ptr[k-1] + mpir_datatype->block_size;
    }
  }
  if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
  return err;
}

/**
 * Splits data from a contiguous buffer into data represented by a
 * vector datatype
 */
static inline int mpir_datatype_vector_split(void *recvbuffer,
					     void *buffer,
					     mpir_datatype_t *mpir_datatype,
					     int count) {
  void *recvptr = recvbuffer;
  void *ptr = buffer;
  int i;
  for(i=0 ; i<count ; i++) {
    int j;
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      MPI_NMAD_TRACE("Copy element %d, %d from %p (+ %d) to %p (+ %d)\n",
		     i, j, recvptr, (int)(recvptr-recvbuffer), ptr, (int)(ptr-buffer));
      memcpy(ptr, recvptr, mpir_datatype->block_size);
      recvptr += mpir_datatype->block_size;
      ptr += mpir_datatype->block_size;
    }
  }
  return MPI_SUCCESS;
}

/**
 * Aggregates data represented by a indexed datatype in a contiguous
 * buffer.
 */
static inline int mpir_datatype_indexed_aggregate(void *newptr,
						  void *buffer,
						  mpir_datatype_t *mpir_datatype,
						  int count) {
#ifdef NMAD_DEBUG
  void *const dest = newptr;
#endif
  int i, j;
  for(i=0 ; i<count ; i++) {
    void *ptr = buffer + i * mpir_datatype->extent;
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      void *subptr = ptr + mpir_datatype->indices[j];
      MPI_NMAD_TRACE("Copy element %d, %d (size %ld) from %p (+%d) to %p (+%d)\n", i, j,
                     (long) mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0], subptr, (int)(subptr-buffer),
                     newptr, (int)(newptr-dest));
      memcpy(newptr, subptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0]);
      newptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0];
    }
  }
  return MPI_SUCCESS;
}

/**
 * Packs into a NM connection data represented by a indexed datatype.
 */
static inline int mpir_datatype_indexed_pack(struct nm_so_cnx *connection,
					     void *buffer,
					     mpir_datatype_t *mpir_datatype,
					     int count) {
  int i, j, err;
  for(i=0 ; i<count ; i++) {
    void *ptr = buffer + i * mpir_datatype->extent;
    MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*mpir_datatype->extent);
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      void *subptr = ptr + mpir_datatype->indices[j];
      MPI_NMAD_TRACE("Sub-element %d,%d starts at %p (%p + %ld) with size %ld\n", i, j, subptr, ptr,
                     (long) mpir_datatype->indices[j], (long) mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0]);
      err = nm_so_pack(connection, subptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0]);
    }
  }
  return err;
}

/**
 * Unpacks from a NM connection data represented by a indexed datatype.
 */
static inline int mpir_datatype_indexed_unpack(struct nm_so_cnx *connection,
					       mpir_request_t *mpir_request,
					       void *buffer,
					       mpir_datatype_t *mpir_datatype,
					       int count) {
  int i, k=0, err;
  mpir_request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(void *));
  mpir_request->request_ptr[0] = buffer;
  for(i=0 ; i<count ; i++) {
    int j;
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      MPI_NMAD_TRACE("Sub-element %d,%d unpacked at %p (%p + %d) with size %ld\n", i, j,
                     mpir_request->request_ptr[k], buffer, (int)(mpir_request->request_ptr[k]-buffer),
                     (long)mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0]);
      err = nm_so_unpack(connection, mpir_request->request_ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0]);
      k++;
      mpir_request->request_ptr[k] = mpir_request->request_ptr[k-1] + mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0];
    }
  }
  if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
  return err;
}

/**
 * Splits data from a contiguous buffer into data represented by a indexed
 * datatype
 */
static inline int mpir_datatype_indexed_split(void *recvbuffer,
					      void *buffer,
					      mpir_datatype_t *mpir_datatype,
					      int count) {
  void *recvptr = recvbuffer;
  void *ptr = buffer;
  int	i;
  for(i=0 ; i<count ; i++) {
    int j;
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      MPI_NMAD_TRACE("Copy element %d, %d (size %ld) from %p (+%d) to %p (+%d)\n", i, j,
                     (long int)mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0],
                     recvptr, (int)(recvptr-recvbuffer), ptr, (int)(ptr-buffer));
      memcpy(ptr, recvptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0]);
      recvptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0];
      ptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0];
    }
  }
  return MPI_SUCCESS;
}

/**
 * Aggregates data represented by a struct datatype in a contiguous
 * buffer.
 */
static inline int mpir_datatype_struct_aggregate(void *newptr,
						 void *buffer,
						 mpir_datatype_t *mpir_datatype,
						 int count) {
  int i, j;
  for(i=0 ; i<count ; i++) {
    void *ptr = buffer + i * mpir_datatype->extent;
    MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*mpir_datatype->extent);
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      ptr += mpir_datatype->indices[j];
      MPI_NMAD_TRACE("copying to %p data from %p (+%ld) with a size %d*%ld\n", newptr, ptr, (long)mpir_datatype->indices[j], mpir_datatype->blocklens[j], (long)mpir_datatype->old_sizes[j]);
      memcpy(newptr, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
      newptr += mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j];
      ptr -= mpir_datatype->indices[j];
    }
  }
  return MPI_SUCCESS;
}

/**
 * Packs into a NM connection data represented by a struct datatype.
 */
static inline int mpir_datatype_struct_pack(struct nm_so_cnx *connection,
					    void *buffer,
					    mpir_datatype_t *mpir_datatype,
					    int count) {
  int i, j, err;
  for(i=0 ; i<count ; i++) {
    void *ptr = buffer + i * mpir_datatype->extent;
    MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*mpir_datatype->extent);
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      ptr += mpir_datatype->indices[j];
      MPI_NMAD_TRACE("packing data at %p (+%ld) with a size %d*%ld\n", ptr, (long)mpir_datatype->indices[j], mpir_datatype->blocklens[j], (long)mpir_datatype->old_sizes[j]);
      err = nm_so_pack(connection, ptr, mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
      ptr -= mpir_datatype->indices[j];
    }
  }
  return err;
}

/**
 * Unpacks from a NM connection data represented by a struct datatype.
 */
static inline int mpir_datatype_struct_unpack(struct nm_so_cnx *connection,
					      mpir_request_t *mpir_request,
					      void *buffer,
					      mpir_datatype_t *mpir_datatype,
					      int count) {
  int i, k=0, err;
  mpir_request->request_ptr = malloc((count*mpir_datatype->elements+1) * sizeof(void *));
  for(i=0 ; i<count ; i++) {
    int j;
    mpir_request->request_ptr[k] = buffer + i*mpir_datatype->extent;
    for(j=0 ; j<mpir_datatype->elements ; j++) {
      mpir_request->request_ptr[k] += mpir_datatype->indices[j];
      err = nm_so_unpack(connection, mpir_request->request_ptr[k], mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[j]);
      k++;
      mpir_request->request_ptr[k] = mpir_request->request_ptr[k-1] - mpir_datatype->indices[j];
    }
  }
  if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
  return err;
}

/**
 * Splits data from a contiguous buffer into data represented by a
 * struct datatype.
 */
static inline int mpir_datatype_struct_split(void *recvbuffer,
					     void *buffer,
					     mpir_datatype_t *mpir_datatype,
					     int count) {
  void *recvptr = recvbuffer;
  int i;
  for(i=0 ; i<count ; i++) {
    void *ptr = buffer + i*mpir_datatype->extent;
    int j;
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
  return MPI_SUCCESS;
}

/**
 * Checks the current sending NM sequence and triggers flushing of
 * previous outgoing requests when needed.
 */
static inline int mpir_check_send_seq(long gate_id,
                                      uint8_t tag) {
  int seq = nm_so_sr_get_current_send_seq(p_so_sr_if, gate_id, tag);
  if (seq == NM_SO_PENDING_PACKS_WINDOW-1) {
    int err = nm_so_sr_stest_range(p_so_sr_if, gate_id, tag, seq-1, 1);
    if (err == -NM_EAGAIN) {
      MPI_NMAD_TRACE("Reaching maximum sequence number in emission. Trigger automatic flushing");
      nm_so_sr_swait_range(p_so_sr_if, gate_id, tag, 0, seq-1);
      MPI_NMAD_TRACE("Automatic flushing over");
    }
  }
  return seq;
}

/**
 * Checks the current receiving NM sequence and triggers flushing of
 * previous incoming requests when needed.
 */
static inline int mpir_check_recv_seq(long gate_id,
                                      uint8_t tag) {
  int seq = nm_so_sr_get_current_recv_seq(p_so_sr_if, gate_id, tag);
  int err;

  if (seq == NM_SO_PENDING_PACKS_WINDOW-1) {
    err = nm_so_sr_rtest_range(p_so_sr_if, gate_id, tag, seq-1, 1);
    if (err == -NM_EAGAIN) {
      MPI_NMAD_TRACE("Reaching maximum sequence number in reception. Trigger automatic flushing");
      nm_so_sr_rwait_range(p_so_sr_if, gate_id, tag, 0, seq-1);
      MPI_NMAD_TRACE("Automatic flushing over");
    }
  }
  return seq;
}

/**
 * Calls the appropriate sending request based on the given request.
 */
static inline int mpir_isend_wrapper(mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  void *buffer;
  int err;

  if (mpir_datatype->is_contig == 1) {
    buffer = mpir_request->buffer;
  }
  else {
    buffer =  mpir_request->contig_buffer;
  }

  MPI_NMAD_TRACE("Sending data\n");
  MPI_NMAD_TRANSFER("Sent --> gate %ld : %ld bytes\n", mpir_request->gate_id, (long)mpir_request->count * mpir_datatype->size);

  if (mpir_request->communication_mode == MPI_IMMEDIATE_MODE) {
    err = nm_so_sr_isend(p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer,
                         mpir_request->count * mpir_datatype->size, &(mpir_request->request_nmad));
  }
  else if (mpir_request->communication_mode == MPI_READY_MODE) {
    err = nm_so_sr_rsend(p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer,
                         mpir_request->count * mpir_datatype->size, &(mpir_request->request_nmad));
  }
  else {
    err = nm_so_sr_isend_extended(p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer,
                                  mpir_request->count * mpir_datatype->size, mpir_request->communication_mode,
                                  &(mpir_request->request_nmad));
  }

  MPI_NMAD_TRANSFER("Sent finished\n");
  return err;
}

/**
 * Calls the appropriate pack function based on the given request.
 */
static inline int mpir_pack_wrapper(mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  struct nm_so_cnx *connection = &(mpir_request->request_cnx);
  int err;

  MPI_NMAD_TRACE("Packing data\n");
  nm_so_begin_packing(p_so_pack_if, mpir_request->gate_id, mpir_request->request_tag, connection);

  if (mpir_datatype->dte_type == MPIR_VECTOR) {
    err = mpir_datatype_vector_pack(connection, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED) {
    MPI_NMAD_TRACE("Sending (h)indexed type: count %d - size %ld - extent %ld\n", mpir_datatype->elements,
		   (long)mpir_datatype->size, (long)mpir_datatype->extent);
    err = mpir_datatype_indexed_pack(connection, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    MPI_NMAD_TRACE("Sending struct type: size %ld\n", (long)mpir_datatype->size);
    err = mpir_datatype_struct_pack(connection, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  return err;
}

int mpir_isend_init(mpir_request_t *mpir_request,
                    int dest,
                    mpir_communicator_t *mpir_communicator) {
  mpir_datatype_t      *mpir_datatype = NULL;
  int                   err = MPI_SUCCESS;

  if (tbx_unlikely(dest >= mpir_communicator->size || out_gate_id[mpir_communicator->global_ranks[dest]] == -1)) {
    NM_DISPF("Cannot find a connection between %d and %d, %d\n", mpir_communicator->rank, dest, mpir_communicator->global_ranks[dest]);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->gate_id = out_gate_id[mpir_communicator->global_ranks[dest]];
  mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_communicator, mpir_request->user_tag);

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid sending tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, mpir_request->user_tag, NM_SO_MAX_TAGS);
    return MPI_ERR_INTERN;
  }

  {
    int seq = mpir_check_send_seq(mpir_request->gate_id, mpir_request->request_tag);
    if (tbx_unlikely((mpir_request->communication_mode == MPI_IS_NOT_COMPLETED) && (seq == NM_SO_PENDING_PACKS_WINDOW-2))) {
      MPI_NMAD_TRACE("Reaching critical maximum sequence number in emission. Force completed mode\n");
      mpir_request->communication_mode = MPI_IS_COMPLETED;
    }
  }

  MPI_NMAD_TRACE("Sending to %d with tag %d (%d, %d)\n", dest, mpir_request->request_tag, mpir_communicator->communicator_id, mpir_request->user_tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Sending data of type %d at address %p with len %ld (%d*%ld)\n", mpir_request->request_datatype, mpir_request->buffer, (long)mpir_request->count*mpir_datatype->size, mpir_request->count, (long)mpir_datatype->size);
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR) {
    MPI_NMAD_TRACE("Sending (h)vector type: stride %d - blocklen %d - count %d - size %ld\n", mpir_datatype->stride, mpir_datatype->blocklens[0], mpir_datatype->elements, (long)mpir_datatype->size);
    if (!mpir_datatype->is_optimized) {
      MPI_NMAD_TRACE("Sending vector datatype in a contiguous buffer\n");

      mpir_request->contig_buffer = malloc(mpir_request->count * mpir_datatype->size);
      if (mpir_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to send struct datatype\n", (long)(mpir_request->count * mpir_datatype->size));
        return MPI_ERR_INTERN;
      }

      mpir_datatype_vector_aggregate(mpir_request->contig_buffer, mpir_request->buffer, mpir_datatype, mpir_request->count);
      MPI_NMAD_TRACE("Sending data of vector type at address %p with len %ld (%d*%d*%ld)\n", mpir_request->contig_buffer, (long)(mpir_request->count * mpir_datatype->size), mpir_request->count, mpir_datatype->elements, (long)mpir_datatype->block_size);
    }
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED) {
    if (!mpir_datatype->is_optimized) {
      MPI_NMAD_TRACE("Sending (h)indexed datatype in a contiguous buffer\n");

      mpir_request->contig_buffer = malloc(mpir_request->count * mpir_datatype->size);
      if (mpir_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to send (h)indexed type\n", (long)(mpir_request->count * mpir_datatype->size));
        return MPI_ERR_INTERN;
      }

      mpir_datatype_indexed_aggregate(mpir_request->contig_buffer, mpir_request->buffer, mpir_datatype, mpir_request->count);
      MPI_NMAD_TRACE("Sending data of (h)indexed type at address %p with len %ld\n", mpir_request->contig_buffer, (long)(mpir_request->count * mpir_datatype->size));
    }
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    if (!mpir_datatype->is_optimized) {
      MPI_NMAD_TRACE("Sending struct datatype in a contiguous buffer\n");

      mpir_request->contig_buffer = malloc(mpir_request->count * mpir_datatype->size);
      if (mpir_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to send struct datatype\n", (long)(mpir_request->count * mpir_datatype->size));
        return MPI_ERR_INTERN;
      }

      mpir_datatype_struct_aggregate(mpir_request->contig_buffer, mpir_request->buffer, mpir_datatype, mpir_request->count);
      MPI_NMAD_TRACE("Sending data of struct type at address %p with len %ld (%d*%ld)\n", mpir_request->contig_buffer, (long)(mpir_request->count * mpir_datatype->size), mpir_request->count, (long)mpir_datatype->size);
    }
  }
  else {
    ERROR("Do not know how to send datatype %d %d\n", mpir_request->request_datatype, mpir_datatype->dte_type);
    return MPI_ERR_INTERN;
  }

  return err;
}

int mpir_isend_start(mpir_request_t *mpir_request) {
  int err = MPI_SUCCESS;
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);

  mpir_datatype->active_communications ++;
  if (mpir_datatype->is_contig || !(mpir_datatype->is_optimized)) {
    err = mpir_isend_wrapper(mpir_request);
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_SEND;
  }
  else {
    err = mpir_pack_wrapper(mpir_request);
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_SEND;
  }

  err = nm_so_sr_progress(p_so_sr_if);
  mpir_inc_nb_outgoing_msg();

  return err;
}

int mpir_isend(mpir_request_t *mpir_request,
               int dest,
               mpir_communicator_t *mpir_communicator) {
  int err;

  err = mpir_isend_init(mpir_request, dest, mpir_communicator);
  if (err == MPI_SUCCESS) {
    err = mpir_isend_start(mpir_request);
  }
  return err;
}

int mpir_set_status(MPI_Request *request,
		    MPI_Status *status) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  int err = MPI_SUCCESS;

  status->MPI_TAG = mpir_request->user_tag;
  status->MPI_ERROR = mpir_request->request_error;

  status->count = mpir_sizeof_datatype(mpir_request->request_datatype);

  if (mpir_request->request_type == MPI_REQUEST_RECV ||
      mpir_request->request_type == MPI_REQUEST_PACK_RECV) {
    if (mpir_request->request_source == MPI_ANY_SOURCE) {
      long gate_id;
      err = nm_so_sr_recv_source(p_so_sr_if, mpir_request->request_nmad, &gate_id);
      status->MPI_SOURCE = in_dest[gate_id];
    }
    else {
      status->MPI_SOURCE = mpir_request->request_source;
    }
  }

  return err;
}

int mpir_irecv_init(mpir_request_t *mpir_request,
                    int source,
                    mpir_communicator_t *mpir_communicator) {
  mpir_datatype_t      *mpir_datatype = NULL;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(source == MPI_ANY_SOURCE)) {
    mpir_request->gate_id = NM_SO_ANY_SRC;
  }
  else {
    if (tbx_unlikely(source >= mpir_communicator->size || in_gate_id[mpir_communicator->global_ranks[source]] == -1)){
      NM_DISPF("Cannot find a connection between %d and %d, %d\n", mpir_communicator->rank, source, mpir_communicator->global_ranks[source]);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
    mpir_request->gate_id = in_gate_id[mpir_communicator->global_ranks[source]];
  }

  mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_communicator, mpir_request->user_tag);
  mpir_request->request_source = source;

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid receiving tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, mpir_request->user_tag, NM_SO_MAX_TAGS);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  {
    int seq;
    if (source != MPI_ANY_SOURCE) {
      seq = mpir_check_recv_seq(mpir_request->gate_id, mpir_request->request_tag);
    }
  }

  MPI_NMAD_TRACE("Receiving from %d at address %p with tag %d (%d, %d)\n", source, mpir_request->buffer, mpir_request->request_tag, mpir_communicator->communicator_id, mpir_request->user_tag);
  if (mpir_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Receiving data of type %d at address %p with len %ld (%d*%ld)\n", mpir_request->request_datatype, mpir_request->buffer, (long)mpir_request->count*mpir_datatype->size, mpir_request->count, (long)mpir_datatype->size);
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_RECV;
  }
  else if (mpir_datatype->dte_type == MPIR_VECTOR) {
    if (!mpir_datatype->is_optimized) {
      mpir_request->contig_buffer = malloc(mpir_request->count * mpir_datatype->size);
      if (mpir_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to receive vector type\n", (long)(mpir_request->count * mpir_datatype->size));
        return MPI_ERR_INTERN;
      }

      MPI_NMAD_TRACE("Receiving vector type %d in a contiguous way at address %p with len %ld (%d*%ld)\n", mpir_request->request_datatype, mpir_request->contig_buffer, (long)(mpir_request->count * mpir_datatype->size), mpir_request->count, (long)mpir_datatype->size);
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_RECV;
    }
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED) {
    if (!mpir_datatype->is_optimized) {
      MPI_NMAD_TRACE("Receiving (h)indexed datatype in a contiguous buffer\n");
      mpir_request->contig_buffer = malloc(mpir_request->count * mpir_datatype->size);
      if (mpir_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to receive (h)indexed type\n", (long)(mpir_request->count * mpir_datatype->size));
        return MPI_ERR_INTERN;
      }

      MPI_NMAD_TRACE("Receiving (h)indexed type %d in a contiguous way at address %p with len %ld\n", mpir_request->request_datatype, mpir_request->contig_buffer, (long)(mpir_request->count * mpir_datatype->size));
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_RECV;
    }
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    if (!mpir_datatype->is_optimized) {
      mpir_request->contig_buffer = malloc(mpir_request->count * mpir_datatype->size);
      if (mpir_request->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to receive struct type\n", (long)(mpir_request->count * mpir_datatype->size));
        return MPI_ERR_INTERN;
      }

      MPI_NMAD_TRACE("Receiving struct type %d in a contiguous way at address %p with len %ld (%d*%ld)\n", mpir_request->request_datatype, mpir_request->contig_buffer, (long)mpir_request->count*mpir_datatype->size, mpir_request->count, (long)mpir_datatype->size);
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_RECV;
    }
  }
  else {
    ERROR("Do not know how to receive datatype %d\n", mpir_request->request_datatype);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * Calls the appropriate receiving request based on the given request.
 */
static inline int mpir_irecv_wrapper(mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  void *buffer;
  int err;

  if (mpir_datatype->is_contig == 1) {
    buffer = mpir_request->buffer;
  }
  else {
    buffer =  mpir_request->contig_buffer;
  }

  MPI_NMAD_TRACE("Posting recv request\n");
  MPI_NMAD_TRANSFER("Recv --< gate %ld: %ld bytes\n", mpir_request->gate_id, (long)mpir_request->count * mpir_datatype->size);
  err = nm_so_sr_irecv(p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer, mpir_request->count * mpir_datatype->size, &(mpir_request->request_nmad));
  MPI_NMAD_TRANSFER("Recv finished, request = %p\n", &(mpir_request->request_nmad));

  return err;
}

/**
 * Calls the appropriate unpack functions based on the given request.
 */
static inline int mpir_unpack_wrapper(mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  struct nm_so_cnx *connection = &(mpir_request->request_cnx);
  int err;

  MPI_NMAD_TRACE("Unpacking data\n");
  nm_so_begin_unpacking(p_so_pack_if, mpir_request->gate_id, mpir_request->request_tag, connection);

  if (mpir_datatype->dte_type == MPIR_VECTOR) {
    MPI_NMAD_TRACE("Receiving vector type: stride %d - blocklen %d - count %d - size %ld\n", mpir_datatype->stride, mpir_datatype->blocklens[0], mpir_datatype->elements, (long)mpir_datatype->size);
    err = mpir_datatype_vector_unpack(connection, mpir_request, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED) {
    MPI_NMAD_TRACE("Receiving (h)indexed type: count %d - size %ld\n", mpir_datatype->elements, (long)mpir_datatype->size);
    err = mpir_datatype_indexed_unpack(connection, mpir_request, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    MPI_NMAD_TRACE("Receiving struct type: size %ld\n", (long)mpir_datatype->size);
    err = mpir_datatype_struct_unpack(connection, mpir_request, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  return err;
}

int mpir_irecv_start(mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);

  mpir_datatype->active_communications ++;

  if (mpir_datatype->is_contig || !(mpir_datatype->is_optimized)) {
    mpir_request->request_error = mpir_irecv_wrapper(mpir_request);

    if (mpir_datatype->is_contig) {
      MPI_NMAD_TRACE("Calling irecv_start for contiguous data\n");
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_RECV;
    }
  }
  else {
    mpir_request->request_error = mpir_unpack_wrapper(mpir_request);
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
  }

  nm_so_sr_progress(p_so_sr_if);

  mpir_inc_nb_incoming_msg();
  MPI_NMAD_TRACE("Irecv_start completed\n");
  MPI_NMAD_LOG_OUT();
  return mpir_request->request_error;
}

int mpir_irecv(mpir_request_t *mpir_request,
               int dest,
               mpir_communicator_t *mpir_communicator) {
  int err;

  err = mpir_irecv_init(mpir_request, dest, mpir_communicator);
  if (err == MPI_SUCCESS) {
    err = mpir_irecv_start(mpir_request);
  }
  return err;
}

int mpir_datatype_split(mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_request->request_datatype);
  int err = MPI_SUCCESS;

  if (mpir_datatype->dte_type == MPIR_VECTOR) {
    err = mpir_datatype_vector_split(mpir_request->contig_buffer, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  else if (mpir_datatype->dte_type == MPIR_INDEXED) {
    err = mpir_datatype_indexed_split(mpir_request->contig_buffer, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }
  else if (mpir_datatype->dte_type == MPIR_STRUCT) {
    err = mpir_datatype_struct_split(mpir_request->contig_buffer, mpir_request->buffer, mpir_datatype, mpir_request->count);
  }

  if (mpir_request->request_persistent_type == MPI_REQUEST_ZERO) {
    FREE_AND_SET_NULL(mpir_request->contig_buffer);
    mpir_request->request_type = MPI_REQUEST_ZERO;
  }
  return err;
}

int mpir_start(mpir_request_t *mpir_request) {
  if (mpir_request->request_persistent_type == MPI_REQUEST_SEND) {
    return mpir_isend_start(mpir_request);
  }
  else if (mpir_request->request_persistent_type == MPI_REQUEST_RECV) {
    return mpir_irecv_start(mpir_request);
  }
  else {
    ERROR("Unknown persistent request type: %d\n", mpir_request->request_persistent_type);
    return MPI_ERR_INTERN;
  }
}

/**
 * Gets the id of the next available datatype.
 */
static int get_available_datatype() {
  if (tbx_slist_is_nil(available_datatypes) == tbx_true) {
    ERROR("Maximum number of datatypes created");
    return MPI_ERR_INTERN;
  }
  else {
    int datatype;
    int *ptr = tbx_slist_extract(available_datatypes);
    datatype = *ptr;
    FREE_AND_SET_NULL(ptr);
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

int mpir_type_size(MPI_Datatype datatype,
		   int *size) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);
  *size = mpir_datatype->size;
  return MPI_SUCCESS;
}

int mpir_type_get_lb_and_extent(MPI_Datatype datatype,
				MPI_Aint *lb,
				MPI_Aint *extent) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);
  if (lb) {
    *lb = mpir_datatype->lb;
  }
  if (extent) {
    *extent = mpir_datatype->extent;
  }
  return MPI_SUCCESS;
}

int mpir_type_create_resized(MPI_Datatype oldtype,
			     MPI_Aint lb,
			     MPI_Aint extent,
			     MPI_Datatype *newtype) {
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
  datatypes[*newtype]->lb        = lb;

  if(datatypes[*newtype]->dte_type == MPIR_CONTIG) {
    datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    datatypes[*newtype]->old_types[0] = oldtype;
    datatypes[*newtype]->elements     = mpir_old_datatype->elements;
  }
  else if (datatypes[*newtype]->dte_type == MPIR_VECTOR) {
    datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    datatypes[*newtype]->old_types[0] = oldtype;
    datatypes[*newtype]->elements     = mpir_old_datatype->elements;
    datatypes[*newtype]->blocklens    = malloc(1 * sizeof(int));
    datatypes[*newtype]->blocklens[0] = mpir_old_datatype->blocklens[0];
    datatypes[*newtype]->block_size   = mpir_old_datatype->block_size;
    datatypes[*newtype]->stride       = mpir_old_datatype->stride;
  }
  else if (datatypes[*newtype]->dte_type == MPIR_INDEXED) {
    datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    datatypes[*newtype]->old_types[0] = oldtype;
    datatypes[*newtype]->elements     = mpir_old_datatype->elements;
    datatypes[*newtype]->blocklens    = malloc(datatypes[*newtype]->elements * sizeof(int));
    datatypes[*newtype]->indices      = malloc(datatypes[*newtype]->elements * sizeof(MPI_Aint));
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
    datatypes[*newtype]->old_types = malloc(datatypes[*newtype]->elements * sizeof(MPI_Datatype));
    for(i=0 ; i<datatypes[*newtype]->elements ; i++) {
      datatypes[*newtype]->blocklens[i] = mpir_old_datatype->blocklens[i];
      datatypes[*newtype]->indices[i]   = mpir_old_datatype->indices[i];
      datatypes[*newtype]->old_sizes[i] = mpir_old_datatype->old_sizes[i];
      datatypes[*newtype]->old_types[i] = mpir_old_datatype->old_types[i];
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

  MPI_NMAD_TRACE_LEVEL(3, "Unlocking datatype %d (%d)\n", datatype, mpir_datatype->active_communications);
  mpir_datatype->active_communications --;
  if (mpir_datatype->active_communications == 0 && mpir_datatype->free_requested == 1) {
    MPI_NMAD_TRACE("Freeing datatype %d\n", datatype);
    mpir_type_free(datatype);
  }
  return MPI_SUCCESS;
}

int mpir_type_free(MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);

  if (mpir_datatype->active_communications != 0) {
    mpir_datatype->free_requested = 1;
    MPI_NMAD_TRACE_LEVEL(3, "Datatype %d still in use (%d). Cannot be released\n", datatype, mpir_datatype->active_communications);
    return MPI_DATATYPE_ACTIVE;
  }
  else {
    MPI_NMAD_TRACE_LEVEL(3, "Releasing datatype %d\n", datatype);
    FREE_AND_SET_NULL(datatypes[datatype]->old_sizes);
    FREE_AND_SET_NULL(datatypes[datatype]->old_types);
    if (datatypes[datatype]->dte_type == MPIR_INDEXED || datatypes[datatype]->dte_type == MPIR_STRUCT) {
      FREE_AND_SET_NULL(datatypes[datatype]->blocklens);
      FREE_AND_SET_NULL(datatypes[datatype]->indices);
    }
    if (datatypes[datatype]->dte_type == MPIR_VECTOR) {
      FREE_AND_SET_NULL(datatypes[datatype]->blocklens);
    }
    int *ptr;
    ptr = malloc(sizeof(int));
    *ptr = datatype;
    tbx_slist_enqueue(available_datatypes, ptr);

    FREE_AND_SET_NULL(datatypes[datatype]);
    return MPI_SUCCESS;
  }
}

int mpir_type_optimized(MPI_Datatype datatype,
			int optimized) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(datatype);
  mpir_datatype->is_optimized = optimized;
  return MPI_SUCCESS;
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
  datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  datatypes[*newtype]->old_types[0] = oldtype;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 1;
  datatypes[*newtype]->size = datatypes[*newtype]->old_sizes[0] * count;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->lb = 0;
  datatypes[*newtype]->extent = datatypes[*newtype]->old_sizes[0] * count;

  MPI_NMAD_TRACE_LEVEL(3, "Creating new contiguous type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		       (long)datatypes[*newtype]->size, (long)datatypes[*newtype]->extent, oldtype, (long)datatypes[*newtype]->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpir_type_vector(int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  mpir_datatype_t *mpir_old_datatype;

  mpir_old_datatype = mpir_get_datatype(oldtype);
  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = MPIR_VECTOR;
  datatypes[*newtype]->basic = 0;
  datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  datatypes[*newtype]->old_types[0] = oldtype;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->size = datatypes[*newtype]->old_sizes[0] * count * blocklength;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->blocklens = malloc(1 * sizeof(int));
  datatypes[*newtype]->blocklens[0] = blocklength;
  datatypes[*newtype]->block_size = blocklength * datatypes[*newtype]->old_sizes[0];
  datatypes[*newtype]->stride = stride;
  datatypes[*newtype]->lb = 0;
  datatypes[*newtype]->extent = datatypes[*newtype]->old_sizes[0] * count * blocklength;

  MPI_NMAD_TRACE_LEVEL(3, "Creating new (h)vector type (%d) with size=%ld, extent=%ld, elements=%d, blocklen=%d based on type %d with a extent %ld\n",
                       *newtype, (long)datatypes[*newtype]->size, (long)datatypes[*newtype]->extent,
                       datatypes[*newtype]->elements, datatypes[*newtype]->blocklens[0],
                       oldtype, (long)datatypes[*newtype]->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpir_type_indexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  int i;
  mpir_datatype_t *mpir_old_datatype;

  MPI_NMAD_TRACE("Creating indexed derived datatype based on %d elements\n", count);
  mpir_old_datatype = mpir_get_datatype(oldtype);

  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = MPIR_INDEXED;
  datatypes[*newtype]->basic = 0;
  datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  datatypes[*newtype]->old_types[0] = oldtype;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->lb = 0;
  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->indices[i] = array_of_displacements[i];
    MPI_NMAD_TRACE("Element %d: length %d, indice %ld\n", i, datatypes[*newtype]->blocklens[i], (long)datatypes[*newtype]->indices[i]);
  }
  datatypes[*newtype]->size = (datatypes[*newtype]->indices[count-1] + datatypes[*newtype]->old_sizes[0] * datatypes[*newtype]->blocklens[count-1]);
  datatypes[*newtype]->extent = (datatypes[*newtype]->indices[count-1] + mpir_old_datatype->extent * datatypes[*newtype]->blocklens[count-1]);

  MPI_NMAD_TRACE_LEVEL(3, "Creating new index type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		       (long)datatypes[*newtype]->size, (long)datatypes[*newtype]->extent, oldtype, (long)datatypes[*newtype]->old_sizes[0]);
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
  datatypes[*newtype]->lb = 0;

  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  datatypes[*newtype]->old_sizes = malloc(count * sizeof(size_t));
  datatypes[*newtype]->old_types = malloc(count * sizeof(MPI_Datatype));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->old_sizes[i] = mpir_get_datatype(array_of_types[i])->size;
    datatypes[*newtype]->old_types[i] = array_of_types[i];
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
    FREE_AND_SET_NULL(ptr);

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
    FREE_AND_SET_NULL(functions[*op]);
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
    FREE_AND_SET_NULL(ptr);

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

int mpir_project_comm_and_tag(mpir_communicator_t *mpir_communicator,
			      int tag) {
  /*
   * NewMadeleine only allows us 7 bits!
   * We suppose that comm is represented on 3 bits and tag on 4 bits.
   * We stick both of them into a new 7-bits representation
   */
  tag += MAX_INTERNAL_TAG;
  int newtag = (mpir_communicator->communicator_id-MPI_COMM_WORLD) << 4;
  newtag += tag;
  return newtag;
}

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
