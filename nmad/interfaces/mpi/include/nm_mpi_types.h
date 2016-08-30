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


#ifndef NM_MPI_TYPES_H
#define NM_MPI_TYPES_H

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
/* base error codes, used as error class */
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
#define MPI_ERR_INFO         19
#define MPI_ERR_NO_MEM       20
/* extended error codes, not a class */
#define MPI_ERR_REQUEST          32
#define MPI_ERR_KEYVAL           33
#define MPI_ERR_DATATYPE_ACTIVE  34 /**< Datatype still in use */
#define MPI_ERR_INFO_KEY         35
#define MPI_ERR_INFO_VALUE       36
#define MPI_ERR_INFO_NOKEY       37
/* error codes for MPI-IO */
#define MPI_ERR_FILE                  64
#define MPI_ERR_IO                    65
#define MPI_ERR_AMODE                 66
#define MPI_ERR_UNSUPPORTED_OPERATION 67
#define MPI_ERR_UNSUPPORTED_DATAREP   68
#define MPI_ERR_READ_ONLY             69
#define MPI_ERR_ACCESS                70
#define MPI_ERR_DUP_DATAREP           71
#define MPI_ERR_NO_SUCH_FILE          72
#define MPI_ERR_NOT_SAME              73
#define MPI_ERR_BAD_FILE              74
/* error codes for MPI-RMA */
#define MPI_ERR_WIN          92  /**< Invalid win argument */
#define MPI_ERR_BASE         93  /**< Invalid base argument */
#define MPI_ERR_SIZE         94  /**< Invalid size argument */
#define MPI_ERR_DISP         96  /**< Invalid disp argument */
#define MPI_ERR_LOCKTYPE     97  /**< invalid locktype argument */
#define MPI_ERR_ASSERT       98  /**< Invalid assert argument */
#define MPI_ERR_RMA_CONFLICT 99  /**< Conflicting accesses to window */
#define MPI_ERR_RMA_SYNC     100 /**< Invalid synchronization of RMA calls */
#define MPI_ERR_RMA_RANGE    101 /**< Target memory is not part of the window, or not attached */
#define MPI_ERR_RMA_ATTACH   102 /**< Memory cannot be attached */
#define MPI_ERR_RMA_SHARED   103 /**< Memory cannot be shared */
#define MPI_ERR_RMA_FLAVOR   104 /**< Passed window has the wrong flavor for the called function */

#define MPI_ERR_LASTCODE              128

/* @} */

/** @name Pre-defined constants */
/* @{ */
#define MPI_UNDEFINED      (-2147483647)
#define MPI_UNDEFINED_RANK MPI_UNDEFINED
/* @} */

/** @name For supported thread levels */
/* @{ */
#define MPI_THREAD_SINGLE	0
#define MPI_THREAD_FUNNELED	1
#define MPI_THREAD_SERIALIZED	2
#define MPI_THREAD_MULTIPLE	3
/* @} */

/** type that holds an address */
typedef intptr_t MPI_Aint;

/** type for counts */
typedef long long int MPI_Count;

/** An info opaque object */
typedef int MPI_Info;
#define MPI_INFO_NULL       ((MPI_Info)0)
/** offset for dynamically allocated info */
#define _NM_MPI_INFO_OFFSET ((MPI_Info)1)
#define MPI_MAX_INFO_KEY 255
#define MPI_MAX_INFO_VAL 1024

/** @name Status of receive operation */
/* @{ */
/** Size of a status handle for the Fortran interface */
#define MPI_STATUS_SIZE		4

/** Status handle */
struct mpi_status_s
{
  int MPI_SOURCE; /**< field name mandatory by spec */
  int MPI_TAG;    /**< field name mandatory by spec */
  int MPI_ERROR;  /**< field name mandatory by spec */
  int size;       /**< size of data received */
  int cancelled;  /**< whether request was cancelled */
};

/** Status handle */
typedef struct mpi_status_s MPI_Status;

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
/** offset for dynamically allocated requests */
#define _NM_MPI_REQUEST_OFFSET           1


/* @} */


/** Group handle */
typedef int MPI_Group;
/** Invalid group handle */
#define MPI_GROUP_NULL  ((MPI_Group)0)
/** Predefined empty group */
#define MPI_GROUP_EMPTY ((MPI_Group)1)
/** offset for dynamically allocated groups */
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

