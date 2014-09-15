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


#include "nm_mpi_private.h"
#include "nm_log.h"

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);

/**
 * Increases by one the counter of incoming messages. The counter is
 * used for termination detection.
 */
static inline void mpir_inc_nb_incoming_msg(void)
{
  nm_mpi_internal_data.nb_incoming_msg ++;
}

/**
 * Increases by one the counter of outgoing messages. The counter is
 * used for termination detection.
 */
static inline void mpir_inc_nb_outgoing_msg(void)
{
  nm_mpi_internal_data.nb_outgoing_msg ++;
}

/**
 * Decreases by one the counter of incoming messages. The counter is
 * used for termination detection.
 */
static inline void mpir_dec_nb_incoming_msg(void)
{
  nm_mpi_internal_data.nb_incoming_msg --;
}

/**
 * Decreases by one the counter of outgoing messages. The counter is
 * used for termination detection.
 */
static inline void mpir_dec_nb_outgoing_msg(void)
{
  nm_mpi_internal_data.nb_outgoing_msg --;
}

int mpir_internal_init(void)
{
  int i;
  int dest;

  int global_size  = -1;
  int process_rank = -1;
  nm_launcher_get_size(&global_size);
  nm_launcher_get_rank(&process_rank);

  nm_mpi_internal_data.nb_outgoing_msg = 0;
  nm_mpi_internal_data.nb_incoming_msg = 0;

  /** Set the NewMadeleine interfaces */
  nm_session_t p_session = NULL;
  nm_launcher_get_session(&p_session);
  nm_sr_init(p_session);

  /** Initialise the basic datatypes */
  for(i = MPI_DATATYPE_NULL ; i <= _MPI_DATATYPE_MAX; i++)
    {
      nm_mpi_internal_data.datatypes[i] = malloc(sizeof(nm_mpi_datatype_t));
      nm_mpi_internal_data.datatypes[i]->basic = 1;
      nm_mpi_internal_data.datatypes[i]->committed = 1;
      nm_mpi_internal_data.datatypes[i]->is_contig = 1;
      nm_mpi_internal_data.datatypes[i]->dte_type = MPIR_BASIC;
      nm_mpi_internal_data.datatypes[i]->active_communications = 100;
      nm_mpi_internal_data.datatypes[i]->free_requested = 0;
      nm_mpi_internal_data.datatypes[i]->lb = 0;
    }

  nm_mpi_internal_data.datatypes[MPI_DATATYPE_NULL]->size = 0;
  nm_mpi_internal_data.datatypes[MPI_CHAR]->size = sizeof(signed char);
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED_CHAR]->size = sizeof(unsigned char);
  nm_mpi_internal_data.datatypes[MPI_BYTE]->size = 1;
  nm_mpi_internal_data.datatypes[MPI_SHORT]->size = sizeof(signed short);
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED_SHORT]->size = sizeof(unsigned short);
  nm_mpi_internal_data.datatypes[MPI_INT]->size = sizeof(signed int);
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED]->size = sizeof(unsigned int);
  nm_mpi_internal_data.datatypes[MPI_LONG]->size = sizeof(signed long);
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED_LONG]->size = sizeof(unsigned long);
  nm_mpi_internal_data.datatypes[MPI_FLOAT]->size = sizeof(float);
  nm_mpi_internal_data.datatypes[MPI_DOUBLE]->size = sizeof(double);
  nm_mpi_internal_data.datatypes[MPI_LONG_DOUBLE]->size = sizeof(long double);
  nm_mpi_internal_data.datatypes[MPI_LONG_LONG_INT]->size = sizeof(long long int);
  nm_mpi_internal_data.datatypes[MPI_LONG_LONG]->size = sizeof(long long);

  nm_mpi_internal_data.datatypes[MPI_LOGICAL]->size = sizeof(float);
  nm_mpi_internal_data.datatypes[MPI_REAL]->size = sizeof(float);
  nm_mpi_internal_data.datatypes[MPI_REAL4]->size = 4*sizeof(char);
  nm_mpi_internal_data.datatypes[MPI_REAL8]->size = 8*sizeof(char);
  nm_mpi_internal_data.datatypes[MPI_DOUBLE_PRECISION]->size = sizeof(double);
  nm_mpi_internal_data.datatypes[MPI_INTEGER]->size = sizeof(float);
  nm_mpi_internal_data.datatypes[MPI_INTEGER4]->size = sizeof(int32_t);
  nm_mpi_internal_data.datatypes[MPI_INTEGER8]->size = sizeof(int64_t);
  nm_mpi_internal_data.datatypes[MPI_PACKED]->size = sizeof(char);

  nm_mpi_internal_data.datatypes[MPI_DATATYPE_NULL]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_CHAR]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED_CHAR]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_BYTE]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_SHORT]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED_SHORT]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_INT]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_LONG]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_UNSIGNED_LONG]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_FLOAT]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_DOUBLE]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_LONG_DOUBLE]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_LONG_LONG_INT]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_LONG_LONG]->elements = 1;

  nm_mpi_internal_data.datatypes[MPI_LOGICAL]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_REAL]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_REAL4]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_REAL8]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_DOUBLE_PRECISION]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_INTEGER]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_INTEGER4]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_INTEGER8]->elements = 1;
  nm_mpi_internal_data.datatypes[MPI_PACKED]->elements = 1;

  /* todo: elements=2 */
  nm_mpi_internal_data.datatypes[MPI_LONG_INT]->size = sizeof(long)+sizeof(int);
  nm_mpi_internal_data.datatypes[MPI_SHORT_INT]->size = sizeof(short)+sizeof(int);
  nm_mpi_internal_data.datatypes[MPI_FLOAT_INT]->size = sizeof(float)+sizeof(int);
  nm_mpi_internal_data.datatypes[MPI_DOUBLE_INT]->size = sizeof(double) + sizeof(int);

  nm_mpi_internal_data.datatypes[MPI_2INT]->size = 2*sizeof(int);
  nm_mpi_internal_data.datatypes[MPI_2INT]->elements = 2;

  nm_mpi_internal_data.datatypes[MPI_2DOUBLE_PRECISION]->size = 2*sizeof(double);
  nm_mpi_internal_data.datatypes[MPI_2DOUBLE_PRECISION]->elements = 2;

  nm_mpi_internal_data.datatypes[MPI_COMPLEX]->size = 2*sizeof(float);
  nm_mpi_internal_data.datatypes[MPI_COMPLEX]->elements = 2;

  nm_mpi_internal_data.datatypes[MPI_DOUBLE_COMPLEX]->size = 2*sizeof(double);
  nm_mpi_internal_data.datatypes[MPI_DOUBLE_COMPLEX]->elements = 2;


  for(i = MPI_DATATYPE_NULL; i <= _MPI_DATATYPE_MAX; i++)
    {
      nm_mpi_internal_data.datatypes[i]->extent = nm_mpi_internal_data.datatypes[i]->size;
    }

  nm_mpi_internal_data.datatypes_pool = puk_int_vect_new();
  for(i = _MPI_DATATYPE_MAX+1 ; i < NUMBER_OF_DATATYPES; i++)
    {
      puk_int_vect_push_back(nm_mpi_internal_data.datatypes_pool, i);
    }


  /** Initialise the collective operators */
  for(i = _MPI_OP_FIRST; i <= _MPI_OP_LAST; i++)
    {
      nm_mpi_internal_data.operators[i] = malloc(sizeof(mpir_operator_t));
      nm_mpi_internal_data.operators[i]->commute = 1;
    }
  nm_mpi_internal_data.operators[MPI_MAX]->function = &mpir_op_max;
  nm_mpi_internal_data.operators[MPI_MIN]->function = &mpir_op_min;
  nm_mpi_internal_data.operators[MPI_SUM]->function = &mpir_op_sum;
  nm_mpi_internal_data.operators[MPI_PROD]->function = &mpir_op_prod;
  nm_mpi_internal_data.operators[MPI_LAND]->function = &mpir_op_land;
  nm_mpi_internal_data.operators[MPI_BAND]->function = &mpir_op_band;
  nm_mpi_internal_data.operators[MPI_LOR]->function = &mpir_op_lor;
  nm_mpi_internal_data.operators[MPI_BOR]->function = &mpir_op_bor;
  nm_mpi_internal_data.operators[MPI_LXOR]->function = &mpir_op_lxor;
  nm_mpi_internal_data.operators[MPI_BXOR]->function = &mpir_op_bxor;
  nm_mpi_internal_data.operators[MPI_MINLOC]->function = &mpir_op_minloc;
  nm_mpi_internal_data.operators[MPI_MAXLOC]->function = &mpir_op_maxloc;

  nm_mpi_internal_data.operators_pool = puk_int_vect_new();
  for(i = 1; i < MPI_MAX; i++)
    {
      puk_int_vect_push_back(nm_mpi_internal_data.operators_pool, i);
    }

  return MPI_SUCCESS;
}

