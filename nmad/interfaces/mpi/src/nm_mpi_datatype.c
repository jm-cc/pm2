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

static int nm_mpi_datatype_alloc(void);
static int nm_mpi_datatype_free(MPI_Datatype datatype);
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
  nm_mpi_datatype_t *mpir_old_datatype = nm_mpi_datatype_get(oldtype);

  *newtype = nm_mpi_datatype_alloc();
  nm_mpi_internal_data.datatypes[*newtype] = malloc(sizeof(nm_mpi_datatype_t));

  nm_mpi_internal_data.datatypes[*newtype]->dte_type  = mpir_old_datatype->dte_type;
  nm_mpi_internal_data.datatypes[*newtype]->basic     = mpir_old_datatype->basic;
  nm_mpi_internal_data.datatypes[*newtype]->committed = 0;
  nm_mpi_internal_data.datatypes[*newtype]->is_contig = mpir_old_datatype->is_contig;
  nm_mpi_internal_data.datatypes[*newtype]->size      = mpir_old_datatype->size;
  nm_mpi_internal_data.datatypes[*newtype]->extent    = extent;
  nm_mpi_internal_data.datatypes[*newtype]->lb        = lb;

  if (nm_mpi_internal_data.datatypes[*newtype]->dte_type == MPIR_CONTIG) {
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    nm_mpi_internal_data.datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    nm_mpi_internal_data.datatypes[*newtype]->old_types[0] = oldtype;
    nm_mpi_internal_data.datatypes[*newtype]->elements     = mpir_old_datatype->elements;
  }
  else if (nm_mpi_internal_data.datatypes[*newtype]->dte_type == MPIR_VECTOR) {
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    nm_mpi_internal_data.datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    nm_mpi_internal_data.datatypes[*newtype]->old_types[0] = oldtype;
    nm_mpi_internal_data.datatypes[*newtype]->elements     = mpir_old_datatype->elements;
    nm_mpi_internal_data.datatypes[*newtype]->blocklens    = malloc(1 * sizeof(int));
    nm_mpi_internal_data.datatypes[*newtype]->blocklens[0] = mpir_old_datatype->blocklens[0];
    nm_mpi_internal_data.datatypes[*newtype]->block_size   = mpir_old_datatype->block_size;
    nm_mpi_internal_data.datatypes[*newtype]->stride       = mpir_old_datatype->stride;
  }
  else if (nm_mpi_internal_data.datatypes[*newtype]->dte_type == MPIR_INDEXED) {
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes    = malloc(1 * sizeof(int));
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->old_sizes[0];
    nm_mpi_internal_data.datatypes[*newtype]->old_types    = malloc(1 * sizeof(MPI_Datatype));
    nm_mpi_internal_data.datatypes[*newtype]->old_types[0] = oldtype;
    nm_mpi_internal_data.datatypes[*newtype]->elements     = mpir_old_datatype->elements;
    nm_mpi_internal_data.datatypes[*newtype]->blocklens    = malloc(nm_mpi_internal_data.datatypes[*newtype]->elements * sizeof(int));
    nm_mpi_internal_data.datatypes[*newtype]->indices      = malloc(nm_mpi_internal_data.datatypes[*newtype]->elements * sizeof(MPI_Aint));
    for(i=0 ; i<nm_mpi_internal_data.datatypes[*newtype]->elements ; i++) {
      nm_mpi_internal_data.datatypes[*newtype]->blocklens[i] = mpir_old_datatype->blocklens[i];
      nm_mpi_internal_data.datatypes[*newtype]->indices[i]   = mpir_old_datatype->indices[i];
    }
  }
  else if (nm_mpi_internal_data.datatypes[*newtype]->dte_type == MPIR_STRUCT) {
    nm_mpi_internal_data.datatypes[*newtype]->elements  = mpir_old_datatype->elements;
    nm_mpi_internal_data.datatypes[*newtype]->blocklens = malloc(nm_mpi_internal_data.datatypes[*newtype]->elements * sizeof(int));
    nm_mpi_internal_data.datatypes[*newtype]->indices   = malloc(nm_mpi_internal_data.datatypes[*newtype]->elements * sizeof(MPI_Aint));
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes = malloc(nm_mpi_internal_data.datatypes[*newtype]->elements * sizeof(size_t));
    nm_mpi_internal_data.datatypes[*newtype]->old_types = malloc(nm_mpi_internal_data.datatypes[*newtype]->elements * sizeof(MPI_Datatype));
    for(i=0 ; i<nm_mpi_internal_data.datatypes[*newtype]->elements ; i++) {
      nm_mpi_internal_data.datatypes[*newtype]->blocklens[i] = mpir_old_datatype->blocklens[i];
      nm_mpi_internal_data.datatypes[*newtype]->indices[i]   = mpir_old_datatype->indices[i];
      nm_mpi_internal_data.datatypes[*newtype]->old_sizes[i] = mpir_old_datatype->old_sizes[i];
      nm_mpi_internal_data.datatypes[*newtype]->old_types[i] = mpir_old_datatype->old_types[i];
    }
  }
  else if (nm_mpi_internal_data.datatypes[*newtype]->dte_type != MPIR_BASIC) {
    ERROR("Datatype %d unknown", oldtype);
    return MPI_ERR_OTHER;
  }

  MPI_NMAD_TRACE("Creating resized type (%d) with extent=%ld, based on type %d with a extent %ld\n",
		 *newtype, (long)nm_mpi_internal_data.datatypes[*newtype]->extent, oldtype, (long)mpir_old_datatype->extent);
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
  int err = nm_mpi_datatype_free(*datatype);
  if (err == MPI_SUCCESS) {
    *datatype = MPI_DATATYPE_NULL;
  }
  else { /* err = MPI_DATATYPE_ACTIVE */
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
  nm_mpi_datatype_t*mpir_old_datatype = nm_mpi_datatype_get(oldtype);
  *newtype = nm_mpi_datatype_alloc();
  nm_mpi_internal_data.datatypes[*newtype] = malloc(sizeof(nm_mpi_datatype_t));
  nm_mpi_internal_data.datatypes[*newtype]->dte_type = MPIR_CONTIG;
  nm_mpi_internal_data.datatypes[*newtype]->basic = 0;
  nm_mpi_internal_data.datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  nm_mpi_internal_data.datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  nm_mpi_internal_data.datatypes[*newtype]->old_types[0] = oldtype;
  nm_mpi_internal_data.datatypes[*newtype]->committed = 0;
  nm_mpi_internal_data.datatypes[*newtype]->is_contig = 1;
  nm_mpi_internal_data.datatypes[*newtype]->size = nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] * count;
  nm_mpi_internal_data.datatypes[*newtype]->elements = count;
  nm_mpi_internal_data.datatypes[*newtype]->lb = 0;
  nm_mpi_internal_data.datatypes[*newtype]->extent = nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] * count;

  MPI_NMAD_TRACE("Creating new contiguous type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		 (long)nm_mpi_internal_data.datatypes[*newtype]->size, (long)nm_mpi_internal_data.datatypes[*newtype]->extent,
		 oldtype, (long)nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0]);
  return MPI_SUCCESS;
}