/** splitting type for communicators */
#define MPI_COMM_TYPE_SHARED 1

/* @} */

/** @name Basic datatypes 
 * @{ */
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
#define MPI_REAL2	      ((MPI_Datatype)19)
#define MPI_REAL4	      ((MPI_Datatype)20)
#define MPI_REAL8	      ((MPI_Datatype)21)
#define MPI_REAL16	      ((MPI_Datatype)22)
#define MPI_DOUBLE_PRECISION  ((MPI_Datatype)23)
#define MPI_INTEGER	      ((MPI_Datatype)24)
#define MPI_INTEGER1	      ((MPI_Datatype)25)
#define MPI_INTEGER2	      ((MPI_Datatype)26)
#define MPI_INTEGER4	      ((MPI_Datatype)27)
#define MPI_INTEGER8	      ((MPI_Datatype)28)
#define MPI_INTEGER16	      ((MPI_Datatype)29)
#define MPI_COMPLEX           ((MPI_Datatype)30)
#define MPI_COMPLEX4          ((MPI_Datatype)31)
#define MPI_COMPLEX8          ((MPI_Datatype)32)
#define MPI_COMPLEX16         ((MPI_Datatype)33)
#define MPI_COMPLEX32         ((MPI_Datatype)34)
#define MPI_DOUBLE_COMPLEX    ((MPI_Datatype)35)
#define MPI_PACKED            ((MPI_Datatype)36)
/* C struct types */
#define MPI_LONG_INT          ((MPI_Datatype)37)
#define MPI_SHORT_INT         ((MPI_Datatype)38)
#define MPI_FLOAT_INT         ((MPI_Datatype)39)
#define MPI_DOUBLE_INT        ((MPI_Datatype)40)
#define MPI_LONG_DOUBLE_INT   ((MPI_Datatype)41)
/* FORTRAN struct types*/
#define MPI_2INT              ((MPI_Datatype)42)
#define MPI_2INTEGER          ((MPI_Datatype)43)
#define MPI_2REAL             ((MPI_Datatype)44)
#define MPI_2DOUBLE_PRECISION ((MPI_Datatype)45)
/* special types for bounds */
#define MPI_UB                ((MPI_Datatype)46)
#define MPI_LB                ((MPI_Datatype)47)
/* MPI-2 C types */
#define MPI_INT8_T            ((MPI_Datatype)48)
#define MPI_INT16_T           ((MPI_Datatype)49)
#define MPI_INT32_T           ((MPI_Datatype)50)
#define MPI_INT64_T           ((MPI_Datatype)51)
#define MPI_UINT8_T           ((MPI_Datatype)52)
#define MPI_UINT16_T          ((MPI_Datatype)53)
#define MPI_UINT32_T          ((MPI_Datatype)54)
#define MPI_UINT64_T          ((MPI_Datatype)55)
#define MPI_AINT              ((MPI_Datatype)56)
#define MPI_OFFSET            ((MPI_Datatype)57)
#define MPI_C_BOOL            ((MPI_Datatype)58)
#define MPI_C_COMPLEX         MPI_C_FLOAT_COMPLEX
#define MPI_C_FLOAT_COMPLEX   ((MPI_Datatype)59)
#define MPI_C_DOUBLE_COMPLEX  ((MPI_Datatype)60)
#define MPI_C_LONG_DOUBLE_COMPLEX ((MPI_Datatype)61)
#define MPI_WCHAR             ((MPI_Datatype)62)
/* MPI-3 MPI_Count */
#define MPI_COUNT             ((MPI_Datatype)63)
/* unsupported C++ types- define to TYPE_NULL */
#define MPI_CXX_BOOL                MPI_DATATYPE_NULL
#define MPI_CXX_FLOAT_COMPLEX       MPI_DATATYPE_NULL
#define MPI_CXX_DOUBLE_COMPLEX      MPI_DATATYPE_NULL
#define MPI_CXX_LONG_DOUBLE_COMPLEX MPI_DATATYPE_NULL
/* offset for dynamically allocated datatypes */
#define _NM_MPI_DATATYPE_OFFSET              64