int mpir_internal_exit(void)
{
  int i;
  for(i = 0; i <= _MPI_DATATYPE_MAX; i++)
    {
      FREE_AND_SET_NULL(nm_mpi_internal_data.datatypes[i]);
    }

  puk_int_vect_delete(nm_mpi_internal_data.datatypes_pool);
  nm_mpi_internal_data.datatypes_pool = NULL;

  for(i = _MPI_OP_FIRST; i <= _MPI_OP_LAST; i++)
    {
      FREE_AND_SET_NULL(nm_mpi_internal_data.operators[i]);
    }

  puk_int_vect_delete(nm_mpi_internal_data.operators_pool);
  nm_mpi_internal_data.operators_pool = NULL;

  return MPI_SUCCESS;
}

/**
 * Aggregates data represented by a vector datatype in a contiguous
 * buffer.
 */
static inline int nm_mpi_datatype_vector_aggregate(void *newptr, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
#ifdef DEBUG
  void * const orig = buffer;
  void * const dest = newptr;
#endif
  int i, j;
  for(i = 0; i < count; i++)
    {
      for(j = 0; j < p_datatype->elements; j++)
	{
	  MPI_NMAD_TRACE("Copy element %d, %d (size %ld) from %p (+%d) to %p (+%d)\n",
			 i, j, (long) p_datatype->block_size, buffer, (int)(buffer-orig),
			 newptr, (int)(newptr-dest));
	  memcpy(newptr, buffer, p_datatype->block_size);
	  newptr += p_datatype->block_size;
	  buffer += p_datatype->stride;
	}
    }
  return MPI_SUCCESS;
}

/**
 * Packs into a NM connection data represented by a vector datatype.
 */
static inline int nm_mpi_datatype_vector_pack(nm_pack_cnx_t *connection, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
#ifdef DEBUG
  void *const orig = buffer;
#endif
  int i, j, err = MPI_SUCCESS;
  for(i = 0 ; i<count ; i++)
    {
      for(j = 0; j < p_datatype->elements; j++)
	{
	  MPI_NMAD_TRACE("Element %d, %d with size %ld starts at %p (+ %d)\n", i, j, (long)p_datatype->block_size, buffer, (int)(buffer-orig));
	  err = nm_pack(connection, buffer, p_datatype->block_size);
	  buffer += p_datatype->stride;
	}
    }
  return err;
}

