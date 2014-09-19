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

static nm_mpi_datatype_t*nm_mpi_datatype_alloc(void);
static int nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype);
static int nm_mpi_datatype_get_lb_and_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
static int nm_mpi_datatype_indexed(int count, int *array_of_blocklengths, MPI_Aint*array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype);


/* ********************************************************* */

int MPI_Type_size(MPI_Datatype datatype,
		  int *size) __attribute__ ((alias ("mpi_type_size")));

int MPI_Type_get_extent(MPI_Datatype datatype,
			MPI_Aint *lb,
			MPI_Aint *extent) __attribute__ ((alias ("mpi_type_get_extent")));

int MPI_Type_extent(MPI_Datatype datatype,
		    MPI_Aint *extent) __attribute__ ((alias ("mpi_type_extent")));

int MPI_Type_lb(MPI_Datatype datatype,
		MPI_Aint *lb) __attribute__ ((alias ("mpi_type_lb")));

int MPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype) __attribute__ ((alias ("mpi_type_create_resized")));

int MPI_Type_commit(MPI_Datatype *datatype) __attribute__ ((alias ("mpi_type_commit")));

int MPI_Type_free(MPI_Datatype *datatype) __attribute__ ((alias ("mpi_type_free")));

int MPI_Type_optimized(MPI_Datatype *datatype,
                       int optimized) __attribute__ ((alias ("mpi_type_optimized")));

int MPI_Type_contiguous(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype) __attribute__ ((alias ("mpi_type_contiguous")));

int MPI_Type_vector(int count,
                    int blocklength,
                    int stride,
                    MPI_Datatype oldtype,
                    MPI_Datatype *newtype) __attribute__ ((alias ("mpi_type_vector")));

int MPI_Type_hvector(int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) __attribute__ ((alias ("mpi_type_hvector")));

int MPI_Type_indexed(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) __attribute__ ((alias ("mpi_type_indexed")));

int MPI_Type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) __attribute__ ((alias ("mpi_type_hindexed")));

int MPI_Type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype) __attribute__ ((alias ("mpi_type_struct")));

/* ********************************************************* */

/** Maximum number of datatypes */
#define NUMBER_OF_DATATYPES 2048

static struct
{
  /** all the defined datatypes */
  nm_mpi_datatype_t *datatypes[NUMBER_OF_DATATYPES];
  /** pool of ids of datatypes that can be created by end-users */
  puk_int_vect_t datatypes_pool;
} nm_mpi_datatypes;

