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
#include "mpi.h"
#include "mpi_nmad_private.h"
#include "nm_so_parameters.h"
#include "nm_log.h"

#if defined(NMAD_DEBUG)
debug_type_t debug_mpi_nmad_trace=NEW_DEBUG_TYPE("MPI_NMAD: ", "mpi_nmad_trace");
debug_type_t debug_mpi_nmad_transfer=NEW_DEBUG_TYPE("MPI_NMAD_TRANSFER: ", "mpi_nmad_transfer");
debug_type_t debug_mpi_nmad_log=NEW_DEBUG_TYPE("MPI_NMAD_LOG: ", "mpi_nmad_log");
#endif /* NMAD_DEBUG */

int mpir_internal_init(mpir_internal_data_t *mpir_internal_data,
		       struct puk_receptacle_NewMad_Launcher_s*r) {
  int i;
  int dest;

  const int global_size  = (*r->driver->get_size)(r->_status);
  const int process_rank = (*r->driver->get_rank)(r->_status);
 
  mpir_internal_data->nb_outgoing_msg = 0;
  mpir_internal_data->nb_incoming_msg = 0;

  /** Set the NewMadeleine interfaces */
  mpir_internal_data->p_so_sr_if   = (*r->driver->get_so_sr_if)(r->_status);
  mpir_internal_data->p_so_pack_if = (*r->driver->get_so_pack_if)(r->_status);

  /** Initialise the basic datatypes */
  mpir_internal_data->datatypes = malloc((NUMBER_OF_DATATYPES+1) * sizeof(mpir_datatype_t *));

  for(i=MPI_DATATYPE_NULL ; i<=MPI_PACKED ; i++) {
    mpir_internal_data->datatypes[i] = malloc(sizeof(mpir_datatype_t));
  }
  for(i=MPI_DATATYPE_NULL ; i<=MPI_PACKED ; i++) {
    mpir_internal_data->datatypes[i]->basic = 1;
    mpir_internal_data->datatypes[i]->committed = 1;
    mpir_internal_data->datatypes[i]->is_contig = 1;
    mpir_internal_data->datatypes[i]->dte_type = MPIR_BASIC;
    mpir_internal_data->datatypes[i]->active_communications = 100;
    mpir_internal_data->datatypes[i]->free_requested = 0;
    mpir_internal_data->datatypes[i]->lb = 0;
  }
  mpir_internal_data->datatypes[MPI_DATATYPE_NULL]->size = 0;
  mpir_internal_data->datatypes[MPI_CHAR]->size = sizeof(signed char);
  mpir_internal_data->datatypes[MPI_UNSIGNED_CHAR]->size = sizeof(unsigned char);
  mpir_internal_data->datatypes[MPI_BYTE]->size = 1;
  mpir_internal_data->datatypes[MPI_SHORT]->size = sizeof(signed short);
  mpir_internal_data->datatypes[MPI_UNSIGNED_SHORT]->size = sizeof(unsigned short);
  mpir_internal_data->datatypes[MPI_INT]->size = sizeof(signed int);
  mpir_internal_data->datatypes[MPI_UNSIGNED]->size = sizeof(unsigned int);
  mpir_internal_data->datatypes[MPI_LONG]->size = sizeof(signed long);
  mpir_internal_data->datatypes[MPI_UNSIGNED_LONG]->size = sizeof(unsigned long);
  mpir_internal_data->datatypes[MPI_FLOAT]->size = sizeof(float);
  mpir_internal_data->datatypes[MPI_DOUBLE]->size = sizeof(double);
  mpir_internal_data->datatypes[MPI_LONG_DOUBLE]->size = sizeof(long double);
  mpir_internal_data->datatypes[MPI_LONG_LONG_INT]->size = sizeof(long long int);
  mpir_internal_data->datatypes[MPI_LONG_LONG]->size = sizeof(long long);
  mpir_internal_data->datatypes[MPI_COMPLEX]->size = 2*sizeof(float);
  mpir_internal_data->datatypes[MPI_DOUBLE_COMPLEX]->size = 2*sizeof(double);
  mpir_internal_data->datatypes[MPI_LOGICAL]->size = sizeof(float);
  mpir_internal_data->datatypes[MPI_REAL]->size = sizeof(float);
  mpir_internal_data->datatypes[MPI_REAL4]->size = 4*sizeof(char);
  mpir_internal_data->datatypes[MPI_REAL8]->size = 8*sizeof(char);
  mpir_internal_data->datatypes[MPI_DOUBLE_PRECISION]->size = sizeof(double);
  mpir_internal_data->datatypes[MPI_INTEGER]->size = sizeof(float);
  mpir_internal_data->datatypes[MPI_PACKED]->size = sizeof(char);

  for(i=MPI_DATATYPE_NULL ; i<=MPI_PACKED ; i++) {
    mpir_internal_data->datatypes[i]->extent = mpir_internal_data->datatypes[i]->size;
  }

  mpir_internal_data->available_datatypes = tbx_slist_nil();
  for(i=MPI_PACKED+1 ; i<=NUMBER_OF_DATATYPES ; i++) {
    int *ptr;
    ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(mpir_internal_data->available_datatypes, ptr);
  }

  /** Initialise data for communicators */
  mpir_internal_data->communicators = malloc(NUMBER_OF_COMMUNICATORS * sizeof(mpir_communicator_t));
  mpir_internal_data->communicators[0] = malloc(sizeof(mpir_communicator_t));
  mpir_internal_data->communicators[0]->communicator_id = MPI_COMM_WORLD;
  mpir_internal_data->communicators[0]->size = global_size;
  mpir_internal_data->communicators[0]->rank = process_rank;
  mpir_internal_data->communicators[0]->global_ranks = malloc(global_size * sizeof(int));
  for(i=0 ; i<global_size ; i++) {
    mpir_internal_data->communicators[0]->global_ranks[i] = i;
  }

  mpir_internal_data->communicators[1] = malloc(sizeof(mpir_communicator_t));
  mpir_internal_data->communicators[1]->communicator_id = MPI_COMM_SELF;
  mpir_internal_data->communicators[1]->size = 1;
  mpir_internal_data->communicators[1]->rank = process_rank;
  mpir_internal_data->communicators[1]->global_ranks = malloc(1 * sizeof(int));
  mpir_internal_data->communicators[1]->global_ranks[0] = process_rank;

  mpir_internal_data->available_communicators = tbx_slist_nil();
  for(i=MPI_COMM_SELF+1 ; i<=NUMBER_OF_COMMUNICATORS ; i++) {
    int *ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(mpir_internal_data->available_communicators, ptr);
  }

  /** Initialise the collective operators */
  mpir_internal_data->operators = malloc((NUMBER_OF_OPERATORS+1) * sizeof(mpir_operator_t));
  for(i=MPI_MAX ; i<=MPI_MAXLOC ; i++) {
    mpir_internal_data->operators[i] = malloc(sizeof(mpir_operator_t));
    mpir_internal_data->operators[i]->commute = 1;
  }
  mpir_internal_data->operators[MPI_MAX]->function = &mpir_op_max;
  mpir_internal_data->operators[MPI_MIN]->function = &mpir_op_min;
  mpir_internal_data->operators[MPI_SUM]->function = &mpir_op_sum;
  mpir_internal_data->operators[MPI_PROD]->function = &mpir_op_prod;
  mpir_internal_data->operators[MPI_LAND]->function = &mpir_op_land;
  mpir_internal_data->operators[MPI_BAND]->function = &mpir_op_band;
  mpir_internal_data->operators[MPI_LOR]->function = &mpir_op_lor;
  mpir_internal_data->operators[MPI_BOR]->function = &mpir_op_bor;
  mpir_internal_data->operators[MPI_LXOR]->function = &mpir_op_lxor;
  mpir_internal_data->operators[MPI_BXOR]->function = &mpir_op_bxor;
  mpir_internal_data->operators[MPI_MINLOC]->function = &mpir_op_minloc;
  mpir_internal_data->operators[MPI_MAXLOC]->function = &mpir_op_maxloc;

  mpir_internal_data->available_operators = tbx_slist_nil();
  for(i=1 ; i<MPI_MAX ; i++) {
    int *ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(mpir_internal_data->available_operators, ptr);
  }

  /** Store the gate id of all the other processes */
  mpir_internal_data->out_gate_id = malloc(global_size * sizeof(nm_gate_id_t));
  mpir_internal_data->in_gate_id = malloc(global_size * sizeof(nm_gate_id_t));
  mpir_internal_data->out_dest = malloc(NUMBER_OF_GATES * sizeof(int));
  mpir_internal_data->in_dest = malloc(NUMBER_OF_GATES * sizeof(int));

  (*r->driver->get_gate_ids)(r->_status, &mpir_internal_data->out_gate_id[0]);
  for(dest = 0; dest < global_size; dest++)
    {
      const nm_gate_id_t gate_id = mpir_internal_data->out_gate_id[dest];
      mpir_internal_data->in_gate_id[dest] = gate_id;
      if(gate_id >= 0)
	{
	  mpir_internal_data->out_dest[gate_id] = dest;
	  mpir_internal_data->in_dest[gate_id] = dest;
	}
    }

  return MPI_SUCCESS;
}

