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


#ifndef NM_MPI_PRIVATE_H
#define NM_MPI_PRIVATE_H

/** \defgroup mpi_private_interface Mad-MPI Private Interface
 *
 * This is the Mad-MPI private interface
 *
 * @{
 */

#include <stdint.h>
#include <unistd.h>
#include <complex.h>

#include <Padico/Puk.h>
#include <nm_public.h>
#include <nm_private.h>
#include <nm_sendrecv_interface.h>
#include <nm_pack_interface.h>
#include <tbx.h>
#include <nm_launcher.h>
#include <nm_coll.h>

#ifdef PIOMAN
#include <pioman.h>
#endif /* PIOMAN */

#include "mpi.h"

#define MADMPI_VERSION    3
#define MADMPI_SUBVERSION 0


/** @name Debugging facilities */
/* @{ */
#define MPI_NMAD_TRACE(fmt, args...)    NM_TRACEF(fmt , ##args)
#define MPI_NMAD_LOG_IN()               NM_LOG_IN()
#define MPI_NMAD_LOG_OUT()              NM_LOG_OUT()
/* @} */

/** error handler */
typedef struct nm_mpi_errhandler_s
{
  int id;
  MPI_Handler_function*function;
} nm_mpi_errhandler_t;

#define ERROR(...) {							\
    fprintf(stderr, "\n# madmpi: FATAL- %s\n\t", __TBX_FUNCTION__);	\
    fprintf(stderr, __VA_ARGS__);					\
    fprintf(stderr, "\n\n");						\
    void*buffer[100];							\
    int nptrs = backtrace(buffer, 100);					\
    backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);			\
    fflush(stderr);							\
    abort();								\
  }

#define FREE_AND_SET_NULL(p) free(p); p = NULL;

/** Maximum value of the tag specified by the end-user */
#define NM_MPI_TAG_MAX           0x7FFFFFFF
/** Mask for private tags */
#define NM_MPI_TAG_PRIVATE_BASE  0xF0000000
#define NM_MPI_TAG_PRIVATE_BARRIER       (NM_MPI_TAG_PRIVATE_BASE | 0x01)
#define NM_MPI_TAG_PRIVATE_BCAST         (NM_MPI_TAG_PRIVATE_BASE | 0x02)
#define NM_MPI_TAG_PRIVATE_GATHER        (NM_MPI_TAG_PRIVATE_BASE | 0x03)
#define NM_MPI_TAG_PRIVATE_GATHERV       (NM_MPI_TAG_PRIVATE_BASE | 0x04)
#define NM_MPI_TAG_PRIVATE_SCATTER       (NM_MPI_TAG_PRIVATE_BASE | 0x05)
#define NM_MPI_TAG_PRIVATE_SCATTERV      (NM_MPI_TAG_PRIVATE_BASE | 0x06)
#define NM_MPI_TAG_PRIVATE_ALLTOALL      (NM_MPI_TAG_PRIVATE_BASE | 0x07)
#define NM_MPI_TAG_PRIVATE_ALLTOALLV     (NM_MPI_TAG_PRIVATE_BASE | 0x08)
#define NM_MPI_TAG_PRIVATE_REDUCE        (NM_MPI_TAG_PRIVATE_BASE | 0x09)
#define NM_MPI_TAG_PRIVATE_REDUCESCATTER (NM_MPI_TAG_PRIVATE_BASE | 0x0A)
#define NM_MPI_TAG_PRIVATE_ALLGATHER     (NM_MPI_TAG_PRIVATE_BASE | 0x0B)

#define NM_MPI_TAG_PRIVATE_COMMSPLIT     (NM_MPI_TAG_PRIVATE_BASE | 0xF1)

/** content for MPI_Info */
struct nm_mpi_info_s
{
  int id; /**< object ID */
  puk_hashtable_t content; /**< hashtable of <keys, values> */
};

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
    struct nm_comm_s*p_local_comm;
    struct nm_comm_s*p_remote_comm;
    int local_leader, remote_leader;
    nm_tag_t tag;
  } intercomm;
} nm_mpi_communicator_t;
/* @} */

