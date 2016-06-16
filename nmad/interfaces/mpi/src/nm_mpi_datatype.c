/*
 * NewMadeleine
 * Copyright (C) 2014-2016 (see AUTHORS file)
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
PADICO_MODULE_HOOK(MadMPI);

NM_MPI_HANDLE_TYPE(datatype, nm_mpi_datatype_t, _NM_MPI_DATATYPE_OFFSET, 64);

static struct nm_mpi_handle_datatype_s nm_mpi_datatypes;


/** store builtin datatypes */
static void nm_mpi_datatype_store(int id, size_t size, int elements, const char*name);
static void nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype);
static void nm_mpi_datatype_properties_compute(nm_mpi_datatype_t*p_datatype);
static void nm_mpi_datatype_update_bounds(int blocklength, MPI_Aint displacement, nm_mpi_datatype_t*p_oldtype, nm_mpi_datatype_t*p_newtype);
static void nm_mpi_datatype_traversal_apply(const void*_content, nm_data_apply_t apply, void*_context);
static void nm_mpi_datatype_data_properties_compute(struct nm_data_s*p_data);

const struct nm_data_ops_s nm_mpi_datatype_ops =
  {
    .p_traversal          = &nm_mpi_datatype_traversal_apply,
    .p_properties_compute = &nm_mpi_datatype_data_properties_compute
  };


/* ********************************************************* */

NM_MPI_ALIAS(MPI_Type_size,                  mpi_type_size);
NM_MPI_ALIAS(MPI_Type_size_x,                mpi_type_size_x);
NM_MPI_ALIAS(MPI_Type_get_extent,            mpi_type_get_extent);
NM_MPI_ALIAS(MPI_Type_get_extent_x,          mpi_type_get_extent_x);
NM_MPI_ALIAS(MPI_Type_get_true_extent,       mpi_type_get_true_extent);
NM_MPI_ALIAS(MPI_Type_get_true_extent_x,     mpi_type_get_true_extent_x);
NM_MPI_ALIAS(MPI_Type_extent,                mpi_type_extent);
NM_MPI_ALIAS(MPI_Type_lb,                    mpi_type_lb);
NM_MPI_ALIAS(MPI_Type_ub,                    mpi_type_ub);
NM_MPI_ALIAS(MPI_Type_dup,                   mpi_type_dup);
NM_MPI_ALIAS(MPI_Type_create_resized,        mpi_type_create_resized);
NM_MPI_ALIAS(MPI_Type_commit,                mpi_type_commit);
NM_MPI_ALIAS(MPI_Type_free,                  mpi_type_free);
NM_MPI_ALIAS(MPI_Type_contiguous,            mpi_type_contiguous);
NM_MPI_ALIAS(MPI_Type_vector,                mpi_type_vector);
NM_MPI_ALIAS(MPI_Type_hvector,               mpi_type_hvector);
NM_MPI_ALIAS(MPI_Type_indexed,               mpi_type_indexed);
NM_MPI_ALIAS(MPI_Type_hindexed,              mpi_type_hindexed);
NM_MPI_ALIAS(MPI_Type_struct,                mpi_type_struct);
NM_MPI_ALIAS(MPI_Type_create_hvector,        mpi_type_hvector);
NM_MPI_ALIAS(MPI_Type_create_hindexed,       mpi_type_create_hindexed);
NM_MPI_ALIAS(MPI_Type_create_indexed_block,  mpi_type_create_indexed_block);
NM_MPI_ALIAS(MPI_Type_create_hindexed_block, mpi_type_create_hindexed_block);
NM_MPI_ALIAS(MPI_Type_create_subarray,       mpi_type_create_subarray);
NM_MPI_ALIAS(MPI_Type_create_darray,         mpi_type_create_darray);
NM_MPI_ALIAS(MPI_Type_create_struct,         mpi_type_create_struct);
NM_MPI_ALIAS(MPI_Type_get_envelope,          mpi_type_get_envelope);
NM_MPI_ALIAS(MPI_Type_get_contents,          mpi_type_get_contents);
NM_MPI_ALIAS(MPI_Type_set_name,              mpi_type_set_name);
NM_MPI_ALIAS(MPI_Type_get_name,              mpi_type_get_name);
NM_MPI_ALIAS(MPI_Pack,                       mpi_pack);
NM_MPI_ALIAS(MPI_Unpack,                     mpi_unpack);
NM_MPI_ALIAS(MPI_Pack_size,                  mpi_pack_size);

/* ********************************************************* */


