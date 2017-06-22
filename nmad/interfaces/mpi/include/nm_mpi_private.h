/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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


#ifndef NM_MPI_PRIVATE_H
#define NM_MPI_PRIVATE_H

/** \defgroup mpi_private_interface Mad-MPI Private Interface
 *
 * This is the Mad-MPI private interface
 *
 * @{
 */

#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <complex.h>
#include <execinfo.h>
#include <sys/stat.h>

#include <Padico/Puk.h>
#include <nm_public.h>
#include <nm_core_interface.h>
#include <nm_sendrecv_interface.h>
#include <nm_pack_interface.h>
#include <nm_launcher_interface.h>
#include <nm_coll_interface.h>

#ifdef PIOMAN
#include <pioman.h>
#endif /* PIOMAN */

#include "mpi.h"

#define MADMPI_VERSION    3
#define MADMPI_SUBVERSION 0


#ifdef PIOMAN
#define nm_mpi_spinlock_t        piom_spinlock_t
#define nm_mpi_spin_init(LOCK)   piom_spin_init(LOCK)
#define nm_mpi_spin_lock(LOCK)   piom_spin_lock(LOCK)
#define nm_mpi_spin_unlock(LOCK) piom_spin_unlock(LOCK)
#else
#define nm_mpi_spinlock_t       int
#define nm_mpi_spin_init(LOCK)
#define nm_mpi_spin_lock(LOCK)
#define nm_mpi_spin_unlock(LOCK)
#endif /* PIOMAN */

/** error handler */
typedef struct nm_mpi_errhandler_s
{
  int id;
  MPI_Handler_function*function;
} nm_mpi_errhandler_t;

#define NM_MPI_WARNING(...) {						\
    fprintf(stderr, "\n# MadMPI: WARNING- %s\n\t", __func__);		\
    fprintf(stderr, __VA_ARGS__);					\
    fprintf(stderr, "\n\n");						\
  }

#define NM_MPI_FATAL_ERROR(...) {					\
    fprintf(stderr, "\n# MadMPI: FATAL- %s\n\t", __func__);		\
    fprintf(stderr, __VA_ARGS__);					\
    fprintf(stderr, "\n\n");						\
    void*buffer[100];							\
    int nptrs = backtrace(buffer, 100);					\
    backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);			\
    fflush(stderr);							\
    abort();								\
  }

#define FREE_AND_SET_NULL(p) do { free(p); p = NULL; } while (0/*CONSTCOND*/)

/** Maximum value of the tag specified by the end-user */
#define NM_MPI_TAG_MAX                   ((nm_tag_t)(0x7FFFFFFF))
/** Mask for private tags */
#define NM_MPI_TAG_PRIVATE_BASE          ((nm_tag_t)(0xF0000000))
#define NM_MPI_TAG_PRIVATE_BARRIER       ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x01))
#define NM_MPI_TAG_PRIVATE_BCAST         ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x02))
#define NM_MPI_TAG_PRIVATE_GATHER        ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x03))
#define NM_MPI_TAG_PRIVATE_GATHERV       ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x04))
#define NM_MPI_TAG_PRIVATE_SCATTER       ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x05))
#define NM_MPI_TAG_PRIVATE_SCATTERV      ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x06))
#define NM_MPI_TAG_PRIVATE_ALLTOALL      ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x07))
#define NM_MPI_TAG_PRIVATE_ALLTOALLV     ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x08))
#define NM_MPI_TAG_PRIVATE_REDUCE        ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x09))
#define NM_MPI_TAG_PRIVATE_REDUCESCATTER ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x0A))
#define NM_MPI_TAG_PRIVATE_ALLGATHER     ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x0B))
#define NM_MPI_TAG_PRIVATE_TYPE_ADD      ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x0C)) /**< add a datatype */
#define NM_MPI_TAG_PRIVATE_TYPE_ADD_ACK  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x0D)) /**< answer to add request */
#define NM_MPI_TAG_PRIVATE_WIN_INIT      ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x0E))
#define NM_MPI_TAG_PRIVATE_WIN_FENCE     ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x0F))
#define NM_MPI_TAG_PRIVATE_WIN_BARRIER   ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x10))
#define NM_MPI_TAG_PRIVATE_COMMSPLIT     ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0xF1))
#define NM_MPI_TAG_PRIVATE_COMMCREATE    ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0xF2))
/** Masks for RMA-communication tags */
#define NM_MPI_TAG_PRIVATE_RMA_BASE      ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x08000000))
#define NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC ((nm_tag_t)(NM_MPI_TAG_PRIVATE_BASE | 0x04000000))
#define NM_MPI_TAG_PRIVATE_RMA_MASK_OP   ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0x0F)) /**< rma operation mask */
#define NM_MPI_TAG_PRIVATE_RMA_MASK_AOP  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0xF0)) /**< acc operation mask */
#define NM_MPI_TAG_PRIVATE_RMA_MASK_FOP  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_MASK_OP    | \
						     NM_MPI_TAG_PRIVATE_RMA_MASK_AOP))    /**< full operation mask */
#define NM_MPI_TAG_PRIVATE_RMA_MASK_SEQ  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0xFFFF00))
#define NM_MPI_TAG_PRIVATE_RMA_MASK_WIN  ((nm_tag_t)(0xFFFFFFFF00000000 | NM_MPI_TAG_PRIVATE_RMA_BASE))
#define NM_MPI_TAG_PRIVATE_RMA_MASK_USER ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_MASK_FOP   | \
						     NM_MPI_TAG_PRIVATE_RMA_MASK_SEQ))
#define NM_MPI_TAG_PRIVATE_RMA_MASK      ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_MASK_WIN   | \
						     NM_MPI_TAG_PRIVATE_RMA_MASK_USER))
#define NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0xFFFFFFFF000000FF))
#define NM_MPI_TAG_PRIVATE_RMA_START     ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0x01))
#define NM_MPI_TAG_PRIVATE_RMA_END       ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0x02))
#define NM_MPI_TAG_PRIVATE_RMA_LOCK      ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0x13)) /**< lock request */
#define NM_MPI_TAG_PRIVATE_RMA_LOCK_R    ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0x04)) /**< lock ACK */
#define NM_MPI_TAG_PRIVATE_RMA_UNLOCK    ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0x05)) /**< unlock request */
#define NM_MPI_TAG_PRIVATE_RMA_UNLOCK_R  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0x06)) /**< unlock ACK */
/** If dynamic window, tells if target address is valid */
#define NM_MPI_TAG_PRIVATE_RMA_OP_CHECK  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE_SYNC | 0x0F)) 
#define NM_MPI_TAG_PRIVATE_RMA_PUT       ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0x1))
#define NM_MPI_TAG_PRIVATE_RMA_GET_REQ   ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0x2))
#define NM_MPI_TAG_PRIVATE_RMA_ACC       ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0x3)) /**< accumulate */
#define NM_MPI_TAG_PRIVATE_RMA_GACC_REQ  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0x4)) /**< get_accumulate */
#define NM_MPI_TAG_PRIVATE_RMA_FAO_REQ   ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0x5)) /**< fetch and op */
#define NM_MPI_TAG_PRIVATE_RMA_CAS_REQ   ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0x6)) /**< compare and swap */
#define NM_MPI_TAG_PRIVATE_RMA_REQ_RESP  ((nm_tag_t)(NM_MPI_TAG_PRIVATE_RMA_BASE | 0xF))