int mpir_internal_exit(mpir_internal_data_t *mpir_internal_data) {
  int i;

  for(i=0 ; i<=MPI_PACKED ; i++) {
    FREE_AND_SET_NULL(mpir_internal_data->datatypes[i]);
  }
  FREE_AND_SET_NULL(mpir_internal_data->datatypes);
  while (tbx_slist_is_nil(mpir_internal_data->available_datatypes) == tbx_false) {
    int *ptr = tbx_slist_extract(mpir_internal_data->available_datatypes);
    FREE_AND_SET_NULL(ptr);
  }
  tbx_slist_clear(mpir_internal_data->available_datatypes);
  tbx_slist_free(mpir_internal_data->available_datatypes);

  FREE_AND_SET_NULL(mpir_internal_data->communicators[0]->global_ranks);
  FREE_AND_SET_NULL(mpir_internal_data->communicators[0]);
  FREE_AND_SET_NULL(mpir_internal_data->communicators[1]->global_ranks);
  FREE_AND_SET_NULL(mpir_internal_data->communicators[1]);
  FREE_AND_SET_NULL(mpir_internal_data->communicators);
  while (tbx_slist_is_nil(mpir_internal_data->available_communicators) == tbx_false) {
    int *ptr = tbx_slist_extract(mpir_internal_data->available_communicators);
    FREE_AND_SET_NULL(ptr);
  }
  tbx_slist_clear(mpir_internal_data->available_communicators);
  tbx_slist_free(mpir_internal_data->available_communicators);

  for(i=MPI_MAX ; i<=MPI_MAXLOC ; i++) {
    FREE_AND_SET_NULL(mpir_internal_data->operators[i]);
  }
  FREE_AND_SET_NULL(mpir_internal_data->operators);
  while (tbx_slist_is_nil(mpir_internal_data->available_operators) == tbx_false) {
    int *ptr = tbx_slist_extract(mpir_internal_data->available_operators);
    FREE_AND_SET_NULL(ptr);
  }
  tbx_slist_clear(mpir_internal_data->available_operators);
  tbx_slist_free(mpir_internal_data->available_operators);

  FREE_AND_SET_NULL(mpir_internal_data->out_gate_id);
  FREE_AND_SET_NULL(mpir_internal_data->in_gate_id);
  FREE_AND_SET_NULL(mpir_internal_data->out_dest);
  FREE_AND_SET_NULL(mpir_internal_data->in_dest);

  return MPI_SUCCESS;
}

nm_gate_id_t mpir_get_in_gate_id(mpir_internal_data_t *mpir_internal_data,
				 int node) {
  return mpir_internal_data->in_gate_id[node];
}

nm_gate_id_t mpir_get_out_gate_id(mpir_internal_data_t *mpir_internal_data,
				  int node) {
  return mpir_internal_data->out_gate_id[node];
}

int mpir_get_in_dest(mpir_internal_data_t *mpir_internal_data,
		     nm_gate_id_t gate) {
  return mpir_internal_data->in_dest[gate];
}

