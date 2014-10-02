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

#define MPI_BSEND_OVERHEAD 0
/** \addtogroup mpi_interface */
/* @{ */

/**
 * Specify a "dummy" source or destination for communication. Can be
 * used instead of a rank wherever a source or a destination argument
 * is required in a communication function.
 */
#define MPI_PROC_NULL   (-1)

/** Wilcard value for source. */
#define MPI_ANY_SOURCE 	(-2)

/**
 * Specifies the root node for an intercommunicator collective
 * communication.
 */
#define MPI_ROOT        (-3)

/** Wilcard value for tag. */
#define MPI_ANY_TAG     (-1)

/** Indicates address zero for the buffer argument. */
#define MPI_BOTTOM      (void *)0

/** In-place collectives */
#define MPI_IN_PLACE    NULL

/** @name Error return classes */
/* @{ */
/** Successful return code */
#define MPI_SUCCESS          0
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
#define MPI_ERR_KEYVAL       31
/* error codes for MPI-IO, not used internally */
#define MPI_ERR_FILE                  20
#define MPI_ERR_IO                    21
#define MPI_ERR_AMODE                 22
#define MPI_ERR_UNSUPPORTED_OPERATION 23
#define MPI_ERR_UNSUPPORTED_DATAREP   24
#define MPI_ERR_READ_ONLY             25
#define MPI_ERR_ACCESS                26
#define MPI_ERR_DUP_DATAREP           27
#define MPI_ERR_NO_SUCH_FILE          28
#define MPI_ERR_NOT_SAME              29
#define MPI_ERR_BAD_FILE              30

#define MPI_ERR_LASTCODE     1073741823

/** Datatype still in use */
#define MPI_DATATYPE_ACTIVE  1
/* @} */

/** @name Pre-defined constants */
/* @{ */
#define MPI_UNDEFINED      (-32766)
#define MPI_UNDEFINED_RANK MPI_UNDEFINED
/* @} */

/** @name For supported thread levels */
#define MPI_THREAD_SINGLE	0
#define MPI_THREAD_FUNNELED	1
#define MPI_THREAD_SERIALIZED	2
#define MPI_THREAD_MULTIPLE	3
/* @} */

typedef size_t MPI_Aint;

/** @name Status of receive operation */
/* @{ */
/** Size of a status handle for the Fortran interface */
#define MPI_STATUS_SIZE		4

/** Status handle */
struct MPI_Status_s {
  int count;
  int size;
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
};

/** Status handle */
typedef struct MPI_Status_s MPI_Status;

#define MPI_STATUS_IGNORE	(MPI_Status *)0
#define MPI_STATUSES_IGNORE	(MPI_Status *)0
/* @} */


/** @name Communication request for a non blocking communication. */
/* @{ */

/** Request handle */
typedef int MPI_Request;

/** The special value MPI_REQUEST_NULL is used to indicate an invalid
 *  request handle.
 */
#define MPI_REQUEST_NULL   ((MPI_Request)0)

#define _NM_MPI_REQUEST_OFFSET           1


/* @} */


/** Group handle */
typedef int MPI_Group;
/** Invalid group handle */
#define MPI_GROUP_NULL  ((MPI_Group)0)
/** Predefined empty group */
#define MPI_GROUP_EMPTY ((MPI_Group)1)

#define _NM_MPI_GROUP_OFFSET        2

/* group and communicators comparison return value */
/** same members and order */
#define MPI_IDENT     0x01
/** same members, order differs */
#define MPI_SIMILAR   0x02
/** same groups, context differs */
#define MPI_CONGRUENT 0x03
/** members differ */
#define MPI_UNEQUAL   0x04

/** @name Communicators */
/** Communicator handle */
typedef int MPI_Comm;
/** Invalide request handle */
#define MPI_COMM_NULL  ((MPI_Comm)0)
/** Default communicator that includes all processes. */
#define MPI_COMM_WORLD ((MPI_Comm)1)
/** Communicator that includes only the process itself. */
#define MPI_COMM_SELF  ((MPI_Comm)2)
/** offset for dynamically allocated communicators */
#define _NM_MPI_COMM_OFFSET 3

/* @} */

/** @name Basic datatypes */
/* @{ */
/** Datatype handle */
typedef int MPI_Datatype;
#define MPI_DATATYPE_NULL      ((MPI_Datatype)0)
/* C types */
#define MPI_CHAR               ((MPI_Datatype)1)
#define MPI_UNSIGNED_CHAR      ((MPI_Datatype)2)
#define MPI_SIGNED_CHAR        ((MPI_Datatype)3)
#define MPI_BYTE               ((MPI_Datatype)4)
#define MPI_SHORT              ((MPI_Datatype)5)
#define MPI_UNSIGNED_SHORT     ((MPI_Datatype)6)
#define MPI_INT                ((MPI_Datatype)7)
#define MPI_UNSIGNED           ((MPI_Datatype)8)
#define MPI_LONG               ((MPI_Datatype)9)
#define MPI_UNSIGNED_LONG      ((MPI_Datatype)10)
#define MPI_FLOAT              ((MPI_Datatype)11)
#define MPI_DOUBLE             ((MPI_Datatype)12)
#define MPI_LONG_DOUBLE        ((MPI_Datatype)13)
#define MPI_LONG_LONG_INT      ((MPI_Datatype)14)
#define MPI_LONG_LONG          MPI_LONG_LONG_INT
#define MPI_UNSIGNED_LONG_LONG ((MPI_Datatype)15)
/* FORTRAN types */
#define MPI_CHARACTER         ((MPI_Datatype)16)
#define MPI_LOGICAL           ((MPI_Datatype)17)
#define MPI_REAL	      ((MPI_Datatype)18)
#define MPI_REAL4	      ((MPI_Datatype)19)
#define MPI_REAL8	      ((MPI_Datatype)20)
#define MPI_DOUBLE_PRECISION  ((MPI_Datatype)21)
#define MPI_INTEGER	      ((MPI_Datatype)22)
#define MPI_INTEGER4	      ((MPI_Datatype)23)
#define MPI_INTEGER8	      ((MPI_Datatype)24)
#define MPI_COMPLEX           ((MPI_Datatype)25)
#define MPI_DOUBLE_COMPLEX    ((MPI_Datatype)26)
#define MPI_PACKED            ((MPI_Datatype)27)
/* C struct types */
#define MPI_LONG_INT          ((MPI_Datatype)28)
#define MPI_SHORT_INT         ((MPI_Datatype)29)
#define MPI_FLOAT_INT         ((MPI_Datatype)30)
#define MPI_DOUBLE_INT        ((MPI_Datatype)31)
#define MPI_LONG_DOUBLE_INT   ((MPI_Datatype)32)
/* FORTRAN struct types*/
#define MPI_2INT              ((MPI_Datatype)33)
#define MPI_2INTEGER          ((MPI_Datatype)34)
#define MPI_2REAL             ((MPI_Datatype)35)
#define MPI_2DOUBLE_PRECISION ((MPI_Datatype)36)
/* offset for dynamically allocated datatypes */
#define _NM_MPI_DATATYPE_OFFSET              37