/** content for MPI_Info */
struct nm_mpi_info_s
{
  int id; /**< object ID */
  puk_hashtable_t content; /**< hashtable of <keys, values> */
};

/** @name keyval and attributes
 * @{ */

/** subroutine types for FORTRAN attributes binding */
typedef void (nm_mpi_copy_subroutine_t)(int*id, int*keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag, int*ierr);
typedef void (nm_mpi_delete_subroutine_t)(int*id, int*keyval, void*attribute_val, void*extra_state, int*ierr);

/** generic C type for comm/win/types attributes */
typedef int (nm_mpi_attr_copy_fn_t)(int id, int comm_keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag);
typedef int (nm_mpi_attr_delete_fn_t)(MPI_Comm comm, int comm_keyval, void*attribute_val, void*extra_state); 

/** a keyval used to index comm/types/win attributes */
struct nm_mpi_keyval_s
{
  int id;
  nm_mpi_attr_copy_fn_t*copy_fn;
  nm_mpi_attr_delete_fn_t*delete_fn;
  nm_mpi_copy_subroutine_t*copy_subroutine;
  nm_mpi_delete_subroutine_t*delete_subroutine;
  void*extra_state;
  int refcount; /**< number of attributes indexed by this keyval */
};

/* @} */

/** @name Communicators */
/* @{ */

/** Internal group */
typedef struct nm_mpi_group_s
{
  int id;                 /**< ID of the group (handle) */
  nm_group_t p_nm_group;  /**< underlying nmad group */
} nm_mpi_group_t;

/** Internal communicator */
typedef struct nm_mpi_communicator_s
{
  int id;                            /**< id of the communicator */
  nm_comm_t p_nm_comm;               /**< underlying nmad intra-communicator (or overlay for inter-communicator) */
  puk_hashtable_t attrs;             /**< communicator attributes, hashed by keyval descriptor */
  nm_mpi_errhandler_t*p_errhandler;  /**< error handler attached to communicator */
  char*name;                         /**< communicator name */
  enum nm_mpi_communicator_kind_e
    {
      NM_MPI_COMMUNICATOR_UNSPEC = 0,
      NM_MPI_COMMUNICATOR_INTRA  = 1,
      NM_MPI_COMMUNICATOR_INTER
    } kind;
  struct nm_mpi_cart_topology_s  /**< cartesian topology */
  {
    int ndims;    /**< number of dimensions */
    int*dims;     /**< number of procs in each dim. */
    int*periods;  /**< whether each dim. is periodic */
    int size;     /**< pre-computed size of cartesian topology */
  } cart_topology;
  struct nm_mpi_intercomm_s
  {
    nm_group_t p_local_group;
    nm_group_t p_remote_group;
    int rank;          /**< rank of self in local group */
    int remote_offset; /**< offset of remote group in global nm_comm */
  } intercomm;
} nm_mpi_communicator_t;
/* @} */

/** @name Window */
/* @{ */
struct nm_mpi_window_s;
typedef struct nm_mpi_window_s nm_mpi_window_t;
struct nm_mpi_win_epoch_s;
typedef struct nm_mpi_win_epoch_s nm_mpi_win_epoch_t;
/* @} */

/** @name Requests */
/* @{ */
/** Type of a communication request */
typedef int8_t nm_mpi_request_type_t;
#define NM_MPI_REQUEST_ZERO       ((nm_mpi_request_type_t)0x00)
#define NM_MPI_REQUEST_SEND       ((nm_mpi_request_type_t)0x01)
#define NM_MPI_REQUEST_RECV       ((nm_mpi_request_type_t)0x02)

typedef int8_t nm_mpi_status_t;
#define NM_MPI_STATUS_NONE        ((nm_mpi_status_t)0x00)
#define NM_MPI_REQUEST_CANCELLED  ((nm_mpi_status_t)0x01) /**< request has been cancelled */
#define NM_MPI_REQUEST_PERSISTENT ((nm_mpi_status_t)0x02) /**< request is persistent */

/** @name Extended modes */
/* @{ */
typedef int8_t nm_mpi_communication_mode_t;
#define NM_MPI_MODE_IMMEDIATE      ((nm_mpi_communication_mode_t)-1)
#define NM_MPI_MODE_READY          ((nm_mpi_communication_mode_t)-2)
#define NM_MPI_MODE_SYNCHRONOUS    ((nm_mpi_communication_mode_t)-3)
/* @} */

#define _NM_MPI_MAX_DATATYPE_SIZE 64

/** Internal communication request */
typedef struct nm_mpi_request_s
{
  /** identifier of the request */
  MPI_Request id;
  /** nmad request for sendrecv interface */
  nm_sr_request_t request_nmad;
  /** type of the request */
  nm_mpi_request_type_t request_type;
  /** communication mode to be used when exchanging data */
  nm_mpi_communication_mode_t communication_mode;
  /** status of request */
  nm_mpi_status_t status;
  /** tag given by the user*/
  int user_tag;
  /** rank of the source node (used for incoming request) */
  int request_source;
  /** error status of the request */
  int request_error;
  /** type of the exchanged data */
  struct nm_mpi_datatype_s*p_datatype;
  /** gate of the destination or the source node */
  nm_gate_t gate;
  /** communicator used for communication */
  nm_mpi_communicator_t*p_comm;
  /** number of elements to be exchanged */
  int count;
  /** pointer to the data to be exchanged */
  union
  {
    void*rbuf;       /**< pointer used for receiving */
    const void*sbuf; /**< pointer for sending */
    char static_buf[_NM_MPI_MAX_DATATYPE_SIZE]; /**< static buffer of max predefined datatype size */
  };
  /** Link for nm_mpi_reqlist_t lists */
  PUK_LIST_LINK(nm_mpi_request);
  /** corresponding epoch management structure for rma operations */
  nm_mpi_win_epoch_t*p_epoch;
  /** corresponding rma window */
  nm_mpi_window_t*p_win;
} __attribute__((__may_alias__)) nm_mpi_request_t;

