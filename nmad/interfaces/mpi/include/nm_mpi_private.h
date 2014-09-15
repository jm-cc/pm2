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


#ifndef MPI_NMAD_PRIVATE_H
#define MPI_NMAD_PRIVATE_H

/** \defgroup mpi_private_interface Mad-MPI Private Interface
 *
 * This is the Mad-MPI private interface
 *
 * @{
 */

#include <stdint.h>
#include <unistd.h>

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

#define MADMPI_VERSION    1
#define MADMPI_SUBVERSION 0


/** @name Debugging facilities */
/* @{ */
#define MPI_NMAD_TRACE(fmt, args...)    NM_TRACEF(fmt , ##args)
#define MPI_NMAD_LOG_IN()               NM_LOG_IN()
#define MPI_NMAD_LOG_OUT()              NM_LOG_OUT()
/* @} */

#define ERROR(...) { fprintf(stderr, "\n\n************\nERROR: %s\n\t", __TBX_FUNCTION__); fprintf(stderr, __VA_ARGS__); \
                     fprintf(stderr, "\n************\n\n"); fflush(stderr); \
                     MPI_Abort(MPI_COMM_WORLD, 1); \
                   }

#define FREE_AND_SET_NULL(p) free(p); p = NULL;

/** Maximum value of the tag specified by the end-user */
#define MPI_NMAD_MAX_VALUE_TAG  0x7FFFFFFF

/** @name Communicators */
/* @{ */

/** Internal communicator */
typedef struct nm_mpi_communicator_s
{
  /** id of the communicator */
  unsigned int communicator_id;
  /** underlying nmad communicator */
  nm_comm_t p_comm;
  /** cartesian topology */
  struct nm_mpi_cart_topology_s
  {
    int ndims;    /**< number of dimensions */
    int*dims;     /**< number of procs in each dim. */
    int*periods;  /**< whether each dim. is periodic */
    int size;     /**< pre-computed size of cartesian topology */
  } cart_topology;
} nm_mpi_communicator_t;
/* @} */

/** @name Requests */
/* @{ */
/** Type of a communication request */
typedef int MPI_Request_type;
#define MPI_REQUEST_ZERO      ((MPI_Request_type)0)
#define MPI_REQUEST_SEND      ((MPI_Request_type)1)
#define MPI_REQUEST_RECV      ((MPI_Request_type)2)
#define MPI_REQUEST_PACK_SEND ((MPI_Request_type)3)
#define MPI_REQUEST_PACK_RECV ((MPI_Request_type)4)
#define MPI_REQUEST_CANCELLED ((MPI_Request_type)5)

/** Internal communication request */
typedef struct nm_mpi_request_s
{
  /* identifier of the request */
  MPI_Request request_id;
  /** type of the request */
  MPI_Request_type request_type;
  /** persistent type of the request */
  MPI_Request_type request_persistent_type;

  union
  {
    /** handle to the NM request when data are exchanged with the \ref sr_interface */
    nm_sr_request_t request_nmad;
    /** handle to the NM connexion when data are exchanged with the \ref pack_interface */
    nm_pack_cnx_t request_cnx;
  };
  /** buffer used when non-contiguous data are exchanged with the \ref sr_interface */
  void *contig_buffer;
  /** array of pointers used when non-contiguous data are exchanged with the \ref pack_interface */
  void **request_ptr;

  /** tag given by the user*/
  int user_tag;
  /** tag used by NM, its value is based on user_tag and the request communicator id */
  nm_tag_t request_tag;
  /** rank of the source node (used for incoming request) */
  int request_source;
  /** error status of the request */
  int request_error;
  /** type of the exchanged data */
  MPI_Datatype request_datatype;
  /** communication mode to be used when exchanging data */
  MPI_Communication_Mode communication_mode;
  /** gate of the destination or the source node */
  nm_gate_t gate;
  /** communicator used for communication */
  nm_mpi_communicator_t*p_comm;
  /** number of elements to be exchanged */
  int count;
  /** pointer to the data to be exchanged */
  void *buffer;
} __attribute__((__may_alias__)) nm_mpi_request_t;

PUK_VECT_TYPE(nm_mpi_request, nm_mpi_request_t*);

/* @} */

/** @name Reduce operators */
/* @{ */
/** Maximum number of reduce operators */
#define NUMBER_OF_OPERATORS (_MPI_OP_LAST+1)

/** Internal reduce operators */
typedef struct mpir_operator_s {
  MPI_User_function *function;
  int commute;
} mpir_operator_t;
/* @} */