int mpir_get_out_dest(mpir_internal_data_t *mpir_internal_data,
		      nm_gate_id_t gate) {
  return mpir_internal_data->out_dest[gate];
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
  int               i, j, err = MPI_SUCCESS;
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
  int i, k=0, err = MPI_SUCCESS;
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
      MPI_NMAD_TRACE("Copy element %d, %d (size %ld = %d * %ld) from %p (+%d) to %p (+%d)\n", i, j,
                     (long) mpir_datatype->blocklens[j] * mpir_datatype->old_sizes[0],
                     mpir_datatype->blocklens[j], (long) mpir_datatype->old_sizes[0],
                     subptr, (int)(subptr-buffer), newptr, (int)(newptr-dest));
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
  int i, j, err = MPI_SUCCESS;
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
  int i, k=0, err = MPI_SUCCESS;
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
  int i, j, err = MPI_SUCCESS;
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
  int i, k=0, err = MPI_SUCCESS;
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
static inline int mpir_check_send_seq(mpir_internal_data_t *mpir_internal_data,
				      nm_gate_id_t gate_id,
                                      uint8_t tag) {
  int seq = nm_so_sr_get_current_send_seq(mpir_internal_data->p_so_sr_if, gate_id, tag);
  if (seq == NM_SO_PENDING_PACKS_WINDOW-1) {
    MPI_NMAD_TRACE("Reaching maximum sequence number in emission. Trigger automatic flushing");
    nm_so_sr_swait_range(mpir_internal_data->p_so_sr_if, gate_id, tag, 0, seq-1);
    MPI_NMAD_TRACE("Automatic flushing over");
  }
  return seq;
}

/**
 * Checks the current receiving NM sequence and triggers flushing of
 * previous incoming requests when needed.
 */
static inline int mpir_check_recv_seq(mpir_internal_data_t *mpir_internal_data,
				      nm_gate_id_t gate_id,
                                      uint8_t tag) {
  int seq = nm_so_sr_get_current_recv_seq(mpir_internal_data->p_so_sr_if, gate_id, tag);

  if (seq == NM_SO_PENDING_PACKS_WINDOW-1) {
    MPI_NMAD_TRACE("Reaching maximum sequence number in reception. Trigger automatic flushing");
    nm_so_sr_rwait_range(mpir_internal_data->p_so_sr_if, gate_id, tag, 0, seq-1);
    MPI_NMAD_TRACE("Automatic flushing over");
  }
  return seq;
}

/**
 * Calls the appropriate sending request based on the given request.
 */
static inline int mpir_isend_wrapper(mpir_internal_data_t *mpir_internal_data,
				     mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
  void *buffer;
  int err;

  if (mpir_datatype->is_contig == 1) {
    buffer = mpir_request->buffer;
  }
  else {
    buffer =  mpir_request->contig_buffer;
  }

  MPI_NMAD_TRACE("Sending data\n");
  MPI_NMAD_TRANSFER("Sent --> gate %d : %ld bytes\n", mpir_request->gate_id, (long)mpir_request->count * mpir_datatype->size);

  if (mpir_request->communication_mode == MPI_IMMEDIATE_MODE) {
    err = nm_so_sr_isend(mpir_internal_data->p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer,
                         mpir_request->count * mpir_datatype->size, &(mpir_request->request_nmad));
  }
  else if (mpir_request->communication_mode == MPI_READY_MODE) {
    err = nm_so_sr_rsend(mpir_internal_data->p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer,
                         mpir_request->count * mpir_datatype->size, &(mpir_request->request_nmad));
  }
  else {
    err = nm_so_sr_isend_extended(mpir_internal_data->p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer,
                                  mpir_request->count * mpir_datatype->size, mpir_request->communication_mode,
                                  &(mpir_request->request_nmad));
  }

  MPI_NMAD_TRANSFER("Sent finished\n");
  return err;
}

/**
 * Calls the appropriate pack function based on the given request.
 */
static inline int mpir_pack_wrapper(mpir_internal_data_t *mpir_internal_data,
				    mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
  struct nm_so_cnx *connection = &(mpir_request->request_cnx);
  int err = MPI_SUCCESS;

  MPI_NMAD_TRACE("Packing data\n");
  nm_so_begin_packing(mpir_internal_data->p_so_pack_if, mpir_request->gate_id, mpir_request->request_tag, connection);

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

int mpir_isend_init(mpir_internal_data_t *mpir_internal_data,
		    mpir_request_t *mpir_request,
                    int dest,
                    mpir_communicator_t *mpir_communicator) {
  mpir_datatype_t      *mpir_datatype = NULL;
  int                   err = MPI_SUCCESS;

  if (tbx_unlikely(dest >= mpir_communicator->size)) {
    TBX_FAILUREF("Dest %d does not belong to communicator %d\n", dest, mpir_communicator->communicator_id);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  if (tbx_unlikely(mpir_internal_data->out_gate_id[mpir_communicator->global_ranks[dest]] == NM_ANY_GATE)) {
    TBX_FAILUREF("Cannot find a connection between %d and %d, %d\n", mpir_communicator->rank, dest, mpir_communicator->global_ranks[dest]);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  mpir_request->gate_id = mpir_internal_data->out_gate_id[mpir_communicator->global_ranks[dest]];
  mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_internal_data, mpir_communicator, mpir_request->user_tag);

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid sending tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, mpir_request->user_tag, NM_SO_MAX_TAGS);
    return MPI_ERR_INTERN;
  }

  {
    int seq = mpir_check_send_seq(mpir_internal_data, mpir_request->gate_id, mpir_request->request_tag);
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
      MPI_NMAD_TRACE("Allocating a buffer of size %ld (%d * %ld) for sending an indexed datatype\n", (long)(mpir_request->count * mpir_datatype->size), mpir_request->count, (long)mpir_datatype->size);

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

int mpir_isend_start(mpir_internal_data_t *mpir_internal_data,
		     mpir_request_t *mpir_request) {
  int err = MPI_SUCCESS;
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);

  mpir_datatype->active_communications ++;
  if (mpir_datatype->is_contig || !(mpir_datatype->is_optimized)) {
    err = mpir_isend_wrapper(mpir_internal_data, mpir_request);
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_SEND;
  }
  else {
    err = mpir_pack_wrapper(mpir_internal_data, mpir_request);
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_SEND;
  }

  err = nm_so_sr_progress(mpir_internal_data->p_so_sr_if);
  mpir_inc_nb_outgoing_msg(mpir_internal_data);

  return err;
}

int mpir_isend(mpir_internal_data_t *mpir_internal_data,
	       mpir_request_t *mpir_request,
               int dest,
               mpir_communicator_t *mpir_communicator) {
  int err;

  err = mpir_isend_init(mpir_internal_data, mpir_request, dest, mpir_communicator);
  if (err == MPI_SUCCESS) {
    err = mpir_isend_start(mpir_internal_data, mpir_request);
  }
  return err;
}

int mpir_set_status(mpir_internal_data_t *mpir_internal_data,
		    MPI_Request *request,
		    MPI_Status *status) {
  mpir_request_t *mpir_request = (mpir_request_t *)request;
  int err = MPI_SUCCESS;

  status->MPI_TAG = mpir_request->user_tag;
  status->MPI_ERROR = mpir_request->request_error;

  if (mpir_request->request_type == MPI_REQUEST_RECV ||
      mpir_request->request_type == MPI_REQUEST_PACK_RECV) {
    if (mpir_request->request_source == MPI_ANY_SOURCE) {
      nm_gate_id_t gate_id;
      err = nm_so_sr_recv_source(mpir_internal_data->p_so_sr_if, mpir_request->request_nmad, &gate_id);
      status->MPI_SOURCE = mpir_internal_data->in_dest[gate_id];
    }
    else {
      status->MPI_SOURCE = mpir_request->request_source;
    }
  }

  nm_so_sr_get_size(mpir_internal_data->p_so_sr_if, &(mpir_request->request_nmad), mpir_request->request_tag, &(status->size));
  MPI_NMAD_TRACE("Size %d Size datatype %lu\n", status->size, (unsigned long)mpir_sizeof_datatype(mpir_internal_data, mpir_request->request_datatype));
  if (mpir_sizeof_datatype(mpir_internal_data, mpir_request->request_datatype) != 0) {
    status->count = status->size / mpir_sizeof_datatype(mpir_internal_data, mpir_request->request_datatype);
  }
  else {
    status->count = -1;
  }

  return err;
}

int mpir_irecv_init(mpir_internal_data_t *mpir_internal_data,
		    mpir_request_t *mpir_request,
                    int source,
                    mpir_communicator_t *mpir_communicator) {
  mpir_datatype_t      *mpir_datatype = NULL;

  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(source == MPI_ANY_SOURCE)) {
    mpir_request->gate_id = NM_ANY_GATE;
  }
  else {
    if (tbx_unlikely(source >= mpir_communicator->size)) {
      TBX_FAILUREF("Source %d does not belong to the communicator %d\n", source, mpir_communicator->communicator_id);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
    if (tbx_unlikely(mpir_internal_data->in_gate_id[mpir_communicator->global_ranks[source]] == NM_ANY_GATE)) {
      TBX_FAILUREF("Cannot find a connection between %d and %d, %d\n", mpir_communicator->rank, source, mpir_communicator->global_ranks[source]);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
    mpir_request->gate_id = mpir_internal_data->in_gate_id[mpir_communicator->global_ranks[source]];
  }

  mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
  mpir_request->request_tag = mpir_project_comm_and_tag(mpir_internal_data, mpir_communicator, mpir_request->user_tag);
  mpir_request->request_source = source;

  if (tbx_unlikely(mpir_request->request_tag > NM_SO_MAX_TAGS)) {
    fprintf(stderr, "Invalid receiving tag %d (%d, %d). Maximum allowed tag: %d\n", mpir_request->request_tag, mpir_communicator->communicator_id, mpir_request->user_tag, NM_SO_MAX_TAGS);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  {
    int seq;
    if (source != MPI_ANY_SOURCE) {
      seq = mpir_check_recv_seq(mpir_internal_data, mpir_request->gate_id, mpir_request->request_tag);
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
static inline int mpir_irecv_wrapper(mpir_internal_data_t *mpir_internal_data,
				     mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
  void *buffer;
  int err;

  if (mpir_datatype->is_contig == 1) {
    buffer = mpir_request->buffer;
  }
  else {
    buffer =  mpir_request->contig_buffer;
  }

  MPI_NMAD_TRACE("Posting recv request\n");
  MPI_NMAD_TRANSFER("Recv --< gate %d: %ld bytes\n", mpir_request->gate_id, (long)mpir_request->count * mpir_datatype->size);
  err = nm_so_sr_irecv(mpir_internal_data->p_so_sr_if, mpir_request->gate_id, mpir_request->request_tag, buffer, mpir_request->count * mpir_datatype->size, &(mpir_request->request_nmad));
  MPI_NMAD_TRANSFER("Recv finished, request = %p\n", &(mpir_request->request_nmad));

  return err;
}

/**
 * Calls the appropriate unpack functions based on the given request.
 */
static inline int mpir_unpack_wrapper(mpir_internal_data_t *mpir_internal_data,
				      mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
  struct nm_so_cnx *connection = &(mpir_request->request_cnx);
  int err = MPI_SUCCESS;

  MPI_NMAD_TRACE("Unpacking data\n");
  nm_so_begin_unpacking(mpir_internal_data->p_so_pack_if, mpir_request->gate_id, mpir_request->request_tag, connection);

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

int mpir_irecv_start(mpir_internal_data_t *mpir_internal_data,
		     mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);

  mpir_datatype->active_communications ++;

  if (mpir_datatype->is_contig || !(mpir_datatype->is_optimized)) {
    mpir_request->request_error = mpir_irecv_wrapper(mpir_internal_data, mpir_request);

    if (mpir_datatype->is_contig) {
      MPI_NMAD_TRACE("Calling irecv_start for contiguous data\n");
      if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_RECV;
    }
  }
  else {
    mpir_request->request_error = mpir_unpack_wrapper(mpir_internal_data, mpir_request);
    if (mpir_request->request_type != MPI_REQUEST_ZERO) mpir_request->request_type = MPI_REQUEST_PACK_RECV;
  }

  nm_so_sr_progress(mpir_internal_data->p_so_sr_if);

  mpir_inc_nb_incoming_msg(mpir_internal_data);
  MPI_NMAD_TRACE("Irecv_start completed\n");
  MPI_NMAD_LOG_OUT();
  return mpir_request->request_error;
}

int mpir_irecv(mpir_internal_data_t *mpir_internal_data,
	       mpir_request_t *mpir_request,
               int dest,
               mpir_communicator_t *mpir_communicator) {
  int err;

  err = mpir_irecv_init(mpir_internal_data, mpir_request, dest, mpir_communicator);
  if (err == MPI_SUCCESS) {
    err = mpir_irecv_start(mpir_internal_data, mpir_request);
  }
  return err;
}

int mpir_datatype_split(mpir_internal_data_t *mpir_internal_data,
			mpir_request_t *mpir_request) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
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

int mpir_start(mpir_internal_data_t *mpir_internal_data,
	       mpir_request_t *mpir_request) {
  if (mpir_request->request_persistent_type == MPI_REQUEST_SEND) {
    return mpir_isend_start(mpir_internal_data, mpir_request);
  }
  else if (mpir_request->request_persistent_type == MPI_REQUEST_RECV) {
    return mpir_irecv_start(mpir_internal_data, mpir_request);
  }
  else {
    ERROR("Unknown persistent request type: %d\n", mpir_request->request_persistent_type);
    return MPI_ERR_INTERN;
  }
}

int mpir_wait(mpir_internal_data_t *mpir_internal_data,
	      mpir_request_t *mpir_request) {
  int err = MPI_SUCCESS;
  mpir_datatype_t *mpir_datatype;

  MPI_NMAD_TRACE("Waiting for a request %d\n", mpir_request->request_type);
  if (mpir_request->request_type == MPI_REQUEST_RECV) {
    MPI_NMAD_TRACE("Calling nm_so_sr_rwait\n");
    MPI_NMAD_TRANSFER("Calling nm_so_sr_rwait for request=%p\n", &(mpir_request->request_nmad));
    err = nm_so_sr_rwait(mpir_internal_data->p_so_sr_if, mpir_request->request_nmad);
    mpir_datatype = mpir_get_datatype(mpir_internal_data, mpir_request->request_datatype);
    if (!mpir_datatype->is_contig && mpir_request->contig_buffer) {
      mpir_datatype_split(mpir_internal_data, mpir_request);
    }
    MPI_NMAD_TRANSFER("Returning from nm_so_sr_rwait\n");
  }
  else if (mpir_request->request_type == MPI_REQUEST_SEND) {
    MPI_NMAD_TRACE("Calling nm_so_sr_swait\n");
    MPI_NMAD_TRANSFER("Calling nm_so_sr_swait\n");
    err = nm_so_sr_swait(mpir_internal_data->p_so_sr_if, mpir_request->request_nmad);
    MPI_NMAD_TRANSFER("Returning from nm_so_sr_swait\n");
    if (mpir_request->request_persistent_type == MPI_REQUEST_ZERO) {
      if (mpir_request->contig_buffer != NULL) {
        FREE_AND_SET_NULL(mpir_request->contig_buffer);
      }
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
    MPI_NMAD_TRANSFER("Returning from nm_so_end_packing\n");
  }
  else {
    MPI_NMAD_TRACE("Waiting operation invalid for request type %d\n", mpir_request->request_type);
  }

  return err;
}

int mpir_test(mpir_internal_data_t *mpir_internal_data,
	      mpir_request_t *mpir_request) {
  int err = MPI_SUCCESS;
  if (mpir_request->request_type == MPI_REQUEST_RECV) {
    err = nm_so_sr_rtest(mpir_internal_data->p_so_sr_if, mpir_request->request_nmad);
  }
  else if (mpir_request->request_type == MPI_REQUEST_SEND) {
    err = nm_so_sr_stest(mpir_internal_data->p_so_sr_if, mpir_request->request_nmad);
  }
  else if (mpir_request->request_type == MPI_REQUEST_PACK_RECV) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    err = nm_so_test_end_unpacking(connection);
  }
  else if (mpir_request->request_type == MPI_REQUEST_PACK_SEND) {
    struct nm_so_cnx *connection = &(mpir_request->request_cnx);
    err = nm_so_test_end_packing(connection);
  }
  return err;
}

int mpir_probe(mpir_internal_data_t *mpir_internal_data,
	       nm_gate_id_t gate_id,
	       nm_gate_id_t *out_gate_id,
	       int tag) {
  return nm_so_sr_probe(mpir_internal_data->p_so_sr_if, gate_id, out_gate_id, tag);
}

/**
 * Gets the id of the next available datatype.
 */
static int get_available_datatype(mpir_internal_data_t *mpir_internal_data) {
  if (tbx_slist_is_nil(mpir_internal_data->available_datatypes) == tbx_true) {
    ERROR("Maximum number of datatypes created");
    return MPI_ERR_INTERN;
  }
  else {
    int datatype;
    int *ptr = tbx_slist_extract(mpir_internal_data->available_datatypes);
    datatype = *ptr;
    FREE_AND_SET_NULL(ptr);
    return datatype;
  }
}

size_t mpir_sizeof_datatype(mpir_internal_data_t *mpir_internal_data,
			    MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, datatype);
  return mpir_datatype->size;
}

mpir_datatype_t* mpir_get_datatype(mpir_internal_data_t *mpir_internal_data,
				   MPI_Datatype datatype) {
  if (tbx_unlikely(datatype <= NUMBER_OF_DATATYPES)) {
    if (tbx_unlikely(mpir_internal_data->datatypes[datatype] == NULL)) {
      ERROR("Datatype %d invalid", datatype);
      return NULL;
    }
    else {
      return mpir_internal_data->datatypes[datatype];
    }
  }
  else {
    ERROR("Datatype %d unknown", datatype);
    return NULL;
  }
}

int mpir_type_size(mpir_internal_data_t *mpir_internal_data,
		   MPI_Datatype datatype,
		   int *size) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, datatype);
  *size = mpir_datatype->size;
  return MPI_SUCCESS;
}

int mpir_type_get_lb_and_extent(mpir_internal_data_t *mpir_internal_data,
				MPI_Datatype datatype,
				MPI_Aint *lb,
				MPI_Aint *extent) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, datatype);
  if (lb) {
    *lb = mpir_datatype->lb;
  }
  if (extent) {
    *extent = mpir_datatype->extent;
  }
  return MPI_SUCCESS;
}

int mpir_type_create_resized(mpir_internal_data_t *mpir_internal_data,
			     MPI_Datatype oldtype,
			     MPI_Aint lb,
			     MPI_Aint extent,
			     MPI_Datatype *newtype) {
  int i;
  mpir_datatype_t *mpir_old_datatype = mpir_get_datatype(mpir_internal_data, oldtype);

  *newtype = get_available_datatype(mpir_internal_data);
  mpir_internal_data->datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  mpir_internal_data->datatypes[*newtype]->dte_type  = mpir_old_datatype->dte_type;
  mpir_internal_data->datatypes[*newtype]->basic     = mpir_old_datatype->basic;
  mpir_internal_data->datatypes[*newtype]->committed = 0;
  mpir_internal_data->datatypes[*newtype]->is_contig = mpir_old_datatype->is_contig;
  mpir_internal_data->datatypes[*newtype]->size      = mpir_old_datatype->size;
  mpir_internal_data->datatypes[*newtype]->extent    = extent;
  mpir_internal_data->datatypes[*newtype]->lb        = lb;

  if (mpir_internal_data->datatypes[*newtype]->dte_type == MPIR_CONTIG) {
    mpir_internal_data->datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    mpir_internal_data->datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    mpir_internal_data->datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    mpir_internal_data->datatypes[*newtype]->old_types[0] = oldtype;
    mpir_internal_data->datatypes[*newtype]->elements     = mpir_old_datatype->elements;
  }
  else if (mpir_internal_data->datatypes[*newtype]->dte_type == MPIR_VECTOR) {
    mpir_internal_data->datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    mpir_internal_data->datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    mpir_internal_data->datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    mpir_internal_data->datatypes[*newtype]->old_types[0] = oldtype;
    mpir_internal_data->datatypes[*newtype]->elements     = mpir_old_datatype->elements;
    mpir_internal_data->datatypes[*newtype]->blocklens    = malloc(1 * sizeof(int));
    mpir_internal_data->datatypes[*newtype]->blocklens[0] = mpir_old_datatype->blocklens[0];
    mpir_internal_data->datatypes[*newtype]->block_size   = mpir_old_datatype->block_size;
    mpir_internal_data->datatypes[*newtype]->stride       = mpir_old_datatype->stride;
  }
  else if (mpir_internal_data->datatypes[*newtype]->dte_type == MPIR_INDEXED) {
    mpir_internal_data->datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    mpir_internal_data->datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    mpir_internal_data->datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    mpir_internal_data->datatypes[*newtype]->old_types[0] = oldtype;
    mpir_internal_data->datatypes[*newtype]->elements     = mpir_old_datatype->elements;
    mpir_internal_data->datatypes[*newtype]->blocklens    = malloc(mpir_internal_data->datatypes[*newtype]->elements * sizeof(int));
    mpir_internal_data->datatypes[*newtype]->indices      = malloc(mpir_internal_data->datatypes[*newtype]->elements * sizeof(MPI_Aint));
    for(i=0 ; i<mpir_internal_data->datatypes[*newtype]->elements ; i++) {
      mpir_internal_data->datatypes[*newtype]->blocklens[i] = mpir_old_datatype->blocklens[i];
      mpir_internal_data->datatypes[*newtype]->indices[i]   = mpir_old_datatype->indices[i];
    }
  }
  else if (mpir_internal_data->datatypes[*newtype]->dte_type == MPIR_STRUCT) {
    mpir_internal_data->datatypes[*newtype]->elements  = mpir_old_datatype->elements;
    mpir_internal_data->datatypes[*newtype]->blocklens = malloc(mpir_internal_data->datatypes[*newtype]->elements * sizeof(int));
    mpir_internal_data->datatypes[*newtype]->indices   = malloc(mpir_internal_data->datatypes[*newtype]->elements * sizeof(MPI_Aint));
    mpir_internal_data->datatypes[*newtype]->old_sizes = malloc(mpir_internal_data->datatypes[*newtype]->elements * sizeof(size_t));
    mpir_internal_data->datatypes[*newtype]->old_types = malloc(mpir_internal_data->datatypes[*newtype]->elements * sizeof(MPI_Datatype));
    for(i=0 ; i<mpir_internal_data->datatypes[*newtype]->elements ; i++) {
      mpir_internal_data->datatypes[*newtype]->blocklens[i] = mpir_old_datatype->blocklens[i];
      mpir_internal_data->datatypes[*newtype]->indices[i]   = mpir_old_datatype->indices[i];
      mpir_internal_data->datatypes[*newtype]->old_sizes[i] = mpir_old_datatype->old_sizes[i];
      mpir_internal_data->datatypes[*newtype]->old_types[i] = mpir_old_datatype->old_types[i];
    }
  }
  else if (mpir_internal_data->datatypes[*newtype]->dte_type != MPIR_BASIC) {
    ERROR("Datatype %d unknown", oldtype);
    return MPI_ERR_OTHER;
  }

  MPI_NMAD_TRACE_LEVEL(3, "Creating resized type (%d) with extent=%ld, based on type %d with a extent %ld\n",
		       *newtype, (long)mpir_internal_data->datatypes[*newtype]->extent, oldtype, (long)mpir_old_datatype->extent);
  return MPI_SUCCESS;
}

int mpir_type_commit(mpir_internal_data_t *mpir_internal_data,
		     MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, datatype);
  mpir_datatype->committed = 1;
  mpir_datatype->is_optimized = 0;
  mpir_datatype->active_communications = 0;
  mpir_datatype->free_requested = 0;
  return MPI_SUCCESS;
}

int mpir_type_unlock(mpir_internal_data_t *mpir_internal_data,
		     MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, datatype);

  MPI_NMAD_TRACE_LEVEL(3, "Unlocking datatype %d (%d)\n", datatype, mpir_datatype->active_communications);
  mpir_datatype->active_communications --;
  if (mpir_datatype->active_communications == 0 && mpir_datatype->free_requested == 1) {
    MPI_NMAD_TRACE("Freeing datatype %d\n", datatype);
    mpir_type_free(mpir_internal_data, datatype);
  }
  return MPI_SUCCESS;
}

