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

/*
 * mpi_types.h
 * ===========
 */

#ifndef MPI_TYPES_H
#define MPI_TYPES_H

#include <tbx.h>

#define MPI_PROC_NULL   (-1)
#define MPI_ANY_SOURCE 	(-2)
#define MPI_ROOT        (-3)
#define MPI_ANY_TAG     (-1)

#define MPI_BOTTOM      (void *)0

/* error return classes */
#define MPI_SUCCESS          0      /* Successful return code */
#define MPI_ERR_BUFFER       1
#define MPI_ERR_COUNT        2
#define MPI_ERR_TYPE         3
#define MPI_ERR_TAG          4
#define MPI_ERR_COMM         5
#define MPI_ERR_RANK         6
#define MPI_ERR_ROOT         7
#define MPI_ERR_GROUP        8
#define MPI_ERR_OP           9
#define MPI_ERR_TOPOLOGY     10
#define MPI_ERR_DIMS         11
#define MPI_ERR_ARG          12
#define MPI_ERR_UNKNOWN      13
#define MPI_ERR_TRUNCATE     14
#define MPI_ERR_OTHER        15
#define MPI_ERR_INTERN       16
#define MPI_ERR_IN_STATUS    17
#define MPI_ERR_PENDING      18
#define MPI_ERR_REQUEST      19
#define MPI_ERR_LASTCODE     1073741823

#define MPI_DATATYPE_ACTIVE  1      /* Datatype still in use */

/* Pre-defined constants */
#define MPI_UNDEFINED      (-32766)
#define MPI_UNDEFINED_RANK MPI_UNDEFINED
#define MPI_KEYVAL_INVALID 0

/* For supported thread levels */
#define MPI_THREAD_SINGLE 0
#define MPI_THREAD_FUNNELED 1
#define MPI_THREAD_SERIALIZED 2
#define MPI_THREAD_MULTIPLE 3

typedef size_t MPI_Aint;

typedef struct {
  int count;
  int size;
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
} MPI_Status;

typedef struct {
  char request[128];
} MPI_Request;

#define MPI_REQUEST_NULL   ((MPI_Request){'\0'})

typedef int MPI_Group;

typedef int MPI_Comm;
#define MPI_COMM_WORLD ((MPI_Comm)91)
#define MPI_COMM_SELF  ((MPI_Comm)92)
#define MPI_COMM_NULL  ((MPI_Comm)0)

/* Data types */
typedef int MPI_Datatype;
#define MPI_DATATYPE_NULL    ((MPI_Datatype)0)
#define MPI_CHAR             ((MPI_Datatype)1)
#define MPI_UNSIGNED_CHAR    ((MPI_Datatype)2)
#define MPI_BYTE             ((MPI_Datatype)3)
#define MPI_SHORT            ((MPI_Datatype)4)
#define MPI_UNSIGNED_SHORT   ((MPI_Datatype)5)
#define MPI_INT              ((MPI_Datatype)6)
#define MPI_UNSIGNED         ((MPI_Datatype)7)
#define MPI_LONG             ((MPI_Datatype)8)
#define MPI_UNSIGNED_LONG    ((MPI_Datatype)9)
#define MPI_FLOAT            ((MPI_Datatype)10)
#define MPI_DOUBLE           ((MPI_Datatype)11)
#define MPI_LONG_DOUBLE      ((MPI_Datatype)12)
#define MPI_LONG_LONG_INT    ((MPI_Datatype)13)
#define MPI_LONG_LONG        ((MPI_Datatype)13)

#define MPI_COMPLEX          ((MPI_Datatype)23)
#define MPI_DOUBLE_COMPLEX   ((MPI_Datatype)24)
#define MPI_LOGICAL          ((MPI_Datatype)25)
#define MPI_REAL	     ((MPI_Datatype)26)
#define MPI_DOUBLE_PRECISION ((MPI_Datatype)27)
#define MPI_INTEGER	     ((MPI_Datatype)28)

/* Collective operations */
typedef int MPI_Op;
#define MPI_OP_NULL (MPI_Op)(999)
#define MPI_MAX     (MPI_Op)(100)
#define MPI_MIN     (MPI_Op)(101)
#define MPI_SUM     (MPI_Op)(102)
#define MPI_PROD    (MPI_Op)(103)
#define MPI_LAND    (MPI_Op)(104)
#define MPI_BAND    (MPI_Op)(105)
#define MPI_LOR     (MPI_Op)(106)
#define MPI_BOR     (MPI_Op)(107)
#define MPI_LXOR    (MPI_Op)(108)
#define MPI_BXOR    (MPI_Op)(109)
#define MPI_MINLOC  (MPI_Op)(110)
#define MPI_MAXLOC  (MPI_Op)(111)

#define MPI_MAX_PROCESSOR_NAME 256
#define MPI_MAX_ERROR_STRING   512
#define MPI_MAX_NAME_STRING    256

/* extended modes */
typedef int MPI_Communication_Mode;
#define MPI_IS_NOT_COMPLETED    ((MPI_Communication_Mode)tbx_false)
#define MPI_IS_COMPLETED        ((MPI_Communication_Mode)tbx_true)
#define MPI_IMMEDIATE_MODE      ((MPI_Communication_Mode)-1)
#define MPI_READY_MODE          ((MPI_Communication_Mode)-2)

#define MPI_STATUS_IGNORE	(MPI_Status *)0
#define MPI_STATUSES_IGNORE	(MPI_Status *)0
#define MPI_STATUS_SIZE		4

/* Error handlers */
typedef int MPI_Errhandler;
#define MPI_ERRHANDLER_NULL ((MPI_Errhandler)0)

#endif /* MPI_TYPES_H */
