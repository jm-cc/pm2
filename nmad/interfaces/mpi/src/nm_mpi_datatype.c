/*
 * NewMadeleine
 * Copyright (C) 2014 (see AUTHORS file)
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
#include <assert.h>
#include <errno.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);

NM_MPI_HANDLE_TYPE(datatype, nm_mpi_datatype_t, _NM_MPI_DATATYPE_OFFSET, 64);

static struct nm_mpi_handle_datatype_s nm_mpi_datatypes;


static int nm_mpi_datatype_indexed(int count, int *array_of_blocklengths, MPI_Aint*array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype);
/** store builtin datatypes */
static void nm_mpi_datatype_store(int id, size_t size, int elements);
static void nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype);


/* ********************************************************* */

NM_MPI_ALIAS(MPI_Type_size,           mpi_type_size);
NM_MPI_ALIAS(MPI_Type_get_extent,     mpi_type_get_extent);
NM_MPI_ALIAS(MPI_Type_extent,         mpi_type_extent);
NM_MPI_ALIAS(MPI_Type_lb,             mpi_type_lb);
NM_MPI_ALIAS(MPI_Type_ub,             mpi_type_ub);
NM_MPI_ALIAS(MPI_Type_create_resized, mpi_type_create_resized);
NM_MPI_ALIAS(MPI_Type_commit,         mpi_type_commit);
NM_MPI_ALIAS(MPI_Type_free,           mpi_type_free);
NM_MPI_ALIAS(MPI_Type_optimized,      mpi_type_optimized);
NM_MPI_ALIAS(MPI_Type_contiguous,     mpi_type_contiguous);
NM_MPI_ALIAS(MPI_Type_vector,         mpi_type_vector);
NM_MPI_ALIAS(MPI_Type_hvector,        mpi_type_hvector);
NM_MPI_ALIAS(MPI_Type_indexed,        mpi_type_indexed);
NM_MPI_ALIAS(MPI_Type_hindexed,       mpi_type_hindexed);
NM_MPI_ALIAS(MPI_Type_struct,         mpi_type_struct);
NM_MPI_ALIAS(MPI_Pack,                mpi_pack);
NM_MPI_ALIAS(MPI_Unpack,              mpi_unpack);
NM_MPI_ALIAS(MPI_Pack_size,           mpi_pack_size);

/* ********************************************************* */


