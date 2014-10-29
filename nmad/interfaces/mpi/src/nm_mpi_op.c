/*
 * NewMadeleine
 * Copyright (C) 2006-2014 (see AUTHORS file)
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
  nm_mpi_operator_store(MPI_MAX,    &nm_mpi_op_max);
  nm_mpi_operator_store(MPI_MIN,    &nm_mpi_op_min);
  nm_mpi_operator_store(MPI_SUM,    &nm_mpi_op_sum);
  nm_mpi_operator_store(MPI_PROD,   &nm_mpi_op_prod);
  nm_mpi_operator_store(MPI_LAND,   &nm_mpi_op_land);
  nm_mpi_operator_store(MPI_BAND,   &nm_mpi_op_band);
  nm_mpi_operator_store(MPI_LOR,    &nm_mpi_op_lor);
  nm_mpi_operator_store(MPI_BOR,    &nm_mpi_op_bor);
  nm_mpi_operator_store(MPI_LXOR,   &nm_mpi_op_lxor);
  nm_mpi_operator_store(MPI_BXOR,   &nm_mpi_op_bxor);
  nm_mpi_operator_store(MPI_MINLOC, &nm_mpi_op_minloc);
  nm_mpi_operator_store(MPI_MAXLOC, &nm_mpi_op_maxloc);
}

__PUK_SYM_INTERNAL
void nm_mpi_op_exit(void)
{
  nm_mpi_handle_operator_finalize(&nm_mpi_operators);
}

__PUK_SYM_INTERNAL
nm_mpi_operator_t*nm_mpi_operator_get(MPI_Op op)
 {
   nm_mpi_operator_t*p_operator = nm_mpi_handle_operator_get(&nm_mpi_operators, op);
   return p_operator;
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


static void nm_mpi_op_max(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */
#warning Implement Operations on complex types
#define DO_MAX(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i]; \
	  }

  case MPI_CHAR             : { DO_MAX(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_MAX(unsigned char); break; }
  case MPI_BYTE             : { DO_MAX(uint8_t); break; }
  case MPI_SHORT            : { DO_MAX(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_MAX(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_MAX(int); break; }
  case MPI_UNSIGNED         : { DO_MAX(unsigned); break; }
  case MPI_LONG             : { DO_MAX(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_MAX(unsigned long); break; }
  case MPI_FLOAT            : { DO_MAX(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_MAX(double); break; }
  case MPI_LONG_DOUBLE      : { DO_MAX(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_MAX(long long); break; }
  case MPI_INTEGER4         : { DO_MAX(uint32_t); break; }
  case MPI_INTEGER8         : { DO_MAX(uint64_t); break; }
    default : {
      ERROR("Datatype %d for MAX Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_min(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */
#define DO_MIN(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i]; \
	  }

  case MPI_CHAR             : { DO_MIN(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_MIN(unsigned char); break; }
  case MPI_BYTE             : { DO_MIN(uint8_t); break; }
  case MPI_SHORT            : { DO_MIN(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_MIN(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_MIN(int); break; }
  case MPI_UNSIGNED         : { DO_MIN(unsigned); break; }
  case MPI_LONG             : { DO_MIN(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_MIN(unsigned long); break; }
  case MPI_FLOAT            : { DO_MIN(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_MIN(double); break; }
  case MPI_LONG_DOUBLE      : { DO_MIN(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_MIN(long long); break; }
  case MPI_INTEGER4         : { DO_MIN(uint32_t); break; }
  case MPI_INTEGER8         : { DO_MIN(uint64_t); break; }

    default : {
      ERROR("Datatype %d for MIN Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_sum(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
#define DO_SUM(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  nm_mpi_datatype_t *dtype = nm_mpi_datatype_get(*type); \
	  for(i=0 ; i<*len* (dtype)->elements ; i++) {			\
		  i_inoutvec[i] += i_invec[i];				\
	  }

  case MPI_CHAR             : { DO_SUM(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_SUM(unsigned char); break; }
  case MPI_BYTE             : { DO_SUM(uint8_t); break; }
  case MPI_SHORT            : { DO_SUM(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_SUM(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_SUM(int); break; }
  case MPI_UNSIGNED         : { DO_SUM(unsigned); break; }
  case MPI_LONG             : { DO_SUM(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_SUM(unsigned long); break; }
  case MPI_FLOAT            : { DO_SUM(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_SUM(double); break; }
  case MPI_LONG_DOUBLE      : { DO_SUM(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_SUM(long long); break; }
  case MPI_INTEGER4         : { DO_SUM(uint32_t); break; }
  case MPI_INTEGER8         : { DO_SUM(uint64_t); break; }
  case MPI_DOUBLE_COMPLEX   : { DO_SUM(double); break; }
  case MPI_COMPLEX          : { DO_SUM(float); break; }

    default : {
      ERROR("Datatype %d for SUM Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_prod(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */
#define DO_PROD(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;	\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;	\
	  for(i=0 ; i<*len ; i++) {			\
		  i_inoutvec[i] *= i_invec[i];		\
      }

  case MPI_CHAR             : { DO_PROD(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_PROD(unsigned char); break; }
  case MPI_BYTE             : { DO_PROD(uint8_t); break; }
  case MPI_SHORT            : { DO_PROD(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_PROD(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_PROD(int); break; }
  case MPI_UNSIGNED         : { DO_PROD(unsigned); break; }
  case MPI_LONG             : { DO_PROD(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_PROD(unsigned long); break; }
  case MPI_FLOAT            : { DO_PROD(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_PROD(double); break; }
  case MPI_LONG_DOUBLE      : { DO_PROD(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_PROD(long long); break; }
  case MPI_INTEGER4         : { DO_PROD(uint32_t); break; }
  case MPI_INTEGER8         : { DO_PROD(uint64_t); break; }
    default : {
      ERROR("Datatype %d for PROD Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_land(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */

#define DO_LAND(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  i_inoutvec[i] = i_inoutvec[i] && i_invec[i];		\
	  }

  case MPI_CHAR             : { DO_LAND(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_LAND(unsigned char); break; }
  case MPI_BYTE             : { DO_LAND(uint8_t); break; }
  case MPI_SHORT            : { DO_LAND(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_LAND(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_LAND(int); break; }
  case MPI_UNSIGNED         : { DO_LAND(unsigned); break; }
  case MPI_LONG             : { DO_LAND(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_LAND(unsigned long); break; }
  case MPI_FLOAT            : { DO_LAND(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_LAND(double); break; }
  case MPI_LONG_DOUBLE      : { DO_LAND(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_LAND(long long); break; }
  case MPI_INTEGER4         : { DO_LAND(uint32_t); break; }
  case MPI_INTEGER8         : { DO_LAND(uint64_t); break; }
    default : {
      ERROR("Datatype %d for LAND Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_band(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */

#define DO_BAND(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  i_inoutvec[i] = i_inoutvec[i] & i_invec[i];		\
	  }

  case MPI_CHAR             : { DO_BAND(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_BAND(unsigned char); break; }
  case MPI_BYTE             : { DO_BAND(uint8_t); break; }
  case MPI_SHORT            : { DO_BAND(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_BAND(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_BAND(int); break; }
  case MPI_UNSIGNED         : { DO_BAND(unsigned); break; }
  case MPI_LONG             : { DO_BAND(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_BAND(unsigned long); break; }
//  case MPI_FLOAT            : { DO_BAND(float); break; }
//  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_BAND(double); break; }
//  case MPI_LONG_DOUBLE      : { DO_BAND(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_BAND(long long); break; }
  case MPI_INTEGER4         : { DO_BAND(uint32_t); break; }
  case MPI_INTEGER8         : { DO_BAND(uint64_t); break; }

    default : {
      ERROR("Datatype %d for BAND Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_lor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */

#define DO_LOR(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  i_inoutvec[i] = i_inoutvec[i] || i_invec[i];		\
	  }

  case MPI_CHAR             : { DO_LOR(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_LOR(unsigned char); break; }
  case MPI_BYTE             : { DO_LOR(uint8_t); break; }
  case MPI_SHORT            : { DO_LOR(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_LOR(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_LOR(int); break; }
  case MPI_UNSIGNED         : { DO_LOR(unsigned); break; }
  case MPI_LONG             : { DO_LOR(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_LOR(unsigned long); break; }
  case MPI_FLOAT            : { DO_LOR(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_LOR(double); break; }
  case MPI_LONG_DOUBLE      : { DO_LOR(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_LOR(long long); break; }
  case MPI_INTEGER4         : { DO_LOR(uint32_t); break; }
  case MPI_INTEGER8         : { DO_LOR(uint64_t); break; }

    default : {
      ERROR("Datatype %d for LOR Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_bor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */

#define DO_BOR(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  i_inoutvec[i] = i_inoutvec[i] | i_invec[i];		\
	  }

  case MPI_CHAR             : { DO_BOR(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_BOR(unsigned char); break; }
  case MPI_BYTE             : { DO_BOR(uint8_t); break; }
  case MPI_SHORT            : { DO_BOR(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_BOR(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_BOR(int); break; }
  case MPI_UNSIGNED         : { DO_BOR(unsigned); break; }
  case MPI_LONG             : { DO_BOR(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_BOR(unsigned long); break; }
//  case MPI_FLOAT            : { DO_BOR(float); break; }
//  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_BOR(double); break; }
//  case MPI_LONG_DOUBLE      : { DO_BOR(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_BOR(long long); break; }
  case MPI_INTEGER4         : { DO_BOR(uint32_t); break; }
  case MPI_INTEGER8         : { DO_BOR(uint64_t); break; }

    default : {
      ERROR("Datatype %d for BOR Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_lxor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */

#define DO_LXOR(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  if ((!i_inoutvec[i] && i_invec[i]) ||			\
		      (i_inoutvec[i] && !i_invec[i]))			\
			  i_inoutvec[i] = 1;				\
		  else							\
			  i_inoutvec[i] = 0;				\
	  }

  case MPI_CHAR             : { DO_LXOR(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_LXOR(unsigned char); break; }
  case MPI_BYTE             : { DO_LXOR(uint8_t); break; }
  case MPI_SHORT            : { DO_LXOR(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_LXOR(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_LXOR(int); break; }
  case MPI_UNSIGNED         : { DO_LXOR(unsigned); break; }
  case MPI_LONG             : { DO_LXOR(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_LXOR(unsigned long); break; }
  case MPI_FLOAT            : { DO_LXOR(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_LXOR(double); break; }
  case MPI_LONG_DOUBLE      : { DO_LXOR(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_LXOR(long long); break; }
  case MPI_INTEGER4         : { DO_LXOR(uint32_t); break; }
  case MPI_INTEGER8         : { DO_LXOR(uint64_t); break; }

    default : {
      ERROR("Datatype %d for LXOR Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_bxor(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i;
  switch (*type) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */

#define DO_BXOR(__type__)						\
	  __type__ *i_invec = (__type__ *) invec;			\
	  __type__ *i_inoutvec = (__type__ *) inoutvec;			\
	  for(i=0 ; i<*len ; i++) {					\
		  i_inoutvec[i] = i_inoutvec[i] ^ i_invec[i];		\
	  }

  case MPI_CHAR             : { DO_BXOR(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_BXOR(unsigned char); break; }
  case MPI_BYTE             : { DO_BXOR(uint8_t); break; }
  case MPI_SHORT            : { DO_BXOR(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_BXOR(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_BXOR(int); break; }
  case MPI_UNSIGNED         : { DO_BXOR(unsigned); break; }
  case MPI_LONG             : { DO_BXOR(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_BXOR(unsigned long); break; }
//  case MPI_FLOAT            : { DO_BXOR(float); break; }
//  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_BXOR(double); break; }
//  case MPI_LONG_DOUBLE      : { DO_BXOR(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_BXOR(long long); break; }
  case MPI_INTEGER4         : { DO_BXOR(uint32_t); break; }
  case MPI_INTEGER8         : { DO_BXOR(uint64_t); break; }

    default : {
      ERROR("Datatype %d for BXOR Reduce operation", *type);
      break;
    }
  }
}

static void nm_mpi_op_maxloc(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i, _len = *len;
  nm_mpi_datatype_t *dtype = nm_mpi_datatype_get(*type);

  if ((dtype)->dte_type == NM_MPI_DATATYPE_CONTIG && ((dtype)->elements == 2))
    {
      MPI_Datatype oldtype = (dtype)->old_types[0];
      /** Set the actual length */
      _len = *len * (dtype)->elements;
      /** Perform the operation */
      switch (oldtype) 
	{
#define DO_MAXLOC(__type__)						\
	    __type__ *a = (__type__ *)inoutvec; const __type__ *b = (__type__ *)invec; \
	    for(i = 0; i < _len; i+=2 ) {				\
	      if(a[i] == b[i])						\
		a[i+1] = tbx_min(a[i+1],b[i+1]);			\
	      else if(a[i] < b[i]) {					\
		a[i] = b[i];						\
		a[i+1] = b[i+1];					\
	      }								\
	    }

	case MPI_CHAR             : { DO_MAXLOC(char); break; }
	case MPI_UNSIGNED_CHAR    : { DO_MAXLOC(unsigned char); break; }
	case MPI_BYTE             : { DO_MAXLOC(uint8_t); break; }
	case MPI_SHORT            : { DO_MAXLOC(short); break; }
	case MPI_UNSIGNED_SHORT   : { DO_MAXLOC(unsigned short); break; }
	case MPI_INTEGER: case MPI_INT: { DO_MAXLOC(int); break; }
	case MPI_UNSIGNED         : { DO_MAXLOC(unsigned); break; }
	case MPI_LONG             : { DO_MAXLOC(long); break; }
	case MPI_UNSIGNED_LONG    : { DO_MAXLOC(unsigned long); break; }
	case MPI_FLOAT            : { DO_MAXLOC(float); break; }
	case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_MAXLOC(double); break; }
	case MPI_LONG_DOUBLE      : { DO_MAXLOC(long double); break; }
	case MPI_LONG_LONG_INT    : { DO_MAXLOC(long long); break; }
	case MPI_INTEGER4         : { DO_MAXLOC(uint32_t); break; }
	case MPI_INTEGER8         : { DO_MAXLOC(uint64_t); break; }
	  
	default:
	  ERROR("Datatype Contiguous(%d) for MAXLOC Reduce operation", *type);
	  break;
	}
    }
  else if((dtype)->dte_type == NM_MPI_DATATYPE_BASIC && (dtype)->elements == 2)
    {
      _len = *len * (dtype)->elements;
      switch(*type)
	{
	case MPI_2DOUBLE_PRECISION: { DO_MAXLOC(double); break; }
	case MPI_2INT: { DO_MAXLOC(int); break; }
	default:
	  ERROR("Datatype Basic(%d) for MAXLOC Reduce operation", *type);
	  break;
	}
    }
  else
    {
      fprintf(stderr, "type %d, elements %d\n",   (dtype)->dte_type, ((dtype)->elements));
      ERROR("Datatype %d for MAXLOC Reduce operation", *type);
    }
}

static void nm_mpi_op_minloc(void*invec, void*inoutvec, int*len, MPI_Datatype*type)
{
  int i, _len = *len;
  nm_mpi_datatype_t *dtype = nm_mpi_datatype_get(*type);

  if ((dtype)->dte_type == NM_MPI_DATATYPE_CONTIG && ((dtype)->elements == 2)) {
    MPI_Datatype oldtype = (dtype)->old_types[0];

    /** Set the actual length */
    _len = *len * (dtype)->elements;

    /** Perform the operation */
    switch (oldtype) {
	  /* todo: this only works for 'simple' types
	   * Implement this for other types (eg. complex)
	   */

#define DO_MINLOC(__type__)						\
	    __type__ *a = (__type__ *)inoutvec; __type__ *b = (__type__ *)invec; \
	    for ( i=0; i<_len; i+=2 ) {					\
		    if (a[i] == b[i])					\
			    a[i+1] = tbx_min(a[i+1],b[i+1]);		\
		    else if (a[i] > b[i]) {				\
			    a[i]   = b[i];				\
			    a[i+1] = b[i+1];				\
		    }							\
	    }

  case MPI_CHAR             : { DO_MINLOC(char); break; }
  case MPI_UNSIGNED_CHAR    : { DO_MINLOC(unsigned char); break; }
  case MPI_BYTE             : { DO_MINLOC(uint8_t); break; }
  case MPI_SHORT            : { DO_MINLOC(short); break; }
  case MPI_UNSIGNED_SHORT   : { DO_MINLOC(unsigned short); break; }
  case MPI_INTEGER: case MPI_INT: { DO_MINLOC(int); break; }
  case MPI_UNSIGNED         : { DO_MINLOC(unsigned); break; }
  case MPI_LONG             : { DO_MINLOC(long); break; }
  case MPI_UNSIGNED_LONG    : { DO_MINLOC(unsigned long); break; }
  case MPI_FLOAT            : { DO_MINLOC(float); break; }
  case MPI_DOUBLE_PRECISION : case MPI_DOUBLE: { DO_MINLOC(double); break; }
  case MPI_LONG_DOUBLE      : { DO_MINLOC(long double); break; }
  case MPI_LONG_LONG_INT    : { DO_MINLOC(long long); break; }
  case MPI_INTEGER4         : { DO_MINLOC(uint32_t); break; }
  case MPI_INTEGER8         : { DO_MINLOC(uint64_t); break; }
    default:
      ERROR("Datatype Contiguous(%d) for MINLOC Reduce operation", *type);
      break;
    }
  }
  else {
    ERROR("Datatype %d for MINLOC Reduce operation", *type);
  }
}