/** @name Datatypes */
/* @{ */
/** Maximum number of datatypes */
#define NUMBER_OF_DATATYPES 2048

/** Types of datatypes */
typedef enum {
    MPIR_BASIC,
    MPIR_CONTIG, MPIR_VECTOR, MPIR_INDEXED, MPIR_STRUCT
} mpir_nodetype_t;

/** Internal datatype */
typedef struct nm_mpi_datatype_s
{
  /** type of datatype element this is */
  mpir_nodetype_t dte_type;
  /** whether basic or user-defined */
  int basic;
  /** whether committed or not */
  int committed;
  /** whether entirely contiguous */
  int is_contig;
  /** whether optimized or not */
  int is_optimized;
  /** extent of type */
  size_t extent;
  /** lower bound of type */
  MPI_Aint lb;
  /** size of type */
  size_t size;
  /** number of active communications using this type */
  int active_communications;
  /** a free request has been posted for this type while it was still active */
  int free_requested;

  /** number of basic elements */
  int elements;
  /** stride, for VECTOR type */
  int stride;
  /** array of indices, for INDEXED, STRUCT */
  MPI_Aint *indices;
  /** block_size, for VECTOR type */
  size_t block_size;
  /** array of blocklenghts */
  int *blocklens;
  /** old types */
  MPI_Datatype *old_types;
  /** size of old types */
  size_t* old_sizes;
} nm_mpi_datatype_t;
/* @} */

/** @name Internal data */
/* @{ */
/** Internal data */
struct nm_mpi_internal_data_s
{
  /** all the defined datatypes */
  nm_mpi_datatype_t *datatypes[NUMBER_OF_DATATYPES];
  /** pool of ids of datatypes that can be created by end-users */
  puk_int_vect_t datatypes_pool;

  /** all the defined reduce operations */
  mpir_operator_t *operators[NUMBER_OF_OPERATORS];
  /** pool of ids of reduce operations that can be created by end-users */
  puk_int_vect_t operators_pool;

  /** total number of incoming messages */
  int 		     nb_incoming_msg;
  /** total number of outgoing messages */
  int 		     nb_outgoing_msg;
};
extern struct nm_mpi_internal_data_s nm_mpi_internal_data;
/* @} */

/**
 * Initialises internal data
 */
int mpir_internal_init(void);

/**
 * Internal shutdown of the application.
 */
int mpir_internal_exit(void);

/** init request sub-system */
void nm_mpi_request_init(void);

/** init communicator sub-system */
void nm_mpi_comm_init(void);

void nm_mpi_comm_exit(void);

/* Accessor functions */

/**
 * Gets the in/out gate for the given node
 */
nm_gate_t nm_mpi_communicator_get_gate(nm_mpi_communicator_t*p_comm, int node);

/**
 * Gets the node associated to the given gate
 */
int nm_mpi_communicator_get_dest(nm_mpi_communicator_t*p_comm, nm_gate_t gate);

/* Requests functions */

nm_mpi_request_t*nm_mpi_request_alloc(void);

void nm_mpi_request_free(nm_mpi_request_t* req);

nm_mpi_request_t*nm_mpi_request_get(MPI_Fint req_id);


/* Send/recv/status functions */

/**
 * Initialises a sending request.
 */
int mpir_isend_init(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm);

/**
 * Starts a sending request.
 */
int mpir_isend_start(nm_mpi_request_t *p_req);

/**
 * Sends data.
 */
int mpir_isend(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm);

/**
 * Initialises a receiving request.
 */
int mpir_irecv_init(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm);

/**
 * Starts a receiving request.
 */
int mpir_irecv_start(nm_mpi_request_t *p_req);

/**
 * Receives data.
 */
int mpir_irecv(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm);

/**
 * Starts a sending or receiving request.
 */
int mpir_start(nm_mpi_request_t *p_req);

/**
 * Waits for a request.
 */
int mpir_wait(nm_mpi_request_t *p_req);

/**
 * Tests the completion of a request.
 */
int mpir_test(nm_mpi_request_t *p_req);

/**
 * Cancels a request.
 */
int mpir_cancel(nm_mpi_request_t *p_req);

/* Datatype functionalities */

/**
 * Gets the size of the given datatype.
 */
size_t mpir_sizeof_datatype(MPI_Datatype datatype);

/**
 * Gets the internal representation of the given datatype.
 */
nm_mpi_datatype_t*nm_mpi_datatype_get(MPI_Datatype datatype);

/**
 * Unlocks the given datatype, when the datatype is fully unlocked,
 * and a freeing request was posted, it will be released.
 */