int mpir_type_free(mpir_internal_data_t *mpir_internal_data,
		   MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, datatype);

  if (mpir_datatype->active_communications != 0) {
    mpir_datatype->free_requested = 1;
    MPI_NMAD_TRACE_LEVEL(3, "Datatype %d still in use (%d). Cannot be released\n", datatype, mpir_datatype->active_communications);
    return MPI_DATATYPE_ACTIVE;
  }
  else {
    MPI_NMAD_TRACE_LEVEL(3, "Releasing datatype %d\n", datatype);
    FREE_AND_SET_NULL(mpir_internal_data->datatypes[datatype]->old_sizes);
    FREE_AND_SET_NULL(mpir_internal_data->datatypes[datatype]->old_types);
    if (mpir_internal_data->datatypes[datatype]->dte_type == MPIR_INDEXED || mpir_internal_data->datatypes[datatype]->dte_type == MPIR_STRUCT) {
      FREE_AND_SET_NULL(mpir_internal_data->datatypes[datatype]->blocklens);
      FREE_AND_SET_NULL(mpir_internal_data->datatypes[datatype]->indices);
    }
    if (mpir_internal_data->datatypes[datatype]->dte_type == MPIR_VECTOR) {
      FREE_AND_SET_NULL(mpir_internal_data->datatypes[datatype]->blocklens);
    }
    int *ptr;
    ptr = malloc(sizeof(int));
    *ptr = datatype;
    tbx_slist_enqueue(mpir_internal_data->available_datatypes, ptr);

    FREE_AND_SET_NULL(mpir_internal_data->datatypes[datatype]);
    return MPI_SUCCESS;
  }
}