int mpi_type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  const int hstride = stride * mpir_sizeof_datatype(oldtype);
  return mpi_type_hvector(count, blocklength, hstride, oldtype, newtype);
}

int mpi_type_hvector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t *mpir_old_datatype = nm_mpi_datatype_get(oldtype);
  *newtype = nm_mpi_datatype_alloc();
  nm_mpi_internal_data.datatypes[*newtype] = malloc(sizeof(nm_mpi_datatype_t));
  nm_mpi_internal_data.datatypes[*newtype]->dte_type = MPIR_VECTOR;
  nm_mpi_internal_data.datatypes[*newtype]->basic = 0;
  nm_mpi_internal_data.datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  nm_mpi_internal_data.datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  nm_mpi_internal_data.datatypes[*newtype]->old_types[0] = oldtype;
  nm_mpi_internal_data.datatypes[*newtype]->committed = 0;
  nm_mpi_internal_data.datatypes[*newtype]->is_contig = 0;
  nm_mpi_internal_data.datatypes[*newtype]->size = nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] * count * blocklength;
  nm_mpi_internal_data.datatypes[*newtype]->elements = count;
  nm_mpi_internal_data.datatypes[*newtype]->blocklens = malloc(1 * sizeof(int));
  nm_mpi_internal_data.datatypes[*newtype]->blocklens[0] = blocklength;
  nm_mpi_internal_data.datatypes[*newtype]->block_size = blocklength * nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0];
  nm_mpi_internal_data.datatypes[*newtype]->stride = stride;
  nm_mpi_internal_data.datatypes[*newtype]->lb = 0;
  nm_mpi_internal_data.datatypes[*newtype]->extent = nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] * count * blocklength;

  MPI_NMAD_TRACE("Creating new (h)vector type (%d) with size=%ld, extent=%ld, elements=%d, blocklen=%d based on type %d with a extent %ld\n",
		 *newtype, (long)nm_mpi_internal_data.datatypes[*newtype]->size, (long)nm_mpi_internal_data.datatypes[*newtype]->extent,
		 nm_mpi_internal_data.datatypes[*newtype]->elements, nm_mpi_internal_data.datatypes[*newtype]->blocklens[0],
		 oldtype, (long)nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0]);
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