PUK_LIST_DECLARE_TYPE(nm_mpi_request);
PUK_LIST_CREATE_FUNCS(nm_mpi_request);

/* @} */

/** @name Reduce operators */
/* @{ */

/** Internal reduce operators */
typedef struct nm_mpi_operator_s
{
  int id;
  MPI_User_function*function;
  int commute;
} nm_mpi_operator_t;
/* @} */

/** @name Datatypes */
/* @{ */

/** Serialized version of the internal datatype */
/** @note: each array in the types are inlined */
typedef struct nm_mpi_datatype_ser_s
{
  /** combiner of datatype elements */
  nm_mpi_type_combiner_t combiner;
  /** number of blocks in type map */
  MPI_Count count;
  /** size of the datatype once serialized */
  size_t ser_size;
  /** (probably unique) datatype hash */
  uint32_t hash;
  /** combiner specific parameters */
  union nm_mpi_datatype_ser_param_u
  {
    struct nm_mpi_datatype_ser_param_dup_s
    {
      uint32_t old_type;
    } DUP;
    struct nm_mpi_datatype_ser_param_resized_s
    {
      uint32_t old_type;
      MPI_Aint lb;
      MPI_Aint extent;
    } RESIZED;
    struct nm_mpi_datatype_ser_param_contiguous_s
    {
      uint32_t old_type;
    } CONTIGUOUS;
    struct nm_mpi_datatype_ser_param_vector_s
    {
      uint32_t old_type;
      int stride;       /**< stride in multiple of datatype extent */
      int blocklength;
    } VECTOR;
    struct nm_mpi_datatype_ser_param_hvector_s
    {
      uint32_t old_type;
      MPI_Aint hstride; /**< stride in bytes */
      int blocklength;
    } HVECTOR;
    struct nm_mpi_datatype_ser_param_indexed_s
    {
      uint32_t old_type;
      /**
       * The two following arrays inlined in the structure :
       * int*blocklengths;
       * int*displacements; < displacement in multiple of oldtype extent
       */
      void*DATA;
    } INDEXED;
    struct nm_mpi_datatype_ser_param_hindexed_s
    {
      uint32_t old_type;
      /**
       * The two following arrays inlined in the structure :
       * int*blocklengths;
       * MPI_Aint*displacements; < displacement in bytes
       */
      void*DATA;
    } HINDEXED;
    struct nm_mpi_datatype_ser_param_indexed_block_s
    {
      uint32_t old_type;
      int blocklength;
      /**
       * The following array inlined in the structure :
       * int*displacements;
       */
      void*DATA;
    } INDEXED_BLOCK;
    struct nm_mpi_datatype_ser_param_hindexed_block_s
    {
      uint32_t old_type;
      int blocklength;
      /**
       * The following array inlined in the structure :
       * MPI_Aint*displacements; < displacement in bytes
       */
      void*DATA;
    } HINDEXED_BLOCK;
    struct nm_mpi_datatype_ser_param_subarray_s
    {
      uint32_t old_type;
      int ndims;
      int order;
      /**
       * The three following arrays inlined in the structure :
       * int*sizes;
       * int*subsizes;
       * int*starts;
       */
      void*DATA;
    } SUBARRAY;
    struct nm_mpi_datatype_ser_param_struct_s
    {
      /**
       * The three following arrays inlined in the structure :
       * uint32_t*old_types;
       * int*blocklengths;
       * MPI_Aint*displacements;  < displacement in bytes
       */
      void*DATA;
    } STRUCT;
  } p;
} nm_mpi_datatype_ser_t;

/** Internal datatype */
typedef struct nm_mpi_datatype_s
{
  int id;
  /** combiner of datatype elements */
  nm_mpi_type_combiner_t combiner;
  /** number of blocks in type map */
  MPI_Count count;
  /** whether contiguous && lb == 0 && extent == size */
  int is_compact;
  /** whether entirely contiguous */
  int is_contig;
  /** lower bound of type */
  MPI_Aint lb;
  /** extent of type; upper bound is lb + extent */
  MPI_Aint extent;
  MPI_Count true_lb;
  MPI_Count true_extent;
  /** size of type */
  size_t size;
  /** pre-computed data properties */
  struct nm_data_properties_s props;
  /** number of references pointing to this type (active communications, handle) */
  int refcount;
  /** total number of basic elements contained in type*/
  MPI_Count elements;
  /** whether committed or not */
  int committed;
  union
  {
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
    } DUP;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
    } RESIZED;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
    } CONTIGUOUS;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      int blocklength;
      int stride;       /**< stride in multiple of datatype extent */
    } VECTOR;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      int blocklength;
      MPI_Aint hstride; /**< stride in bytes */
    } HVECTOR;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      struct nm_mpi_type_indexed_map_s
      {
	int blocklength;
	int displacement; /**< displacement in multiple of oldtype extent */
      } *p_map;
    } INDEXED;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      struct nm_mpi_type_hindexed_map_s
      {
	int blocklength;
	MPI_Aint displacement; /**< displacement in bytes */
      } *p_map;
    } HINDEXED;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      int blocklength;
      int*array_of_displacements;
    } INDEXED_BLOCK;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      int blocklength;
      MPI_Aint*array_of_displacements;
    } HINDEXED_BLOCK;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      int ndims;
      int order;
      struct nm_mpi_type_subarray_dim_s
      {
	int size;
	int subsize;
	int start;
      } *p_dims;
    } SUBARRAY;
    struct
    {
      struct nm_mpi_type_struct_map_s
      {
	struct nm_mpi_datatype_s*p_old_type;
	int blocklength;
	MPI_Aint displacement; /**< displacement in bytes */
      } *p_map;
    } STRUCT;
  };
  char*name;
  puk_hashtable_t attrs; /**< datatype attributes, hashed by keyval descriptor */
  size_t ser_size;       /**< size of the datatype once serialized */
  nm_mpi_datatype_ser_t*p_serialized; /**< serialized version of the type. */
  uint32_t hash;         /**< (probably unique) datatype hash */
} nm_mpi_datatype_t;

/** content for datatype traversal */
struct nm_data_mpi_datatype_s
{
  void*ptr;                             /**< pointer to base data */
  struct nm_mpi_datatype_s*p_datatype;  /**< datatype describing data layout */
  int count;                            /**< number of elements */
};
__PUK_SYM_INTERNAL const struct nm_data_ops_s nm_mpi_datatype_ops;
NM_DATA_TYPE(mpi_datatype, struct nm_data_mpi_datatype_s, &nm_mpi_datatype_ops);