/** @name Requests */
/* @{ */
/** Type of a communication request */
typedef int nm_mpi_request_type_t;
#define NM_MPI_REQUEST_ZERO      ((nm_mpi_request_type_t)0)
#define NM_MPI_REQUEST_SEND      ((nm_mpi_request_type_t)1)
#define NM_MPI_REQUEST_RECV      ((nm_mpi_request_type_t)2)
#define NM_MPI_REQUEST_CANCELLED ((nm_mpi_request_type_t)5)

/** @name Extended modes */
/* @{ */
typedef int nm_mpi_communication_mode_t;
#define NM_MPI_MODE_IMMEDIATE      ((nm_mpi_communication_mode_t)-1)
#define NM_MPI_MODE_READY          ((nm_mpi_communication_mode_t)-2)
#define NM_MPI_MODE_SYNCHRONOUS    ((nm_mpi_communication_mode_t)-3)
/* @} */

/** Internal communication request */
typedef struct nm_mpi_request_s
{
  /** identifier of the request */
  MPI_Request id;
  /** type of the request */
  nm_mpi_request_type_t request_type;
  /** persistent type of the request */
  nm_mpi_request_type_t request_persistent_type;
  /** nmad request for sendrecv interface */
  nm_sr_request_t request_nmad;
  /** tag given by the user*/
  int user_tag;
  /** rank of the source node (used for incoming request) */
  int request_source;
  /** error status of the request */
  int request_error;
  /** type of the exchanged data */
  struct nm_mpi_datatype_s*p_datatype;
  /** communication mode to be used when exchanging data */
  nm_mpi_communication_mode_t communication_mode;
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
  };
} __attribute__((__may_alias__)) nm_mpi_request_t;

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

/** Internal datatype */
typedef struct nm_mpi_datatype_s
{
  int id;
  /** combiner of datatype elements */
  nm_mpi_type_combiner_t combiner;
  /** number of blocks in type map */
  int count;
  /** total number of basic elements contained in type*/
  int elements;
  /** whether entirely contiguous */
  int is_contig;
  /** lower bound of type */
  MPI_Aint lb;
  /** extent of type; upper bound is lb + extent */
  MPI_Aint extent;
  /** size of type */
  size_t size;
  /** number of references pointing to this type (active communications, handle) */
  int refcount;
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
      int stride;       /**< stride in multiple of datatype extent */
      int blocklength;
    } VECTOR;
    struct
    {
      struct nm_mpi_datatype_s*p_old_type;
      MPI_Aint hstride; /**< stride in bytes */
      int blocklength;
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
      struct nm_mpi_type_struct_map_s
      {
	struct nm_mpi_datatype_s*p_old_type;
	int blocklength;
	MPI_Aint displacement; /**< displacement in bytes */
      } *p_map;
    } STRUCT;
  };
  char*name;
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

/* @} */