__PUK_SYM_INTERNAL
void nm_mpi_datatype_init(void)
{
  /** Initialise the basic datatypes */
  int i;
  for(i = MPI_DATATYPE_NULL ; i <= _MPI_DATATYPE_MAX; i++)
    {
      nm_mpi_datatypes.datatypes[i] = malloc(sizeof(nm_mpi_datatype_t));
      nm_mpi_datatypes.datatypes[i]->basic = 1;
      nm_mpi_datatypes.datatypes[i]->committed = 1;
      nm_mpi_datatypes.datatypes[i]->is_contig = 1;
      nm_mpi_datatypes.datatypes[i]->dte_type = MPIR_BASIC;
      nm_mpi_datatypes.datatypes[i]->active_communications = 100;
      nm_mpi_datatypes.datatypes[i]->free_requested = 0;
      nm_mpi_datatypes.datatypes[i]->lb = 0;
    }

  nm_mpi_datatypes.datatypes[MPI_DATATYPE_NULL]->size = 0;
  nm_mpi_datatypes.datatypes[MPI_CHAR]->size = sizeof(signed char);
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED_CHAR]->size = sizeof(unsigned char);
  nm_mpi_datatypes.datatypes[MPI_BYTE]->size = 1;
  nm_mpi_datatypes.datatypes[MPI_SHORT]->size = sizeof(signed short);
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED_SHORT]->size = sizeof(unsigned short);
  nm_mpi_datatypes.datatypes[MPI_INT]->size = sizeof(signed int);
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED]->size = sizeof(unsigned int);
  nm_mpi_datatypes.datatypes[MPI_LONG]->size = sizeof(signed long);
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED_LONG]->size = sizeof(unsigned long);
  nm_mpi_datatypes.datatypes[MPI_FLOAT]->size = sizeof(float);
  nm_mpi_datatypes.datatypes[MPI_DOUBLE]->size = sizeof(double);
  nm_mpi_datatypes.datatypes[MPI_LONG_DOUBLE]->size = sizeof(long double);
  nm_mpi_datatypes.datatypes[MPI_LONG_LONG_INT]->size = sizeof(long long int);
  nm_mpi_datatypes.datatypes[MPI_LONG_LONG]->size = sizeof(long long);

  nm_mpi_datatypes.datatypes[MPI_LOGICAL]->size = sizeof(float);
  nm_mpi_datatypes.datatypes[MPI_REAL]->size = sizeof(float);
  nm_mpi_datatypes.datatypes[MPI_REAL4]->size = 4*sizeof(char);
  nm_mpi_datatypes.datatypes[MPI_REAL8]->size = 8*sizeof(char);
  nm_mpi_datatypes.datatypes[MPI_DOUBLE_PRECISION]->size = sizeof(double);
  nm_mpi_datatypes.datatypes[MPI_INTEGER]->size = sizeof(float);
  nm_mpi_datatypes.datatypes[MPI_INTEGER4]->size = sizeof(int32_t);
  nm_mpi_datatypes.datatypes[MPI_INTEGER8]->size = sizeof(int64_t);
  nm_mpi_datatypes.datatypes[MPI_PACKED]->size = sizeof(char);

  nm_mpi_datatypes.datatypes[MPI_DATATYPE_NULL]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_CHAR]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED_CHAR]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_BYTE]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_SHORT]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED_SHORT]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_INT]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_LONG]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_UNSIGNED_LONG]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_FLOAT]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_DOUBLE]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_LONG_DOUBLE]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_LONG_LONG_INT]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_LONG_LONG]->elements = 1;

  nm_mpi_datatypes.datatypes[MPI_LOGICAL]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_REAL]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_REAL4]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_REAL8]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_DOUBLE_PRECISION]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_INTEGER]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_INTEGER4]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_INTEGER8]->elements = 1;
  nm_mpi_datatypes.datatypes[MPI_PACKED]->elements = 1;

  /* todo: elements=2 */
  nm_mpi_datatypes.datatypes[MPI_LONG_INT]->size = sizeof(long)+sizeof(int);
  nm_mpi_datatypes.datatypes[MPI_SHORT_INT]->size = sizeof(short)+sizeof(int);
  nm_mpi_datatypes.datatypes[MPI_FLOAT_INT]->size = sizeof(float)+sizeof(int);
  nm_mpi_datatypes.datatypes[MPI_DOUBLE_INT]->size = sizeof(double) + sizeof(int);

  nm_mpi_datatypes.datatypes[MPI_2INT]->size = 2*sizeof(int);
  nm_mpi_datatypes.datatypes[MPI_2INT]->elements = 2;

  nm_mpi_datatypes.datatypes[MPI_2DOUBLE_PRECISION]->size = 2*sizeof(double);
  nm_mpi_datatypes.datatypes[MPI_2DOUBLE_PRECISION]->elements = 2;

  nm_mpi_datatypes.datatypes[MPI_COMPLEX]->size = 2*sizeof(float);
  nm_mpi_datatypes.datatypes[MPI_COMPLEX]->elements = 2;

  nm_mpi_datatypes.datatypes[MPI_DOUBLE_COMPLEX]->size = 2*sizeof(double);
  nm_mpi_datatypes.datatypes[MPI_DOUBLE_COMPLEX]->elements = 2;


  for(i = MPI_DATATYPE_NULL; i <= _MPI_DATATYPE_MAX; i++)
    {
      nm_mpi_datatypes.datatypes[i]->extent = nm_mpi_datatypes.datatypes[i]->size;
    }

  nm_mpi_datatypes.datatypes_pool = puk_int_vect_new();
  for(i = _MPI_DATATYPE_MAX+1 ; i < NUMBER_OF_DATATYPES; i++)
    {
      puk_int_vect_push_back(nm_mpi_datatypes.datatypes_pool, i);
    }
}