/**
 * Unpacks from a NM connection data represented by a vector datatype.
 */
static inline int nm_mpi_datatype_vector_unpack(nm_pack_cnx_t *connection, nm_mpi_request_t *p_req, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  int i, k=0, err = MPI_SUCCESS;
  p_req->request_ptr = malloc((count*p_datatype->elements+1) * sizeof(void *));
  p_req->request_ptr[0] = buffer;
  for(i = 0; i < count; i++)
    {
      int j;
      for(j = 0; j < p_datatype->elements; j++)
	{
	  err = nm_unpack(connection, p_req->request_ptr[k], p_datatype->block_size);
	  k++;
	  p_req->request_ptr[k] = p_req->request_ptr[k-1] + p_datatype->block_size;
	}
    }
  if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_PACK_RECV;
  return err;
}

/**
 * Splits data from a contiguous buffer into data represented by a
 * vector datatype
 */
static inline int nm_mpi_datatype_vector_split(void *recvbuffer, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  void *recvptr = recvbuffer;
  void *ptr = buffer;
  int i;
  for(i = 0; i < count; i++)
    {
      int j;
      for(j = 0; j < p_datatype->elements; j++)
	{
	  MPI_NMAD_TRACE("Copy element %d, %d from %p (+ %d) to %p (+ %d)\n",
			 i, j, recvptr, (int)(recvptr-recvbuffer), ptr, (int)(ptr-buffer));
	  memcpy(ptr, recvptr, p_datatype->block_size);
	  recvptr += p_datatype->block_size;
	  ptr += p_datatype->block_size;
	}
    }
  return MPI_SUCCESS;
}

/**
 * Aggregates data represented by a indexed datatype in a contiguous
 * buffer.
 */
static inline int nm_mpi_datatype_indexed_aggregate(void *newptr, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
#ifdef DEBUG
  void *const dest = newptr;
#endif
  int i, j;
  for(i = 0; i < count; i++)
    {
      void *ptr = buffer + i * p_datatype->extent;
      for(j = 0; j < p_datatype->elements; j++)
	{
	  void *subptr = ptr + p_datatype->indices[j];
	  MPI_NMAD_TRACE("Copy element %d, %d (size %ld = %d * %ld) from %p (+%d) to %p (+%d)\n", i, j,
			 (long)p_datatype->blocklens[j] * p_datatype->old_sizes[0],
			 p_datatype->blocklens[j], (long)p_datatype->old_sizes[0],
			 subptr, (int)(subptr-buffer), newptr, (int)(newptr-dest));
	  memcpy(newptr, subptr, p_datatype->blocklens[j] * p_datatype->old_sizes[0]);
	  newptr += p_datatype->blocklens[j] * p_datatype->old_sizes[0];
    }
  }
  return MPI_SUCCESS;
}

/**
 * Packs into a NM connection data represented by a indexed datatype.
 */
static inline int nm_mpi_datatype_indexed_pack(nm_pack_cnx_t *connection, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  int i, j, err = MPI_SUCCESS;
  for(i = 0; i < count; i++)
    {
      void *ptr = buffer + i * p_datatype->extent;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*p_datatype->extent);
      for(j = 0; j < p_datatype->elements; j++)
	{
	  void *subptr = ptr + p_datatype->indices[j];
	  MPI_NMAD_TRACE("Sub-element %d,%d starts at %p (%p + %ld) with size %ld\n", i, j, subptr, ptr,
			 (long)p_datatype->indices[j], (long)p_datatype->blocklens[j] * p_datatype->old_sizes[0]);
	  err = nm_pack(connection, subptr, p_datatype->blocklens[j] * p_datatype->old_sizes[0]);
	}
    }
  return err;
}

/**
 * Unpacks from a NM connection data represented by a indexed datatype.
 */
static inline int nm_mpi_datatype_indexed_unpack(nm_pack_cnx_t *connection, nm_mpi_request_t *p_req, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  int i, k=0, err = MPI_SUCCESS;
  p_req->request_ptr = malloc((count*p_datatype->elements+1) * sizeof(void *));
  p_req->request_ptr[0] = buffer;
  for(i = 0; i < count ; i++)
    {
      int j;
      for(j = 0; j < p_datatype->elements; j++)
	{
	  MPI_NMAD_TRACE("Sub-element %d,%d unpacked at %p (%p + %d) with size %ld\n", i, j,
			 p_req->request_ptr[k], buffer, (int)(p_req->request_ptr[k]-buffer),
			 (long)p_datatype->blocklens[j] * p_datatype->old_sizes[0]);
	  err = nm_unpack(connection, p_req->request_ptr[k], p_datatype->blocklens[j] * p_datatype->old_sizes[0]);
	  k++;
	  p_req->request_ptr[k] = p_req->request_ptr[k-1] + p_datatype->blocklens[j] * p_datatype->old_sizes[0];
	}
    }
  if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_PACK_RECV;
  return err;
}

/**
 * Splits data from a contiguous buffer into data represented by a indexed
 * datatype
 */