__PUK_SYM_INTERNAL
void nm_mpi_datatype_init(void)
{
  nm_mpi_handle_datatype_init(&nm_mpi_datatypes);

  /* Initialise the basic datatypes */

  /* C types */
  nm_mpi_datatype_store(MPI_CHAR,               sizeof(char), 1);
  nm_mpi_datatype_store(MPI_UNSIGNED_CHAR,      sizeof(unsigned char), 1);
  nm_mpi_datatype_store(MPI_SIGNED_CHAR,        sizeof(signed char), 1);
  nm_mpi_datatype_store(MPI_BYTE,               1, 1);
  nm_mpi_datatype_store(MPI_SHORT,              sizeof(signed short), 1);
  nm_mpi_datatype_store(MPI_UNSIGNED_SHORT,     sizeof(unsigned short), 1);
  nm_mpi_datatype_store(MPI_INT,                sizeof(signed int), 1);
  nm_mpi_datatype_store(MPI_UNSIGNED,           sizeof(unsigned int), 1);
  nm_mpi_datatype_store(MPI_LONG,               sizeof(signed long), 1);
  nm_mpi_datatype_store(MPI_UNSIGNED_LONG,      sizeof(unsigned long), 1);
  nm_mpi_datatype_store(MPI_FLOAT,              sizeof(float), 1);
  nm_mpi_datatype_store(MPI_DOUBLE,             sizeof(double), 1);
  nm_mpi_datatype_store(MPI_LONG_DOUBLE,        sizeof(long double), 1);
  nm_mpi_datatype_store(MPI_LONG_LONG_INT,      sizeof(long long int), 1);
  nm_mpi_datatype_store(MPI_UNSIGNED_LONG_LONG, sizeof(unsigned long long int), 1);

  /* FORTRAN types */
  nm_mpi_datatype_store(MPI_LOGICAL,            sizeof(float), 1);
  nm_mpi_datatype_store(MPI_REAL,               sizeof(float), 1);
  nm_mpi_datatype_store(MPI_REAL4,              4, 1);
  nm_mpi_datatype_store(MPI_REAL8,              8, 1);
  nm_mpi_datatype_store(MPI_REAL16,             16, 1);
  nm_mpi_datatype_store(MPI_DOUBLE_PRECISION,   sizeof(double), 1);
  nm_mpi_datatype_store(MPI_INTEGER,            sizeof(float), 1);
  nm_mpi_datatype_store(MPI_INTEGER1,           1, 1);
  nm_mpi_datatype_store(MPI_INTEGER2,           2, 1);
  nm_mpi_datatype_store(MPI_INTEGER4,           4, 1);
  nm_mpi_datatype_store(MPI_INTEGER8,           8, 1);
  nm_mpi_datatype_store(MPI_PACKED,             sizeof(char), 1);

  /* C struct types */
  nm_mpi_datatype_store(MPI_2INT,               sizeof(int) + sizeof(int), 2);
  nm_mpi_datatype_store(MPI_SHORT_INT,          sizeof(short) + sizeof(int), 2);
  nm_mpi_datatype_store(MPI_LONG_INT,           sizeof(long) + sizeof(int), 2);
  nm_mpi_datatype_store(MPI_FLOAT_INT,          sizeof(float) + sizeof(int), 2);
  nm_mpi_datatype_store(MPI_DOUBLE_INT,         sizeof(double) + sizeof(int), 2);

  /* FORTRAN struct types */
  nm_mpi_datatype_store(MPI_2INTEGER,           sizeof(float) + sizeof(float), 2);
  nm_mpi_datatype_store(MPI_2REAL,              sizeof(float) + sizeof(float), 2);
  nm_mpi_datatype_store(MPI_2DOUBLE_PRECISION,  sizeof(double) + sizeof(double), 2);
  
  /* TODO- Fortran types: 2COMPLEX, 2DOUBLE_COMPLEX; check mapping for: COMPLEX, DOUBLE_COMPLEX. */

  nm_mpi_datatype_store(MPI_COMPLEX,            2 * sizeof(float), 2);
  nm_mpi_datatype_store(MPI_DOUBLE_COMPLEX,     2 * sizeof(double), 2);

  nm_mpi_datatype_store(MPI_UB,                 0, 0);
  nm_mpi_datatype_store(MPI_LB,                 0, 0);
}

__PUK_SYM_INTERNAL
void nm_mpi_datatype_exit(void)
{
  nm_mpi_handle_datatype_finalize(&nm_mpi_datatypes);
}

/* ********************************************************* */

/** store a basic datatype */
static void nm_mpi_datatype_store(int id, size_t size, int elements)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_handle_datatype_store(&nm_mpi_datatypes, id);
  p_datatype->committed = 1;
  p_datatype->is_contig = 1;
  p_datatype->combiner = MPI_COMBINER_NAMED;
  p_datatype->refcount = 2;
  p_datatype->lb = 0;
  p_datatype->size = size;
  p_datatype->elements = elements;
  p_datatype->extent = size;
}

/** allocate a new datatype and init with default values */
static nm_mpi_datatype_t*nm_mpi_datatype_alloc(nm_mpi_type_combiner_t combiner, size_t size, int elements)
{
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  p_newtype->combiner = combiner;
  p_newtype->committed = 0;
  p_newtype->size = size;
  p_newtype->elements = elements;
  p_newtype->lb = 0;
  p_newtype->extent = size;
  p_newtype->refcount = 1;
  p_newtype->is_contig = 0;
  return p_newtype;
}

/* ********************************************************* */

int mpi_type_size(MPI_Datatype datatype, int *size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *size = p_datatype->size;
  return MPI_SUCCESS;
}