__PUK_SYM_INTERNAL
void nm_mpi_datatype_exit(void)
{
  int i;
  for(i = 0; i <= _MPI_DATATYPE_MAX; i++)
    {
      FREE_AND_SET_NULL(nm_mpi_datatypes.datatypes[i]);
    }

  puk_int_vect_delete(nm_mpi_datatypes.datatypes_pool);
  nm_mpi_datatypes.datatypes_pool = NULL;
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
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc();
  *newtype = p_newtype->id;
  p_newtype->dte_type  = p_oldtype->dte_type;
  p_newtype->basic     = p_oldtype->basic;
  p_newtype->committed = 0;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->size      = p_oldtype->size;
  p_newtype->extent    = extent;
  p_newtype->lb        = lb;

  if(p_newtype->dte_type == MPIR_CONTIG)
    {
      p_newtype->old_sizes    = malloc(1 * sizeof(int));
      p_newtype->old_sizes[0] = p_oldtype->old_sizes[0];
      p_newtype->old_types    = malloc(1 * sizeof(MPI_Datatype));
      p_newtype->old_types[0] = oldtype;
      p_newtype->elements     = p_oldtype->elements;
    }
  else if(p_newtype->dte_type == MPIR_VECTOR)
    {
      p_newtype->old_sizes    = malloc(1 * sizeof(int));
      p_newtype->old_sizes[0] = p_oldtype->old_sizes[0];
      p_newtype->old_types    = malloc(1 * sizeof(MPI_Datatype));
      p_newtype->old_types[0] = oldtype;
      p_newtype->elements     = p_oldtype->elements;
      p_newtype->blocklens    = malloc(1 * sizeof(int));
      p_newtype->blocklens[0] = p_oldtype->blocklens[0];
      p_newtype->block_size   = p_oldtype->block_size;
      p_newtype->stride       = p_oldtype->stride;
  }
  else if(p_newtype->dte_type == MPIR_INDEXED) 
    {
      p_newtype->old_sizes    = malloc(1 * sizeof(int));
      p_newtype->old_sizes[0] = p_oldtype->old_sizes[0];
      p_newtype->old_types    = malloc(1 * sizeof(MPI_Datatype));
      p_newtype->old_types[0] = oldtype;
      p_newtype->elements     = p_oldtype->elements;
      p_newtype->blocklens    = malloc(p_newtype->elements * sizeof(int));
      p_newtype->indices      = malloc(p_newtype->elements * sizeof(MPI_Aint));
      for(i = 0; i < p_newtype->elements ; i++)
	{
	  p_newtype->blocklens[i] = p_oldtype->blocklens[i];
	  p_newtype->indices[i]   = p_oldtype->indices[i];
	}
    }
  else if(p_newtype->dte_type == MPIR_STRUCT)
    {
      p_newtype->elements  = p_oldtype->elements;
    p_newtype->blocklens = malloc(p_newtype->elements * sizeof(int));
    p_newtype->indices   = malloc(p_newtype->elements * sizeof(MPI_Aint));
    p_newtype->old_sizes = malloc(p_newtype->elements * sizeof(size_t));
    p_newtype->old_types = malloc(p_newtype->elements * sizeof(MPI_Datatype));
    for(i=0 ; i<p_newtype->elements ; i++) {
      p_newtype->blocklens[i] = p_oldtype->blocklens[i];
      p_newtype->indices[i]   = p_oldtype->indices[i];
      p_newtype->old_sizes[i] = p_oldtype->old_sizes[i];
      p_newtype->old_types[i] = p_oldtype->old_types[i];
    }
  }
  else if (p_newtype->dte_type != MPIR_BASIC) {
    ERROR("Datatype %d unknown", oldtype);
    return MPI_ERR_OTHER;
  }

  MPI_NMAD_TRACE("Creating resized type (%d) with extent=%ld, based on type %d with a extent %ld\n",
		 *newtype, (long)p_newtype->extent, oldtype, (long)p_oldtype->extent);
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
    { /* err = MPI_DATATYPE_ACTIVE */
      err = MPI_SUCCESS;
    }  
  return err;
}

int mpi_type_optimized(MPI_Datatype *datatype, int optimized)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  p_datatype->is_optimized = optimized;
  return MPI_SUCCESS;
}