static inline int nm_mpi_datatype_indexed_split(void *recvbuffer, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  void *recvptr = recvbuffer;
  void *ptr = buffer;
  int	i;
  for(i = 0; i < count ; i++)
    {
      int j;
      for(j = 0; j < p_datatype->elements; j++)
	{
	  MPI_NMAD_TRACE("Copy element %d, %d (size %ld) from %p (+%d) to %p (+%d)\n", i, j,
			 (long int)p_datatype->blocklens[j] * p_datatype->old_sizes[0],
			 recvptr, (int)(recvptr-recvbuffer), ptr, (int)(ptr-buffer));
	  memcpy(ptr, recvptr, p_datatype->blocklens[j] * p_datatype->old_sizes[0]);
	  recvptr += p_datatype->blocklens[j] * p_datatype->old_sizes[0];
	  ptr += p_datatype->blocklens[j] * p_datatype->old_sizes[0];
	}
    }
  return MPI_SUCCESS;
}

/**
 * Aggregates data represented by a struct datatype in a contiguous
 * buffer.
 */
static inline int nm_mpi_datatype_struct_aggregate(void *newptr, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  int i, j;
  for(i = 0; i < count; i++)
    {
      void *ptr = buffer + i * p_datatype->extent;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*p_datatype->extent);
      for(j = 0; j < p_datatype->elements; j++)
	{
	  ptr += p_datatype->indices[j];
	  MPI_NMAD_TRACE("copying to %p data from %p (+%ld) with a size %d*%ld\n", newptr, ptr, (long)p_datatype->indices[j], p_datatype->blocklens[j], (long)p_datatype->old_sizes[j]);
	  memcpy(newptr, ptr, p_datatype->blocklens[j] * p_datatype->old_sizes[j]);
	  newptr += p_datatype->blocklens[j] * p_datatype->old_sizes[j];
	  ptr -= p_datatype->indices[j];
	}
    }
  return MPI_SUCCESS;
}

/**
 * Packs into a NM connection data represented by a struct datatype.
 */
static inline int nm_mpi_datatype_struct_pack(nm_pack_cnx_t *connection, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  int i, j, err = MPI_SUCCESS;
  for(i = 0; i < count; i++)
    {
      void *ptr = buffer + i * p_datatype->extent;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*p_datatype->extent);
      for(j = 0; j < p_datatype->elements; j++)
	{
	  ptr += p_datatype->indices[j];
	  MPI_NMAD_TRACE("packing data at %p (+%ld) with a size %d*%ld\n", ptr, (long)p_datatype->indices[j], p_datatype->blocklens[j], (long)p_datatype->old_sizes[j]);
	  err = nm_pack(connection, ptr, p_datatype->blocklens[j] * p_datatype->old_sizes[j]);
	  ptr -= p_datatype->indices[j];
	}
    }
  return err;
}

/**
 * Unpacks from a NM connection data represented by a struct datatype.
 */
static inline int nm_mpi_datatype_struct_unpack(nm_pack_cnx_t *connection, nm_mpi_request_t *p_req, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  int i, k=0, err = MPI_SUCCESS;
  p_req->request_ptr = malloc((count*p_datatype->elements+1) * sizeof(void *));
  for(i = 0; i < count ; i++)
    {
      int j;
      p_req->request_ptr[k] = buffer + i*p_datatype->extent;
      for(j = 0; j < p_datatype->elements; j++)
	{
	  p_req->request_ptr[k] += p_datatype->indices[j];
	  err = nm_unpack(connection, p_req->request_ptr[k], p_datatype->blocklens[j] * p_datatype->old_sizes[j]);
	  k++;
	  p_req->request_ptr[k] = p_req->request_ptr[k-1] - p_datatype->indices[j];
	}
    }
  if (p_req->request_type != MPI_REQUEST_ZERO)
    p_req->request_type = MPI_REQUEST_PACK_RECV;
  return err;
}

/**
 * Splits data from a contiguous buffer into data represented by a
 * struct datatype.
 */
static inline int nm_mpi_datatype_struct_split(void *recvbuffer, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  void *recvptr = recvbuffer;
  int i;
  for(i = 0; i < count; i++)
    {
      void *ptr = buffer + i*p_datatype->extent;
      int j;
      MPI_NMAD_TRACE("Element %d starts at %p (%p + %ld)\n", i, ptr, buffer, (long)i*p_datatype->extent);
      for(j = 0; j < p_datatype->elements; j++)
	{
	  ptr += p_datatype->indices[j];
	  MPI_NMAD_TRACE("Sub-element %d starts at %p (%p + %ld)\n", j, ptr, ptr-p_datatype->indices[j], (long)p_datatype->indices[j]);
	  memcpy(ptr, recvptr, p_datatype->blocklens[j] * p_datatype->old_sizes[j]);
	  MPI_NMAD_TRACE("Copying from %p and moving by %ld\n", recvptr, (long)p_datatype->blocklens[j] * p_datatype->old_sizes[j]);
	  recvptr += p_datatype->blocklens[j] * p_datatype->old_sizes[j];
	  ptr -= p_datatype->indices[j];
	}
    }
  return MPI_SUCCESS;
}

/**
 * Calls the appropriate sending request based on the given request.
 */
static inline int mpir_isend_wrapper(nm_mpi_request_t *p_req)
 {
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  void *buffer;
  int err;
  if(p_datatype->is_contig == 1)
    {
      buffer = p_req->buffer;
    }
  else
    {
      buffer =  p_req->contig_buffer;
    }

  MPI_NMAD_TRACE("Sending data\n");
  MPI_NMAD_TRACE("Sent --> gate %p : %ld bytes\n", p_req->gate, (long)p_req->count * p_datatype->size);

  switch(p_req->communication_mode)
    {
    case MPI_IMMEDIATE_MODE:
      err = nm_sr_isend(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, buffer,
			p_req->count * p_datatype->size, &(p_req->request_nmad));
      break;
    case MPI_READY_MODE:
      err = nm_sr_rsend(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, buffer,
			p_req->count * p_datatype->size, &(p_req->request_nmad));
      break;
    case MPI_SYNCHRONOUS_MODE:
      TBX_FAILURE("madmpi: synchronous mode not supported yet.\n");
      break;
    default:
      TBX_FAILUREF("madmpi: unkown mode %d for isend", p_req->communication_mode);
      break;
    }
  MPI_NMAD_TRACE("Sent finished\n");
  return err;
}