int mpi_type_get_extent(MPI_Datatype datatype, MPI_Aint*lb, MPI_Aint*extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(lb != NULL)
    *lb = p_datatype->lb;
  if(extent != NULL)
    *extent = p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_extent(MPI_Datatype datatype, MPI_Aint*extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *extent = p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_lb(MPI_Datatype datatype,MPI_Aint*lb)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *lb = p_datatype->lb;
  return MPI_SUCCESS;
}
int mpi_type_ub(MPI_Datatype datatype, MPI_Aint*displacement)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *displacement = p_datatype->extent; /* UB is extent since we support only LB=0 */
  return MPI_SUCCESS;
}

int mpi_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype)
{
  if(lb != 0)
    {
      ERROR("madmpi: lb != 0 not supported.");
      return MPI_ERR_INTERN;
    }
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_RESIZED, p_oldtype->size, 1);
  *newtype = p_newtype->id;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->extent    = extent;
  p_newtype->lb        = lb;
  p_newtype->RESIZED.p_old_type = p_oldtype;
  return MPI_SUCCESS;
}

int mpi_type_commit(MPI_Datatype *datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  p_datatype->committed = 1;
  p_datatype->is_optimized = 0;
  return MPI_SUCCESS;
}

int mpi_type_free(MPI_Datatype *datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  p_datatype->refcount--;
  *datatype = MPI_DATATYPE_NULL;
  if(p_datatype->refcount > 0)
    {
      return MPI_ERR_DATATYPE_ACTIVE;
    }
  else
    {
      nm_mpi_datatype_free(p_datatype);
      return MPI_SUCCESS;
    }
}

int mpi_type_optimized(MPI_Datatype *datatype, int optimized)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  p_datatype->is_optimized = 0;
  return MPI_SUCCESS;
}

int mpi_type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_CONTIGUOUS, p_oldtype->size * count, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->extent = p_oldtype->extent * count;
  p_newtype->CONTIGUOUS.p_old_type = p_oldtype;
  return MPI_SUCCESS;
}

int mpi_type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_old_type = nm_mpi_datatype_get(oldtype);
  const int hstride = stride * nm_mpi_datatype_size(p_old_type);
  return mpi_type_hvector(count, blocklength, hstride, oldtype, newtype);
}

int mpi_type_hvector(int count, int blocklength, int hstride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_VECTOR, p_oldtype->size * count * blocklength, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->extent = p_oldtype->extent * blocklength + (count - 1) * hstride;
  p_newtype->VECTOR.p_old_type = p_oldtype;
  p_newtype->VECTOR.hstride = hstride;
  p_newtype->VECTOR.blocklength = blocklength;
  return MPI_SUCCESS;
}

int mpi_type_indexed(int count, int *array_of_blocklengths, int *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  nm_mpi_datatype_t*p_old_type = nm_mpi_datatype_get(oldtype);
  const int old_size = nm_mpi_datatype_size(p_old_type);
  int i;
  for(i = 0; i < count; i++)
    {
      displacements[i] = array_of_displacements[i] * old_size;
    }
  int err = nm_mpi_datatype_indexed(count, array_of_blocklengths, displacements, oldtype, newtype);
  FREE_AND_SET_NULL(displacements);
  return err;
}

int mpi_type_hindexed(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype) 
{
  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  int i;
  for(i = 0; i < count; i++)
    {
      displacements[i] = array_of_displacements[i];
    }
  int err = nm_mpi_datatype_indexed(count, array_of_blocklengths, displacements, oldtype, newtype);
  FREE_AND_SET_NULL(displacements);
  return err;
}