int mpi_type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc();
  *newtype = p_newtype->id;
  p_newtype->dte_type = MPIR_CONTIG;
  p_newtype->basic = 0;
  p_newtype->old_sizes = malloc(1 * sizeof(int));
  p_newtype->old_sizes[0] = p_oldtype->extent;
  p_newtype->old_types = malloc(1 * sizeof(MPI_Datatype));
  p_newtype->old_types[0] = oldtype;
  p_newtype->committed = 0;
  p_newtype->is_contig = 1;
  p_newtype->size = p_newtype->old_sizes[0] * count;
  p_newtype->elements = count;
  p_newtype->lb = 0;
  p_newtype->extent = p_newtype->old_sizes[0] * count;

  MPI_NMAD_TRACE("Creating new contiguous type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		 (long)p_newtype->size, (long)p_newtype->extent,
		 oldtype, (long)p_newtype->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpi_type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_old_type = nm_mpi_datatype_get(oldtype);
  const int hstride = stride * nm_mpi_datatype_size(p_old_type);
  return mpi_type_hvector(count, blocklength, hstride, oldtype, newtype);
}

int mpi_type_hvector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc();
  *newtype = p_newtype->id;
  p_newtype->dte_type = MPIR_VECTOR;
  p_newtype->basic = 0;
  p_newtype->old_sizes = malloc(1 * sizeof(int));
  p_newtype->old_sizes[0] = p_oldtype->extent;
  p_newtype->old_types = malloc(1 * sizeof(MPI_Datatype));
  p_newtype->old_types[0] = oldtype;
  p_newtype->committed = 0;
  p_newtype->is_contig = 0;
  p_newtype->size = p_newtype->old_sizes[0] * count * blocklength;
  p_newtype->elements = count;
  p_newtype->blocklens = malloc(1 * sizeof(int));
  p_newtype->blocklens[0] = blocklength;
  p_newtype->block_size = blocklength * p_newtype->old_sizes[0];
  p_newtype->stride = stride;
  p_newtype->lb = 0;
  p_newtype->extent = p_newtype->old_sizes[0] * count * blocklength;

  MPI_NMAD_TRACE("Creating new (h)vector type (%d) with size=%ld, extent=%ld, elements=%d, blocklen=%d based on type %d with a extent %ld\n",
		 *newtype, (long)p_newtype->size, (long)p_newtype->extent,
		 p_newtype->elements, p_newtype->blocklens[0],
		 oldtype, (long)p_newtype->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpi_type_indexed(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  int err, i;

  MPI_NMAD_LOG_IN();

  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  for(i=0 ; i<count ; i++) {
    displacements[i] = array_of_displacements[i];
  }

  err = nm_mpi_datatype_indexed(count, array_of_blocklengths, displacements, oldtype, newtype);

  FREE_AND_SET_NULL(displacements);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_type_hindexed(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype) 
{
  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  nm_mpi_datatype_t*p_old_type = nm_mpi_datatype_get(oldtype);
  int old_size = nm_mpi_datatype_size(p_old_type);
  int i;
  for(i = 0; i < count; i++)
    {
      displacements[i] = array_of_displacements[i] * old_size;
    }
  int err = nm_mpi_datatype_indexed(count, array_of_blocklengths, displacements, oldtype, newtype);
  FREE_AND_SET_NULL(displacements);
  return err;
}

int mpi_type_struct(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype *array_of_types, MPI_Datatype *newtype)
{
  int i;

  MPI_NMAD_TRACE("Creating struct derived datatype based on %d elements\n", count);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc();
  *newtype = p_newtype->id;
  p_newtype->dte_type = MPIR_STRUCT;
  p_newtype->basic = 0;
  p_newtype->committed = 0;
  p_newtype->is_contig = 0;
  p_newtype->elements = count;
  p_newtype->size = 0;
  p_newtype->lb = 0;

  p_newtype->blocklens = malloc(count * sizeof(int));
  p_newtype->indices = malloc(count * sizeof(MPI_Aint));
  p_newtype->old_sizes = malloc(count * sizeof(size_t));
  p_newtype->old_types = malloc(count * sizeof(MPI_Datatype));
  for(i = 0; i < count; i++)
    {
      p_newtype->blocklens[i] = array_of_blocklengths[i];
      p_newtype->old_sizes[i] = nm_mpi_datatype_get(array_of_types[i])->size;
      p_newtype->old_types[i] = array_of_types[i];
      p_newtype->indices[i] = array_of_displacements[i];
      
      MPI_NMAD_TRACE("Element %d: length %d, old_type size %ld, indice %ld\n", i, p_newtype->blocklens[i],
		     (long)p_newtype->old_sizes[i], (long)p_newtype->indices[i]);
      p_newtype->size += p_newtype->blocklens[i] * p_newtype->old_sizes[i];
    }
  /** We suppose here that the last field of the struct does not need
   * an alignment. In case, one sends an array of struct, the 1st
   * field of the 2nd struct immediatly follows the last field of the
   * previous struct.
   */
  p_newtype->extent = p_newtype->indices[count-1] + p_newtype->blocklens[count-1] * p_newtype->old_sizes[count-1];
  MPI_NMAD_TRACE("Creating new struct type (%d) with size=%ld and extent=%ld\n", *newtype,
		 (long)p_newtype->size, (long)p_newtype->extent);
  return MPI_SUCCESS;
}



/**
 * Gets the id of the next available datatype.
 */
static inline nm_mpi_datatype_t*nm_mpi_datatype_alloc(void)
{
  nm_mpi_datatype_t*p_datatype = malloc(sizeof(nm_mpi_datatype_t));
  if(puk_int_vect_empty(nm_mpi_datatypes.datatypes_pool))
    {
      ERROR("Maximum number of datatypes created");
      return NULL;
    }
  const int datatype = puk_int_vect_pop_back(nm_mpi_datatypes.datatypes_pool);
  nm_mpi_datatypes.datatypes[datatype] = p_datatype;
  p_datatype->id = datatype;
  return p_datatype;
}

__PUK_SYM_INTERNAL
size_t nm_mpi_datatype_size(nm_mpi_datatype_t*p_datatype)
{
  return p_datatype->size;
}

nm_mpi_datatype_t* nm_mpi_datatype_get(MPI_Datatype datatype)
{
  if (datatype >= 0 && datatype < NUMBER_OF_DATATYPES)
    {
      if(tbx_unlikely(nm_mpi_datatypes.datatypes[datatype] == NULL))
	{
	  ERROR("Datatype %d invalid", datatype);
	  return NULL;
	}
      else 
	{
	  return nm_mpi_datatypes.datatypes[datatype];
	}
    }
  else 
    {
      ERROR("Datatype %d unknown", datatype);
      return NULL;
    }
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
  const int datatype = p_datatype->id;
  if(p_datatype->active_communications != 0)
    {
      p_datatype->free_requested = 1;
      return MPI_DATATYPE_ACTIVE;
    }
  else
    {
      FREE_AND_SET_NULL(nm_mpi_datatypes.datatypes[datatype]->old_sizes);
      FREE_AND_SET_NULL(nm_mpi_datatypes.datatypes[datatype]->old_types);
      if (nm_mpi_datatypes.datatypes[datatype]->dte_type == MPIR_INDEXED || nm_mpi_datatypes.datatypes[datatype]->dte_type == MPIR_STRUCT)
	{
	  FREE_AND_SET_NULL(nm_mpi_datatypes.datatypes[datatype]->blocklens);
	  FREE_AND_SET_NULL(nm_mpi_datatypes.datatypes[datatype]->indices);
	}
      if (nm_mpi_datatypes.datatypes[datatype]->dte_type == MPIR_VECTOR) 
	{
	  FREE_AND_SET_NULL(nm_mpi_datatypes.datatypes[datatype]->blocklens);
	}
      puk_int_vect_push_back(nm_mpi_datatypes.datatypes_pool, datatype);
      FREE_AND_SET_NULL(nm_mpi_datatypes.datatypes[datatype]);
      return MPI_SUCCESS;
    }
}


static int nm_mpi_datatype_indexed(int count, int *array_of_blocklengths, MPI_Aint*array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc();
  *newtype = p_newtype->id;
  p_newtype->dte_type = MPIR_INDEXED;
  p_newtype->basic = 0;
  p_newtype->old_sizes = malloc(1 * sizeof(int));
  p_newtype->old_sizes[0] = p_oldtype->extent;
  p_newtype->old_types = malloc(1 * sizeof(MPI_Datatype));
  p_newtype->old_types[0] = oldtype;
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
      p_newtype->size += p_newtype->old_sizes[0] * p_newtype->blocklens[i];
      MPI_NMAD_TRACE("Element %d: length %d, indice %ld, new size %ld\n", i, p_newtype->blocklens[i], (long)p_newtype->indices[i], (long) p_newtype->size);
    }
  p_newtype->extent = (p_newtype->indices[count-1] + p_oldtype->extent * p_newtype->blocklens[count-1]);

  MPI_NMAD_TRACE("Creating new index type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		 (long)p_newtype->size, (long)p_newtype->extent,
		 oldtype, (long)p_newtype->old_sizes[0]);
  return MPI_SUCCESS;
}


