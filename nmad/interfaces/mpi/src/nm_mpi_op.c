/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);


/* ********************************************************* */

NM_MPI_HANDLE_TYPE(operator, nm_mpi_operator_t, _NM_MPI_OP_OFFSET, 16);

static struct nm_mpi_handle_operator_s nm_mpi_operators;

static void nm_mpi_op_max(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_min(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_sum(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_prod(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_land(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_band(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_lor(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_bor(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_lxor(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_bxor(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_maxloc(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_minloc(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_replace(void*invec, void*inoutvec, int*len, MPI_Datatype*type);
static void nm_mpi_op_no_op(void*invec, void*inoutvec, int*len, MPI_Datatype*type);


/* ********************************************************* */

NM_MPI_ALIAS(MPI_Op_create,      mpi_op_create);
NM_MPI_ALIAS(MPI_Op_free,        mpi_op_free);

/* ********************************************************* */


/** store builtin operators */
static void nm_mpi_operator_store(int id, MPI_User_function*function)
{
  nm_mpi_operator_t*p_operator = nm_mpi_handle_operator_store(&nm_mpi_operators, id);
  p_operator->function = function;
  p_operator->commute = 1;
}

__PUK_SYM_INTERNAL
void nm_mpi_op_init(void)
{
  /** Initialise the collective operators */
  nm_mpi_handle_operator_init(&nm_mpi_operators);
  nm_mpi_operator_store(MPI_MAX,     &nm_mpi_op_max);
  nm_mpi_operator_store(MPI_MIN,     &nm_mpi_op_min);
  nm_mpi_operator_store(MPI_SUM,     &nm_mpi_op_sum);
  nm_mpi_operator_store(MPI_PROD,    &nm_mpi_op_prod);
  nm_mpi_operator_store(MPI_LAND,    &nm_mpi_op_land);
  nm_mpi_operator_store(MPI_BAND,    &nm_mpi_op_band);
  nm_mpi_operator_store(MPI_LOR,     &nm_mpi_op_lor);
  nm_mpi_operator_store(MPI_BOR,     &nm_mpi_op_bor);
  nm_mpi_operator_store(MPI_LXOR,    &nm_mpi_op_lxor);
  nm_mpi_operator_store(MPI_BXOR,    &nm_mpi_op_bxor);
  nm_mpi_operator_store(MPI_MINLOC,  &nm_mpi_op_minloc);
  nm_mpi_operator_store(MPI_MAXLOC,  &nm_mpi_op_maxloc);
  nm_mpi_operator_store(MPI_REPLACE, &nm_mpi_op_replace);
  nm_mpi_operator_store(MPI_NO_OP,   &nm_mpi_op_no_op);
}

__PUK_SYM_INTERNAL
void nm_mpi_op_exit(void)
{
  nm_mpi_handle_operator_finalize(&nm_mpi_operators, NULL);
}

__PUK_SYM_INTERNAL
nm_mpi_operator_t*nm_mpi_operator_get(MPI_Op op)
{
  nm_mpi_operator_t*p_operator = nm_mpi_handle_operator_get(&nm_mpi_operators, op);
  return p_operator;
}

__PUK_SYM_INTERNAL
void nm_mpi_operator_apply(void*invec, void*outvec, int count, nm_mpi_datatype_t*p_datatype,
			   nm_mpi_operator_t*p_operator)
{
  int i, j;
  for(i = 0; i < count; i++)
    {
      switch(p_datatype->combiner)
	{
	case MPI_COMBINER_NAMED:
	  p_operator->function(*(void**)invec, outvec, &count, &p_datatype->id);
	  *(void**)invec += p_datatype->extent * count;
	  return;

	case MPI_COMBINER_CONTIGUOUS:
	  {
	    nm_mpi_operator_apply(invec, outvec, p_datatype->count, p_datatype->CONTIGUOUS.p_old_type,
				  p_operator);
	  }
	  break;

	case MPI_COMBINER_DUP:
	  {
	    nm_mpi_operator_apply(invec, outvec, 1, p_datatype->DUP.p_old_type, p_operator);
	  }
	  break;

	case MPI_COMBINER_RESIZED:
	  {
	    nm_mpi_operator_apply(invec, outvec, 1, p_datatype->RESIZED.p_old_type, p_operator);
	  }
	  break;
	      
	case MPI_COMBINER_VECTOR:
	  for(j = 0; j < p_datatype->count; j++)
	    {
	      nm_mpi_operator_apply(invec,
				    outvec + j * p_datatype->VECTOR.stride * p_datatype->VECTOR.p_old_type->extent,
				    p_datatype->VECTOR.blocklength, p_datatype->VECTOR.p_old_type,
				    p_operator);
	    }
	  break;

	case MPI_COMBINER_HVECTOR:
	  for(j = 0; j < p_datatype->count; j++)
	    {
	      nm_mpi_operator_apply(invec,
				    outvec + j * p_datatype->HVECTOR.hstride,
				    p_datatype->HVECTOR.blocklength, p_datatype->HVECTOR.p_old_type,
				    p_operator);
	    }
	  break;
	      
	case MPI_COMBINER_INDEXED:
	  for(j = 0; j < p_datatype->count; j++)
	    {
	      nm_mpi_operator_apply(invec,
				    outvec + p_datatype->INDEXED.p_map[j].displacement * p_datatype->INDEXED.p_old_type->extent,
				    p_datatype->INDEXED.p_map[j].blocklength,
				    p_datatype->INDEXED.p_old_type, p_operator);
	    }
	  break;

	case MPI_COMBINER_HINDEXED:
	  for(j = 0; j < p_datatype->count; j++)
	    {
	      nm_mpi_operator_apply(invec, outvec + p_datatype->HINDEXED.p_map[j].displacement,
				    p_datatype->HINDEXED.p_map[j].blocklength,
				    p_datatype->HINDEXED.p_old_type, p_operator);
	    }
	  break;

	case MPI_COMBINER_INDEXED_BLOCK:
	  for(j = 0; j < p_datatype->count; j++)
	    {
	      nm_mpi_operator_apply(invec,
				    outvec + p_datatype->INDEXED_BLOCK.array_of_displacements[j] * p_datatype->INDEXED_BLOCK.p_old_type->extent,
				    p_datatype->INDEXED_BLOCK.blocklength,
				    p_datatype->INDEXED_BLOCK.p_old_type, p_operator);
	    }
	  break;
	      
	case MPI_COMBINER_HINDEXED_BLOCK:
	  for(j = 0; j < p_datatype->count; j++)
	    {
	      nm_mpi_operator_apply(invec, outvec + p_datatype->HINDEXED_BLOCK.array_of_displacements[j],
				    p_datatype->HINDEXED_BLOCK.blocklength,
				    p_datatype->HINDEXED_BLOCK.p_old_type, p_operator);
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
		nm_mpi_operator_apply(invec, outvec + offset * p_datatype->SUBARRAY.p_old_type->extent
				      , 1, p_datatype->SUBARRAY.p_old_type, p_operator);
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
	      nm_mpi_operator_apply(invec,
				    outvec + p_datatype->STRUCT.p_map[j].displacement,
				    p_datatype->STRUCT.p_map[j].blocklength,
				    p_datatype->STRUCT.p_map[j].p_old_type,
				    p_operator);
	    }
	  break;
	      
	default:
	  NM_MPI_FATAL_ERROR("madmpi: cannot filter datatype with combiner %d\n", p_datatype->combiner);
	}
      outvec += p_datatype->extent;
    }
}

/* ********************************************************* */


int mpi_op_create(MPI_User_function*function, int commute, MPI_Op*op)
{
  nm_mpi_operator_t*p_operator = nm_mpi_handle_operator_alloc(&nm_mpi_operators);
  p_operator->function = function;
  p_operator->commute = commute;
  *op = p_operator->id;
  return MPI_SUCCESS;
}

int mpi_op_free(MPI_Op*op)
{
  if(*op < 0 ||
     *op == MPI_OP_NULL ||
     *op < _NM_MPI_OP_OFFSET)
    {
      return MPI_ERR_OP;
    }
  nm_mpi_operator_t*p_operator = nm_mpi_handle_operator_get(&nm_mpi_operators, *op);
  nm_mpi_handle_operator_free(&nm_mpi_operators, p_operator);
  *op = MPI_OP_NULL;
  return MPI_SUCCESS;
}

/** apply a macro to integers <MPI type, C type> pairs */
#define NM_MPI_TYPES_APPLY_INTEGERS(TYPE_FUNC)			\
  TYPE_FUNC(MPI_CHAR,             char);			\
  TYPE_FUNC(MPI_UNSIGNED_CHAR,    unsigned char);		\
  TYPE_FUNC(MPI_SIGNED_CHAR,      signed char);			\
  TYPE_FUNC(MPI_BYTE,             uint8_t);			\
  TYPE_FUNC(MPI_SHORT,            short);			\
  TYPE_FUNC(MPI_UNSIGNED_SHORT,   unsigned short);		\
  TYPE_FUNC(MPI_INTEGER,          int);				\
  TYPE_FUNC(MPI_INT,              int);				\
  TYPE_FUNC(MPI_UNSIGNED,         unsigned);			\
  TYPE_FUNC(MPI_LONG,             long);			\
  TYPE_FUNC(MPI_UNSIGNED_LONG,    unsigned long);		\
  TYPE_FUNC(MPI_LONG_LONG_INT,    long long int);		\
  TYPE_FUNC(MPI_CHARACTER,        char);			\
  TYPE_FUNC(MPI_INTEGER1,         uint8_t);			\
  TYPE_FUNC(MPI_INTEGER2,         uint16_t);			\
  TYPE_FUNC(MPI_INTEGER4,         uint32_t);			\
  TYPE_FUNC(MPI_INTEGER8,         uint64_t);			\
  TYPE_FUNC(MPI_INT8_T,           int8_t);			\
  TYPE_FUNC(MPI_INT16_T,          int16_t);			\
  TYPE_FUNC(MPI_INT32_T,          int32_t);			\
  TYPE_FUNC(MPI_INT64_T,          int64_t);			\
  TYPE_FUNC(MPI_UINT8_T,          uint8_t);			\
  TYPE_FUNC(MPI_UINT16_T,         uint16_t);			\
  TYPE_FUNC(MPI_UINT32_T,         uint32_t);			\
  TYPE_FUNC(MPI_UINT64_T,         uint64_t);			\
  TYPE_FUNC(MPI_AINT,             MPI_Aint);			\
  TYPE_FUNC(MPI_OFFSET,           MPI_Offset);			\
  TYPE_FUNC(MPI_C_BOOL,           _Bool);			\
  TYPE_FUNC(MPI_COUNT,            MPI_Count);

/** apply a macro to floating point <MPI type, C type> pairs */
#define NM_MPI_TYPES_APPLY_FLOATS(TYPE_FUNC)			\
  TYPE_FUNC(MPI_FLOAT,            float);			\
  TYPE_FUNC(MPI_DOUBLE_PRECISION, double);			\
  TYPE_FUNC(MPI_DOUBLE,           double);			\
  TYPE_FUNC(MPI_LONG_DOUBLE,      long double);			\
  TYPE_FUNC(MPI_REAL,             float);

/** apply a macro to COMPLEX <MPI type, C type> pairs */
#define NM_MPI_TYPES_APPLY_COMPLEX(TYPE_FUNC)			\
  TYPE_FUNC(MPI_COMPLEX,          complex float);		\
  TYPE_FUNC(MPI_DOUBLE_COMPLEX,   complex double);		\
  TYPE_FUNC(MPI_C_FLOAT_COMPLEX,  float _Complex);		\
  TYPE_FUNC(MPI_C_DOUBLE_COMPLEX, double _Complex);		\
  TYPE_FUNC(MPI_C_LONG_DOUBLE_COMPLEX, long double _Complex);


/** generate a switch case stanza for a given type and operation */
#define CASE_OP(__mpi_type__, __type__, BODY)	\
  case __mpi_type__:							\
  {									\
    int i;								\
    __type__*i_invec = (__type__*)invec;				\
    __type__*i_inoutvec = (__type__*)inoutvec;				\
    for(i = 0; i < *len; i++)						\
      {									\
	BODY;								\
      }									\
    break;								\
  }

static void nm_mpi_op_max(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_MAX(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  if(i_invec[i] > i_inoutvec[i])				\
	    i_inoutvec[i] = i_invec[i];					\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_MAX);
      NM_MPI_TYPES_APPLY_FLOATS(CASE_OP_MAX);
      
    default: 
      {
	NM_MPI_FATAL_ERROR("Datatype %d for MAX Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_min(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_MIN(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  if(i_invec[i] < i_inoutvec[i])				\
	    i_inoutvec[i] = i_invec[i];					\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_MIN);
      NM_MPI_TYPES_APPLY_FLOATS(CASE_OP_MIN);
      
    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for MIN Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_sum(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_SUM(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  i_inoutvec[i] += i_invec[i];					\
	  )
  switch(*type) 
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_SUM);
      NM_MPI_TYPES_APPLY_FLOATS(CASE_OP_SUM);
      NM_MPI_TYPES_APPLY_COMPLEX(CASE_OP_SUM);
      
    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for SUM Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_prod(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_PROD(__mpi_type__, __type__)	\
  CASE_OP(__mpi_type__, __type__,		\
	  i_inoutvec[i] *= i_invec[i];		\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_PROD);
      NM_MPI_TYPES_APPLY_FLOATS(CASE_OP_PROD);
      NM_MPI_TYPES_APPLY_COMPLEX(CASE_OP_PROD);

    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for PROD Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_land(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_LAND(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  i_inoutvec[i] = i_inoutvec[i] && i_invec[i];			\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_LAND);
      NM_MPI_TYPES_APPLY_FLOATS(CASE_OP_LAND);

    default : 
      {
	NM_MPI_FATAL_ERROR("Datatype %d for LAND Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_band(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_BAND(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  i_inoutvec[i] = i_inoutvec[i] & i_invec[i];			\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_BAND);
      
    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for BAND Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_lor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_LOR(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  i_inoutvec[i] = i_inoutvec[i] || i_invec[i];			\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_LOR);
      NM_MPI_TYPES_APPLY_FLOATS(CASE_OP_LOR);

    default: 
      {
	NM_MPI_FATAL_ERROR("Datatype %d for LOR Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_bor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_BOR(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  i_inoutvec[i] = i_inoutvec[i] | i_invec[i];			\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_BOR);

    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for BOR Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_lxor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_LXOR(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  if((!i_inoutvec[i] && i_invec[i]) ||				\
	     (i_inoutvec[i] && !i_invec[i]))				\
	    i_inoutvec[i] = 1;						\
	  else								\
	    i_inoutvec[i] = 0;						\
	  )
  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_LXOR);

    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for LXOR Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_bxor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
#define CASE_OP_BXOR(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  i_inoutvec[i] = i_inoutvec[i] ^ i_invec[i];			\
	  )
  switch (*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_BXOR);

    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for BXOR Reduce operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_maxloc(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i, _len = *len;
  const nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*type);

  if(p_datatype->combiner == MPI_COMBINER_CONTIGUOUS && p_datatype->count == 2)
    {
      MPI_Datatype oldtype = p_datatype->CONTIGUOUS.p_old_type->id;
      /** Set the actual length */
      _len = *len;
      /** Perform the operation */
      switch (oldtype) 
	{
#define DO_MAXLOC(__type1__, __type2__)					\
	  struct _maxloc_s { __type1__ val; __type2__ loc; };		\
	  struct _maxloc_s*a = inoutvec;				\
	  const struct _maxloc_s*b = invec;				\
	  for(i = 0; i < _len; i += 1 ) {				\
	    if(a[i].val == b[i].val)					\
	      a[i].loc = tbx_min(a[i].loc, b[i].loc);			\
	    else if(a[i].val < b[i].val) {				\
	      a[i].val = b[i].val;					\
	      a[i].loc = b[i].loc;					\
	    }								\
	  }
	  
	case MPI_CHAR             : { DO_MAXLOC(char, char); break; }
	case MPI_UNSIGNED_CHAR    : { DO_MAXLOC(unsigned char, unsigned char); break; }
	case MPI_BYTE             : { DO_MAXLOC(uint8_t, uint8_t); break; }
	case MPI_SHORT            : { DO_MAXLOC(short, short); break; }
	case MPI_UNSIGNED_SHORT   : { DO_MAXLOC(unsigned short, unsigned short); break; }
	case MPI_INTEGER: case MPI_INT: { DO_MAXLOC(int, int); break; }
	case MPI_UNSIGNED         : { DO_MAXLOC(unsigned, unsigned); break; }
	case MPI_LONG             : { DO_MAXLOC(long, long); break; }
	case MPI_UNSIGNED_LONG    : { DO_MAXLOC(unsigned long, unsigned long); break; }
	case MPI_FLOAT            : { DO_MAXLOC(float, float); break; }
	case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_MAXLOC(double, double); break; }
	case MPI_LONG_DOUBLE      : { DO_MAXLOC(long double, long double); break; }
	case MPI_LONG_LONG_INT    : { DO_MAXLOC(long long, long long); break; }
	case MPI_INTEGER4         : { DO_MAXLOC(uint32_t, uint32_t); break; }
	case MPI_INTEGER8         : { DO_MAXLOC(uint64_t, uint64_t); break; }
	  
	default:
	  NM_MPI_FATAL_ERROR("Datatype Contiguous(%d) for MAXLOC Reduce operation", *type);
	  break;
	}
    }
  else if(p_datatype->combiner == MPI_COMBINER_NAMED && p_datatype->count == 2)
    {
      _len = *len;
      switch(*type)
	{
	case MPI_2DOUBLE_PRECISION: { DO_MAXLOC(double, double); break; }
	case MPI_2INT:              { DO_MAXLOC(int, int); break; }
	case MPI_FLOAT_INT:         { DO_MAXLOC(float, int); break; }
	case MPI_DOUBLE_INT:        { DO_MAXLOC(double, int); break; }
	case MPI_LONG_DOUBLE_INT:   { DO_MAXLOC(long double, int); break; }
	case MPI_LONG_INT:          { DO_MAXLOC(long, int); break; }
	case MPI_SHORT_INT:         { DO_MAXLOC(short, int); break; }
	default:
	  NM_MPI_FATAL_ERROR("Datatype Basic(%d) for MAXLOC Reduce operation", *type);
	  break;
	}
    }
  else
    {
      fprintf(stderr, "type %d, elements %lld\n", p_datatype->combiner, p_datatype->count);
      NM_MPI_FATAL_ERROR("Datatype %d for MAXLOC Reduce operation", *type);
    }
}

static void nm_mpi_op_minloc(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i, _len = *len;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*type);

  if(p_datatype->combiner == MPI_COMBINER_CONTIGUOUS && p_datatype->count == 2)
    {
      MPI_Datatype oldtype = p_datatype->CONTIGUOUS.p_old_type->id;
      /** Set the actual length */
      _len = *len;
      /** Perform the operation */
      switch (oldtype) 
	{
#define DO_MINLOC(__type1__, __type2__)					\
	  struct _maxloc_s { __type1__ val; __type2__ loc; };		\
	  struct _maxloc_s*a = inoutvec;				\
	  const struct _maxloc_s*b = invec;				\
	  for(i = 0; i < _len; i += 1 ) {				\
	    if(a[i].val == b[i].val)					\
	      a[i].loc = tbx_min(a[i].loc, b[i].loc);			\
	    else if(a[i].val > b[i].val) {				\
	      a[i].val = b[i].val;					\
	      a[i].loc = b[i].loc;					\
	    }								\
	  }

	case MPI_CHAR             : { DO_MINLOC(char, char); break; }
	case MPI_UNSIGNED_CHAR    : { DO_MINLOC(unsigned char, unsigned char); break; }
	case MPI_BYTE             : { DO_MINLOC(uint8_t, uint8_t); break; }
	case MPI_SHORT            : { DO_MINLOC(short, short); break; }
	case MPI_UNSIGNED_SHORT   : { DO_MINLOC(unsigned short, unsigned short); break; }
	case MPI_INTEGER: case MPI_INT: { DO_MINLOC(int, int); break; }
	case MPI_UNSIGNED         : { DO_MINLOC(unsigned, unsigned); break; }
	case MPI_LONG             : { DO_MINLOC(long, long); break; }
	case MPI_UNSIGNED_LONG    : { DO_MINLOC(unsigned long, unsigned long); break; }
	case MPI_FLOAT            : { DO_MINLOC(float, float); break; }
	case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_MINLOC(double, double); break; }
	case MPI_LONG_DOUBLE      : { DO_MINLOC(long double, long double); break; }
	case MPI_LONG_LONG_INT    : { DO_MINLOC(long long, long long); break; }
	case MPI_INTEGER4         : { DO_MINLOC(uint32_t, uint32_t); break; }
	case MPI_INTEGER8         : { DO_MINLOC(uint64_t, uint64_t); break; }
	default:
	  NM_MPI_FATAL_ERROR("Datatype Contiguous(%d) for MINLOC Reduce operation", *type);
	  break;
	}
    }
  else if(p_datatype->combiner == MPI_COMBINER_NAMED && p_datatype->count == 2)
    {
      _len = *len;
      switch(*type)
	{
	case MPI_2DOUBLE_PRECISION: { DO_MINLOC(double, double); break; }
	case MPI_2INT:              { DO_MINLOC(int, int); break; }
	case MPI_FLOAT_INT:         { DO_MINLOC(float, int); break; }
	case MPI_DOUBLE_INT:        { DO_MINLOC(double, int); break; }
	case MPI_LONG_DOUBLE_INT:   { DO_MINLOC(long double, int); break; }
	case MPI_LONG_INT:          { DO_MINLOC(long, int); break; }
	case MPI_SHORT_INT:         { DO_MINLOC(short, int); break; }
	default:
	  NM_MPI_FATAL_ERROR("Datatype Basic(%d) for MAXLOC Reduce operation", *type);
	  break;
	}
    }

  else
    {
      NM_MPI_FATAL_ERROR("Datatype %d for MINLOC Reduce operation", *type);
    }
}

static void nm_mpi_op_replace(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  
#define CASE_OP_REPLACE(__mpi_type__, __type__)				\
  CASE_OP(__mpi_type__, __type__,					\
	  i_inoutvec[i] = i_invec[i];					\
	  )

#define DO_REPLACE2(__type1__, __type2__)				\
	  struct _replace_s { __type1__ val; __type2__ loc; };		\
	  struct _replace_s*a = inoutvec;				\
	  const struct _replace_s*b = invec;				\
	  for(i = 0; i < *len; i += 1 ) {				\
	    a[i] = b[i];						\
	  }

  switch(*type)
    {
      NM_MPI_TYPES_APPLY_INTEGERS(CASE_OP_REPLACE);
      NM_MPI_TYPES_APPLY_FLOATS(CASE_OP_REPLACE);
      NM_MPI_TYPES_APPLY_COMPLEX(CASE_OP_REPLACE);
      
    case MPI_2INT:              { DO_REPLACE2(int, int); break; }
    case MPI_SHORT_INT:         { DO_REPLACE2(short, int); break; }
    case MPI_LONG_INT:          { DO_REPLACE2(long, int); break; }
    case MPI_FLOAT_INT:         { DO_REPLACE2(float, int); break; }
    case MPI_DOUBLE_INT:        { DO_REPLACE2(double, int); break; }
    case MPI_LONG_DOUBLE_INT:   { DO_REPLACE2(long double, int); break; }
    case MPI_2INTEGER:          { DO_REPLACE2(float, float); break; }
    case MPI_2REAL:             { DO_REPLACE2(float, float); break; }
    case MPI_2DOUBLE_PRECISION: { DO_REPLACE2(double, double); break; }
    default:
      {
	NM_MPI_FATAL_ERROR("Datatype %d for REPLACE operation", *type);
	break;
      }
    }
}

static void nm_mpi_op_no_op(void*invec, void*inoutvec, int*len, MPI_Datatype*type){}