int mpir_type_optimized(mpir_internal_data_t *mpir_internal_data,
			MPI_Datatype datatype,
			int optimized) {
  mpir_datatype_t *mpir_datatype = mpir_get_datatype(mpir_internal_data, datatype);
  mpir_datatype->is_optimized = optimized;
  return MPI_SUCCESS;
}

int mpir_type_contiguous(mpir_internal_data_t *mpir_internal_data,
			 int count,
                         MPI_Datatype oldtype,
                         MPI_Datatype *newtype) {
  mpir_datatype_t *mpir_old_datatype;

  mpir_old_datatype = mpir_get_datatype(mpir_internal_data, oldtype);
  *newtype = get_available_datatype(mpir_internal_data);
  mpir_internal_data->datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  mpir_internal_data->datatypes[*newtype]->dte_type = MPIR_CONTIG;
  mpir_internal_data->datatypes[*newtype]->basic = 0;
  mpir_internal_data->datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  mpir_internal_data->datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  mpir_internal_data->datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  mpir_internal_data->datatypes[*newtype]->old_types[0] = oldtype;
  mpir_internal_data->datatypes[*newtype]->committed = 0;
  mpir_internal_data->datatypes[*newtype]->is_contig = 1;
  mpir_internal_data->datatypes[*newtype]->size = mpir_internal_data->datatypes[*newtype]->old_sizes[0] * count;
  mpir_internal_data->datatypes[*newtype]->elements = count;
  mpir_internal_data->datatypes[*newtype]->lb = 0;
  mpir_internal_data->datatypes[*newtype]->extent = mpir_internal_data->datatypes[*newtype]->old_sizes[0] * count;

  MPI_NMAD_TRACE_LEVEL(3, "Creating new contiguous type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		       (long)mpir_internal_data->datatypes[*newtype]->size, (long)mpir_internal_data->datatypes[*newtype]->extent,
		       oldtype, (long)mpir_internal_data->datatypes[*newtype]->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpir_type_vector(mpir_internal_data_t *mpir_internal_data,
		     int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  mpir_datatype_t *mpir_old_datatype;

  mpir_old_datatype = mpir_get_datatype(mpir_internal_data, oldtype);
  *newtype = get_available_datatype(mpir_internal_data);
  mpir_internal_data->datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  mpir_internal_data->datatypes[*newtype]->dte_type = MPIR_VECTOR;
  mpir_internal_data->datatypes[*newtype]->basic = 0;
  mpir_internal_data->datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  mpir_internal_data->datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  mpir_internal_data->datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  mpir_internal_data->datatypes[*newtype]->old_types[0] = oldtype;
  mpir_internal_data->datatypes[*newtype]->committed = 0;
  mpir_internal_data->datatypes[*newtype]->is_contig = 0;
  mpir_internal_data->datatypes[*newtype]->size = mpir_internal_data->datatypes[*newtype]->old_sizes[0] * count * blocklength;
  mpir_internal_data->datatypes[*newtype]->elements = count;
  mpir_internal_data->datatypes[*newtype]->blocklens = malloc(1 * sizeof(int));
  mpir_internal_data->datatypes[*newtype]->blocklens[0] = blocklength;
  mpir_internal_data->datatypes[*newtype]->block_size = blocklength * mpir_internal_data->datatypes[*newtype]->old_sizes[0];
  mpir_internal_data->datatypes[*newtype]->stride = stride;
  mpir_internal_data->datatypes[*newtype]->lb = 0;
  mpir_internal_data->datatypes[*newtype]->extent = mpir_internal_data->datatypes[*newtype]->old_sizes[0] * count * blocklength;

  MPI_NMAD_TRACE_LEVEL(3, "Creating new (h)vector type (%d) with size=%ld, extent=%ld, elements=%d, blocklen=%d based on type %d with a extent %ld\n",
                       *newtype, (long)mpir_internal_data->datatypes[*newtype]->size, (long)mpir_internal_data->datatypes[*newtype]->extent,
                       mpir_internal_data->datatypes[*newtype]->elements, mpir_internal_data->datatypes[*newtype]->blocklens[0],
                       oldtype, (long)mpir_internal_data->datatypes[*newtype]->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpir_type_indexed(mpir_internal_data_t *mpir_internal_data,
		      int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  int i;
  mpir_datatype_t *mpir_old_datatype;

  MPI_NMAD_TRACE("Creating indexed derived datatype based on %d elements\n", count);
  mpir_old_datatype = mpir_get_datatype(mpir_internal_data, oldtype);

  *newtype = get_available_datatype(mpir_internal_data);
  mpir_internal_data->datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  mpir_internal_data->datatypes[*newtype]->dte_type = MPIR_INDEXED;
  mpir_internal_data->datatypes[*newtype]->basic = 0;
  mpir_internal_data->datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  mpir_internal_data->datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  mpir_internal_data->datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  mpir_internal_data->datatypes[*newtype]->old_types[0] = oldtype;
  mpir_internal_data->datatypes[*newtype]->committed = 0;
  mpir_internal_data->datatypes[*newtype]->is_contig = 0;
  mpir_internal_data->datatypes[*newtype]->elements = count;
  mpir_internal_data->datatypes[*newtype]->lb = 0;
  mpir_internal_data->datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  mpir_internal_data->datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  mpir_internal_data->datatypes[*newtype]->size = 0;

  for(i=0 ; i<count ; i++) {
    mpir_internal_data->datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    mpir_internal_data->datatypes[*newtype]->indices[i] = array_of_displacements[i];
    mpir_internal_data->datatypes[*newtype]->size += mpir_internal_data->datatypes[*newtype]->old_sizes[0] * mpir_internal_data->datatypes[*newtype]->blocklens[i];
    MPI_NMAD_TRACE("Element %d: length %d, indice %ld, new size %ld\n", i, mpir_internal_data->datatypes[*newtype]->blocklens[i], (long)mpir_internal_data->datatypes[*newtype]->indices[i], (long) mpir_internal_data->datatypes[*newtype]->size);
  }
  mpir_internal_data->datatypes[*newtype]->extent = (mpir_internal_data->datatypes[*newtype]->indices[count-1] + mpir_old_datatype->extent * mpir_internal_data->datatypes[*newtype]->blocklens[count-1]);

  MPI_NMAD_TRACE_LEVEL(3, "Creating new index type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		       (long)mpir_internal_data->datatypes[*newtype]->size, (long)mpir_internal_data->datatypes[*newtype]->extent,
		       oldtype, (long)mpir_internal_data->datatypes[*newtype]->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpir_type_struct(mpir_internal_data_t *mpir_internal_data,
		     int count,
                     int *array_of_blocklengths,
                     MPI_Aint *array_of_displacements,
                     MPI_Datatype *array_of_types,
                     MPI_Datatype *newtype) {
  int i;

  MPI_NMAD_TRACE("Creating struct derived datatype based on %d elements\n", count);
  *newtype = get_available_datatype(mpir_internal_data);
  mpir_internal_data->datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  mpir_internal_data->datatypes[*newtype]->dte_type = MPIR_STRUCT;
  mpir_internal_data->datatypes[*newtype]->basic = 0;
  mpir_internal_data->datatypes[*newtype]->committed = 0;
  mpir_internal_data->datatypes[*newtype]->is_contig = 0;
  mpir_internal_data->datatypes[*newtype]->elements = count;
  mpir_internal_data->datatypes[*newtype]->size = 0;
  mpir_internal_data->datatypes[*newtype]->lb = 0;

  mpir_internal_data->datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  mpir_internal_data->datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  mpir_internal_data->datatypes[*newtype]->old_sizes = malloc(count * sizeof(size_t));
  mpir_internal_data->datatypes[*newtype]->old_types = malloc(count * sizeof(MPI_Datatype));
  for(i=0 ; i<count ; i++) {
    mpir_internal_data->datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    mpir_internal_data->datatypes[*newtype]->old_sizes[i] = mpir_get_datatype(mpir_internal_data, array_of_types[i])->size;
    mpir_internal_data->datatypes[*newtype]->old_types[i] = array_of_types[i];
    mpir_internal_data->datatypes[*newtype]->indices[i] = array_of_displacements[i];

    MPI_NMAD_TRACE("Element %d: length %d, old_type size %ld, indice %ld\n", i, mpir_internal_data->datatypes[*newtype]->blocklens[i],
		   (long)mpir_internal_data->datatypes[*newtype]->old_sizes[i], (long)mpir_internal_data->datatypes[*newtype]->indices[i]);
    mpir_internal_data->datatypes[*newtype]->size += mpir_internal_data->datatypes[*newtype]->blocklens[i] * mpir_internal_data->datatypes[*newtype]->old_sizes[i];
  }
  /** We suppose here that the last field of the struct does not need
   * an alignment. In case, one sends an array of struct, the 1st
   * field of the 2nd struct immediatly follows the last field of the
   * previous struct.
   */
  mpir_internal_data->datatypes[*newtype]->extent = mpir_internal_data->datatypes[*newtype]->indices[count-1] + mpir_internal_data->datatypes[*newtype]->blocklens[count-1] * mpir_internal_data->datatypes[*newtype]->old_sizes[count-1];
  MPI_NMAD_TRACE_LEVEL(3, "Creating new struct type (%d) with size=%ld and extent=%ld\n", *newtype,
		       (long)mpir_internal_data->datatypes[*newtype]->size, (long)mpir_internal_data->datatypes[*newtype]->extent);
  return MPI_SUCCESS;
}

int mpir_op_create(mpir_internal_data_t *mpir_internal_data,
		   MPI_User_function *function,
                   int commute,
                   MPI_Op *op) {
  if (tbx_slist_is_nil(mpir_internal_data->available_operators) == tbx_true) {
    ERROR("Maximum number of operations created");
    return MPI_ERR_INTERN;
  }
  else {
    int *ptr = tbx_slist_extract(mpir_internal_data->available_operators);
    *op = *ptr;
    FREE_AND_SET_NULL(ptr);

    mpir_internal_data->operators[*op] = malloc(sizeof(mpir_operator_t));
    mpir_internal_data->operators[*op]->function = function;
    mpir_internal_data->operators[*op]->commute = commute;
    return MPI_SUCCESS;
  }
}

int mpir_op_free(mpir_internal_data_t *mpir_internal_data,
		 MPI_Op *op) {
  if (*op > NUMBER_OF_OPERATORS || mpir_internal_data->operators[*op] == NULL) {
    ERROR("Operator %d unknown\n", *op);
    return MPI_ERR_OTHER;
  }
  else {
    int *ptr;
    FREE_AND_SET_NULL(mpir_internal_data->operators[*op]);
    ptr = malloc(sizeof(int));
    *ptr = *op;
    tbx_slist_enqueue(mpir_internal_data->available_operators, ptr);
    *op = MPI_OP_NULL;
    return MPI_SUCCESS;
  }
}

mpir_operator_t *mpir_get_operator(mpir_internal_data_t *mpir_internal_data,
				   MPI_Op op) {
  if (mpir_internal_data->operators[op] != NULL) {
    return mpir_internal_data->operators[op];
  }
  else {
    ERROR("Operation %d unknown", op);
    return NULL;
  }
}

mpir_communicator_t *mpir_get_communicator(mpir_internal_data_t *mpir_internal_data,
					   MPI_Comm comm) {
  if (tbx_unlikely(comm <= NUMBER_OF_COMMUNICATORS)) {
    if (mpir_internal_data->communicators[comm-MPI_COMM_WORLD] == NULL) {
      ERROR("Communicator %d invalid", comm);
      return NULL;
    }
    else {
      return mpir_internal_data->communicators[comm-MPI_COMM_WORLD];
    }
  }
  else {
    ERROR("Communicator %d unknown", comm);
    return NULL;
  }
}

int mpir_comm_dup(mpir_internal_data_t *mpir_internal_data,
		  MPI_Comm comm,
		  MPI_Comm *newcomm) {
  if (tbx_slist_is_nil(mpir_internal_data->available_communicators) == tbx_true) {
    ERROR("Maximum number of communicators created");
    return MPI_ERR_INTERN;
  }
  else if (mpir_internal_data->communicators[comm-MPI_COMM_WORLD] == NULL) {
    ERROR("Communicator %d is not valid", comm);
    return MPI_ERR_OTHER;
  }
  else {
    int i;
    int *ptr = tbx_slist_extract(mpir_internal_data->available_communicators);
    *newcomm = *ptr;
    FREE_AND_SET_NULL(ptr);

    mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD] = malloc(sizeof(mpir_communicator_t));
    mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD]->communicator_id = *newcomm;
    mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD]->size = mpir_internal_data->communicators[comm - MPI_COMM_WORLD]->size;
    mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD]->rank = mpir_internal_data->communicators[comm - MPI_COMM_WORLD]->rank;
    mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD]->global_ranks = malloc(mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD]->size * sizeof(int));
    for(i=0 ; i<mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD]->size ; i++) {
      mpir_internal_data->communicators[*newcomm - MPI_COMM_WORLD]->global_ranks[i] = mpir_internal_data->communicators[comm - MPI_COMM_WORLD]->global_ranks[i];
    }

    return MPI_SUCCESS;
  }
}