__PUK_SYM_INTERNAL
void nm_mpi_datatype_init(void)
{
  nm_mpi_handle_datatype_init(&nm_mpi_datatypes);

  /* Initialise the basic datatypes */

  /* C types */
  nm_mpi_datatype_store(MPI_CHAR,               sizeof(char), 1, "MPI_CHAR");
  nm_mpi_datatype_store(MPI_UNSIGNED_CHAR,      sizeof(unsigned char), 1, "MPI_UNSIGNED_CHAR");
  nm_mpi_datatype_store(MPI_SIGNED_CHAR,        sizeof(signed char), 1, "MPI_SIGNED_CHAR");
  nm_mpi_datatype_store(MPI_BYTE,               1, 1, "MPI_BYTE");
  nm_mpi_datatype_store(MPI_SHORT,              sizeof(signed short), 1, "MPI_SHORT");
  nm_mpi_datatype_store(MPI_UNSIGNED_SHORT,     sizeof(unsigned short), 1, "MPI_UNSIGNED_INT");
  nm_mpi_datatype_store(MPI_INT,                sizeof(signed int), 1, "MPI_INT");
  nm_mpi_datatype_store(MPI_UNSIGNED,           sizeof(unsigned int), 1, "MPI_UNSIGNED");
  nm_mpi_datatype_store(MPI_LONG,               sizeof(signed long), 1, "MPI_LONG");
  nm_mpi_datatype_store(MPI_UNSIGNED_LONG,      sizeof(unsigned long), 1, "MPI_UNSIGNED_LONG");
  nm_mpi_datatype_store(MPI_FLOAT,              sizeof(float), 1, "MPI_FLOAT");
  nm_mpi_datatype_store(MPI_DOUBLE,             sizeof(double), 1, "MPI_DOUBLE");
  nm_mpi_datatype_store(MPI_LONG_DOUBLE,        sizeof(long double), 1, "MPI_LONG_DOUBLE");
  nm_mpi_datatype_store(MPI_LONG_LONG_INT,      sizeof(long long int), 1, "MPI_LONG_LONG_INT");
  nm_mpi_datatype_store(MPI_UNSIGNED_LONG_LONG, sizeof(unsigned long long int), 1, "MPI_UNSIGNED_LONG_LONG");

  /* MPI-2 C additions */
  nm_mpi_datatype_store(MPI_WCHAR,              sizeof(wchar_t), 1, "MPI_WCHAR");
  nm_mpi_datatype_store(MPI_INT8_T,             sizeof(int8_t), 1, "MPI_INT8_T");
  nm_mpi_datatype_store(MPI_INT16_T,            sizeof(int16_t), 1, "MPI_INT16_T");
  nm_mpi_datatype_store(MPI_INT32_T,            sizeof(int32_t), 1, "MPI_INT32_T");
  nm_mpi_datatype_store(MPI_INT64_T,            sizeof(int64_t), 1, "MPI_INT64_T");
  nm_mpi_datatype_store(MPI_UINT8_T,            sizeof(uint8_t), 1, "MPI_UINT8_T");
  nm_mpi_datatype_store(MPI_UINT16_T,           sizeof(uint16_t), 1, "MPI_UINT16_T");
  nm_mpi_datatype_store(MPI_UINT32_T,           sizeof(uint32_t), 1, "MPI_UINT32_T");
  nm_mpi_datatype_store(MPI_UINT64_T,           sizeof(uint64_t), 1, "MPI_UINT64_T");
  nm_mpi_datatype_store(MPI_AINT,               sizeof(MPI_Aint), 1, "MPI_AINT");
  nm_mpi_datatype_store(MPI_OFFSET,             sizeof(MPI_Offset), 1, "MPI_OFFSET");
  nm_mpi_datatype_store(MPI_C_BOOL,             sizeof(_Bool), 1, "MPI_C_BOOLD");
  nm_mpi_datatype_store(MPI_C_FLOAT_COMPLEX,    sizeof(float _Complex), 1, "MPI_C_FLOAT_COMPLEX");
  nm_mpi_datatype_store(MPI_C_DOUBLE_COMPLEX,   sizeof(double _Complex), 1, "MPI_C_DOUBLE_COMPLEX");
  nm_mpi_datatype_store(MPI_C_LONG_DOUBLE_COMPLEX, sizeof(long double _Complex), 1, "MPI_C_LONG_DOUBLE_COMPLEX");
  
  /* FORTRAN types */
  nm_mpi_datatype_store(MPI_CHARACTER,          sizeof(char), 1, "MPI_CHARACTER");
  nm_mpi_datatype_store(MPI_LOGICAL,            sizeof(float), 1, "MPI_LOGICAL");
  nm_mpi_datatype_store(MPI_REAL,               sizeof(float), 1, "MPI_REAL");
  nm_mpi_datatype_store(MPI_REAL4,              4, 1, "MPI_REAL4");
  nm_mpi_datatype_store(MPI_REAL8,              8, 1, "MPI_REAL8");
  nm_mpi_datatype_store(MPI_REAL16,             16, 1, "MPI_REAL16");
  nm_mpi_datatype_store(MPI_DOUBLE_PRECISION,   sizeof(double), 1, "MPI_DOUBLE_PRECISION");
  nm_mpi_datatype_store(MPI_INTEGER,            sizeof(float), 1, "MPI_INTEGER");
  nm_mpi_datatype_store(MPI_INTEGER1,           1, 1, "MPI_INTEGER1");
  nm_mpi_datatype_store(MPI_INTEGER2,           2, 1, "MPI_INTEGER2");
  nm_mpi_datatype_store(MPI_INTEGER4,           4, 1, "MPI_INTEGER4");
  nm_mpi_datatype_store(MPI_INTEGER8,           8, 1, "MPI_INTEGER8");
  nm_mpi_datatype_store(MPI_PACKED,             sizeof(char), 1, "MPI_PACKED");

  /* FORTRAN COMPLEX types */
  nm_mpi_datatype_store(MPI_COMPLEX,            sizeof(complex float), 1, "MPI_COMPLEX");
  nm_mpi_datatype_store(MPI_DOUBLE_COMPLEX,     sizeof(complex double), 1, "MPI_DOUBLE_COMPLEX");

  /* C struct types */
  nm_mpi_datatype_store(MPI_2INT,               sizeof(struct { int    val; int loc; }), 2, "MPI_2INT");
  nm_mpi_datatype_store(MPI_SHORT_INT,          sizeof(struct { short  val; int loc; }), 2, "MPI_SHORT_INT");
  nm_mpi_datatype_store(MPI_LONG_INT,           sizeof(struct { long   val; int loc; }), 2, "MPI_LONG_INT");
  nm_mpi_datatype_store(MPI_FLOAT_INT,          sizeof(struct { float  val; int loc; }), 2, "MPI_FLOAT_INT");
  nm_mpi_datatype_store(MPI_DOUBLE_INT,         sizeof(struct { double val; int loc; }), 2, "MPI_DOUBLE_INT");
  nm_mpi_datatype_store(MPI_LONG_DOUBLE_INT,    sizeof(struct { long double val; int loc; }), 2, "MPI_LONG_DOUBLE_INT");

  /* FORTRAN struct types */
  nm_mpi_datatype_store(MPI_2INTEGER,           sizeof(float) + sizeof(float), 2, "MPI_2INTEGER");
  nm_mpi_datatype_store(MPI_2REAL,              sizeof(float) + sizeof(float), 2, "MPI_2REAL");
  nm_mpi_datatype_store(MPI_2DOUBLE_PRECISION,  sizeof(double) + sizeof(double), 2, "MPI_2DOUBLE_PRECISION");
  /* MPI-3 */
  nm_mpi_datatype_store(MPI_COUNT,              sizeof(MPI_Count), 1, "MPI_COUNT");

  /* TODO- Fortran types: 2COMPLEX, 2DOUBLE_COMPLEX */
  nm_mpi_datatype_store(MPI_UB,                 0, 0, "MPI_UB");
  nm_mpi_datatype_store(MPI_LB,                 0, 0, "MPI_LB");
}