/* ********************************************************* */

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
  /** initialize the allocator */								\
  static void nm_mpi_handle_##ENAME##_init(struct nm_mpi_handle_##ENAME##_s*p_allocator) \
  {									\
    nm_mpi_entry_##ENAME##_vect_init(&p_allocator->table);		\
    p_allocator->pool = puk_int_vect_new();				\
    p_allocator->next_id = OFFSET;					\
    nm_mpi_spin_init(&p_allocator->lock);					\
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
	ERROR("madmpi: cannot store invalid %s handle id %d\n", #ENAME, id); \
      }									\
    if(nm_mpi_entry_##ENAME##_vect_at(&p_allocator->table, id) != NULL) \
      {									\
	ERROR("madmpi: %s handle %d busy; cannot store.", #ENAME, id);	\
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
    nm_mpi_spin_lock(&p_allocator->lock);					\
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
  /** free an entry  */							\
  static void nm_mpi_handle_##ENAME##_free(struct nm_mpi_handle_##ENAME##_s*p_allocator, TYPE*e) \
  {									\
    const int id = e->id;						\
    nm_mpi_spin_lock(&p_allocator->lock);					\
    puk_int_vect_push_back(p_allocator->pool, id);			\
    nm_mpi_entry_##ENAME##_vect_put(&p_allocator->table, NULL, id);	\
    nm_mpi_spin_unlock(&p_allocator->lock);				\
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
	ERROR("madmpi: cannot get invalid handle id %d\n", id);		\
      }									\
    return e;								\
  }


/* ********************************************************* */

/** Initialises internal data */
int nm_mpi_internal_init(void);

/** Internal shutdown of the application. */
int nm_mpi_internal_exit(void);

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

/** init MPI I/O subsystem */
void nm_mpi_io_init(void);

void nm_mpi_io_exit(void);


/* Accessor functions */

/** Gets the in/out gate for the given node */
static inline nm_gate_t nm_mpi_communicator_get_gate(nm_mpi_communicator_t*p_comm, int node)
{
  assert(p_comm->kind != NM_MPI_COMMUNICATOR_UNSPEC);
  if(p_comm->kind == NM_MPI_COMMUNICATOR_INTRA)
    return nm_comm_get_gate(p_comm->p_nm_comm, node);
  else if(p_comm->kind == NM_MPI_COMMUNICATOR_INTER)
    return nm_comm_get_gate(p_comm->intercomm.p_remote_comm, node);
  else
    padico_fatal("# madmpi: wrong kind %d for communicator %d.\n", p_comm->kind, p_comm->id);
  return NULL;
}

/** Gets the node associated to the given gate */
static inline int nm_mpi_communicator_get_dest(nm_mpi_communicator_t*p_comm, nm_gate_t p_gate)
{
  return nm_comm_get_dest(p_comm->p_nm_comm, p_gate);
}

static inline nm_session_t nm_mpi_communicator_get_session(nm_mpi_communicator_t*p_comm)
{
  return nm_comm_get_session(p_comm->p_nm_comm);
}

struct nm_mpi_errhandler_s*nm_mpi_errhandler_get(int errhandler);

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
 * Unlocks the given datatype, when the datatype is fully unlocked,
 * and a freeing request was posted, it will be released.
 */
int nm_mpi_datatype_unlock(nm_mpi_datatype_t*p_datatype);

void nm_mpi_datatype_pack(void*dest_ptr, const void*src_ptr, nm_mpi_datatype_t*p_datatype, int count);

void nm_mpi_datatype_unpack(const void*src_ptr, void*dest_ptr, nm_mpi_datatype_t*p_datatype, int count);

void nm_mpi_datatype_copy(const void*src_buf, nm_mpi_datatype_t*p_src_type, int src_count,
			  void*dest_buf, nm_mpi_datatype_t*p_dest_type, int dest_count);

/** get a pointer on the count'th data buffer */
static inline void*nm_mpi_datatype_get_ptr(void*buf, int count, const nm_mpi_datatype_t*p_datatype)
{
  return buf + count * p_datatype->extent;
}

/* Reduce operation functionalities */

/**
 * Gets the function associated to the given operator.
 */
nm_mpi_operator_t*nm_mpi_operator_get(MPI_Op op);

/* Communicator operations */

/**
 * Gets the internal representation of the given communicator.
 */
nm_mpi_communicator_t*nm_mpi_communicator_get(MPI_Comm comm);


/**
 * Gets the NM tag for the given user tag and communicator.
 */
void nm_mpi_get_tag(nm_mpi_communicator_t*p_comm, int user_tag, nm_tag_t*nm_tag, nm_tag_t*tag_mask);

/**
 * Checks whether the given tag is in the permitted bounds
 */
int nm_mpi_check_tag(int user_tag);


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
                    int required TBX_UNUSED,
                    int *provided);

int mpi_initialized(int *flag);

int mpi_finalize(void);

int mpi_abort(MPI_Comm comm TBX_UNUSED,
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
                  MPI_Datatype datatype TBX_UNUSED,
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