int mpi_type_struct(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype *array_of_types, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_STRUCT, 0, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->extent = -1;
  p_newtype->STRUCT.p_map = malloc(count * sizeof(struct nm_mpi_type_struct_map_s));
  for(i = 0; i < count; i++)
    {
      nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(array_of_types[i]);
      if(p_datatype == NULL)
	{
	  nm_mpi_handle_datatype_free(&nm_mpi_datatypes, p_datatype);
	  *newtype = MPI_DATATYPE_NULL;
	  return MPI_ERR_TYPE;
	}
      p_newtype->STRUCT.p_map[i].p_old_type   = p_datatype;
      p_newtype->STRUCT.p_map[i].blocklength  = array_of_blocklengths[i];
      p_newtype->STRUCT.p_map[i].displacement = array_of_displacements[i];
      p_newtype->size += p_newtype->STRUCT.p_map[i].blocklength * p_datatype->size;
      if(array_of_types[i] == MPI_UB)
	{
	  p_newtype->extent = array_of_displacements[i];
	  break;
	}
      if(array_of_types[i] == MPI_LB && array_of_displacements[i] != 0)
	{
	  ERROR("madmpi: non-zero MPI_LB not supported.\n");
	  break;
	}
    }
  /** We suppose here that the last field of the struct does not need
   * an alignment. In case, one sends an array of struct, the 1st
   * field of the 2nd struct immediatly follows the last field of the
   * previous struct.
   */
  if(p_newtype->extent == -1)
    p_newtype->extent = p_newtype->STRUCT.p_map[count - 1].displacement + 
      p_newtype->STRUCT.p_map[count - 1].blocklength * p_newtype->STRUCT.p_map[count - 1].p_old_type->extent;
  return MPI_SUCCESS;
}


__PUK_SYM_INTERNAL
size_t nm_mpi_datatype_size(nm_mpi_datatype_t*p_datatype)
{
  return p_datatype->size;
}

__PUK_SYM_INTERNAL
nm_mpi_datatype_t* nm_mpi_datatype_get(MPI_Datatype datatype)
{
  return nm_mpi_handle_datatype_get(&nm_mpi_datatypes, datatype);
}

__PUK_SYM_INTERNAL
int nm_mpi_datatype_unlock(nm_mpi_datatype_t*p_datatype)
{
  p_datatype->refcount--;
  assert(p_datatype->refcount >= 0);
  if(p_datatype->refcount == 0)
    {
      nm_mpi_datatype_free(p_datatype);
    }
  return MPI_SUCCESS;
}

static void nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype)
{
  assert(p_datatype->refcount == 0);
  assert(p_datatype->id >= _NM_MPI_DATATYPE_OFFSET);
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_INDEXED:
      if(p_datatype->INDEXED.p_map != NULL)
	FREE_AND_SET_NULL(p_datatype->INDEXED.p_map);
      break;
    case MPI_COMBINER_STRUCT:
      if(p_datatype->STRUCT.p_map != NULL)
	FREE_AND_SET_NULL(p_datatype->STRUCT.p_map);
      break;
    default:
      break;
    }
  nm_mpi_handle_datatype_free(&nm_mpi_datatypes, p_datatype);
}


static int nm_mpi_datatype_indexed(int count, int*array_of_blocklengths, MPI_Aint*array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_INDEXED, 0, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->INDEXED.p_old_type = p_oldtype;
  p_newtype->INDEXED.p_map = malloc(count * sizeof(struct nm_mpi_type_indexed_map_s));
  for(i = 0; i < count ; i++)
    {
      p_newtype->INDEXED.p_map[i].blocklength = array_of_blocklengths[i];
      p_newtype->INDEXED.p_map[i].displacement = array_of_displacements[i];
      p_newtype->size += p_oldtype->size * array_of_blocklengths[i];
    }
  p_newtype->extent = (p_newtype->INDEXED.p_map[count - 1].displacement + p_oldtype->extent * p_newtype->INDEXED.p_map[count - 1].blocklength);
  return MPI_SUCCESS;
}