/** Types of datatypes */
typedef enum 
  {
    MPI_COMBINER_NAMED,       /**< basic type built-in MPI */
    MPI_COMBINER_CONTIGUOUS,  /**< contiguous array */
    MPI_COMBINER_VECTOR,      /**< vector with stride */
    MPI_COMBINER_HVECTOR,     /**< vector with stride in bytes */
    MPI_COMBINER_INDEXED,     /**< indexed type */
    MPI_COMBINER_HINDEXED,    /**< indexed type, offset in bytes */
    MPI_COMBINER_STRUCT,      /**< structured type */
    MPI_COMBINER_RESIZED,     /**< type is resized from another type */
    MPI_COMBINER_DUP,         /**< type is duplicated from another type */
    MPI_COMBINER_INDEXED_BLOCK,
    MPI_COMBINER_HINDEXED_BLOCK,
    MPI_COMBINER_SUBARRAY
  }
  nm_mpi_type_combiner_t;

/* ** order of sub-arrays */
#define MPI_ORDER_C        1  /**< array is row-major */
#define MPI_ORDER_FORTRAN  2  /**< array is column-major */
/* ** distributed arrays (not implemented yet) */
#define MPI_DISTRIBUTE_BLOCK     1
#define MPI_DISTRIBUTE_CYCLIC    2
#define MPI_DISTRIBUTE_NONE      3
#define MPI_DISTRIBUTE_DFLT_DARG 4

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
#define MPI_REPLACE ((MPI_Op)13)
#define MPI_NO_OP   ((MPI_Op)14)
#define _NM_MPI_OP_OFFSET    15
/* @} */

#define MPI_MAX_PROCESSOR_NAME 256
#define MPI_MAX_ERROR_STRING   512
#define MPI_MAX_NAME_STRING    256
#define MPI_MAX_OBJECT_NAME    256

/** @name Attributes for one-sided operations */
/* @{ */
/** Window handle */
typedef int MPI_Win;
#define MPI_WIN_NULL       ((MPI_Win)0)
#define _NM_MPI_WIN_OFFSET ((MPI_Win)1)

/** @name Assert modes */
/* @{ */
#define MPI_MODE_NO_ASSERT 0x00
#define MPI_MODE_NOCHECK   0x01
#define MPI_MODE_NOSTORE   0x02
#define MPI_MODE_NOPUT     0x04
#define MPI_MODE_NOPRECEDE 0x08
#define MPI_MODE_NOSUCCEED 0x10
/* @} */

/** @name Window flavors */
/* @{ */
#define MPI_WIN_FLAVOR_CREATE   0
#define MPI_WIN_FLAVOR_ALLOCATE 1
#define MPI_WIN_FLAVOR_DYNAMIC  2
#define MPI_WIN_FLAVOR_SHARED   3
/* @} */

/** @name Key values for window attributes */
/* @{ */
#define MPI_WIN_BASE          0
#define MPI_WIN_SIZE          1
#define MPI_WIN_DISP_UNIT     2
#define MPI_WIN_CREATE_FLAVOR 3
#define MPI_WIN_MODEL         4
/* @} */

/** @name Window memory models */
/* @{ */
#define MPI_WIN_SEPARATE 0
#define MPI_WIN_UNIFIED  1
/* @} */

/** @name Lock types for passive target windows */
/* @{ */
#define MPI_LOCK_EXCLUSIVE  1
#define MPI_LOCK_SHARED     2
/* @} */

/* @} */

/** @name Error handlers */
/* @{ */
typedef int MPI_Errhandler;
#define MPI_ERRHANDLER_NULL  ((MPI_Errhandler)0)

#define MPI_ERRORS_ARE_FATAL ((MPI_Errhandler)1)
#define MPI_ERRORS_RETURN    ((MPI_Errhandler)2)
#define _NM_MPI_ERRHANDLER_OFFSET             3

/* MPI-1 */
typedef void (MPI_Handler_function)(MPI_Comm *, int *, ...);
/* MPI-2 */
typedef void (MPI_Comm_errhandler_fn)(MPI_Comm *, int *, ...);
/* MPI-3 */
typedef void (MPI_Win_errhandler_fn)(MPI_Win *, int *, ...);
/* @} */