/** function apply to each datatype or sub-datatype upon traversal */
typedef void (*nm_mpi_datatype_apply_t)(void*ptr, nm_len_t len, void*_ref);

struct nm_data_mpi_datatype_header_s
{
  MPI_Aint offset;        /**< data offset */
  uint32_t datatype_hash; /**< datatype-to-be-send hash */
  int count;              /**< number of elements */
};

/** content for datatype traversal and sending */
struct nm_data_mpi_datatype_wrapper_s
{
  struct nm_data_mpi_datatype_header_s header; /**< header for datatype */
  struct nm_data_mpi_datatype_s data;          /**< datatype describing data layout */
};
__PUK_SYM_INTERNAL const struct nm_data_ops_s nm_mpi_datatype_wrapper_ops;
NM_DATA_TYPE(mpi_datatype_wrapper, struct nm_data_mpi_datatype_wrapper_s, &nm_mpi_datatype_wrapper_ops);

__PUK_SYM_INTERNAL const struct nm_data_ops_s nm_mpi_datatype_serialize_ops;
NM_DATA_TYPE(mpi_datatype_serial, struct nm_mpi_datatype_s*, &nm_mpi_datatype_serialize_ops);

/* @} */

/** @name Window */
/* @{ */

/** @name Requests list with its locks */
/* @{ */

typedef struct nm_mpi_win_locklist_s
{
  nm_mpi_spinlock_t lock;       /**< lock for pending requests access */
  struct nm_mpi_request_list_s pending; /**< pending requests */
} nm_mpi_win_locklist_t;

/* @} */

/** @name Asynchronous management for incoming request */
/* @{ */

struct nm_mpi_win_epoch_s
{
  volatile int mode;  /**< window mode for the given pair */
  uint64_t nmsg;      /**< number of messages exchanges during this epoch */
  uint64_t completed; /**< counter of completed requests */
};

/* @} */

/** @name Management structure for passive target mode */
/* @{ */

typedef struct nm_mpi_win_pass_mngmt_s
{
  uint64_t excl_pending;   /**< whether there is an exclusive lock request pending */
  uint16_t lock_type;      /**< current lock type. 0 = NONE, 1 = EXCLUSIVE, 2 = SHARED */
  uint64_t naccess;        /**< number of procs that are currently accessing this window */
  nm_mpi_win_locklist_t q; /**< lock protected pending lock request list */
} nm_mpi_win_pass_mngmt_t;

/* @} */

/** Content for MPI_Win */
struct nm_mpi_window_s
{
  /** identifier of the window */
  int id;
  /** window's rank into the communicator */
  int myrank;
  /** data base address of the window */
  void*p_base;
  /** window's memory size */
  MPI_Aint*size;
  /** displacement unit */
  int*disp_unit;
  /** Distant windows id */
  int*win_ids;
  /** window communicator */
  nm_mpi_communicator_t*p_comm;
  /** group of processor that interact with this window */
  nm_group_t p_group;
  /** window core monitor for asynchronous call */
  struct nm_sr_monitor_s monitor;
  /** window core monitor for asynchronous lock */
  struct nm_sr_monitor_s monitor_sync;
  /**   WINDOW ATTRIBUTES   */
  /** infos about the window */
  struct nm_mpi_info_s*p_info;
  /** bit-vector window's flavor (the function used to allocate the window */
  int flavor;
  /** bit-vector OR of MPI_MODE_* constants (used in assert) */
  int mode;
  /** window memory model */
  int mem_model;
  /** error handler attached to window */
  nm_mpi_errhandler_t*p_errhandler;
  /** window name */
  char*name;
  /** window attributes, hashed by keyval descriptor */
  puk_hashtable_t attrs;
  /**   SEQUENCE/EPOCH NUMBER MANAGEMENT   */
  /** sequence number for thread safety and message scheduling 
   *  /!\ WARNING : As this number is on a 16 bits uint, if a lot of
   *  operations are issued (more than 65536 per epoch for one
   *  target), there is a risk for mismatching responses.
   */  
  uint16_t next_seq;
  /** epoch management */
  nm_mpi_win_epoch_t*access;
  nm_mpi_win_epoch_t*exposure;
  /** pending closing exposing epoch requests */
  nm_mpi_request_t**end_reqs;
  /** counter to receive the amount of messages to be expected from a given distant window */
  uint64_t*msg_count;
  /** management queue for passive target */
  nm_mpi_win_pass_mngmt_t waiting_queue;
  /** pending passive RMA operation */
  nm_mpi_win_locklist_t*pending_ops;
  /** memory access protecting lock */
  nm_mpi_spinlock_t lock;
  /** shared memory file name */
  char shared_file_name[32];
};

/** @name Target communication modes */
/* @{ */
#define NM_MPI_WIN_UNUSED                 0
#define NM_MPI_WIN_ACTIVE_TARGET          1
#define NM_MPI_WIN_PASSIVE_TARGET         2
#define NM_MPI_WIN_PASSIVE_TARGET_END     4
#define NM_MPI_WIN_PASSIVE_TARGET_PENDING 8
/* @} */

/** @name Lock types for private use */
/* @{ */
#define NM_MPI_LOCK_NONE      ((uint16_t)0)
#define NM_MPI_LOCK_EXCLUSIVE ((uint16_t)MPI_LOCK_EXCLUSIVE)
#define NM_MPI_LOCK_SHARED    ((uint16_t)MPI_LOCK_SHARED)
#define _NM_MPI_LOCK_OFFSET   ((uint16_t)3)
/* @} */

/* @} */

/* ********************************************************* */

/* typed handle manager and object allocator for objects indexed by ID
 */