static void nm_mpi_datatype_filter_apply(const struct nm_mpi_datatype_filter_s*filter, const void*data_ptr, nm_mpi_datatype_t*p_datatype, int count)
{
  if(p_datatype->is_contig)
    {
      assert(p_datatype->lb == 0);
      (*filter->apply)(filter->_status, data_ptr, count * p_datatype->size);
    }
  else
    {
      int i, j;
      for(i = 0; i < count; i++)
	{
	  switch(p_datatype->combiner)
	    {
	    case MPI_COMBINER_RESIZED:
	      nm_mpi_datatype_filter_apply(filter, data_ptr, p_datatype->RESIZED.p_old_type, 1);
	      break;
	      
	    case MPI_COMBINER_CONTIGUOUS:
	      nm_mpi_datatype_filter_apply(filter, data_ptr, p_datatype->CONTIGUOUS.p_old_type, p_datatype->elements);
	      break;
	      
	    case MPI_COMBINER_VECTOR:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  nm_mpi_datatype_filter_apply(filter, data_ptr + j * p_datatype->VECTOR.hstride,
					       p_datatype->VECTOR.p_old_type, p_datatype->VECTOR.blocklength);
		}
	      break;
	      
	    case MPI_COMBINER_INDEXED:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  nm_mpi_datatype_filter_apply(filter, data_ptr + p_datatype->INDEXED.p_map[j].displacement, 
					       p_datatype->INDEXED.p_old_type, p_datatype->INDEXED.p_map[j].blocklength);
		}
	      break;
	      
	    case MPI_COMBINER_STRUCT:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  nm_mpi_datatype_filter_apply(filter, data_ptr + p_datatype->STRUCT.p_map[j].displacement,
					       p_datatype->STRUCT.p_map[j].p_old_type, p_datatype->STRUCT.p_map[j].blocklength);
		}
	      break;
	      
	    case MPI_COMBINER_NAMED:
	      {
		ERROR("madmpi: trying to apply filter to NAMED datatype as sparse (should be contiguous).\n");
	      }
	      break;
	      
	    default:
	      ERROR("madmpi: cannot filter datatype with combiner %d\n", p_datatype->combiner);
	    }
	  data_ptr += p_datatype->extent;
	}
    }
}

/** status for nm_mpi_datatype_*_memcpy */
struct nm_mpi_datatype_filter_memcpy_s
{
  void*pack_ptr;
};
/** pack data to memory */
static void nm_mpi_datatype_pack_memcpy(void*_status, void*data_ptr, int size)
{
  struct nm_mpi_datatype_filter_memcpy_s*status = _status;
  memcpy(status->pack_ptr, data_ptr, size);
  status->pack_ptr += size;
}
/** unpack data from memory */
static void nm_mpi_datatype_unpack_memcpy(void*_status, void*data_ptr, int size)
{
  struct nm_mpi_datatype_filter_memcpy_s*status = _status;
  memcpy(data_ptr, status->pack_ptr, size);
  status->pack_ptr += size;
}

/**
 * Pack data into a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_pack(void*pack_ptr, const void*data_ptr, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s status = { .pack_ptr = (void*)pack_ptr };
  const struct nm_mpi_datatype_filter_s filter = { .apply = &nm_mpi_datatype_pack_memcpy, &status };
  nm_mpi_datatype_filter_apply(&filter, (void*)data_ptr, p_datatype, count);
}

/**
 * Unpack data from a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_unpack(const void*src_ptr, void*dest_ptr, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s status = { .pack_ptr = (void*)src_ptr };
  const struct nm_mpi_datatype_filter_s filter = { .apply = &nm_mpi_datatype_unpack_memcpy, &status };
  nm_mpi_datatype_filter_apply(&filter, (void*)dest_ptr, p_datatype, count);
}


int mpi_pack(void*inbuf, int incount, MPI_Datatype datatype, void*outbuf, int outsize, int*position, MPI_Comm comm)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  size_t size = incount * nm_mpi_datatype_size(p_datatype);
  assert(outsize >= size);
  nm_mpi_datatype_pack(outbuf + *position, inbuf, p_datatype, incount);
  *position += size;
  return MPI_SUCCESS;
}

int mpi_unpack(void*inbuf, int insize, int*position, void*outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  size_t size = outcount * nm_mpi_datatype_size(p_datatype);
  assert(insize >= size);
  nm_mpi_datatype_unpack(inbuf + *position, outbuf, p_datatype, outcount);
  *position += size;
  return MPI_SUCCESS;
}

int mpi_pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int*size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *size = incount * nm_mpi_datatype_size(p_datatype);
  return MPI_SUCCESS;
}

