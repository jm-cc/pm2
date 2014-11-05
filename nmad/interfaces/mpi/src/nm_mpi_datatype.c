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


static int nm_mpi_datatype_get_lb_and_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
static int nm_mpi_datatype_indexed(int count, int *array_of_blocklengths, MPI_Aint*array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype);
/** store builtin datatypes */
static void nm_mpi_datatype_store(int id, size_t size, int elements);
static int nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype);


/* ********************************************************* */

NM_MPI_ALIAS(MPI_Type_size,           mpi_type_size);
NM_MPI_ALIAS(MPI_Type_get_extent,     mpi_type_get_extent);
NM_MPI_ALIAS(MPI_Type_extent,         mpi_type_extent);
NM_MPI_ALIAS(MPI_Type_lb,             mpi_type_lb);
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
  nm_mpi_datatype_store(MPI_DOUBLE_PRECISION,   sizeof(double), 1);
  nm_mpi_datatype_store(MPI_INTEGER,            sizeof(float), 1);
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
  p_datatype->dte_type = NM_MPI_DATATYPE_BASIC;
  p_datatype->active_communications = 100;
  p_datatype->free_requested = 0;
  p_datatype->lb = 0;
  p_datatype->size = size;
  p_datatype->elements = elements;
  p_datatype->extent = size;
  p_datatype->p_old_type = NULL;
  p_datatype->p_old_types = NULL;
}

/* ********************************************************* */

int mpi_type_size(MPI_Datatype datatype, int *size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  *size = p_datatype->size;
  return MPI_SUCCESS;
}

int mpi_type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
  return nm_mpi_datatype_get_lb_and_extent(datatype, lb, extent);
}

int mpi_type_extent(MPI_Datatype datatype, MPI_Aint *extent)
{
  return nm_mpi_datatype_get_lb_and_extent(datatype, NULL, extent);
}

int mpi_type_lb(MPI_Datatype datatype,MPI_Aint *lb)
{
  return nm_mpi_datatype_get_lb_and_extent(datatype, lb, NULL);
}