#define NM_MPI_HANDLE_TYPE(ENAME, TYPE, OFFSET, SLABSIZE)		\
  PUK_VECT_TYPE(nm_mpi_entry_ ## ENAME, TYPE*);				\
  PUK_ALLOCATOR_TYPE(ENAME, TYPE);					\
  struct nm_mpi_handle_##ENAME##_s					\
  {									\
    struct nm_mpi_entry_##ENAME##_vect_s table;				\
    puk_int_vect_t pool;						\
    MPI_Fint next_id;							\
    nm_mpi_spinlock_t lock;						\
    ENAME##_allocator_t allocator;					\
  };									\
  /** initialize the allocator */					\
  static void nm_mpi_handle_##ENAME##_init(struct nm_mpi_handle_##ENAME##_s*p_allocator) \
  {									\
    nm_mpi_entry_##ENAME##_vect_init(&p_allocator->table);		\
    p_allocator->pool = puk_int_vect_new();				\
    p_allocator->next_id = OFFSET;					\
    nm_mpi_spin_init(&p_allocator->lock);				\
    p_allocator->allocator = ENAME ## _allocator_new(SLABSIZE);		\
    int i;								\
    for(i = 0; i < OFFSET; i++)						\
      {									\
	nm_mpi_entry_##ENAME##_vect_push_back(&p_allocator->table, NULL); \
      }									\
  }									\
  static void nm_mpi_handle_##ENAME##_finalize(struct nm_mpi_handle_##ENAME##_s*p_allocator, void(*destructor)(TYPE*)) \
  {									\
    if(destructor != NULL)						\
      {									\
	nm_mpi_entry_##ENAME##_vect_itor_t i;				\
	puk_vect_foreach(i, nm_mpi_entry_##ENAME, &p_allocator->table)	\
	  {								\
	    if(*i != NULL)						\
	      (*destructor)(*i);					\
	  }								\
      }									\
    nm_mpi_entry_##ENAME##_vect_destroy(&p_allocator->table);		\
    puk_int_vect_delete(p_allocator->pool);				\
    ENAME##_allocator_delete(p_allocator->allocator);			\
  }									\
  /** store a builtin object (constant ID) */				\
  static TYPE*nm_mpi_handle_##ENAME##_store(struct nm_mpi_handle_##ENAME##_s*p_allocator, int id) __attribute__((unused)); \
  static TYPE*nm_mpi_handle_##ENAME##_store(struct nm_mpi_handle_##ENAME##_s*p_allocator, int id) \
  {									\
    TYPE*e = ENAME ## _malloc(p_allocator->allocator);			\
    if((id <= 0) || (id > OFFSET))					\
      {									\
	NM_MPI_FATAL_ERROR("madmpi: cannot store invalid %s handle id %d\n", #ENAME, id); \
      }									\
    if(nm_mpi_entry_##ENAME##_vect_at(&p_allocator->table, id) != NULL) \
      {									\
	NM_MPI_FATAL_ERROR("madmpi: %s handle %d busy; cannot store.", #ENAME, id);	\
      }									\
    nm_mpi_entry_##ENAME##_vect_put(&p_allocator->table, e, id);	\
    e->id = id;								\
    return e;								\
  }									\
  /** allocate a dynamic entry */					\
  static TYPE*nm_mpi_handle_##ENAME##_alloc(struct nm_mpi_handle_##ENAME##_s*p_allocator) \
  {									\
    TYPE*e = ENAME ## _malloc(p_allocator->allocator);			\
    int new_id = -1;							\
    nm_mpi_spin_lock(&p_allocator->lock);				\
    if(puk_int_vect_empty(p_allocator->pool))				\
      {									\
	new_id = p_allocator->next_id++;				\
	nm_mpi_entry_##ENAME##_vect_resize(&p_allocator->table, new_id); \
      }									\
    else								\
      {									\
	new_id = puk_int_vect_pop_back(p_allocator->pool);		\
      }									\
    nm_mpi_entry_##ENAME##_vect_put(&p_allocator->table, e, new_id);	\
    nm_mpi_spin_unlock(&p_allocator->lock);				\
    e->id = new_id;							\
    return e;								\
  }									\
  /** release the ID, but do not free block */				\
  static void nm_mpi_handle_##ENAME##_release(struct nm_mpi_handle_##ENAME##_s*p_allocator, TYPE*e) \
  {									\
    const int id = e->id;						\
    assert(id > 0);							\
    nm_mpi_spin_lock(&p_allocator->lock);				\
    nm_mpi_entry_##ENAME##_vect_put(&p_allocator->table, NULL, id);	\
    puk_int_vect_push_back(p_allocator->pool, id);			\
    e->id = -1;								\
    nm_mpi_spin_unlock(&p_allocator->lock);				\
  }									\
  /** free an entry  */							\
  static void nm_mpi_handle_##ENAME##_free(struct nm_mpi_handle_##ENAME##_s*p_allocator, TYPE*e) \
  {									\
    if(e->id > 0)							\
      nm_mpi_handle_##ENAME##_release(p_allocator, e);			\
    ENAME ## _free(p_allocator->allocator, e);				\
  }									\
  /** get a pointer on an entry from its ID */				\
  static TYPE*nm_mpi_handle_##ENAME##_get(struct nm_mpi_handle_##ENAME##_s*p_allocator, int id) \
  {									\
    TYPE*e = NULL;							\
    if((id > 0) && (id < nm_mpi_entry_##ENAME##_vect_size(&p_allocator->table))) \
      {									\
	nm_mpi_spin_lock(&p_allocator->lock);				\
	e = nm_mpi_entry_##ENAME##_vect_at(&p_allocator->table, id);	\
	nm_mpi_spin_unlock(&p_allocator->lock);				\
      }									\
    else								\
      {									\
	return NULL;							\
      }									\
    return e;								\
  }


/* ********************************************************* */

/** init request sub-system */
void nm_mpi_request_init(void);

void nm_mpi_request_exit(void);

/** init communicator sub-system */
void nm_mpi_comm_init(void);

void nm_mpi_comm_exit(void);

void nm_mpi_datatype_init(void);

void nm_mpi_datatype_exit(void);

void nm_mpi_op_init(void);

void nm_mpi_op_exit(void);

/** init MPI I/O sub-system */
void nm_mpi_io_init(void);

void nm_mpi_io_exit(void);

/** init attribute sub-system */
void nm_mpi_attrs_init(void);

void nm_mpi_attrs_exit(void);

/** init window sub-system */
void nm_mpi_win_init(void);

void nm_mpi_win_exit(void);

/** init rma sub-system */
void nm_mpi_rma_init(void);

void nm_mpi_rma_exit(void);

/* Accessor functions */

/** Gets the in/out gate for the given node */
static inline nm_gate_t nm_mpi_communicator_get_gate(nm_mpi_communicator_t*p_comm, int node)
{
  assert(p_comm->kind != NM_MPI_COMMUNICATOR_UNSPEC);
  if(p_comm->kind == NM_MPI_COMMUNICATOR_INTRA)
    return nm_comm_get_gate(p_comm->p_nm_comm, node);
  else if(p_comm->kind == NM_MPI_COMMUNICATOR_INTER)
    return nm_group_get_gate(p_comm->intercomm.p_remote_group, node);
  else
    padico_fatal("# madmpi: wrong kind %d for communicator %d.\n", p_comm->kind, p_comm->id);
  return NULL;
}

/** Gets the node associated to the given gate */
static inline int nm_mpi_communicator_get_dest(nm_mpi_communicator_t*p_comm, nm_gate_t p_gate)
{
  int rank = nm_comm_get_dest(p_comm->p_nm_comm, p_gate);
  if(p_comm->kind == NM_MPI_COMMUNICATOR_INTER)
    {
      rank -= p_comm->intercomm.remote_offset;
      assert(rank < nm_group_size(p_comm->intercomm.p_remote_group));
      assert(rank >= 0);
    }
  return rank;
}

static inline nm_session_t nm_mpi_communicator_get_session(nm_mpi_communicator_t*p_comm)
{
  return nm_comm_get_session(p_comm->p_nm_comm);
}

/**
 * Allocate a new internal representation of a group.
 */
nm_mpi_group_t*nm_mpi_group_alloc(void);

/**
 * Gets the internal representation of the given group.
 */
nm_mpi_group_t*nm_mpi_group_get(MPI_Group group);

/**
 * Allocate a new internal representation of a errhandler.
 */
struct nm_mpi_errhandler_s*nm_mpi_errhandler_alloc(void);

/**
 * Gets the internal representation of the given errhandler.
 */
struct nm_mpi_errhandler_s*nm_mpi_errhandler_get(int errhandler);

/* Info operations */

/**
 * Allocates a new instance of the internal representation of an info.
 */
struct nm_mpi_info_s*nm_mpi_info_alloc(void);

/**
 * Frees the given instance of the internal representation of an info.
 */
void nm_mpi_info_free(struct nm_mpi_info_s*p_info);

/**
 * Gets the internal representation of the given info.
 */
struct nm_mpi_info_s*nm_mpi_info_get(MPI_Info info);

/**
 * Duplicates the internal representation of the given info.
 */
struct nm_mpi_info_s*nm_mpi_info_dup(struct nm_mpi_info_s*p_info);

/**
 * Update or add new entry in p_info_origin based on the entry from
 * p_info_up. This function only update already existing entries. It
 * does NOT create any new entry.
 */
void nm_mpi_info_update(const struct nm_mpi_info_s*p_info_up, struct nm_mpi_info_s*p_info_origin);

/* Requests functions */

nm_mpi_request_t*nm_mpi_request_alloc(void);

void nm_mpi_request_free(nm_mpi_request_t* req);

nm_mpi_request_t*nm_mpi_request_get(MPI_Fint req_id);

int nm_mpi_request_test(nm_mpi_request_t *p_req);

int nm_mpi_request_wait(nm_mpi_request_t *p_req);

void nm_mpi_request_complete(nm_mpi_request_t*p_req);

/* Send/recv/status functions */

/**
 * Initialises a sending request.
 */
int nm_mpi_isend_init(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm);

/**
 * Starts a sending request.
 */
int nm_mpi_isend_start(nm_mpi_request_t *p_req);

/**
 * Sends data.
 */
int nm_mpi_isend(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm);

/**
 * Initialises a receiving request.
 */
int nm_mpi_irecv_init(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm);

/**
 * Starts a receiving request.
 */
int nm_mpi_irecv_start(nm_mpi_request_t *p_req);

/**
 * Receives data.
 */
int nm_mpi_irecv(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm);

/**
 * Collective send over the communicator
 */
nm_mpi_request_t*nm_mpi_coll_isend(const void*buffer, int count, nm_mpi_datatype_t*p_datatype, int dest, int tag, nm_mpi_communicator_t*p_comm);

/**
 * Recieve data sent over the communicator via nm_mpi_coll_isend
 */
nm_mpi_request_t*nm_mpi_coll_irecv(void*buffer, int count, nm_mpi_datatype_t*p_datatype, int source, int tag, nm_mpi_communicator_t*p_comm);

/**
 * Wait for the end of the communication over communicator via nm_mpi_coll_isend
 */
void nm_mpi_coll_wait(nm_mpi_request_t*p_req);

/* Datatype functionalities */

/**
 * Gets the size of the given datatype.
 */
size_t nm_mpi_datatype_size(nm_mpi_datatype_t*p_datatype);

/**
 * Gets the internal representation of the given datatype.
 */
nm_mpi_datatype_t*nm_mpi_datatype_get(MPI_Datatype datatype);

/**
 * Gets the internal representation of the given datatype from its
 * hash.
 */
nm_mpi_datatype_t*nm_mpi_datatype_hashtable_get(uint32_t datatype_hash);

/** increment refcount on given datatype */
static inline void nm_mpi_datatype_ref_inc(nm_mpi_datatype_t*p_datatype)
{
  assert(p_datatype->refcount >= 0);
  __sync_fetch_and_add(&p_datatype->refcount, 1);
}

/** decrements refcount on given datatype, free datatype if refcount reaches 0. */
int nm_mpi_datatype_ref_dec(nm_mpi_datatype_t*p_datatype);

void nm_mpi_datatype_pack(void*dest_ptr, const void*src_ptr, nm_mpi_datatype_t*p_datatype, int count);

void nm_mpi_datatype_unpack(const void*src_ptr, void*dest_ptr, nm_mpi_datatype_t*p_datatype, int count);

void nm_mpi_datatype_copy(const void*src_buf, nm_mpi_datatype_t*p_src_type, int src_count,
			  void*dest_buf, nm_mpi_datatype_t*p_dest_type, int dest_count);

/** get a pointer on the count'th data buffer */
static inline void*nm_mpi_datatype_get_ptr(void*buf, int count, const nm_mpi_datatype_t*p_datatype)
{
  return buf + count * p_datatype->extent;
}

/** build a data descriptor for mpi data */
static inline void nm_mpi_data_build(struct nm_data_s*p_data, void*ptr, struct nm_mpi_datatype_s*p_datatype, int count)
{
  nm_data_mpi_datatype_set(p_data, (struct nm_data_mpi_datatype_s){ .ptr = ptr, .p_datatype = p_datatype, .count = count });
}

/**
 * Deserialize a new datatype from the informations given in
 * p_datatype.
 */
void nm_mpi_datatype_deserialize(nm_mpi_datatype_ser_t*p_datatype, MPI_Datatype*p_newtype);

/**
 * Send a datatype to a distant node identified by its gate. This
 * function blocks until it recieve an acknowledgement from the
 * distant node that the type can now be used.
 */
int nm_mpi_datatype_send(nm_gate_t gate, nm_mpi_datatype_t*p_datatype);

/* Reduce operation functionalities */

/**
 * Gets the function associated to the given operator.
 */
nm_mpi_operator_t*nm_mpi_operator_get(MPI_Op op);

/**
 * Apply the operator to every chunk of data described by p_datatype
 * and pointed by outvec, with the result stored in outvec. The invec
 * parameter is an address of a pointer to a contiguous buffer of
 * predefined types, corresponding to each element pointed by outvec.
 */
void nm_mpi_operator_apply(void*invec, void*outvec, int count, nm_mpi_datatype_t*p_datatype,
			   nm_mpi_operator_t*p_operator);

/* Communicator operations */

/**
 * Gets the internal representation of the given communicator.
 */
nm_mpi_communicator_t*nm_mpi_communicator_get(MPI_Comm comm);

/* Attributes */

struct nm_mpi_keyval_s*nm_mpi_keyval_new(void);

void nm_mpi_keyval_delete(struct nm_mpi_keyval_s*p_keyval);

struct nm_mpi_keyval_s*nm_mpi_keyval_get(int id);

int nm_mpi_attrs_copy(int id, puk_hashtable_t p_old_attrs, puk_hashtable_t*p_new_attrs);

void nm_mpi_attrs_destroy(int id, puk_hashtable_t*p_old_attrs);

int nm_mpi_attr_put(int id, puk_hashtable_t p_attrs, struct nm_mpi_keyval_s*p_keyval, void*attr_value);

void nm_mpi_attr_get(puk_hashtable_t p_attrs, struct nm_mpi_keyval_s*p_keyval, void**p_attr_value, int*flag);

int nm_mpi_attr_delete(int id, puk_hashtable_t p_attrs, struct nm_mpi_keyval_s*p_keyval);

/* Window operations */

/**
 * Gets the internal representation of the given window.
 */
nm_mpi_window_t*nm_mpi_window_get(MPI_Win win);

/**
 * Returns the next sequence number for the given window, and
 * increments it, lock free.
 */
static inline uint16_t nm_mpi_win_get_next_seq(nm_mpi_window_t*p_win)
{
  return __sync_fetch_and_add(&p_win->next_seq, 1);
}

/**
 * Returns whether the epoch requests are all completed
 */
static inline int nm_mpi_win_completed_epoch(nm_mpi_win_epoch_t*p_epoch)
{
  int ret = __sync_bool_compare_and_swap(&p_epoch->nmsg, p_epoch->completed, p_epoch->nmsg);
  return ret;
}

/**
 * Check whether the epoch (exposure or acces, passive or active) is
 * opened.
 */
static inline int nm_mpi_win_is_ready(nm_mpi_win_epoch_t*p_epoch)
{
  int ret = NM_MPI_WIN_UNUSED != p_epoch->mode;
  return ret;
}

/**
 * Check whether or not an assert if only a combination of defined
 * symbols.
 */
static inline int nm_mpi_win_valid_assert(int assert)
{
  int ret = assert >= 0 && assert <= (MPI_MODE_NOCHECK | MPI_MODE_NOSTORE | MPI_MODE_NOPUT
				      | MPI_MODE_NOPRECEDE | MPI_MODE_NOSUCCEED);
  return ret;
}

/**
 * Check whether base is part of a memory segment attached to p_win,
 * and if the data fit, based on its extent.
 */
int nm_mpi_win_addr_is_valid(void*base, MPI_Aint extent, nm_mpi_window_t*p_win);

/**
 * Looks up in the hashtable if the datatype is already exchanged with
 * the target node. If it is, it returns immediately. Otherwhise, it
 * exchanges the datatype information with the target, and set the
 * proper entry in the hashtable.
 */
int nm_mpi_win_send_datatype(nm_mpi_datatype_t*p_datatype, int target_rank, nm_mpi_window_t*p_win);

/* RMA operations */

/**
 * Decode the win_id from the tag.
 */
static inline int nm_mpi_rma_tag_to_win(nm_tag_t tag)
{
  return (tag & NM_MPI_TAG_PRIVATE_RMA_MASK_WIN) >> 32;
}

/**
 * Encode a window id inside the tag to handle the multiple windows.
 */
static inline nm_tag_t nm_mpi_rma_win_to_tag(int win_id)
{
  return ((nm_tag_t)win_id) << 32;
}

/**
 * Decode the sequence number from the tag.
 */
static inline uint16_t nm_mpi_rma_tag_to_seq(nm_tag_t tag)
{
  return (tag & NM_MPI_TAG_PRIVATE_RMA_MASK_SEQ) >> 8;
}

/**
 * Encode a sequence number inside the tag to handle THREAD_MULTIPLE.
 */
static inline nm_tag_t nm_mpi_rma_seq_to_tag(uint16_t seq)
{
  return (nm_tag_t)((((nm_tag_t)seq) << 8) & 0xFFFF00);
}

/**
 * Decode the operation id from the tag.
 */
static inline MPI_Op nm_mpi_rma_tag_to_mpi_op(nm_tag_t tag)
{
  return (tag & 0xF0) >> 4;
}

/**
 * Encode an operation id inside the tag for MPI_Accumulate and
 * MPI_Get_accumulate functions.
 */
static inline nm_tag_t nm_mpi_rma_mpi_op_to_tag(MPI_Op op)
{
  return (nm_tag_t)((((nm_tag_t)op) << 4) & 0xF0);
}
 
/**
 * Create the tag for private rma operations, without the window encoded
 * id.
 */
static inline nm_tag_t nm_mpi_rma_create_usertag(uint16_t seq, int user_tag)
{
  return nm_mpi_rma_seq_to_tag(seq) + (((nm_tag_t)user_tag) & 0xFC0000FF);
}
 
/**
 * Create the tag for private rma operations, with the first 32 bits used for window
 * id.
 */
static inline nm_tag_t nm_mpi_rma_create_tag(int win_id,  uint16_t seq_num, int user_tag)
{
  return nm_mpi_rma_win_to_tag(win_id) + nm_mpi_rma_create_usertag(seq_num, user_tag);
}

/* ********************************************************* */

/** defines public symbols MPI_* and PMPI_* as alias on internal symbols mpi_* */
#define NM_MPI_ALIAS(SYM_MPI, SYM_INTERNAL)				\
  /* enforces symbol type consistency */				\
  typeof(SYM_MPI) SYM_INTERNAL;						\
  /* define public MPI_* symbol */			                \
  typeof(SYM_MPI)      SYM_MPI __attribute__ ((alias (#SYM_INTERNAL)));	\
  /* define public PMPI_* profiling symbol */                           \
  typeof(SYM_MPI) P ## SYM_MPI __attribute__ ((alias (#SYM_INTERNAL)));

/* Internal symbols for MPI functions */

int mpi_issend(const void* buf,
	       int count,
	       MPI_Datatype datatype,
	       int dest,
               int tag,
	       MPI_Comm comm,
	       MPI_Request *request);

int mpi_waitsome(int incount,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *array_of_indices,
		 MPI_Status *array_of_statuses);

int mpi_init(int *argc,
             char ***argv);

int mpi_init_thread(int *argc,
                    char ***argv,
                    int required __attribute__((unused)),
                    int *provided);

int mpi_initialized(int *flag);

int mpi_finalize(void);

int mpi_abort(MPI_Comm comm __attribute__((unused)),
	      int errorcode);

int mpi_comm_size(MPI_Comm comm,
		  int *size);

int mpi_comm_rank(MPI_Comm comm,
                  int *rank);

int mpi_attr_get(MPI_Comm comm,
		 int keyval,
		 void *attr_value,
		 int *flag );
int mpi_get_processor_name(char *name,
			   int *resultlen);

double mpi_wtime(void);

double mpi_wtick(void);

int mpi_error_string(int errorcode,
		     char *string,
		     int *resultlen);

int mpi_errhandler_set(MPI_Comm comm,
		       MPI_Errhandler errhandler);

int mpi_get_version(int *version,
		    int *subversion);

int mpi_bsend(const void *buffer,
	      int count,
	      MPI_Datatype datatype,
	      int dest,
	      int tag,
	      MPI_Comm comm);

int mpi_send(const void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm);

int mpi_isend(const void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

int mpi_rsend(const void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

int mpi_ssend(const void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

int mpi_pack(void* inbuf,
             int incount,
             MPI_Datatype datatype,
             void *outbuf,
             int outsize,
             int *position,
             MPI_Comm comm);

int mpi_pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int*size);

int mpi_recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status);

int mpi_irecv(void *buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

int mpi_sendrecv(const void *sendbuf,
                 int sendcount,
                 MPI_Datatype sendtype,
                 int dest,
                 int sendtag,
                 void *recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int source,
                 int recvtag,
                 MPI_Comm comm,
                 MPI_Status *status);

int mpi_unpack(void* inbuf,
               int insize,
               int *position,
               void *outbuf,
               int outcount,
               MPI_Datatype datatype,
               MPI_Comm comm);

int mpi_wait(MPI_Request *request,
	     MPI_Status *status);

int mpi_waitall(int count,
		MPI_Request *array_of_requests,
		MPI_Status *array_of_statuses);

int mpi_waitany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                MPI_Status *status);

int mpi_test(MPI_Request *request,
             int *flag,
             MPI_Status *status);

int mpi_testany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                int *flag,
                MPI_Status *status);

int mpi_testall(int count,
		MPI_Request *array_of_requests,
		int *flag,
		MPI_Status *statuses);

int mpi_testsome(int count,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *indices,
		 MPI_Status *statuses);

int mpi_iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status);

int mpi_probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status);

int mpi_cancel(MPI_Request *request);

int mpi_request_free(MPI_Request *request);

int mpi_get_count(MPI_Status *status,
                  MPI_Datatype datatype __attribute__((unused)),
                  int *count);

int mpi_request_is_equal(MPI_Request request1,
			 MPI_Request request2);

int mpi_send_init(const void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int dest,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request);

int mpi_recv_init(void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int source,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request);

int mpi_start(MPI_Request *request);

int mpi_startall(int count,
                 MPI_Request *array_of_requests);

int mpi_barrier(MPI_Comm comm);

int mpi_bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm);

int mpi_gather(const void *sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               int root,
               MPI_Comm comm);

int mpi_gatherv(const void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                const int *recvcounts,
                const int *displs,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm);

int mpi_allgather(const void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  MPI_Comm comm);

int mpi_allgatherv(const void *sendbuf,
                   int sendcount,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   const int *recvcounts,
                   const int *displs,
                   MPI_Datatype recvtype,
                   MPI_Comm comm);

int mpi_scatter(const void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm);

int mpi_alltoall(const void* sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 int recvcount,
		 MPI_Datatype recvtype,
		 MPI_Comm comm);

int mpi_alltoallv(const void* sendbuf,
		  const int *sendcounts,
		  const int *sdispls,
		  MPI_Datatype sendtype,
		  void *recvbuf,
		  const int *recvcounts,
		  const int *rdispls,
		  MPI_Datatype recvtype,
		  MPI_Comm comm);

int mpi_op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op);

int mpi_op_free(MPI_Op *op);

int mpi_reduce(const void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm);

int mpi_allreduce(const void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm);

int mpi_reduce_scatter(const void *sendbuf,
                       void *recvbuf,
                       const int *recvcounts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm);

int mpi_get_address(void *location,
		    MPI_Aint *address);

int mpi_address(void *location,
		MPI_Aint *address);

int mpi_type_size(MPI_Datatype datatype,
		  int *size);

int mpi_type_get_extent(MPI_Datatype datatype,
			MPI_Aint *lb,
			MPI_Aint *extent);

int mpi_type_extent(MPI_Datatype datatype,
		    MPI_Aint *extent);

int mpi_type_lb(MPI_Datatype datatype,
		MPI_Aint *lb);

int mpi_type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype);

int mpi_type_commit(MPI_Datatype *datatype);

int mpi_type_free(MPI_Datatype *datatype);

int mpi_type_contiguous(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype);

int mpi_type_vector(int count,
                    int blocklength,
                    int stride,
                    MPI_Datatype oldtype,
                    MPI_Datatype *newtype);

int mpi_type_hvector(int count,
                     int blocklength,
                     MPI_Aint stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype);

int mpi_type_indexed(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype);

int mpi_type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype);

int mpi_type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype);

int mpi_comm_group(MPI_Comm comm, MPI_Group *group);

int mpi_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm*newcomm);


int mpi_comm_split(MPI_Comm comm,
		   int color,
		   int key,
		   MPI_Comm *newcomm);

int mpi_comm_dup(MPI_Comm comm,
		 MPI_Comm *newcomm);

int mpi_comm_free(MPI_Comm *comm);

int mpi_group_size(MPI_Group group, int*size);

int mpi_group_rank(MPI_Group group, int*rank);

int mpi_group_union(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup);

int mpi_group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup);

int mpi_group_difference(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup);

int mpi_group_compare(MPI_Group group1, MPI_Group group2, int*result);

int mpi_group_incl(MPI_Group group, int n, int*ranks, MPI_Group*newgroup);

int mpi_group_excl(MPI_Group group, int n, int*ranks, MPI_Group*newgroup);

int mpi_group_free(MPI_Group*group);

int mpi_group_translate_ranks(MPI_Group group1,
			      int n,
			      int *ranks1,
			      MPI_Group group2,
			      int *ranks2);

int mpi_status_c2f(MPI_Status *c_status,
		   MPI_Fint *f_status);

int mpi_status_f2c(MPI_Fint *f_status,
		   MPI_Status *c_status);

/* @} */

#endif /* NM_MPI_PRIVATE_H */