/* @} */

/** @name Collective operations */
/* @{ */
/** Operator handle */
typedef int MPI_Op;
#define MPI_OP_NULL ((MPI_Op)0)
#define MPI_MAX     ((MPI_Op)1)
#define MPI_MIN     ((MPI_Op)2)
#define MPI_SUM     ((MPI_Op)3)
#define MPI_PROD    ((MPI_Op)4)
#define MPI_LAND    ((MPI_Op)5)
#define MPI_BAND    ((MPI_Op)6)
#define MPI_LOR     ((MPI_Op)7)
#define MPI_BOR     ((MPI_Op)8)
#define MPI_LXOR    ((MPI_Op)9)
#define MPI_BXOR    ((MPI_Op)10)
#define MPI_MINLOC  ((MPI_Op)11)
#define MPI_MAXLOC  ((MPI_Op)12)
#define _NM_MPI_OP_OFFSET    13
/* @} */

#define MPI_MAX_PROCESSOR_NAME 256
#define MPI_MAX_ERROR_STRING   512
#define MPI_MAX_NAME_STRING    256

/** @name Error handlers */
/* @{ */
typedef int MPI_Errhandler;
#define MPI_ERRHANDLER_NULL  ((MPI_Errhandler)0)

#define MPI_ERRORS_ARE_FATAL ((MPI_Errhandler)1)
#define MPI_ERRORS_RETURN    ((MPI_Errhandler)2)
/* @} */

/** @name Communicator attributes */
/* @{ */
/* MPI-1 */
typedef int (MPI_Copy_function)(MPI_Comm oldcomm, int keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag);
typedef int (MPI_Delete_function)(MPI_Comm comm, int keyval, void*attribute_val, void*extra_state);
#define MPI_NULL_COPY_FN   ((MPI_Copy_function*)NULL)
#define MPI_NULL_DELETE_FN ((MPI_Delete_function*)NULL)
/* MPI-2 */
typedef int (MPI_Comm_copy_attr_function)(MPI_Comm oldcomm, int comm_keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag);
typedef int (MPI_Comm_delete_attr_function)(MPI_Comm comm, int comm_keyval, void*attribute_val, void*extra_state); 
/** empty copy function */
#define MPI_COMM_NULL_COPY_FN ((MPI_Comm_copy_attr_function*)NULL)
/** empty delete function */
#define MPI_COMM_NULL_DELETE_FN ((MPI_Comm_delete_attr_function*)NULL)
/** simple dup function */
#define MPI_COMM_DUP_FN ((MPI_Comm_copy_attr_function*)-1)

#define MPI_KEYVAL_INVALID  0

#define MPI_TAG_UB          1
#define MPI_HOST            2
#define MPI_IO              3
#define MPI_WTIME_IS_GLOBAL 4
#define _NM_MPI_ATTR_OFFSET 5

/* @} */

/* @}*/

/* Programs that need to convert types used in MPICH should use these */
typedef int MPI_Fint;
#define MPI_Comm_c2f(comm) (MPI_Fint)(comm)
#define MPI_Comm_f2c(comm) (MPI_Comm)(comm)
#define MPI_Type_c2f(datatype) (MPI_Fint)(datatype)
#define MPI_Type_f2c(datatype) (MPI_Datatype)(datatype)
#define MPI_Group_c2f(group) (MPI_Fint)(group)
#define MPI_Group_f2c(group) (MPI_Group)(group)
#define MPI_Info_c2f(info) (MPI_Fint)(info)
#define MPI_Info_f2c(info) (MPI_Info)(info)
#define MPI_Request_c2f(req) (MPI_Fint)(req)
#define MPI_Request_f2c(req) (MPI_Request)(req)
#define MPI_Op_c2f(op) (MPI_Fint)(op)
#define MPI_Op_f2c(op) (MPI_Op)(op)

/* Translates a Fortran status into a C status.
 * @param f_status Fortan status
 * @param c_status C status
 * @return MPI status
 */
int MPI_Status_f2c(MPI_Fint *f_status, MPI_Status *c_status);

/* Translates a C status into a Fortran status.
 * @param f_status Fortan status
 * @param c_status C status
 */
int MPI_Status_c2f(MPI_Status *c_status, MPI_Fint *f_status);

#endif /* MPI_TYPES_H */