int nm_mpi_datatype_unlock(MPI_Datatype datatype);


/* Reduce operation functionalities */

/**
 * Commits a new operator for reduce operations.
 */
int mpir_op_create(MPI_User_function *function, int commute, MPI_Op *op);

/**
 * Releases the given operator for reduce operations.
 */
int mpir_op_free(MPI_Op *op);

/**
 * Gets the function associated to the given operator.
 */
mpir_operator_t *mpir_get_operator(MPI_Op op);

/**
 * Defines the MAX function for reduce operations.
 */
void mpir_op_max(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type);

/**
 * Defines the MIN function for reduce operations.
 */
void mpir_op_min(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type);

/**
 * Defines the SUM function for reduce operations.
 */
void mpir_op_sum(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type);

/**
 * Defines the PROD function for reduce operations.
 */
void mpir_op_prod(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type);

/**
 * Defines the LAND function for reduce operations.
 */
void mpir_op_land(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type);

/**
 * Defines the BAND function for reduce operations.
 */
void mpir_op_band(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type);

/**
 * Defines the LOR function for reduce operations.
 */
void mpir_op_lor(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type);

/**
 * Defines the BOR function for reduce operations.
 */
void mpir_op_bor(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type);

/**
 * Defines the LXOR function for reduce operations.
 */
void mpir_op_lxor(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type);

/**
 * Defines the BXOR function for reduce operations.
 */
void mpir_op_bxor(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type);

/**
 * Defines the MINLOC function for reduce operations.
 */
void mpir_op_minloc(void *invec,
		    void *inoutvec,
		    int *len,
		    MPI_Datatype *type);

/**
 * Defines the MAXLOC function for reduce operations.
 */
void mpir_op_maxloc(void *invec,
		    void *inoutvec,
		    int *len,
		    MPI_Datatype *type);

/* Communicator operations */

/**
 * Gets the internal representation of the given communicator.
 */
nm_mpi_communicator_t*nm_mpi_communicator_get(MPI_Comm comm);


/**
 * Gets the NM tag for the given user tag and communicator.
 */
nm_tag_t nm_mpi_get_tag(nm_mpi_communicator_t*p_comm, int tag);

/* Termination functionalities */

/**
 * Using Friedmann  Mattern's Four Counter Method to detect
 * termination detection.
 * "Algorithms for Distributed Termination Detection." Distributed
 * Computing, vol 2, pp 161-175, 1987.
 */
tbx_bool_t mpir_test_termination(MPI_Comm comm);

/* Internal symbols for MPI functions */
int mpi_issend(void* buf,
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

int mpi_bsend(void *buffer,
	      int count,
	      MPI_Datatype datatype,
	      int dest,
	      int tag,
	      MPI_Comm comm);

int mpi_send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm);

int mpi_isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

int mpi_rsend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

int mpi_ssend(void* buffer,
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

int mpi_sendrecv(void *sendbuf,
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

int mpi_send_init(void* buf,
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

int mpi_gather(void *sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               int root,
               MPI_Comm comm);

int mpi_gatherv(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *displs,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm);

int mpi_allgather(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  MPI_Comm comm);

int mpi_allgatherv(void *sendbuf,
                   int sendcount,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int *recvcounts,
                   int *displs,
                   MPI_Datatype recvtype,
                   MPI_Comm comm);

int mpi_scatter(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm);

int mpi_alltoall(void* sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 int recvcount,
		 MPI_Datatype recvtype,
		 MPI_Comm comm);

int mpi_alltoallv(void* sendbuf,
		  int *sendcounts,
		  int *sdispls,
		  MPI_Datatype sendtype,
		  void *recvbuf,
		  int *recvcounts,
		  int *rdispls,
		  MPI_Datatype recvtype,
		  MPI_Comm comm);

int mpi_op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op);

int mpi_op_free(MPI_Op *op);

int mpi_reduce(void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm);

int mpi_allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm);

int mpi_reduce_scatter(void *sendbuf,
                       void *recvbuf,
                       int *recvcounts,
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

int mpi_type_optimized(MPI_Datatype *datatype,
                       int optimized);

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
                     int stride,
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

int mpi_comm_group(MPI_Comm comm,
		   MPI_Group *group);


int mpi_comm_split(MPI_Comm comm,
		   int color,
		   int key,
		   MPI_Comm *newcomm);

int mpi_comm_dup(MPI_Comm comm,
		 MPI_Comm *newcomm);

int mpi_comm_free(MPI_Comm *comm);

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

#endif /* MPI_NMAD_PRIVATE_H */