int mpir_comm_free(mpir_internal_data_t *mpir_internal_data,
		   MPI_Comm *comm) {
  if (*comm == MPI_COMM_WORLD) {
    ERROR("Cannot free communicator MPI_COMM_WORLD");
    return MPI_ERR_OTHER;
  }
  else if (*comm == MPI_COMM_SELF) {
    ERROR("Cannot free communicator MPI_COMM_SELF");
    return MPI_ERR_OTHER;
  }
  else if (*comm > NUMBER_OF_COMMUNICATORS || mpir_internal_data->communicators[*comm-MPI_COMM_WORLD] == NULL) {
    ERROR("Communicator %d unknown\n", *comm);
    return MPI_ERR_OTHER;
  }
  else {
    int *ptr;
    free(mpir_internal_data->communicators[*comm-MPI_COMM_WORLD]->global_ranks);
    free(mpir_internal_data->communicators[*comm-MPI_COMM_WORLD]);
    mpir_internal_data->communicators[*comm-MPI_COMM_WORLD] = NULL;
    ptr = malloc(sizeof(int));
    *ptr = *comm;
    tbx_slist_enqueue(mpir_internal_data->available_communicators, ptr);
    *comm = MPI_COMM_NULL;
    return MPI_SUCCESS;
  }
}

