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


int mpir_internal_init(void)
{
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

  return MPI_SUCCESS;
}

int mpir_internal_exit(void)
{
  return MPI_SUCCESS;
}

__PUK_SYM_INTERNAL
nm_tag_t nm_mpi_get_tag(nm_mpi_communicator_t *p_comm, int tag)
{
  const nm_tag_t newtag = tag;
  return newtag;
}
__PUK_SYM_INTERNAL
int nm_mpi_check_tag(int tag)
{
  if(tag < 0 || tag > NM_MPI_TAG_MAX)
    {
      ERROR("tag too large");
    }
  return tag;
}


/**
 * Aggregates data represented by a vector datatype in a contiguous
 * buffer.
 */
static inline int nm_mpi_datatype_vector_aggregate(void *newptr, void *buffer, nm_mpi_datatype_t*p_datatype, int count)
{
  int i, j;
  for(i = 0; i < count; i++)
    {
      for(j = 0; j < p_datatype->elements; j++)
	{
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
  int i, j, err = MPI_SUCCESS;
  for(i = 0 ; i<count ; i++)
    {
      for(j = 0; j < p_datatype->elements; j++)
	{
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
  if(p_req->request_type != NM_MPI_REQUEST_ZERO) p_req->request_type = NM_MPI_REQUEST_PACK_RECV;
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
  int i, j;
  for(i = 0; i < count; i++)
    {
      void *ptr = buffer + i * p_datatype->extent;
      for(j = 0; j < p_datatype->elements; j++)
	{
	  void *subptr = ptr + p_datatype->indices[j];
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
  if(p_req->request_type != NM_MPI_REQUEST_ZERO) p_req->request_type = NM_MPI_REQUEST_PACK_RECV;
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
  if(p_req->request_type != NM_MPI_REQUEST_ZERO)
    p_req->request_type = NM_MPI_REQUEST_PACK_RECV;
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

__PUK_SYM_INTERNAL
int nm_mpi_isend_init(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm)
{
  nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
  int err = MPI_SUCCESS;
  nm_gate_t p_gate = nm_mpi_communicator_get_gate(p_comm, dest);
  if(p_gate == NULL)
    {
      TBX_FAILUREF("Cannot find rank %d in comm %p.\n", dest, p_comm);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  p_req->gate = p_gate;
  p_req->request_tag = nm_mpi_get_tag(p_comm, p_req->user_tag);

  if(p_datatype->is_contig == 1)
    {
      /* nothing to do */
    }
  else if(p_datatype->dte_type == MPIR_VECTOR) 
    {
      if(!p_datatype->is_optimized) 
	{
	  p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
	  if(p_req->contig_buffer == NULL)
	    {
	      ERROR("Cannot allocate memory with size %ld to send struct datatype\n", (long)(p_req->count * p_datatype->size));
	      return MPI_ERR_INTERN;
	    }
	  nm_mpi_datatype_vector_aggregate(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
	}
    }
  else if(p_datatype->dte_type == MPIR_INDEXED)
    {
      if(!p_datatype->is_optimized)
	{
	  p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
	  if(p_req->contig_buffer == NULL)
	    {
	      ERROR("Cannot allocate memory with size %ld to send (h)indexed type\n", (long)(p_req->count * p_datatype->size));
	      return MPI_ERR_INTERN;
	    }
	  nm_mpi_datatype_indexed_aggregate(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
	}
    }
  else if(p_datatype->dte_type == MPIR_STRUCT)
    {
      if(!p_datatype->is_optimized)
	{
	  p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
	  if(p_req->contig_buffer == NULL)
	    {
	      ERROR("Cannot allocate memory with size %ld to send struct datatype\n", (long)(p_req->count * p_datatype->size));
	      return MPI_ERR_INTERN;
	    }
	  nm_mpi_datatype_struct_aggregate(p_req->contig_buffer, p_req->buffer, p_datatype, p_req->count);
	}
    }
  else
    {
      ERROR("Do not know how to send datatype %d\n", p_datatype->dte_type);
      return MPI_ERR_INTERN;
    }
  return err;
}

__PUK_SYM_INTERNAL
int nm_mpi_isend_start(nm_mpi_request_t *p_req)
{
  int err = MPI_SUCCESS;
  nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
  p_datatype->active_communications ++;
  if(p_datatype->is_contig || !(p_datatype->is_optimized))
    {
      void *buffer = (p_datatype->is_contig == 1) ? p_req->buffer : p_req->contig_buffer;
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
	  ERROR("madmpi: synchronous mode not supported yet.\n");
	  break;
	default:
	  ERROR("madmpi: unkown mode %d for isend", p_req->communication_mode);
	  break;
	}
      if(p_req->request_type != NM_MPI_REQUEST_ZERO)
	p_req->request_type = NM_MPI_REQUEST_SEND;
    }
  else
    {
      nm_pack_cnx_t*connection = &(p_req->request_cnx);
      nm_begin_packing(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, connection);
      if(p_datatype->dte_type == MPIR_VECTOR)
	{
	  err = nm_mpi_datatype_vector_pack(connection, p_req->buffer, p_datatype, p_req->count);
	}
      else if(p_datatype->dte_type == MPIR_INDEXED) 
	{
	  err = nm_mpi_datatype_indexed_pack(connection, p_req->buffer, p_datatype, p_req->count);
	}
      else if(p_datatype->dte_type == MPIR_STRUCT)
	{
	  err = nm_mpi_datatype_struct_pack(connection, p_req->buffer, p_datatype, p_req->count);
	}
      if(p_req->request_type != NM_MPI_REQUEST_ZERO) 
	p_req->request_type = NM_MPI_REQUEST_PACK_SEND;
    }
  err = nm_sr_progress(nm_comm_get_session(p_req->p_comm->p_comm));
  mpir_inc_nb_outgoing_msg();

  return err;
}

__PUK_SYM_INTERNAL
int nm_mpi_isend(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm)
{
  int err;
  err = nm_mpi_isend_init(p_req, dest, p_comm);
  if(err == MPI_SUCCESS)
    {
      err = nm_mpi_isend_start(p_req);
    }
  return err;
}


__PUK_SYM_INTERNAL
int nm_mpi_irecv_init(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm)
{
  nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
  if(tbx_unlikely(source == MPI_ANY_SOURCE)) 
    {
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
  p_req->request_tag = nm_mpi_get_tag(p_comm, p_req->user_tag);
  p_req->request_source = source;

  if(p_datatype->is_contig == 1)
    {
      if(p_req->request_type != NM_MPI_REQUEST_ZERO) 
	p_req->request_type = NM_MPI_REQUEST_RECV;
    }
  else if(p_datatype->dte_type == MPIR_VECTOR)
    {
      if(!p_datatype->is_optimized)
	{
	  p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
	  if(p_req->contig_buffer == NULL)
	    {
	      ERROR("Cannot allocate memory with size %ld to receive vector type\n", (long)(p_req->count * p_datatype->size));
	      return MPI_ERR_INTERN;
	    }
	  if(p_req->request_type != NM_MPI_REQUEST_ZERO) 
	    p_req->request_type = NM_MPI_REQUEST_RECV;
	}
    }
  else if(p_datatype->dte_type == MPIR_INDEXED)
    {
      if(!p_datatype->is_optimized)
	{
	  p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
	  if(p_req->contig_buffer == NULL)
	    {
	      ERROR("Cannot allocate memory with size %ld to receive (h)indexed type\n", (long)(p_req->count * p_datatype->size));
	      return MPI_ERR_INTERN;
	    }
	  if(p_req->request_type != NM_MPI_REQUEST_ZERO)
	    p_req->request_type = NM_MPI_REQUEST_RECV;
	}
    }
  else if(p_datatype->dte_type == MPIR_STRUCT)
    {
      if(!p_datatype->is_optimized)
	{
	  p_req->contig_buffer = malloc(p_req->count * p_datatype->size);
	  if(p_req->contig_buffer == NULL)
	    {
	      ERROR("Cannot allocate memory with size %ld to receive struct type\n", (long)(p_req->count * p_datatype->size));
	      return MPI_ERR_INTERN;
	    }
	  if(p_req->request_type != NM_MPI_REQUEST_ZERO)
	    p_req->request_type = NM_MPI_REQUEST_RECV;
	}
    }
  else
    {
      ERROR("Do not know how to receive datatype %p\n", p_req->p_datatype);
      return MPI_ERR_INTERN;
    }
  return MPI_SUCCESS;
}



__PUK_SYM_INTERNAL
int nm_mpi_irecv_start(nm_mpi_request_t *p_req)
{
  nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
  p_datatype->active_communications ++;
  if(p_datatype->is_contig || !(p_datatype->is_optimized))
    {
      void *buffer = (p_datatype->is_contig == 1) ? p_req->buffer : p_req->contig_buffer;
      p_req->request_error = nm_sr_irecv(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, buffer, p_req->count * p_datatype->size, &(p_req->request_nmad));
      if(p_datatype->is_contig)
	{
	  MPI_NMAD_TRACE("Calling irecv_start for contiguous data\n");
	  if(p_req->request_type != NM_MPI_REQUEST_ZERO) p_req->request_type = NM_MPI_REQUEST_RECV;
	}
    }
  else
    {
      nm_pack_cnx_t*connection = &(p_req->request_cnx);
      int err = NM_ESUCCESS;
      nm_begin_unpacking(nm_comm_get_session(p_req->p_comm->p_comm), p_req->gate, p_req->request_tag, connection);
      if(p_datatype->dte_type == MPIR_VECTOR)
	{
	  err = nm_mpi_datatype_vector_unpack(connection, p_req, p_req->buffer, p_datatype, p_req->count);
	}
      else if(p_datatype->dte_type == MPIR_INDEXED)
	{
	  err = nm_mpi_datatype_indexed_unpack(connection, p_req, p_req->buffer, p_datatype, p_req->count);
	}
      else if(p_datatype->dte_type == MPIR_STRUCT)
	{
	  err = nm_mpi_datatype_struct_unpack(connection, p_req, p_req->buffer, p_datatype, p_req->count);
	}
      p_req->request_error = err;
      if(p_req->request_type != NM_MPI_REQUEST_ZERO)
	p_req->request_type = NM_MPI_REQUEST_PACK_RECV;
    }
  nm_sr_progress(nm_comm_get_session(p_req->p_comm->p_comm));
  mpir_inc_nb_incoming_msg();
  return p_req->request_error;
}

__PUK_SYM_INTERNAL
int nm_mpi_irecv(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm)
{
  int err;
  err = nm_mpi_irecv_init(p_req, source, p_comm);
  if(err == MPI_SUCCESS)
    {
      err = nm_mpi_irecv_start(p_req);
    }
  return err;
}

/**
 * Calls the appropriate splitting function based on the given request.
 */
__PUK_SYM_INTERNAL
int nm_mpi_datatype_split(nm_mpi_request_t *p_req)
{
  nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
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
  if(p_req->request_persistent_type == NM_MPI_REQUEST_ZERO)
    {
      FREE_AND_SET_NULL(p_req->contig_buffer);
      p_req->request_type = NM_MPI_REQUEST_ZERO;
    }
  return err;
}


tbx_bool_t mpir_test_termination(MPI_Comm comm)
{
  int process_rank, global_size;
  mpi_comm_rank(comm, &process_rank);
  mpi_comm_size(comm, &global_size);
  int tag = 31;

  if(process_rank == 0) {
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
    if(global_nb_incoming_msg != global_nb_outgoing_msg) {
      answer = tbx_false;
    }

    for(i=1 ; i<global_size ; i++) {
      mpi_send(&answer, 1, MPI_INT, i, tag, comm);
    }

    if(answer == tbx_false) {
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
      if(global_nb_incoming_msg != global_nb_outgoing_msg) {
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
    if(answer == tbx_false) {
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