int mpi_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype)
{
  MPI_NMAD_LOG_IN();
  if(lb != 0)
    {
      ERROR("<Using lb not equals to 0> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  int i;
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  *newtype = p_newtype->id;
  p_newtype->dte_type  = p_oldtype->dte_type;
  p_newtype->committed = 0;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->size      = p_oldtype->size;
  p_newtype->extent    = extent;
  p_newtype->lb        = lb;

  if(p_newtype->dte_type == NM_MPI_DATATYPE_CONTIG)
    {
      p_newtype->p_old_type   = p_oldtype;
      p_newtype->p_old_types  = &p_newtype->p_old_type;
      p_newtype->elements     = p_oldtype->elements;
    }
  else if(p_newtype->dte_type == NM_MPI_DATATYPE_VECTOR)
    {
      p_newtype->p_old_type   = p_oldtype;
      p_newtype->p_old_types  = &p_newtype->p_old_type;
      p_newtype->elements     = p_oldtype->elements;
      p_newtype->blocklens    = malloc(1 * sizeof(int));
      p_newtype->blocklens[0] = p_oldtype->blocklens[0];
      p_newtype->block_size   = p_oldtype->block_size;
      p_newtype->hstride      = p_oldtype->hstride;
  }
  else if(p_newtype->dte_type == NM_MPI_DATATYPE_INDEXED) 
    {
      p_newtype->p_old_type   = p_oldtype;
      p_newtype->p_old_types  = &p_newtype->p_old_type;
      p_newtype->elements     = p_oldtype->elements;
      p_newtype->blocklens    = malloc(p_newtype->elements * sizeof(int));
      p_newtype->indices      = malloc(p_newtype->elements * sizeof(MPI_Aint));
      for(i = 0; i < p_newtype->elements ; i++)
	{
	  p_newtype->blocklens[i] = p_oldtype->blocklens[i];
	  p_newtype->indices[i]   = p_oldtype->indices[i];
	}
    }
  else if(p_newtype->dte_type == NM_MPI_DATATYPE_STRUCT)
    {
      p_newtype->elements  = p_oldtype->elements;
      p_newtype->blocklens = malloc(p_newtype->elements * sizeof(int));
      p_newtype->indices   = malloc(p_newtype->elements * sizeof(MPI_Aint));
      p_newtype->p_old_types = malloc(p_newtype->elements * sizeof(nm_mpi_datatype_t*));
      p_newtype->p_old_type = NULL;
      for(i = 0; i < p_newtype->elements; i++)
	{
	  p_newtype->blocklens[i] = p_oldtype->blocklens[i];
	  p_newtype->indices[i]   = p_oldtype->indices[i];
	  p_newtype->p_old_types[i] = p_oldtype->p_old_types[i];
	}
    }
  else if (p_newtype->dte_type != NM_MPI_DATATYPE_BASIC)
    {
      ERROR("Datatype %d unknown", oldtype);
      return MPI_ERR_OTHER;
    }
  return MPI_SUCCESS;
}

int mpi_type_commit(MPI_Datatype *datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  p_datatype->committed = 1;
  p_datatype->is_optimized = 0;
  p_datatype->active_communications = 0;
  p_datatype->free_requested = 0;
  return MPI_SUCCESS;
}

int mpi_type_free(MPI_Datatype *datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  int err = nm_mpi_datatype_free(p_datatype);
  if (err == MPI_SUCCESS) 
    {
      *datatype = MPI_DATATYPE_NULL;
    }
  else
    { /* err = MPI_ERR_DATATYPE_ACTIVE */
      err = MPI_SUCCESS;
    }  
  return err;
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
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  *newtype = p_newtype->id;
  p_newtype->dte_type = NM_MPI_DATATYPE_CONTIG;
  p_newtype->p_old_type = p_oldtype;
  p_newtype->p_old_types = &p_newtype->p_old_type;
  p_newtype->committed = 0;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->size = p_oldtype->size * count;
  p_newtype->elements = count;
  p_newtype->lb = 0;
  p_newtype->extent = p_oldtype->extent * count;
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
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  *newtype = p_newtype->id;
  p_newtype->dte_type = NM_MPI_DATATYPE_VECTOR;
  p_newtype->p_old_type = p_oldtype;
  p_newtype->p_old_types = &p_newtype->p_old_type;
  p_newtype->committed = 0;
  p_newtype->is_contig = 0;
  p_newtype->size = p_oldtype->size * count * blocklength;
  p_newtype->elements = count;
  p_newtype->blocklens = malloc(1 * sizeof(int));
  p_newtype->blocklens[0] = blocklength;
  p_newtype->block_size = blocklength * p_oldtype->size;
  p_newtype->hstride = hstride;
  p_newtype->lb = 0;
  p_newtype->extent = p_oldtype->extent * count * blocklength;
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
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  *newtype = p_newtype->id;
  p_newtype->dte_type = NM_MPI_DATATYPE_STRUCT;
  p_newtype->committed = 0;
  p_newtype->is_contig = 0;
  p_newtype->elements = count;
  p_newtype->size = 0;
  p_newtype->lb = 0;

  p_newtype->blocklens = malloc(count * sizeof(int));
  p_newtype->indices = malloc(count * sizeof(MPI_Aint));
  p_newtype->p_old_types = malloc(count * sizeof(nm_mpi_datatype_t*));
  p_newtype->p_old_type = NULL;
  for(i = 0; i < count; i++)
    {
      nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(array_of_types[i]);
      if(p_datatype == NULL)
	{
	  nm_mpi_handle_datatype_free(&nm_mpi_datatypes, p_datatype);
	  *newtype = MPI_DATATYPE_NULL;
	  return MPI_ERR_TYPE;
	}
      p_newtype->blocklens[i] = array_of_blocklengths[i];
      p_newtype->p_old_types[i] = p_datatype;
      p_newtype->indices[i] = array_of_displacements[i];
      p_newtype->size += p_newtype->blocklens[i] * p_datatype->size;
    }
  /** We suppose here that the last field of the struct does not need
   * an alignment. In case, one sends an array of struct, the 1st
   * field of the 2nd struct immediatly follows the last field of the
   * previous struct.
   */
  p_newtype->extent = p_newtype->indices[count-1] + p_newtype->blocklens[count-1] * p_newtype->p_old_types[count-1]->extent;
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

static int nm_mpi_datatype_get_lb_and_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(lb)
    {
      *lb = p_datatype->lb;
    }
  if(extent)
    {
      *extent = p_datatype->extent;
    }
  return MPI_SUCCESS;
}

__PUK_SYM_INTERNAL
int nm_mpi_datatype_unlock(nm_mpi_datatype_t*p_datatype)
{
  p_datatype->active_communications--;
  if(p_datatype->active_communications == 0 && p_datatype->free_requested == 1)
    {
      nm_mpi_datatype_free(p_datatype);
    }
  return MPI_SUCCESS;
}

static int nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype)
{
  if(p_datatype->active_communications != 0)
    {
      p_datatype->free_requested = 1;
      return MPI_ERR_DATATYPE_ACTIVE;
    }
  else
    {
      if(p_datatype->p_old_types != &p_datatype->p_old_type)
	{
	  FREE_AND_SET_NULL(p_datatype->p_old_types);
	}
      if(p_datatype->dte_type == NM_MPI_DATATYPE_INDEXED || p_datatype->dte_type == NM_MPI_DATATYPE_STRUCT)
	{
	  FREE_AND_SET_NULL(p_datatype->blocklens);
	  FREE_AND_SET_NULL(p_datatype->indices);
	}
      if(p_datatype->dte_type == NM_MPI_DATATYPE_VECTOR) 
	{
	  FREE_AND_SET_NULL(p_datatype->blocklens);
	}
      nm_mpi_handle_datatype_free(&nm_mpi_datatypes, p_datatype);
      return MPI_SUCCESS;
    }
}


static int nm_mpi_datatype_indexed(int count, int*array_of_blocklengths, MPI_Aint*array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  *newtype = p_newtype->id;
  p_newtype->dte_type = NM_MPI_DATATYPE_INDEXED;
  p_newtype->p_old_type = p_oldtype;
  p_newtype->p_old_types = &p_newtype->p_old_type;
  p_newtype->committed = 0;
  p_newtype->is_contig = 0;
  p_newtype->elements = count;
  p_newtype->lb = 0;
  p_newtype->blocklens = malloc(count * sizeof(int));
  p_newtype->indices = malloc(count * sizeof(MPI_Aint));
  p_newtype->size = 0;
  for(i = 0; i < count ; i++)
    {
      p_newtype->blocklens[i] = array_of_blocklengths[i];
      p_newtype->indices[i] = array_of_displacements[i];
      p_newtype->size += p_oldtype->size * p_newtype->blocklens[i];
    }
  p_newtype->extent = (p_newtype->indices[count-1] + p_oldtype->extent * p_newtype->blocklens[count-1]);
  return MPI_SUCCESS;
}

/**
 * Pack data into a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_pack(void*dest_ptr, const void*src_ptr, nm_mpi_datatype_t*p_datatype, int count)
{
  if(p_datatype->is_contig)
    {
      assert(p_datatype->lb == 0);
      assert(p_datatype->extent == p_datatype->size);
      memcpy(dest_ptr, src_ptr, count * p_datatype->size);
    }
  else
    {
      int i, j;
      switch(p_datatype->dte_type)
	{
	case NM_MPI_DATATYPE_CONTIG:
	  {
	    for(i = 0; i < count; i++)
	      {
		nm_mpi_datatype_pack(dest_ptr, src_ptr, p_datatype->p_old_type, p_datatype->elements);
		dest_ptr += p_datatype->size;
		src_ptr  += p_datatype->extent;
	      }
	  }
	  break;
	  
	case NM_MPI_DATATYPE_VECTOR:
	  {
	    for(i = 0; i < count; i++)
	      {
		for(j = 0; j < p_datatype->elements; j++)
		  {
		    nm_mpi_datatype_pack(dest_ptr, src_ptr + j * p_datatype->hstride, p_datatype->p_old_type, p_datatype->blocklens[0]);
		    dest_ptr += p_datatype->p_old_type->size * p_datatype->blocklens[0];
		  }
		src_ptr += p_datatype->extent;
	      }
	  }
	  break;
	  
	case NM_MPI_DATATYPE_INDEXED:
	  {
	    for(i = 0; i < count; i++)
	      {
		for(j = 0; j < p_datatype->elements; j++)
		  {
		    nm_mpi_datatype_pack(dest_ptr, src_ptr + p_datatype->indices[j], p_datatype->p_old_type, p_datatype->blocklens[j]);
		    dest_ptr += p_datatype->blocklens[j] * p_datatype->p_old_type->size;
		  }
		src_ptr += p_datatype->extent;
	      }
	  }
	  break;
	  
	case NM_MPI_DATATYPE_STRUCT:
	  {
	    for(i = 0; i < count; i++)
	      {
		for(j = 0; j < p_datatype->elements; j++)
		  {
		    nm_mpi_datatype_pack(dest_ptr, src_ptr + p_datatype->indices[j], p_datatype->p_old_types[j], p_datatype->blocklens[j]);
		    dest_ptr += p_datatype->blocklens[j] * p_datatype->p_old_types[j]->size;
		  }
		src_ptr += p_datatype->extent;
	      }
	  }
	  break;
	  
	case NM_MPI_DATATYPE_BASIC:
	  {
	    ERROR("madmpi: trying to pack BASIC datatype as sparse (should be contiguous).\n");
	  }
	  break;

	default:
	  ERROR("madmpi: cannot pack datatype of type %d\n", p_datatype->dte_type);
	}
    }
}

__PUK_SYM_INTERNAL
void nm_mpi_datatype_unpack(const void*src_ptr, void*dest_ptr, nm_mpi_datatype_t*p_datatype, int count)
{
  if(p_datatype->is_contig)
    {
      assert(p_datatype->lb == 0);
      assert(p_datatype->extent == p_datatype->size);
      memcpy(dest_ptr, src_ptr, count * p_datatype->size);
    }
  else
    {
      int i, j;
      switch(p_datatype->dte_type)
	{
	case NM_MPI_DATATYPE_CONTIG:
	  {
	    for(i = 0; i < count; i++)
	      {
		nm_mpi_datatype_unpack(src_ptr, dest_ptr, p_datatype->p_old_type, p_datatype->elements);
		dest_ptr += p_datatype->extent;
		src_ptr  += p_datatype->size;
	      }
	  }
	  break;

	case NM_MPI_DATATYPE_VECTOR:
	  {
	    for(i = 0; i < count; i++)
	      {
		for(j = 0; j < p_datatype->elements; j++)
		  {
		    nm_mpi_datatype_unpack(src_ptr, dest_ptr + j * p_datatype->hstride, p_datatype->p_old_type, p_datatype->blocklens[0]);
		    src_ptr += p_datatype->p_old_type->size * p_datatype->blocklens[0];
		  }
		dest_ptr += p_datatype->extent;
	      }
	  }
	  break;
	  
	case NM_MPI_DATATYPE_INDEXED:
	  {
	    for(i = 0; i < count ; i++)
	      {
		for(j = 0; j < p_datatype->elements; j++)
		  {
		    nm_mpi_datatype_unpack(src_ptr, dest_ptr + p_datatype->indices[j], p_datatype->p_old_type, p_datatype->blocklens[j]);
		    src_ptr += p_datatype->blocklens[j] * p_datatype->p_old_type->size;
		  }
		dest_ptr += p_datatype->extent;
	      }
	  }
	  break;
	  
	case NM_MPI_DATATYPE_STRUCT:
	  {
	    for(i = 0; i < count; i++)
	      {
		for(j = 0; j < p_datatype->elements; j++)
		  {
		    nm_mpi_datatype_unpack(src_ptr, dest_ptr + p_datatype->indices[j], p_datatype->p_old_types[j], p_datatype->blocklens[j]);
		    src_ptr += p_datatype->blocklens[j] * p_datatype->p_old_types[j]->size;
		  }
		dest_ptr += p_datatype->extent;
	      }
	  }
	  break;
	  
	case NM_MPI_DATATYPE_BASIC:
	  {
	    ERROR("madmpi: trying to unpack BASIC datatype as sparse (should be contiguous).\n");
	  }
	  break;

	default:
	  ERROR("madmpi: cannot unpack datatype of type %d\n", p_datatype->dte_type);
	}
    }
}


int mpi_pack(void*inbuf, int incount, MPI_Datatype datatype, void*outbuf, int outsize, int*position, MPI_Comm comm)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  size_t size = incount * nm_mpi_datatype_size(p_datatype);
  assert(outsize >= size);
  nm_mpi_datatype_pack(outbuf + *position, inbuf, p_datatype, incount);
  *position += size;
  return MPI_SUCCESS;
}

int mpi_unpack(void*inbuf, int insize, int*position, void*outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  size_t size = outcount * nm_mpi_datatype_size(p_datatype);
  assert(insize >= size);
  nm_mpi_datatype_unpack(inbuf + *position, outbuf, p_datatype, outcount);
  *position += size;
  return MPI_SUCCESS;
}