/**
 * Calls the appropriate pack function based on the given request.
 */
static inline int mpir_pack_wrapper(nm_mpi_request_t *p_req)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  nm_pack_cnx_t *connection = &(p_req->request_cnx);
  int err = MPI_SUCCESS;

  MPI_NMAD_TRACE("Packing data\n");
  nm_begin_packing(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, connection);

  if(p_datatype->dte_type == MPIR_VECTOR)
    {
      err = nm_mpi_datatype_vector_pack(connection, p_req->buffer, p_datatype, p_req->count);
    }
  else if (p_datatype->dte_type == MPIR_INDEXED) 
    {
      MPI_NMAD_TRACE("Sending (h)indexed type: count %d - size %ld - extent %ld\n", p_datatype->elements,
		     (long)p_datatype->size, (long)p_datatype->extent);
      err = nm_mpi_datatype_indexed_pack(connection, p_req->buffer, p_datatype, p_req->count);
    }
  else if (p_datatype->dte_type == MPIR_STRUCT)
    {
      MPI_NMAD_TRACE("Sending struct type: size %ld\n", (long)p_datatype->size);
      err = nm_mpi_datatype_struct_pack(connection, p_req->buffer, p_datatype, p_req->count);
    }
  return err;
}

int mpir_isend_init(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm)
{
  nm_mpi_datatype_t*p_datatype = NULL;
  int err = MPI_SUCCESS;
  nm_gate_t p_gate = nm_mpi_communicator_get_gate(p_comm, dest);
  if(p_gate == NULL)
    {
      TBX_FAILUREF("Cannot find rank %d in comm %p.\n", dest, p_comm);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }

  p_req->gate =p_gate;
  p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  p_req->request_tag = mpir_comm_and_tag(p_comm, p_req->user_tag);

  if(p_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Sending data of type %d at address %p with len %ld (%d*%ld)\n", p_req->request_datatype, p_req->buffer, (long)p_req->count*p_datatype->size, p_req->count, (long)p_datatype->size);
  }
  else if (p_datatype->dte_type == MPIR_VECTOR) {
    MPI_NMAD_TRACE("Sending (h)vector type: stride %d - blocklen %d - count %d - size %ld\n", p_datatype->stride, p_datatype->blocklens[0], p_datatype->elements, (long)p_datatype->size);
    if (!p_datatype->is_optimized) {
      MPI_NMAD_TRACE("Sending vector datatype in a contiguous buffer\n");

      p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
      if (p_req->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to send struct datatype\n", (long)(p_req->count * p_datatype->size));
        return MPI_ERR_INTERN;
      }

      nm_mpi_datatype_vector_aggregate(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
      MPI_NMAD_TRACE("Sending data of vector type at address %p with len %ld (%d*%d*%ld)\n", p_req->contig_buffer, (long)(p_req->count * p_datatype->size), p_req->count, p_datatype->elements, (long)p_datatype->block_size);
    }
  }
  else if (p_datatype->dte_type == MPIR_INDEXED) {
    if (!p_datatype->is_optimized) {
      MPI_NMAD_TRACE("Sending (h)indexed datatype in a contiguous buffer\n");

      p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
      if (p_req->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to send (h)indexed type\n", (long)(p_req->count * p_datatype->size));
        return MPI_ERR_INTERN;
      }
      MPI_NMAD_TRACE("Allocating a buffer of size %ld (%d * %ld) for sending an indexed datatype\n", (long)(p_req->count * p_datatype->size), p_req->count, (long)p_datatype->size);

      nm_mpi_datatype_indexed_aggregate(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
      MPI_NMAD_TRACE("Sending data of (h)indexed type at address %p with len %ld\n", p_req->contig_buffer, (long)(p_req->count * p_datatype->size));
    }
  }
  else if (p_datatype->dte_type == MPIR_STRUCT) {
    if (!p_datatype->is_optimized) {
      MPI_NMAD_TRACE("Sending struct datatype in a contiguous buffer\n");

      p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
      if (p_req->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to send struct datatype\n", (long)(p_req->count * p_datatype->size));
        return MPI_ERR_INTERN;
      }

      nm_mpi_datatype_struct_aggregate(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
      MPI_NMAD_TRACE("Sending data of struct type at address %p with len %ld (%d*%ld)\n", p_req->contig_buffer, (long)(p_req->count * p_datatype->size), p_req->count, (long)p_datatype->size);
    }
  }
  else {
    ERROR("Do not know how to send datatype %d %d\n", p_req->request_datatype, p_datatype->dte_type);
    return MPI_ERR_INTERN;
  }

  return err;
}

int mpir_isend_start(nm_mpi_request_t *p_req)
{
  int err = MPI_SUCCESS;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  p_datatype->active_communications ++;
  if(p_datatype->is_contig || !(p_datatype->is_optimized)) {
    err = mpir_isend_wrapper(p_req);
    if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_SEND;
  }
  else {
    err = mpir_pack_wrapper(p_req);
    if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_PACK_SEND;
  }

  err = nm_sr_progress(nm_comm_get_session(p_req->p_comm->p_comm));
  mpir_inc_nb_outgoing_msg();

  return err;
}

int mpir_isend(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm)
{
  int err;
  err = mpir_isend_init(p_req, dest, p_comm);
  if (err == MPI_SUCCESS) {
    err = mpir_isend_start(p_req);
  }
  return err;
}


int mpir_irecv_init(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm)
{
  nm_mpi_datatype_t*p_datatype = NULL;
  MPI_NMAD_LOG_IN();

  if (tbx_unlikely(source == MPI_ANY_SOURCE)) {
    p_req->gate = NM_ANY_GATE;
  }
  else
    {
      p_req->gate = nm_mpi_communicator_get_gate(p_comm, source);
      if(tbx_unlikely(p_req->gate == NULL))
	{
	  TBX_FAILUREF("Cannot find rank %d in comm %p\n", source, p_comm);
	  MPI_NMAD_LOG_OUT();
	  return MPI_ERR_INTERN;
	}
    }

  p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  p_req->request_tag = mpir_comm_and_tag(p_comm, p_req->user_tag);
  p_req->request_source = source;

  MPI_NMAD_TRACE("Receiving from %d at address %p with tag %d (comm=%p, %d)\n", source, p_req->buffer, p_req->request_tag, p_comm, p_req->user_tag);
  if (p_datatype->is_contig == 1) {
    MPI_NMAD_TRACE("Receiving data of type %d at address %p with len %ld (%d*%ld)\n", p_req->request_datatype, p_req->buffer, (long)p_req->count*p_datatype->size, p_req->count, (long)p_datatype->size);
    if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_RECV;
  }
  else if (p_datatype->dte_type == MPIR_VECTOR) {
    if (!p_datatype->is_optimized) {
      p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
      if (p_req->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to receive vector type\n", (long)(p_req->count * p_datatype->size));
        return MPI_ERR_INTERN;
      }

      MPI_NMAD_TRACE("Receiving vector type %d in a contiguous way at address %p with len %ld (%d*%ld)\n", p_req->request_datatype, p_req->contig_buffer, (long)(p_req->count * p_datatype->size), p_req->count, (long)p_datatype->size);
      if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_RECV;
    }
  }
  else if (p_datatype->dte_type == MPIR_INDEXED) {
    if (!p_datatype->is_optimized) {
      MPI_NMAD_TRACE("Receiving (h)indexed datatype in a contiguous buffer\n");
      p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
      if (p_req->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to receive (h)indexed type\n", (long)(p_req->count * p_datatype->size));
        return MPI_ERR_INTERN;
      }

      MPI_NMAD_TRACE("Receiving (h)indexed type %d in a contiguous way at address %p with len %ld\n", p_req->request_datatype, p_req->contig_buffer, (long)(p_req->count * p_datatype->size));
      if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_RECV;
    }
  }
  else if (p_datatype->dte_type == MPIR_STRUCT) {
    if (!p_datatype->is_optimized) {
      p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
      if (p_req->contig_buffer == NULL) {
        ERROR("Cannot allocate memory with size %ld to receive struct type\n", (long)(p_req->count * p_datatype->size));
        return MPI_ERR_INTERN;
      }

      MPI_NMAD_TRACE("Receiving struct type %d in a contiguous way at address %p with len %ld (%d*%ld)\n", p_req->request_datatype, p_req->contig_buffer, (long)p_req->count*p_datatype->size, p_req->count, (long)p_datatype->size);
      if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_RECV;
    }
  }
  else {
    ERROR("Do not know how to receive datatype %d\n", p_req->request_datatype);
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

/**
 * Calls the appropriate receiving request based on the given request.
 */
static inline int mpir_irecv_wrapper(nm_mpi_request_t *p_req)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  void *buffer;
  int err;

  if(p_datatype->is_contig == 1) {
    buffer = p_req->buffer;
  }
  else {
    buffer =  p_req->contig_buffer;
  }

  MPI_NMAD_TRACE("Posting recv request\n");
  MPI_NMAD_TRACE("Recv --< gate %p: %ld bytes\n", p_req->gate, (long)p_req->count * p_datatype->size);
  err = nm_sr_irecv(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, buffer, p_req->count * p_datatype->size, &(p_req->request_nmad));
  MPI_NMAD_TRACE("Recv finished, request = %p\n", &(p_req->request_nmad));

  return err;
}

/**
 * Calls the appropriate unpack functions based on the given request.
 */
static inline int mpir_unpack_wrapper(nm_mpi_request_t *p_req)
 {
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  nm_pack_cnx_t *connection = &(p_req->request_cnx);
  int err = MPI_SUCCESS;

  MPI_NMAD_TRACE("Unpacking data\n");
  nm_begin_unpacking(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, connection);

  if (p_datatype->dte_type == MPIR_VECTOR) {
    MPI_NMAD_TRACE("Receiving vector type: stride %d - blocklen %d - count %d - size %ld\n", p_datatype->stride, p_datatype->blocklens[0], p_datatype->elements, (long)p_datatype->size);
    err = nm_mpi_datatype_vector_unpack(connection, p_req, p_req->buffer, p_datatype, p_req->count);
  }
  else if (p_datatype->dte_type == MPIR_INDEXED) {
    MPI_NMAD_TRACE("Receiving (h)indexed type: count %d - size %ld\n", p_datatype->elements, (long)p_datatype->size);
    err = nm_mpi_datatype_indexed_unpack(connection, p_req, p_req->buffer, p_datatype, p_req->count);
  }
  else if (p_datatype->dte_type == MPIR_STRUCT) {
    MPI_NMAD_TRACE("Receiving struct type: size %ld\n", (long)p_datatype->size);
    err = nm_mpi_datatype_struct_unpack(connection, p_req, p_req->buffer, p_datatype, p_req->count);
  }
  return err;
}

int mpir_irecv_start(nm_mpi_request_t *p_req)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  p_datatype->active_communications ++;
  if(p_datatype->is_contig || !(p_datatype->is_optimized))
    {
      p_req->request_error = mpir_irecv_wrapper(p_req);
      if(p_datatype->is_contig)
	{
	  MPI_NMAD_TRACE("Calling irecv_start for contiguous data\n");
	  if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_RECV;
	}
    }
  else
    {
      p_req->request_error = mpir_unpack_wrapper(p_req);
      if (p_req->request_type != MPI_REQUEST_ZERO) p_req->request_type = MPI_REQUEST_PACK_RECV;
    }

  nm_sr_progress(nm_comm_get_session(p_req->p_comm->p_comm));

  mpir_inc_nb_incoming_msg();
  MPI_NMAD_TRACE("Irecv_start completed\n");
  MPI_NMAD_LOG_OUT();
  return p_req->request_error;
}

int mpir_irecv(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm)
{
  int err;
  err = mpir_irecv_init(p_req, source, p_comm);
  if (err == MPI_SUCCESS) {
    err = mpir_irecv_start(p_req);
  }
  return err;
}

/**
 * Calls the appropriate splitting function based on the given request.
 */
static inline int nm_mpi_datatype_split(nm_mpi_request_t *p_req)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
  int err = MPI_SUCCESS;

  if(p_datatype->dte_type == MPIR_VECTOR) 
    {
      err = nm_mpi_datatype_vector_split(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
    }
  else if(p_datatype->dte_type == MPIR_INDEXED)
    {
      err = nm_mpi_datatype_indexed_split(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
    }
  else if(p_datatype->dte_type == MPIR_STRUCT)
    {
      err = nm_mpi_datatype_struct_split(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
    }
  if(p_req->request_persistent_type == MPI_REQUEST_ZERO)
    {
      FREE_AND_SET_NULL(p_req->contig_buffer);
      p_req->request_type = MPI_REQUEST_ZERO;
    }
  return err;
}

int mpir_start(nm_mpi_request_t *p_req)
 {
  if (p_req->request_persistent_type == MPI_REQUEST_SEND) {
    return mpir_isend_start(p_req);
  }
  else if (p_req->request_persistent_type == MPI_REQUEST_RECV) {
    return mpir_irecv_start(p_req);
  }
  else {
    ERROR("Unknown persistent request type: %d\n", p_req->request_persistent_type);
    return MPI_ERR_INTERN;
  }
}

int mpir_wait(nm_mpi_request_t *p_req)
{
  int err = MPI_SUCCESS;

  MPI_NMAD_TRACE("Waiting for a request %d\n", p_req->request_type);
  if (p_req->request_type == MPI_REQUEST_RECV) {
    MPI_NMAD_TRACE("Calling nm_sr_rwait\n");
    MPI_NMAD_TRACE("Calling nm_sr_rwait for request=%p\n", &(p_req->request_nmad));
    err = nm_sr_rwait(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(p_req->request_datatype);
    if (!p_datatype->is_contig && p_req->contig_buffer) 
      {
	nm_mpi_datatype_split(p_req);
      }
    MPI_NMAD_TRACE("Returning from nm_sr_rwait\n");
  }
  else if (p_req->request_type == MPI_REQUEST_SEND) {
    MPI_NMAD_TRACE("Calling nm_sr_swait\n");
    MPI_NMAD_TRACE("Calling nm_sr_swait\n");
    err = nm_sr_swait(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
    MPI_NMAD_TRACE("Returning from nm_sr_swait\n");
    if (p_req->request_persistent_type == MPI_REQUEST_ZERO) {
      if (p_req->contig_buffer != NULL) {
        FREE_AND_SET_NULL(p_req->contig_buffer);
      }
    }
  }
  else if (p_req->request_type == MPI_REQUEST_PACK_RECV) {
    nm_pack_cnx_t *connection = &(p_req->request_cnx);
    MPI_NMAD_TRACE("Calling nm_end_unpacking\n");
    err = nm_end_unpacking(connection);
    MPI_NMAD_TRACE("Returning from nm_end_unpacking\n");
  }
  else if (p_req->request_type == MPI_REQUEST_PACK_SEND) {
    nm_pack_cnx_t *connection = &(p_req->request_cnx);
    MPI_NMAD_TRACE("Waiting for completion end_packing\n");
    err = nm_end_packing(connection);
    MPI_NMAD_TRACE("Returning from nm_end_packing\n");
  }
  else {
    MPI_NMAD_TRACE("Waiting operation invalid for request type %d\n", p_req->request_type);
  }

  return err;
}

int mpir_test(nm_mpi_request_t *p_req)
 {
  int err = MPI_SUCCESS;
  if (p_req->request_type == MPI_REQUEST_RECV) {
    err = nm_sr_rtest(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
  }
  else if (p_req->request_type == MPI_REQUEST_SEND) {
    err = nm_sr_stest(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
  }
  else if (p_req->request_type == MPI_REQUEST_PACK_RECV) {
    nm_pack_cnx_t *connection = &(p_req->request_cnx);
    err = nm_test_end_unpacking(connection);
  }
  else if (p_req->request_type == MPI_REQUEST_PACK_SEND) {
    nm_pack_cnx_t *connection = &(p_req->request_cnx);
    err = nm_test_end_packing(connection);
  }
  else {
    MPI_NMAD_TRACE("Request type %d invalid\n", p_req->request_type);
  }
  return err;
}

int mpir_cancel(nm_mpi_request_t *p_req)
{
  int err = MPI_SUCCESS;
  if (p_req->request_type == MPI_REQUEST_RECV) {
    mpir_dec_nb_incoming_msg();
    err = nm_sr_rcancel(nm_comm_get_session(p_req->p_comm->p_comm), &p_req->request_nmad);
  }
  else if (p_req->request_type == MPI_REQUEST_SEND) {
    mpir_dec_nb_outgoing_msg();
  }
  else {
    MPI_NMAD_TRACE("Request type %d incorrect\n", p_req->request_type);
  }

  return err;
}

int mpir_op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
  if(puk_int_vect_empty(nm_mpi_internal_data.operators_pool))
    {
      ERROR("Maximum number of operations created");
      return MPI_ERR_INTERN;
    }
  else
    {
      *op = puk_int_vect_pop_back(nm_mpi_internal_data.operators_pool);
      
      nm_mpi_internal_data.operators[*op] = malloc(sizeof(mpir_operator_t));
      nm_mpi_internal_data.operators[*op]->function = function;
      nm_mpi_internal_data.operators[*op]->commute = commute;
      return MPI_SUCCESS;
    }
}

int mpir_op_free(MPI_Op *op)
{
  if (*op > NUMBER_OF_OPERATORS || nm_mpi_internal_data.operators[*op] == NULL) 
    {
      ERROR("Operator %d unknown\n", *op);
      return MPI_ERR_OTHER;
    }
  else
    {
      FREE_AND_SET_NULL(nm_mpi_internal_data.operators[*op]);
      puk_int_vect_push_back(nm_mpi_internal_data.operators_pool, *op);
      *op = MPI_OP_NULL;
      return MPI_SUCCESS;
    }
}

mpir_operator_t *mpir_get_operator(MPI_Op op)
 {
  if (nm_mpi_internal_data.operators[op] != NULL) {
    return nm_mpi_internal_data.operators[op];
  }
  else {
    ERROR("Operation %d unknown", op);
    return NULL;
  }
}


tbx_bool_t mpir_test_termination(MPI_Comm comm)
{
  int process_rank, global_size;
  mpi_comm_rank(comm, &process_rank);
  mpi_comm_size(comm, &global_size);
  int tag = 31;

  if (process_rank == 0) {
    // 1st phase
    int global_nb_incoming_msg = 0;
    int global_nb_outgoing_msg = 0;
    int i, remote_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    global_nb_incoming_msg = nm_mpi_internal_data.nb_incoming_msg;
    global_nb_outgoing_msg = nm_mpi_internal_data.nb_outgoing_msg;
    MPI_NMAD_TRACE("Local counters [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);
    for(i=1 ; i<global_size ; i++) {
      mpi_recv(remote_counters, 2, MPI_INT, i, tag, comm, MPI_STATUS_IGNORE);
      global_nb_incoming_msg += remote_counters[0];
      global_nb_outgoing_msg += remote_counters[1];
      MPI_NMAD_TRACE("Remote counters [incoming msg=%d, outgoing msg=%d]\n", remote_counters[0], remote_counters[1]);
    }
    MPI_NMAD_TRACE("Global counters [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);

    tbx_bool_t answer = tbx_true;
    if (global_nb_incoming_msg != global_nb_outgoing_msg) {
      answer = tbx_false;
    }

    for(i=1 ; i<global_size ; i++) {
      mpi_send(&answer, 1, MPI_INT, i, tag, comm);
    }

    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      global_nb_incoming_msg = nm_mpi_internal_data.nb_incoming_msg;
      global_nb_outgoing_msg = nm_mpi_internal_data.nb_outgoing_msg;
      MPI_NMAD_TRACE("Local counters [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);
      for(i=1 ; i<global_size ; i++) {
        mpi_recv(remote_counters, 2, MPI_INT, i, tag, comm, MPI_STATUS_IGNORE);
        global_nb_incoming_msg += remote_counters[0];
        global_nb_outgoing_msg += remote_counters[1];
        MPI_NMAD_TRACE("Remote counters [incoming msg=%d, outgoing msg=%d]\n", remote_counters[0], remote_counters[1]);
      }

      MPI_NMAD_TRACE("Global counters [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);
      answer = tbx_true;
      if (global_nb_incoming_msg != global_nb_outgoing_msg) {
        answer = tbx_false;
      }

      for(i=1 ; i<global_size ; i++) {
        mpi_send(&answer, 1, MPI_INT, i, tag, comm);
      }

      return answer;
    }
  }
  else {
    int answer, local_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    local_counters[0] = nm_mpi_internal_data.nb_incoming_msg;
    local_counters[1] = nm_mpi_internal_data.nb_outgoing_msg;
    mpi_send(local_counters, 2, MPI_INT, 0, tag, comm);
    MPI_NMAD_TRACE("Local counters [incoming msg=%d, outgoing msg=%d]\n", local_counters[0], local_counters[1]);

    mpi_recv(&answer, 1, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      local_counters[0] = nm_mpi_internal_data.nb_incoming_msg;
      local_counters[1] = nm_mpi_internal_data.nb_outgoing_msg;
      mpi_send(local_counters, 2, MPI_INT, 0, tag, comm);
      MPI_NMAD_TRACE("Local counters [incoming msg=%d, outgoing msg=%d]\n", local_counters[0], local_counters[1]);

      mpi_recv(&answer, 1, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
      return answer;
    }
  }
}