/** @name Communicator attributes */
/* @{ */
/* MPI-1 */
typedef int (MPI_Copy_function)(MPI_Comm oldcomm, int keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag);
typedef int (MPI_Delete_function)(MPI_Comm comm, int keyval, void*attribute_val, void*extra_state);
#define MPI_NULL_COPY_FN   ((MPI_Copy_function*)0)
#define MPI_DUP_FN         ((MPI_Copy_function*)-1)
#define MPI_NULL_DELETE_FN ((MPI_Delete_function*)0)
/* MPI-2 */
typedef int (MPI_Comm_copy_attr_function)(MPI_Comm oldcomm, int comm_keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag);
typedef int (MPI_Comm_delete_attr_function)(MPI_Comm comm, int comm_keyval, void*attribute_val, void*extra_state);
/** empty copy function */
#define MPI_COMM_NULL_COPY_FN   ((MPI_Comm_copy_attr_function*)0)
/** empty delete function */
#define MPI_COMM_NULL_DELETE_FN ((MPI_Comm_delete_attr_function*)0)
/** simple dup function */
#define MPI_COMM_DUP_FN         ((MPI_Comm_copy_attr_function*)-1)
/* MPI-3 */
typedef int (MPI_Win_copy_attr_function)(MPI_Win oldwin, int win_keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag);
typedef int (MPI_Win_delete_attr_function)(MPI_Win win, int win_keyval, void*attribute_val, void*extra_state);
typedef int (MPI_Type_copy_attr_function)(MPI_Datatype oldtype, int type_keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag);
typedef int (MPI_Type_delete_attr_function)(MPI_Datatype datatype, int type_keyval, void*attribute_val, void*extra_state);
/** empty copy function */
#define MPI_WIN_NULL_COPY_FN    ((MPI_Win_copy_attr_function*)0)
#define MPI_TYPE_NULL_COPY_FN   ((MPI_Type_copy_attr_function*)0)
/** empty delete function */
#define MPI_WIN_NULL_DELETE_FN  ((MPI_Win_delete_attr_function*)0)
#define MPI_TYPE_NULL_DELETE_FN ((MPI_Type_delete_attr_function*)0)
/** simple dup function */
#define MPI_WIN_DUP_FN          ((MPI_Win_copy_attr_function*)-1)
#define MPI_TYPE_DUP_FN         ((MPI_Type_copy_attr_function*)-1)

#define MPI_KEYVAL_INVALID  0

#define MPI_TAG_UB          1
#define MPI_HOST            2
#define MPI_IO              3
#define MPI_WTIME_IS_GLOBAL 4
#define MPI_UNIVERSE_SIZE   5
#define MPI_APPNUM          6
#define MPI_LASTUSEDCODE    7
#define _NM_MPI_ATTR_OFFSET 8

/* @} */

/* @}*/

/** integer type interoperable between C and FORTRAN */
typedef int MPI_Fint;

static inline MPI_Comm MPI_Comm_f2c(MPI_Fint comm)
{
  return comm;
}
static inline MPI_Fint MPI_Comm_c2f(MPI_Comm comm)
{
  return comm;
}
static inline MPI_Datatype MPI_Type_f2c(MPI_Fint datatype)
{
  return datatype;
}
static inline MPI_Fint MPI_Type_c2f(MPI_Datatype datatype)
{
  return datatype;
}
static inline MPI_Group MPI_Group_f2c(MPI_Fint group)
{
  return group;
}
static inline MPI_Fint MPI_Group_c2f(MPI_Group group)
{
  return group;
}
static inline MPI_Info MPI_Info_f2c(MPI_Fint info)
{
  return info;
}
static inline MPI_Fint MPI_Info_c2f(MPI_Info info)
{
  return info;
}
static inline MPI_Request MPI_Request_f2c(MPI_Fint request)
{
  return request;
}
static inline MPI_Fint MPI_Request_c2f(MPI_Request request)
{
  return request;
}
static inline MPI_Op MPI_Op_f2c(MPI_Fint op)
{
  return op;
}
static inline MPI_Fint MPI_Op_c2f(MPI_Op op)
{
  return op;
}
static inline MPI_Win MPI_Win_f2c(MPI_Fint win)
{
  return win;
}
static inline MPI_Fint MPI_Win_c2f(MPI_Op win)
{
  return win;
}


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

#endif /* NM_MPI_TYPES_H */
