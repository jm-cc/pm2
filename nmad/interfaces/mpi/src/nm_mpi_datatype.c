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
NM_MPI_ALIAS(MPI_Type_contiguous,     mpi_type_contiguous);
NM_MPI_ALIAS(MPI_Type_vector,         mpi_type_vector);
NM_MPI_ALIAS(MPI_Type_hvector,        mpi_type_hvector);
NM_MPI_ALIAS(MPI_Type_create_hvector, mpi_type_hvector);
NM_MPI_ALIAS(MPI_Type_indexed,        mpi_type_indexed);
NM_MPI_ALIAS(MPI_Type_hindexed,       mpi_type_hindexed);
NM_MPI_ALIAS(MPI_Type_create_hindexed, mpi_type_create_hindexed);
NM_MPI_ALIAS(MPI_Type_struct,         mpi_type_struct);
NM_MPI_ALIAS(MPI_Type_create_struct,  mpi_type_create_struct);
NM_MPI_ALIAS(MPI_Type_get_envelope,   mpi_type_get_envelope);
NM_MPI_ALIAS(MPI_Type_get_contents,   mpi_type_get_contents);
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
  nm_mpi_datatype_store(MPI_CHARACTER,          sizeof(char), 1);
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
  p_newtype->is_contig = (p_oldtype->is_contig && (lb == 0) && (extent == p_oldtype->size));
  p_newtype->extent    = extent;
  p_newtype->lb        = lb;
  p_newtype->RESIZED.p_old_type = p_oldtype;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_commit(MPI_Datatype *datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  p_datatype->committed = 1;
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

int mpi_type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_CONTIGUOUS, p_oldtype->size * count, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->extent = p_oldtype->extent * count;
  p_newtype->CONTIGUOUS.p_old_type = p_oldtype;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_VECTOR, p_oldtype->size * count * blocklength, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->extent = p_oldtype->extent * blocklength + (count - 1) * stride * p_oldtype->size;
  p_newtype->VECTOR.p_old_type = p_oldtype;
  p_newtype->VECTOR.stride = stride;
  p_newtype->VECTOR.blocklength = blocklength;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_hvector(int count, int blocklength, MPI_Aint hstride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_HVECTOR, p_oldtype->size * count * blocklength, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->extent = p_oldtype->extent * blocklength + (count - 1) * hstride;
  p_newtype->HVECTOR.p_old_type = p_oldtype;
  p_newtype->HVECTOR.hstride = hstride;
  p_newtype->HVECTOR.blocklength = blocklength;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_indexed(int count, int *array_of_blocklengths, int *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
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
  p_newtype->extent = (p_newtype->INDEXED.p_map[count - 1].displacement * p_oldtype->size + 
		       p_oldtype->extent * p_newtype->INDEXED.p_map[count - 1].blocklength);
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_hindexed(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype) 
{
  int i;
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_HINDEXED, 0, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->HINDEXED.p_old_type = p_oldtype;
  p_newtype->HINDEXED.p_map = malloc(count * sizeof(struct nm_mpi_type_hindexed_map_s));
  for(i = 0; i < count ; i++)
    {
      p_newtype->HINDEXED.p_map[i].blocklength = array_of_blocklengths[i];
      p_newtype->HINDEXED.p_map[i].displacement = array_of_displacements[i];
      p_newtype->size += p_oldtype->size * array_of_blocklengths[i];
    }
  p_newtype->extent = (p_newtype->HINDEXED.p_map[count - 1].displacement + 
		       p_oldtype->extent * p_newtype->HINDEXED.p_map[count - 1].blocklength);
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  return mpi_type_hindexed(count, (int*)array_of_blocklengths, (MPI_Aint*)array_of_displacements, oldtype, newtype);
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
      p_datatype->refcount++;
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

int mpi_type_create_struct(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Datatype array_of_types[], MPI_Datatype *newtype)
{
  return mpi_type_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype);
}


int mpi_type_get_envelope(MPI_Datatype datatype, int*num_integers, int*num_addresses, int*num_datatypes, int*combiner)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *combiner = p_datatype->combiner;
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_NAMED:
      *num_integers  = 0;
      *num_addresses = 0;
      *num_datatypes = 0;
      break;
    case MPI_COMBINER_CONTIGUOUS:
      *num_integers  = 1;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_RESIZED:
      *num_integers  = 0;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_VECTOR:
      *num_integers  = 3;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_HVECTOR:
      *num_integers  = 2;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_INDEXED:
      *num_integers  = p_datatype->elements;
      *num_addresses = p_datatype->elements;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_HINDEXED:
      *num_integers  = p_datatype->elements;
      *num_addresses = p_datatype->elements;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_STRUCT:
      *num_integers  = p_datatype->elements;
      *num_addresses = p_datatype->elements;
      *num_datatypes = p_datatype->elements;
      break;
    }
  return MPI_SUCCESS;
}

int mpi_type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes,
			  int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[])
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_NAMED:
      break;
    case MPI_COMBINER_CONTIGUOUS:
      array_of_datatypes[0] = p_datatype->CONTIGUOUS.p_old_type->id;
      break;
    case MPI_COMBINER_RESIZED:
      array_of_integers[0]  = p_datatype->elements;
      array_of_datatypes[0] = p_datatype->RESIZED.p_old_type->id;
      break;
    case MPI_COMBINER_VECTOR:
      array_of_integers[0]  = p_datatype->elements;
      array_of_integers[1]  = p_datatype->VECTOR.stride;
      array_of_integers[2]  = p_datatype->VECTOR.blocklength;
      array_of_datatypes[0] = p_datatype->VECTOR.p_old_type->id;
      break;
    case MPI_COMBINER_HVECTOR:
      array_of_integers[0]  = p_datatype->elements;
      array_of_integers[1]  = p_datatype->HVECTOR.hstride;
      array_of_integers[2]  = p_datatype->HVECTOR.blocklength;
      array_of_datatypes[0] = p_datatype->HVECTOR.p_old_type->id;
      break;
    case MPI_COMBINER_INDEXED:
      array_of_integers[0]  = p_datatype->elements;
      break;
    case MPI_COMBINER_HINDEXED:
      break;
    case MPI_COMBINER_STRUCT:
      break;
    }
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
    case MPI_COMBINER_HINDEXED:
      if(p_datatype->HINDEXED.p_map != NULL)
	FREE_AND_SET_NULL(p_datatype->HINDEXED.p_map);
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