__PUK_SYM_INTERNAL
void nm_mpi_datatype_exit(void)
{
  nm_mpi_handle_datatype_finalize(&nm_mpi_datatypes, &nm_mpi_datatype_free);
}

/* ********************************************************* */

/** store a basic datatype */
static void nm_mpi_datatype_store(int id, size_t size, int count, const char*name)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_handle_datatype_store(&nm_mpi_datatypes, id);
  /* initialize known properties */
  p_datatype->committed = 1;
  p_datatype->is_contig = 1;
  p_datatype->combiner = MPI_COMBINER_NAMED;
  p_datatype->refcount = 2;
  p_datatype->lb = 0;
  p_datatype->size = size;
  p_datatype->count = count;
  p_datatype->elements = count;
  p_datatype->extent = size;
  p_datatype->name = strdup(name);
  p_datatype->true_lb = 0;
  p_datatype->true_extent = size;
  /* compute properties through traversal */
  nm_mpi_datatype_properties_compute(p_datatype);
  /* check computed values consistency*/
  assert(p_datatype->is_contig);
  assert(p_datatype->props.size == size);
  assert(p_datatype->props.blocks == 1);
}

/** allocate a new datatype and init with default values */
static nm_mpi_datatype_t*nm_mpi_datatype_alloc(nm_mpi_type_combiner_t combiner, size_t size, int count)
{
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  p_newtype->combiner = combiner;
  p_newtype->committed = 0;
  p_newtype->size = size;
  p_newtype->count = count;
  p_newtype->elements = count;
  p_newtype->lb = MPI_UNDEFINED;
  p_newtype->extent = MPI_UNDEFINED;
  p_newtype->refcount = 1;
  p_newtype->is_contig = 0;
  p_newtype->name = NULL;
  p_newtype->true_lb = MPI_UNDEFINED;
  p_newtype->true_extent = MPI_UNDEFINED;
  return p_newtype;
}

/* ********************************************************* */