int mpi_type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  int err, i;
  size_t old_size;

  MPI_NMAD_LOG_IN();

  MPI_Aint *displacements = malloc(count * sizeof(MPI_Aint));
  old_size = mpir_sizeof_datatype(oldtype);
  for(i=0 ; i<count ; i++) {
    displacements[i] = array_of_displacements[i] * old_size;
  }

  err = nm_mpi_datatype_indexed(count, array_of_blocklengths, displacements, oldtype, newtype);

  FREE_AND_SET_NULL(displacements);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_type_struct(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype *array_of_types, MPI_Datatype *newtype)
{
  int i;

  MPI_NMAD_TRACE("Creating struct derived datatype based on %d elements\n", count);
  *newtype = nm_mpi_datatype_alloc();
  nm_mpi_internal_data.datatypes[*newtype] = malloc(sizeof(nm_mpi_datatype_t));

  nm_mpi_internal_data.datatypes[*newtype]->dte_type = MPIR_STRUCT;
  nm_mpi_internal_data.datatypes[*newtype]->basic = 0;
  nm_mpi_internal_data.datatypes[*newtype]->committed = 0;
  nm_mpi_internal_data.datatypes[*newtype]->is_contig = 0;
  nm_mpi_internal_data.datatypes[*newtype]->elements = count;
  nm_mpi_internal_data.datatypes[*newtype]->size = 0;
  nm_mpi_internal_data.datatypes[*newtype]->lb = 0;

  nm_mpi_internal_data.datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  nm_mpi_internal_data.datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  nm_mpi_internal_data.datatypes[*newtype]->old_sizes = malloc(count * sizeof(size_t));
  nm_mpi_internal_data.datatypes[*newtype]->old_types = malloc(count * sizeof(MPI_Datatype));
  for(i=0 ; i<count ; i++) {
    nm_mpi_internal_data.datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    nm_mpi_internal_data.datatypes[*newtype]->old_sizes[i] = nm_mpi_datatype_get(array_of_types[i])->size;
    nm_mpi_internal_data.datatypes[*newtype]->old_types[i] = array_of_types[i];
    nm_mpi_internal_data.datatypes[*newtype]->indices[i] = array_of_displacements[i];

    MPI_NMAD_TRACE("Element %d: length %d, old_type size %ld, indice %ld\n", i, nm_mpi_internal_data.datatypes[*newtype]->blocklens[i],
		   (long)nm_mpi_internal_data.datatypes[*newtype]->old_sizes[i], (long)nm_mpi_internal_data.datatypes[*newtype]->indices[i]);
    nm_mpi_internal_data.datatypes[*newtype]->size += nm_mpi_internal_data.datatypes[*newtype]->blocklens[i] * nm_mpi_internal_data.datatypes[*newtype]->old_sizes[i];
  }
  /** We suppose here that the last field of the struct does not need
   * an alignment. In case, one sends an array of struct, the 1st
   * field of the 2nd struct immediatly follows the last field of the
   * previous struct.
   */
  nm_mpi_internal_data.datatypes[*newtype]->extent = nm_mpi_internal_data.datatypes[*newtype]->indices[count-1] + nm_mpi_internal_data.datatypes[*newtype]->blocklens[count-1] * nm_mpi_internal_data.datatypes[*newtype]->old_sizes[count-1];
  MPI_NMAD_TRACE("Creating new struct type (%d) with size=%ld and extent=%ld\n", *newtype,
		 (long)nm_mpi_internal_data.datatypes[*newtype]->size, (long)nm_mpi_internal_data.datatypes[*newtype]->extent);
  return MPI_SUCCESS;
}



/**
 * Gets the id of the next available datatype.
 */
static inline int nm_mpi_datatype_alloc(void)
{
  if (puk_int_vect_empty(nm_mpi_internal_data.datatypes_pool))
    {
      ERROR("Maximum number of datatypes created");
      return MPI_ERR_INTERN;
    }
  else
    {
      const int datatype = puk_int_vect_pop_back(nm_mpi_internal_data.datatypes_pool);
      return datatype;
    }
}

size_t mpir_sizeof_datatype(MPI_Datatype datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  return p_datatype->size;
}