int mpir_project_comm_and_tag(mpir_internal_data_t *mpir_internal_data,
			      mpir_communicator_t *mpir_communicator,
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

tbx_bool_t mpir_test_termination(mpir_internal_data_t *mpir_internal_data,
				 MPI_Comm comm) {
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
    global_nb_incoming_msg = mpir_internal_data->nb_incoming_msg;
    global_nb_outgoing_msg = mpir_internal_data->nb_outgoing_msg;
    for(i=1 ; i<global_size ; i++) {
      MPI_Recv(remote_counters, 2, MPI_INT, i, tag, comm, MPI_STATUS_IGNORE);
      global_nb_incoming_msg += remote_counters[0];
      global_nb_outgoing_msg += remote_counters[1];
    }
    MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);

    tbx_bool_t answer = tbx_true;
    if (global_nb_incoming_msg != global_nb_outgoing_msg) {
      answer = tbx_false;
    }

    for(i=1 ; i<global_size ; i++) {
      MPI_Send(&answer, 1, MPI_INT, i, tag, comm);
    }

    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      global_nb_incoming_msg = mpir_internal_data->nb_incoming_msg;
      global_nb_outgoing_msg = mpir_internal_data->nb_outgoing_msg;
      for(i=1 ; i<global_size ; i++) {
        MPI_Recv(remote_counters, 2, MPI_INT, i, tag, comm, MPI_STATUS_IGNORE);
        global_nb_incoming_msg += remote_counters[0];
        global_nb_outgoing_msg += remote_counters[1];
      }

      MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);
      answer = tbx_true;
      if (global_nb_incoming_msg != global_nb_outgoing_msg) {
        answer = tbx_false;
      }

      for(i=1 ; i<global_size ; i++) {
        MPI_Send(&answer, 1, MPI_INT, i, tag, comm);
      }

      return answer;
    }
  }
  else {
    int answer, local_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    local_counters[0] = mpir_internal_data->nb_incoming_msg;
    local_counters[1] = mpir_internal_data->nb_outgoing_msg;
    MPI_Send(local_counters, 2, MPI_INT, 0, tag, comm);

    MPI_Recv(&answer, 1, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      local_counters[0] = mpir_internal_data->nb_incoming_msg;
      local_counters[1] = mpir_internal_data->nb_outgoing_msg;
      MPI_Send(local_counters, 2, MPI_INT, 0, tag, comm);

      MPI_Recv(&answer, 1, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
      return answer;
    }
  }
}

void mpir_inc_nb_incoming_msg(mpir_internal_data_t *mpir_internal_data) {
  mpir_internal_data->nb_incoming_msg ++;
}

void mpir_inc_nb_outgoing_msg(mpir_internal_data_t *mpir_internal_data) {
  mpir_internal_data->nb_outgoing_msg ++;
}