int mpi_type_size(MPI_Datatype datatype, int *size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *size = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  *size = p_datatype->size;
  return MPI_SUCCESS;
}

int mpi_type_size_x(MPI_Datatype datatype, MPI_Count*size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *size = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  *size = p_datatype->size;
  return MPI_SUCCESS;
}

int mpi_type_get_extent(MPI_Datatype datatype, MPI_Aint*lb, MPI_Aint*extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  if(lb != NULL)
    *lb = p_datatype->lb;
  if(extent != NULL)
    *extent = p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_get_extent_x(MPI_Datatype datatype, MPI_Count*lb, MPI_Count*extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
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
    {
      *extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  *extent = p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_get_true_extent(MPI_Datatype datatype,  MPI_Aint*true_lb, MPI_Aint*true_extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *true_extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  if(true_lb != NULL)
    *true_lb = p_datatype->true_lb;
  if(true_extent != NULL)
    *true_extent = p_datatype->true_extent;
  return MPI_SUCCESS;
}

int mpi_type_get_true_extent_x(MPI_Datatype datatype, MPI_Count*true_lb, MPI_Count*true_extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *true_extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  if(true_lb != NULL)
    *true_lb = p_datatype->true_lb;
  if(true_extent != NULL)
    *true_extent = p_datatype->true_extent;
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
  *displacement = p_datatype->lb + p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_dup(MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_DUP, p_oldtype->size, 1);
  *newtype = p_newtype->id;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->extent    = p_oldtype->extent;
  p_newtype->lb        = p_oldtype->lb;
  p_newtype->DUP.p_old_type = p_oldtype;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_RESIZED, p_oldtype->size, 1);
  *newtype = p_newtype->id;
  p_newtype->is_contig = (p_oldtype->is_contig && (lb == 0) && (extent == p_oldtype->size));
  p_newtype->lb        = lb;
  p_newtype->extent    = extent;
  p_newtype->RESIZED.p_old_type = p_oldtype;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_commit(MPI_Datatype*datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  nm_mpi_datatype_properties_compute(p_datatype);
  assert(p_datatype->lb != MPI_UNDEFINED);
  assert(p_datatype->extent != MPI_UNDEFINED);
  p_datatype->committed = 1;
  return MPI_SUCCESS;
}

int mpi_type_free(MPI_Datatype*datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
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

int mpi_type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_CONTIGUOUS, p_oldtype->size * count, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->lb = p_oldtype->lb;
  p_newtype->extent = p_oldtype->extent * count;
  p_newtype->CONTIGUOUS.p_old_type = p_oldtype;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_VECTOR, p_oldtype->size * count * blocklength, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->lb = p_oldtype->lb;
  p_newtype->extent = p_oldtype->extent * blocklength + (count - 1) * stride * p_oldtype->extent;
  p_newtype->VECTOR.p_old_type = p_oldtype;
  p_newtype->VECTOR.stride = stride;
  p_newtype->VECTOR.blocklength = blocklength;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_hvector(int count, int blocklength, MPI_Aint hstride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_HVECTOR, p_oldtype->size * count * blocklength, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->lb = p_oldtype->lb;
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
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
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
      nm_mpi_datatype_update_bounds(p_newtype->INDEXED.p_map[i].blocklength,
				    p_newtype->INDEXED.p_map[i].displacement * p_oldtype->extent,
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_hindexed(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype) 
{
  int i;
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
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
      nm_mpi_datatype_update_bounds(p_newtype->HINDEXED.p_map[i].blocklength,
				    p_newtype->HINDEXED.p_map[i].displacement,
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  return mpi_type_hindexed(count, (int*)array_of_blocklengths, (MPI_Aint*)array_of_displacements, oldtype, newtype);
}

int mpi_type_create_indexed_block(int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_INDEXED_BLOCK, count * blocklength * p_oldtype->size, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->INDEXED_BLOCK.p_old_type = p_oldtype;
  p_newtype->INDEXED_BLOCK.blocklength = blocklength;
  p_newtype->INDEXED_BLOCK.array_of_displacements = malloc(count * sizeof(int));
  int i;
  for(i = 0; i < count ; i++)
    {
      p_newtype->INDEXED_BLOCK.array_of_displacements[i] = array_of_displacements[i];
      nm_mpi_datatype_update_bounds(p_newtype->INDEXED_BLOCK.blocklength,
				    p_newtype->INDEXED_BLOCK.array_of_displacements[i] * p_oldtype->extent,
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_create_hindexed_block(int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_HINDEXED_BLOCK, count * blocklength * p_oldtype->size, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->HINDEXED_BLOCK.p_old_type = p_oldtype;
  p_newtype->HINDEXED_BLOCK.blocklength = blocklength;
  p_newtype->HINDEXED_BLOCK.array_of_displacements = malloc(count * sizeof(int));
  int i;
  for(i = 0; i < count ; i++)
    {
      p_newtype->HINDEXED_BLOCK.array_of_displacements[i] = array_of_displacements[i];
      nm_mpi_datatype_update_bounds(p_newtype->HINDEXED_BLOCK.blocklength,
				    p_newtype->HINDEXED_BLOCK.array_of_displacements[i],
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_oldtype->refcount++;
  return MPI_SUCCESS;
}

int mpi_type_struct(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype *array_of_types, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_STRUCT, 0, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
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
      nm_mpi_datatype_update_bounds(p_newtype->STRUCT.p_map[i].blocklength,
				    p_newtype->STRUCT.p_map[i].displacement,
				    p_newtype->STRUCT.p_map[i].p_old_type,
				    p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  return MPI_SUCCESS;
}

int mpi_type_create_subarray(int ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  p_oldtype->refcount++;
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_SUBARRAY, p_oldtype->size, 1);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->SUBARRAY.p_dims = malloc(ndims * sizeof(struct nm_mpi_type_subarray_dim_s));
  p_newtype->SUBARRAY.ndims = ndims;
  p_newtype->SUBARRAY.order = order;
  p_newtype->SUBARRAY.p_old_type = p_oldtype;
  MPI_Count elements = 1;
  MPI_Aint last_offset = 0, first_offset = 0;
  MPI_Count blocklength = 1;
  int d;
  for(d = ndims - 1; d >= 0; d--)
    {
      p_newtype->SUBARRAY.p_dims[d].size     = array_of_sizes[d];
      p_newtype->SUBARRAY.p_dims[d].subsize  = array_of_subsizes[d];
      p_newtype->SUBARRAY.p_dims[d].start    = array_of_starts[d];
      elements     *= array_of_subsizes[d];
      blocklength  *= array_of_sizes[d];
      first_offset += array_of_starts[d] * blocklength;
      last_offset  += (array_of_starts[d] + array_of_subsizes[d]) * blocklength;
    }
  nm_mpi_datatype_update_bounds(1, first_offset, p_oldtype, p_newtype);
  nm_mpi_datatype_update_bounds(1, last_offset, p_oldtype, p_newtype);
  p_newtype->size     = elements * p_oldtype->size;
  p_newtype->elements = elements;
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  return MPI_SUCCESS;
}

int mpi_type_create_darray(int size, int rank, int ndims,
			   const int array_of_gsizes[], const int array_of_distribs[],
			   const int array_of_dargs[], const int array_of_psizes[],
			   int order, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  padico_warning("madmpi: MPI_Type_create_darray()- unsupported.\n");
  return MPI_ERR_OTHER;
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
    case MPI_COMBINER_DUP:
      *num_integers  = 0;
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
      *num_integers  = p_datatype->count;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_HINDEXED:
      *num_integers  = 1;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_INDEXED_BLOCK:
      *num_integers  = 1;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      *num_integers  = 1;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_SUBARRAY:
      *num_integers  = 3 * p_datatype->SUBARRAY.ndims;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_STRUCT:
      *num_integers  = p_datatype->count;
      *num_addresses = p_datatype->count;
      *num_datatypes = p_datatype->count;
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
    case MPI_COMBINER_DUP:
      array_of_integers[0]  = p_datatype->count;
      array_of_datatypes[0] = p_datatype->DUP.p_old_type->id;
      break;
    case MPI_COMBINER_RESIZED:
      array_of_integers[0]  = p_datatype->count;
      array_of_datatypes[0] = p_datatype->RESIZED.p_old_type->id;
      break;
    case MPI_COMBINER_VECTOR:
      array_of_integers[0]  = p_datatype->count;
      array_of_integers[1]  = p_datatype->VECTOR.stride;
      array_of_integers[2]  = p_datatype->VECTOR.blocklength;
      array_of_datatypes[0] = p_datatype->VECTOR.p_old_type->id;
      break;
    case MPI_COMBINER_HVECTOR:
      array_of_integers[0]  = p_datatype->count;
      array_of_integers[1]  = p_datatype->HVECTOR.hstride;
      array_of_integers[2]  = p_datatype->HVECTOR.blocklength;
      array_of_datatypes[0] = p_datatype->HVECTOR.p_old_type->id;
      break;
    case MPI_COMBINER_INDEXED:
      array_of_integers[0]  = p_datatype->count;
      break;
    case MPI_COMBINER_HINDEXED:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_INDEXED_BLOCK:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_SUBARRAY:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_STRUCT:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
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
  if((p_datatype->refcount == 0) && (p_datatype->id >= _NM_MPI_DATATYPE_OFFSET))
    {
      nm_mpi_datatype_free(p_datatype);
    }
  return MPI_SUCCESS;
}

static void nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype)
{
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
    case MPI_COMBINER_INDEXED_BLOCK:
      if(p_datatype->INDEXED_BLOCK.array_of_displacements != NULL)
	FREE_AND_SET_NULL(p_datatype->INDEXED_BLOCK.array_of_displacements);
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      if(p_datatype->HINDEXED_BLOCK.array_of_displacements != NULL)
	FREE_AND_SET_NULL(p_datatype->HINDEXED_BLOCK.array_of_displacements);
      break;
    case MPI_COMBINER_SUBARRAY:
      if(p_datatype->SUBARRAY.p_dims != NULL)
	FREE_AND_SET_NULL(p_datatype->SUBARRAY.p_dims);
      break;
    case MPI_COMBINER_STRUCT:
      if(p_datatype->STRUCT.p_map != NULL)
	FREE_AND_SET_NULL(p_datatype->STRUCT.p_map);
      break;
    default:
      break;
    }
  if(p_datatype->name != NULL)
    {
      free(p_datatype->name);
    }
  nm_mpi_handle_datatype_free(&nm_mpi_datatypes, p_datatype);
}

static inline int nm_mpi_datatype_traversal_may_unroll(const struct nm_mpi_datatype_s*const p_datatype, int count)
{
  return ((p_datatype->is_contig && (p_datatype->size == p_datatype->extent) && (p_datatype->lb == 0)) || (count == 0));
}
						   
/** apply a function to every chunk of data in datatype */
static void nm_mpi_datatype_traversal_apply(const void*_content, const nm_data_apply_t apply, void*_context)
{
  const struct nm_data_mpi_datatype_s*const p_data = _content;
  const struct nm_mpi_datatype_s*const p_datatype = p_data->p_datatype;
  const int count = p_data->count;
  void*ptr = p_data->ptr;
  assert(p_datatype->refcount > 0);
  if(nm_mpi_datatype_traversal_may_unroll(p_datatype, count))
    {
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
		  { .ptr = ptr, .p_datatype = p_datatype->CONTIGUOUS.p_old_type, .count = p_datatype->count };
		nm_mpi_datatype_traversal_apply(&sub, apply, _context);
	      }
	      break;

	    case MPI_COMBINER_DUP:
	      {
		const struct nm_data_mpi_datatype_s sub = 
		  { .ptr = ptr, .p_datatype = p_datatype->DUP.p_old_type, .count = 1 };
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
	      if(nm_mpi_datatype_traversal_may_unroll(p_datatype->VECTOR.p_old_type, p_datatype->VECTOR.blocklength))
		{
		  for(j = 0; j < p_datatype->count; j++)
		    {
		      void*const lptr = ptr + j * p_datatype->VECTOR.stride * p_datatype->VECTOR.p_old_type->extent;
		      (*apply)(lptr, p_datatype->VECTOR.blocklength * p_datatype->VECTOR.p_old_type->size, _context);
		    }
		}
	      else
		{
		  for(j = 0; j < p_datatype->count; j++)
		    {
		      const struct nm_data_mpi_datatype_s sub = 
			{ .ptr        = ptr + j * p_datatype->VECTOR.stride * p_datatype->VECTOR.p_old_type->extent,
			  .p_datatype = p_datatype->VECTOR.p_old_type,
			  .count      = p_datatype->VECTOR.blocklength };
		      nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		    }
		}
	      break;

	    case MPI_COMBINER_HVECTOR:
	      if(nm_mpi_datatype_traversal_may_unroll(p_datatype->HVECTOR.p_old_type, p_datatype->HVECTOR.blocklength))
		{
		  for(j = 0; j < p_datatype->count; j++)
		    {
		      void*const lptr = ptr + j * p_datatype->HVECTOR.hstride;
		      (*apply)(lptr, p_datatype->HVECTOR.blocklength * p_datatype->HVECTOR.p_old_type->size, _context);
		    }
		}
	      else
		{
		  for(j = 0; j < p_datatype->count; j++)
		    {
		      const struct nm_data_mpi_datatype_s sub = 
			{ .ptr        = ptr + j * p_datatype->HVECTOR.hstride, 
			  .p_datatype = p_datatype->HVECTOR.p_old_type,
			  .count      = p_datatype->HVECTOR.blocklength };
		      nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		    }
		}
	      break;
	      
	    case MPI_COMBINER_INDEXED:
	      for(j = 0; j < p_datatype->count; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + p_datatype->INDEXED.p_map[j].displacement * p_datatype->INDEXED.p_old_type->extent,
		      .p_datatype = p_datatype->INDEXED.p_old_type,
		      .count      = p_datatype->INDEXED.p_map[j].blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;

	    case MPI_COMBINER_HINDEXED:
	      for(j = 0; j < p_datatype->count; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + p_datatype->HINDEXED.p_map[j].displacement,
		      .p_datatype = p_datatype->HINDEXED.p_old_type,
		      .count      = p_datatype->HINDEXED.p_map[j].blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;

	    case MPI_COMBINER_INDEXED_BLOCK:
	      for(j = 0; j < p_datatype->count; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + p_datatype->INDEXED_BLOCK.array_of_displacements[j] * p_datatype->INDEXED_BLOCK.p_old_type->extent,
		      .p_datatype = p_datatype->INDEXED_BLOCK.p_old_type,
		      .count      = p_datatype->INDEXED_BLOCK.blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;
	      
	    case MPI_COMBINER_HINDEXED_BLOCK:
	      for(j = 0; j < p_datatype->count; j++)
		{
		  const struct nm_data_mpi_datatype_s sub = 
		    { .ptr        = ptr + p_datatype->HINDEXED_BLOCK.array_of_displacements[j],
		      .p_datatype = p_datatype->HINDEXED_BLOCK.p_old_type,
		      .count      = p_datatype->HINDEXED_BLOCK.blocklength };
		  nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		}
	      break;

	    case MPI_COMBINER_SUBARRAY:
	      {
		int done = 0;
		int cur[p_datatype->SUBARRAY.ndims]; /* current N-dim index */
		int k;
		/* init index */
		for(k = 0; k < p_datatype->SUBARRAY.ndims; k++)
		  {
		    cur[k] = p_datatype->SUBARRAY.p_dims[k].start;
		  }
		while(!done)
		  {
		    /* calculate linear offset for current N-dim index */
		    MPI_Count blocklength = 1;
		    MPI_Aint offset = 0;
		    for(k = 0; k < p_datatype->SUBARRAY.ndims; k++)
		      {
			const int d = (p_datatype->SUBARRAY.order == MPI_ORDER_C) ?
			  (p_datatype->SUBARRAY.ndims - k - 1) : k;
			offset      += cur[d] * blocklength;
			blocklength *= p_datatype->SUBARRAY.p_dims[d].size;
		      }
		    /* apply datatype function */
		    const struct nm_data_mpi_datatype_s sub =
		      { .ptr        = ptr + offset * p_datatype->SUBARRAY.p_old_type->extent,
			.p_datatype = p_datatype->SUBARRAY.p_old_type,
			.count      = 1 };
		    nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		    /* increment N-dim index */
		    for(k = 0; k < p_datatype->SUBARRAY.ndims; k++)
		      {
			const int d = (p_datatype->SUBARRAY.order == MPI_ORDER_C) ?
			  (p_datatype->SUBARRAY.ndims - k - 1) : k;
			if(cur[d] + 1 < p_datatype->SUBARRAY.p_dims[d].start + p_datatype->SUBARRAY.p_dims[d].subsize)
			  {
			    cur[d]++;
			    break;
			  }
			cur[d] = p_datatype->SUBARRAY.p_dims[d].start; /* reset dim and propagate carry */
		      }
		    done = (k >= p_datatype->SUBARRAY.ndims);
		  }
	      }
	      break;

	    case MPI_COMBINER_STRUCT:
	      for(j = 0; j < p_datatype->count; j++)
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

/** update bounds (lb, ub, extent) in newtype to take into account the given block of data */
static void nm_mpi_datatype_update_bounds(int blocklength, MPI_Aint displacement, nm_mpi_datatype_t*p_oldtype, nm_mpi_datatype_t*p_newtype)
{
  p_newtype->elements += blocklength - 1; /* 'elements' was initialized with 'count'; don't count it twice */
  if(blocklength > 0)
    {
      const intptr_t cur_lb = p_newtype->lb;
      const intptr_t cur_ub = ((p_newtype->lb != MPI_UNDEFINED) && (p_newtype->extent != MPI_UNDEFINED)) ? (p_newtype->lb + p_newtype->extent) : MPI_UNDEFINED;
      intptr_t new_lb = cur_lb;
      intptr_t new_ub = cur_ub;
      if(p_oldtype->id == MPI_LB)
	{
	  new_lb = displacement;
	}
      else if(p_oldtype->id == MPI_UB)
	{
	  new_ub = displacement;
	}
      else
	{
	  const intptr_t local_lb = displacement + p_oldtype->lb ;
	  const intptr_t local_ub = displacement + p_oldtype->lb + blocklength * p_oldtype->extent;
	  if((cur_lb == MPI_UNDEFINED) || (local_lb < cur_lb))
	    {
	      new_lb = local_lb;
	    }
	  if((cur_ub == MPI_UNDEFINED) || (local_ub > cur_ub))
	    {
	      new_ub = local_ub;
	    }
	}
      if(new_lb != MPI_UNDEFINED)
	p_newtype->lb = new_lb;
      if(new_lb != MPI_UNDEFINED)
	{
	  if(new_ub != MPI_UNDEFINED)
	    p_newtype->extent = new_ub - new_lb;
	  else
	    p_newtype->extent = 0 - new_lb; /* negative extent in case only lb is given */
	}
    }
}

static void nm_mpi_datatype_data_properties_compute(struct nm_data_s*p_data)
{
  const struct nm_data_mpi_datatype_s*const p_data_mpi = nm_data_mpi_datatype_content(p_data);
  const struct nm_mpi_datatype_s*const p_datatype = p_data_mpi->p_datatype;
  const int count = p_data_mpi->count;
  const struct nm_data_properties_s props =
    {
      .blocks = count * p_datatype->props.blocks,
      .size   = count * p_datatype->props.size,
      .is_contig = p_datatype->props.is_contig && ((count < 2) ||
						   (p_datatype->lb == 0 && p_datatype->extent == p_datatype->size))
    };
  p_data->props = props;
}

struct nm_mpi_datatype_properties_context_s
{
  void*baseptr;
  void*blockend; /**< end of previous block */
  struct nm_mpi_datatype_properties_s
  {
    intptr_t true_lb;
    intptr_t true_ub;
  } props;
  struct nm_data_properties_s nm_data_props;
};
static void nm_mpi_datatype_properties_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_mpi_datatype_properties_context_s*p_context = _context;
  p_context->nm_data_props.size += len;
  p_context->nm_data_props.blocks += 1;
  if(p_context->nm_data_props.is_contig)
    {
      if((p_context->blockend != NULL) && (ptr != p_context->blockend))
	p_context->nm_data_props.is_contig = 0;
    }
  p_context->blockend = ptr + len;
  const intptr_t local_lb = ptr - p_context->baseptr;
  if(local_lb < p_context->props.true_lb || p_context->props.true_lb == MPI_UNDEFINED)
    p_context->props.true_lb = local_lb;
  const intptr_t local_ub = (ptr + len) - p_context->baseptr;
  if(local_ub > p_context->props.true_ub || p_context->props.true_ub == MPI_UNDEFINED)
    p_context->props.true_ub = local_ub;
}
static void nm_mpi_datatype_properties_compute(nm_mpi_datatype_t*p_datatype)
{
  struct nm_mpi_datatype_properties_context_s context =
    {
      .baseptr = NULL,
      .blockend = NULL,
      .props.true_lb = MPI_UNDEFINED,
      .props.true_ub = MPI_UNDEFINED,
      .nm_data_props.is_contig = 1,
      .nm_data_props.blocks    = 0,
      .nm_data_props.size      = 0
    };
  const struct nm_data_mpi_datatype_s content = { .ptr = NULL, .p_datatype = p_datatype, .count = 1 };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_properties_apply, &context);
  p_datatype->is_contig   = context.nm_data_props.is_contig;
  p_datatype->true_lb     = context.props.true_lb;
  p_datatype->true_extent = context.props.true_ub - context.props.true_lb;
  p_datatype->props       = context.nm_data_props;
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
void nm_mpi_datatype_pack(void*outbuf, const void*inbuf, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s context = { .pack_ptr = (void*)outbuf };
  const struct nm_data_mpi_datatype_s content = { .ptr = (void*)inbuf, .p_datatype = p_datatype, .count = count };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_pack_memcpy, &context);
}

/** Unpack data from a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_unpack(const void*inbuf, void*outbuf, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s context = { .pack_ptr = (void*)inbuf };
  const struct nm_data_mpi_datatype_s content = { .ptr = outbuf, .p_datatype = p_datatype, .count = count };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_unpack_memcpy, &context);
}

/** Copy data using datatype layout
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_copy(const void*src_buf, nm_mpi_datatype_t*p_src_type, int src_count,
			  void*dest_buf, nm_mpi_datatype_t*p_dest_type, int dest_count)
{
  if(p_src_type == p_dest_type && p_src_type->is_contig && src_count == dest_count)
    {
      int i;
      for(i = 0; i < src_count; i++)
	{
	  memcpy(dest_buf + i * p_dest_type->extent, src_buf + i * p_src_type->extent, p_src_type->size);
	}
    }
  else if(src_count > 0)
    {
      void*ptr = malloc(p_src_type->size * src_count);
      nm_mpi_datatype_pack(ptr, src_buf, p_src_type, src_count);
      nm_mpi_datatype_unpack(ptr, dest_buf, p_dest_type, dest_count);
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

int mpi_type_set_name(MPI_Datatype datatype, char*type_name)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(p_datatype->name != NULL)
    {
      free(p_datatype->name);
    }
  p_datatype->name = strndup(type_name, MPI_MAX_OBJECT_NAME);
  return MPI_SUCCESS;
}

int mpi_type_get_name(MPI_Datatype datatype, char*type_name, int*resultlen)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(p_datatype->name != NULL)
    {
      strncpy(type_name, p_datatype->name, MPI_MAX_OBJECT_NAME);
    }
  else
    {
      type_name[0] = '\0';
    }
  *resultlen = strlen(type_name);
  return MPI_SUCCESS;
}