nm_mpi_datatype_t* nm_mpi_datatype_get(MPI_Datatype datatype)
{
  if (datatype >= 0 && datatype < NUMBER_OF_DATATYPES)
    {
      if(tbx_unlikely(nm_mpi_internal_data.datatypes[datatype] == NULL))
	{
	  ERROR("Datatype %d invalid", datatype);
	  return NULL;
	}
      else 
	{
	  return nm_mpi_internal_data.datatypes[datatype];
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

int nm_mpi_datatype_unlock(MPI_Datatype datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  MPI_NMAD_TRACE("Unlocking datatype %d (%d)\n", datatype, p_datatype->active_communications);
  p_datatype->active_communications--;
  if(p_datatype->active_communications == 0 && p_datatype->free_requested == 1)
    {
      MPI_NMAD_TRACE("Freeing datatype %d\n", datatype);
      nm_mpi_datatype_free(datatype);
    }
  return MPI_SUCCESS;
}

static int nm_mpi_datatype_free(MPI_Datatype datatype)
 {
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype->active_communications != 0)
    {
      p_datatype->free_requested = 1;
      MPI_NMAD_TRACE("Datatype %d still in use (%d). Cannot be released\n", datatype, p_datatype->active_communications);
      return MPI_DATATYPE_ACTIVE;
    }
  else
    {
      MPI_NMAD_TRACE("Releasing datatype %d\n", datatype);
      FREE_AND_SET_NULL(nm_mpi_internal_data.datatypes[datatype]->old_sizes);
      FREE_AND_SET_NULL(nm_mpi_internal_data.datatypes[datatype]->old_types);
      if (nm_mpi_internal_data.datatypes[datatype]->dte_type == MPIR_INDEXED || nm_mpi_internal_data.datatypes[datatype]->dte_type == MPIR_STRUCT)
	{
	  FREE_AND_SET_NULL(nm_mpi_internal_data.datatypes[datatype]->blocklens);
	  FREE_AND_SET_NULL(nm_mpi_internal_data.datatypes[datatype]->indices);
	}
      if (nm_mpi_internal_data.datatypes[datatype]->dte_type == MPIR_VECTOR) 
	{
	  FREE_AND_SET_NULL(nm_mpi_internal_data.datatypes[datatype]->blocklens);
	}
      puk_int_vect_push_back(nm_mpi_internal_data.datatypes_pool, datatype);
      FREE_AND_SET_NULL(nm_mpi_internal_data.datatypes[datatype]);
      return MPI_SUCCESS;
    }
}


static int nm_mpi_datatype_indexed(int count, int *array_of_blocklengths, MPI_Aint*array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t *mpir_old_datatype;

  MPI_NMAD_TRACE("Creating indexed derived datatype based on %d elements\n", count);
  mpir_old_datatype = nm_mpi_datatype_get(oldtype);

  *newtype = nm_mpi_datatype_alloc();
  nm_mpi_internal_data.datatypes[*newtype] = malloc(sizeof(nm_mpi_datatype_t));

  nm_mpi_internal_data.datatypes[*newtype]->dte_type = MPIR_INDEXED;
  nm_mpi_internal_data.datatypes[*newtype]->basic = 0;
  nm_mpi_internal_data.datatypes[*newtype]->old_sizes = malloc(1 * sizeof(int));
  nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] = mpir_old_datatype->extent;
  nm_mpi_internal_data.datatypes[*newtype]->old_types = malloc(1 * sizeof(MPI_Datatype));
  nm_mpi_internal_data.datatypes[*newtype]->old_types[0] = oldtype;
  nm_mpi_internal_data.datatypes[*newtype]->committed = 0;
  nm_mpi_internal_data.datatypes[*newtype]->is_contig = 0;
  nm_mpi_internal_data.datatypes[*newtype]->elements = count;
  nm_mpi_internal_data.datatypes[*newtype]->lb = 0;
  nm_mpi_internal_data.datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  nm_mpi_internal_data.datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  nm_mpi_internal_data.datatypes[*newtype]->size = 0;

  for(i=0 ; i<count ; i++) {
    nm_mpi_internal_data.datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    nm_mpi_internal_data.datatypes[*newtype]->indices[i] = array_of_displacements[i];
    nm_mpi_internal_data.datatypes[*newtype]->size += nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0] * nm_mpi_internal_data.datatypes[*newtype]->blocklens[i];
    MPI_NMAD_TRACE("Element %d: length %d, indice %ld, new size %ld\n", i, nm_mpi_internal_data.datatypes[*newtype]->blocklens[i], (long)nm_mpi_internal_data.datatypes[*newtype]->indices[i], (long) nm_mpi_internal_data.datatypes[*newtype]->size);
  }
  nm_mpi_internal_data.datatypes[*newtype]->extent = (nm_mpi_internal_data.datatypes[*newtype]->indices[count-1] + mpir_old_datatype->extent * nm_mpi_internal_data.datatypes[*newtype]->blocklens[count-1]);

  MPI_NMAD_TRACE("Creating new index type (%d) with size=%ld, extent=%ld based on type %d with a extent %ld\n", *newtype,
		 (long)nm_mpi_internal_data.datatypes[*newtype]->size, (long)nm_mpi_internal_data.datatypes[*newtype]->extent,
		 oldtype, (long)nm_mpi_internal_data.datatypes[*newtype]->old_sizes[0]);
  return MPI_SUCCESS;
}