/** apply a function to every chunk of data in datatype */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_traversal_apply(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_mpi_datatype_s*const p_data = _content;
  const struct nm_mpi_datatype_s*const p_datatype = p_data->p_datatype;
  const int count = p_data->count;
  void*ptr = p_data->ptr;
  assert(p_datatype->refcount > 0);
  if(p_datatype->is_contig || (count == 0))
    {
      assert(p_datatype->lb == 0);
      (*apply)((void*)ptr, count * p_datatype->size, _context);
    }
  else
    {
      int i, j;
      for(i = 0; i < count; i++)
	{
	  switch(p_datatype->combiner)
	    {
	    case MPI_COMBINER_NAMED:
	      (*apply)((void*)ptr, p_datatype->size, _context);
	      break;

	    case MPI_COMBINER_CONTIGUOUS:
	      {
		const struct nm_data_mpi_datatype_s sub = 
		  { .ptr = ptr, .p_datatype = p_datatype->CONTIGUOUS.p_old_type, .count = p_datatype->elements };
		nm_mpi_datatype_traversal_apply(&sub, apply, _context);
	      }
	      break;

	    case MPI_COMBINER_RESIZED:
	      {
		const struct nm_data_mpi_datatype_s sub = 
		  { .ptr = ptr, .p_datatype = p_datatype->RESIZED.p_old_type, .count = 1 };
		nm_mpi_datatype_traversal_apply(&sub, apply, _context);
	      }
	      break;
	      
	    case MPI_COMBINER_VECTOR:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + j * p_datatype->VECTOR.stride * p_datatype->VECTOR.p_old_type->size,
		      .p_datatype = p_datatype->VECTOR.p_old_type,
		      .count      = p_datatype->VECTOR.blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;

	    case MPI_COMBINER_HVECTOR:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + j * p_datatype->HVECTOR.hstride, 
		      .p_datatype = p_datatype->HVECTOR.p_old_type,
		      .count      = p_datatype->HVECTOR.blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;
	      
	    case MPI_COMBINER_INDEXED:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + p_datatype->INDEXED.p_map[j].displacement * p_datatype->INDEXED.p_old_type->size,
		      .p_datatype = p_datatype->INDEXED.p_old_type,
		      .count      = p_datatype->INDEXED.p_map[j].blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;

	    case MPI_COMBINER_HINDEXED:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + p_datatype->HINDEXED.p_map[j].displacement,
		      .p_datatype = p_datatype->HINDEXED.p_old_type,
		      .count      = p_datatype->HINDEXED.p_map[j].blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;
	      
	    case MPI_COMBINER_STRUCT:
	      for(j = 0; j < p_datatype->elements; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + p_datatype->STRUCT.p_map[j].displacement,
		      .p_datatype = p_datatype->STRUCT.p_map[j].p_old_type,
		      .count      = p_datatype->STRUCT.p_map[j].blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;
	      
	    default:
	      ERROR("madmpi: cannot filter datatype with combiner %d\n", p_datatype->combiner);
	    }
	  ptr += p_datatype->extent;
	}
    }
}

/** status for nm_mpi_datatype_*_memcpy */
struct nm_mpi_datatype_filter_memcpy_s
{
  void*pack_ptr; /**< pointer on packed data in memory */
};
/** Pack data to memory 
 */
static void nm_mpi_datatype_pack_memcpy(void*data_ptr, nm_len_t size, void*_status)
{
  struct nm_mpi_datatype_filter_memcpy_s*status = _status;
  memcpy(status->pack_ptr, data_ptr, size);
  status->pack_ptr += size;
}
/** Unpack data from memory
 */
static void nm_mpi_datatype_unpack_memcpy(void*data_ptr, nm_len_t size, void*_status)
{
  struct nm_mpi_datatype_filter_memcpy_s*status = _status;
  memcpy(data_ptr, status->pack_ptr, size);
  status->pack_ptr += size;
}

/** Pack data into a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_pack(void*pack_ptr, const void*data_ptr, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s context = { .pack_ptr = (void*)pack_ptr };
  const struct nm_data_mpi_datatype_s content = { .ptr = (void*)data_ptr, .p_datatype = p_datatype, .count = count };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_pack_memcpy, &context);
}

/** Unpack data from a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_unpack(const void*src_ptr, void*dest_ptr, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s context = { .pack_ptr = (void*)src_ptr };
  const struct nm_data_mpi_datatype_s content = { .ptr = dest_ptr, .p_datatype = p_datatype, .count = count };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_unpack_memcpy, &context);
}

/** Copy data using datatype layout
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_copy(const void*src_buf, nm_mpi_datatype_t*p_src_type, int src_count,
			  void*dest_buf, nm_mpi_datatype_t*p_dest_type, int dest_count)
{
  if(p_src_type == p_dest_type && p_src_type->is_contig)
    {
      int i;
      for(i = 0; i < src_count; i++)
	{
	  memcpy(dest_buf + i * p_dest_type->extent, src_buf + i * p_src_type->extent, p_src_type->size);
	}
    }
  else
    {
      void*ptr = malloc(p_src_type->size * src_count);
      nm_mpi_datatype_pack(ptr, src_buf, p_src_type, src_count);
      nm_mpi_datatype_unpack(dest_buf, ptr, p_dest_type, dest_count);
      free(ptr);
    }
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

