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
 * mpi_nmad_private.h
 * ==================
 */

#ifndef MPI_NMAD_PRIVATE_H
#define MPI_NMAD_PRIVATE_H

#include <stdint.h>
#include <unistd.h>

#include <madeleine.h>
#include <nm_public.h>
#include <nm_so_public.h>
#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>
#include <tbx.h>
#include <nm_mad3_private.h>

#define MADMPI_VERSION    1
#define MADMPI_SUBVERSION 0

#define CHECK_RETURN_CODE(err, message) { if (err != NM_ESUCCESS) { printf("%s return err = %d\n", message, err); return 1; }}

#undef MPI_TIMER

extern debug_type_t debug_mpi_nmad_trace;
extern debug_type_t debug_mpi_nmad_transfer;
extern debug_type_t debug_mpi_nmad_log;

#ifdef NMAD_DEBUG
#  define MPI_NMAD_TRACE(fmt, args...) debug_printf(&debug_mpi_nmad_trace, "[%s] " fmt ,__TBX_FUNCTION__ ,##args)
#  define MPI_NMAD_TRACE_NOF(fmt, args...) debug_printf(&debug_mpi_nmad_trace, fmt, ##args)
#  define MPI_NMAD_TRACE_LEVEL(level, fmt, args...) debug_printfl(&debug_mpi_nmad_trace, level, "[%s] " fmt ,__TBX_FUNCTION__  , ##args)
#  define MPI_NMAD_TRACE_NOF_LEVEL(level, fmt, args...) debug_printfl(&debug_mpi_nmad_trace, level, fmt, ##args)
#  define MPI_NMAD_TRANSFER(fmt, args...) debug_printf(&debug_mpi_nmad_transfer, "[%s] " fmt ,__TBX_FUNCTION__ ,##args)
#  define MPI_NMAD_TRANSFER_LEVEL(level, fmt, args...) debug_printfl(&debug_mpi_nmad_transfer, level, "[%s] " fmt ,__TBX_FUNCTION__  , ##args)
#  define MPI_NMAD_LOG_IN()  debug_printf(&debug_mpi_nmad_log, "%s: -->\n", __TBX_FUNCTION__)
#  define MPI_NMAD_LOG_OUT() debug_printf(&debug_mpi_nmad_log, "%s: <--\n", __TBX_FUNCTION__)
#else
#  define MPI_NMAD_TRACE(fmt, args...) {}
#  define MPI_NMAD_TRACE_NOF(fmt, args...) {}
#  define MPI_NMAD_TRACE_LEVEL(level, fmt, args...) {}
#  define MPI_NMAD_TRACE_NOF_LEVEL(level, fmt, args...) {}
#  define MPI_NMAD_TRANSFER(fmt, args...)
#  define MPI_NMAD_TRANSFER_LEVEL(level, fmt, args...)
#  define MPI_NMAD_LOG_IN()
#  define MPI_NMAD_LOG_OUT()
#endif /* NMAD_DEBUG */

#if defined(MPI_TIMER)
#  define MPI_NMAD_TIMER_IN() double timer_start=MPI_Wtime();
#  define MPI_NMAD_TIMER_OUT() { double timer_stop=MPI_Wtime(); fprintf(stderr, "TIMER %s: %f\n", __TBX_FUNCTION__, timer_stop-timer_start) ; }
#else
#  define MPI_NMAD_TIMER_IN() { }
#  define MPI_NMAD_TIMER_OUT() { }
#endif /* MPI_TIMER */

#define ERROR(...) { fprintf(stderr, "\n\n************\nERROR: %s\n\t", __TBX_FUNCTION__); fprintf(stderr, __VA_ARGS__); \
                     fprintf(stderr, "\n************\n\n"); fflush(stderr); \
                     MPI_Abort(MPI_COMM_WORLD, 1); \
                   }

#define FREE_AND_SET_NULL(p) free(p); p = NULL;

/*
 * Maximum value of the tag used internally in MAD-MPI
 */
#define MAX_INTERNAL_TAG 10

#define NUMBER_OF_COMMUNICATORS (MPI_COMM_SELF + 20)

typedef struct mpir_communicator_s {
  int communicator_id;
  int size;
  int rank;
  int *global_ranks;
} mpir_communicator_t;

typedef int MPI_Request_type;
#define MPI_REQUEST_ZERO ((MPI_Request_type)0)
#define MPI_REQUEST_SEND ((MPI_Request_type)1)
#define MPI_REQUEST_RECV ((MPI_Request_type)2)
#define MPI_REQUEST_PACK_SEND ((MPI_Request_type)3)
#define MPI_REQUEST_PACK_RECV ((MPI_Request_type)4)
#define MPI_REQUEST_CANCELLED ((MPI_Request_type)5)

typedef struct mpir_request_s {
  MPI_Request_type request_type;
  MPI_Request_type request_persistent_type;
  nm_so_request request_nmad;
  struct nm_so_cnx request_cnx;
  uint8_t request_tag;
  void **request_ptr;
  void *contig_buffer;
  int user_tag;
  int request_source;
  int request_error;
  MPI_Datatype request_datatype;
  MPI_Communication_Mode communication_mode;
  long gate_id;
  int count;
  void *buffer;
} mpir_request_t;

#define NUMBER_OF_FUNCTIONS MPI_MAXLOC

typedef struct mpir_function_s {
  MPI_User_function *function;
  int commute;
} mpir_function_t;

#define NUMBER_OF_DATATYPES (MPI_INTEGER + 2020)

typedef enum {
    MPIR_BASIC,
    MPIR_CONTIG, MPIR_VECTOR, MPIR_HVECTOR,
    MPIR_INDEXED, MPIR_HINDEXED, MPIR_STRUCT
} mpir_nodetype_t;

typedef struct mpir_datatype_s {
  mpir_nodetype_t    dte_type; /* type of datatype element this is */
  int basic;
  int committed; /* whether committed or not */
  int is_contig; /* whether entirely contiguous */
  size_t extent;
  MPI_Aint lb; /* lower bound of type */
  size_t size; /* size of type */
  int elements; /* number of basic elements */
  int stride; /* stride, for VECTOR and HVECTOR types */
  MPI_Aint *indices; /* array of indices, for (H)INDEXED, STRUCT */
  int blocklen; /* blocklen, for VECTOR and HVECTOR types */
  int block_size; /* blocklen, for VECTOR and HVECTOR types */
  int *blocklens; /* array of blocklens for (H)INDEXED, STRUCT */
  size_t old_size; /* size of old type */
  size_t* old_sizes; /* size of old types */
  int is_optimized;
  int active_communications; /* number of active communications using this type */
  int free_requested; /* a free request has been posted for this type while it was still active */
} mpir_datatype_t;

/**
 * Initialises internal data
 */
int mpir_internal_init(int global_size, int process_rank, p_mad_madeleine_t madeleine,
                       struct nm_so_interface *_p_so_sr_if, nm_so_pack_interface _p_so_pack_if);

/**
 * Internal shutdown of the application. 
 */
int mpir_internal_exit();

/* Accessor functions */

/**
 * Gets the incoming gate id for the given node
 */
long mpir_get_in_gate_id(int node);

/**
 * Gets the outgoing gate id for the given node
 */
long mpir_get_out_gate_id(int node);

/**
 * Gets the node associated to the given incoming gate id
 */
int mpir_get_in_dest(long gate);

/**
 * Gets the node associated to the given outgoing gate id
 */
int mpir_get_out_dest(long gate);

/* Send/recv/status functions */

/**
 * Initialises a sending request.
 */
int mpir_isend_init(mpir_request_t *mpir_request,
                    int dest,
                    mpir_communicator_t *mpir_communicator);

/**
 * Starts a sending request.
 */
int mpir_isend_start(mpir_request_t *mpir_request);

/**
 * Sends data.
 */
int mpir_isend(mpir_request_t *mpir_request,
               int dest,
               mpir_communicator_t *mpir_communicator);

/**
 * Sets the status based on a given request.
 */
int mpir_set_status(MPI_Request *request,
		    MPI_Status *status);

/**
 * Initialises a receiving request.
 */
int mpir_irecv_init(mpir_request_t *mpir_request,
                    int source,
                    mpir_communicator_t *mpir_communicator);

/**
 * Initialises a receiving request.
 */
int mpir_irecv_start(mpir_request_t *mpir_request);

/**
 * Receives data.
 */
int mpir_irecv(mpir_request_t *mpir_request,
               int source,
               mpir_communicator_t *mpir_communicator);

/**
 * Calls the appropriate splitting function based on the given request.
 */
int mpir_datatype_split(mpir_request_t *mpir_request);

/**
 * Starts a sending or receiving request.
 */
int mpir_start(mpir_request_t *mpir_request);

/* Datatype functionalities */

/**
 * Gets the size of the given datatype.
 */
size_t mpir_sizeof_datatype(MPI_Datatype datatype);

/**
 * Gets the internal representation of the given datatype.
 */
mpir_datatype_t* mpir_get_datatype(MPI_Datatype datatype);

/**
 * Gets the size of the given datatype.
 */
int mpir_type_size(MPI_Datatype datatype, int *size);

/**
 * Gets the extent and lower bound of the given datatype.
 */
int mpir_type_get_lb_and_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);

/**
 * Creates a new datatype based on the given datatype and the new
 * lower bound and extent.
 */
int mpir_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype);

/**
 * Commits the given datatype.
 */
int mpir_type_commit(MPI_Datatype datatype);

/**
 * Unlocks the given datatype, when the datatype is fully unlocked,
 * and a freeing request was posted, it will be released.
 */
int mpir_type_unlock(MPI_Datatype datatype);

/**
 * Releases the given datatype.
 */
int mpir_type_free(MPI_Datatype datatype);

/**
 * Sets the optimised functionality for the given datatype.
 */
int mpir_type_optimized(MPI_Datatype datatype, int optimized);

/**
 * Creates a contiguous datatype.
 */
int mpir_type_contiguous(int count,
                         MPI_Datatype oldtype,
                         MPI_Datatype *newtype);

/**
 * Creates a new vector datatype.
 */
int mpir_type_vector(int count,
                     int blocklength,
                     int stride,
                     mpir_nodetype_t type,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype);

/**
 * Creates an indexed datatype.
 */
int mpir_type_indexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      mpir_nodetype_t type,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype);

/**
 * Creates a struct datatype.
 */
int mpir_type_struct(int count,
                     int *array_of_blocklengths,
                     MPI_Aint *array_of_displacements,
                     MPI_Datatype *array_of_types,
                     MPI_Datatype *newtype);


/* Reduction operation functionalities */

/**
 * Commits a new operator for reduction operations.
 */
int mpir_op_create(MPI_User_function *function,
                   int commute,
                   MPI_Op *op);

/**
 * Releases the given operator for reduction operations.
 */
int mpir_op_free(MPI_Op *op);

/**
 * Gets the function associated to the given operator.
 */
mpir_function_t *mpir_get_function(MPI_Op op);

/**
 * Defines the MAX function for reduction operations.
 */
void mpir_op_max(void *invec, void *inoutvec, int *len, MPI_Datatype *type);

/**
 * Defines the MIN function for reduction operations.
 */
void mpir_op_min(void *invec, void *inoutvec, int *len, MPI_Datatype *type);

/**
 * Defines the SUM function for reduction operations.
 */
void mpir_op_sum(void *invec, void *inoutvec, int *len, MPI_Datatype *type);

/**
 * Defines the PROD function for reduction operations.
 */
void mpir_op_prod(void *invec, void *inoutvec, int *len, MPI_Datatype *type);

/* Communicator operations */

/**
 * Gets the internal representation of the given communicator.
 */
mpir_communicator_t *mpir_get_communicator(MPI_Comm comm);

/**
 * Duplicates the given communicator.
 */
int mpir_comm_dup(MPI_Comm comm, MPI_Comm *newcomm);

/**
 * Releases the given communicator.
 */
int mpir_comm_free(MPI_Comm *comm);

/**
 * Gets the NM tag for the given user tag and communicator.
 */
int mpir_project_comm_and_tag(mpir_communicator_t *mpir_communicator, int tag);

/* Termination functionalities */

/**
 * Using Friedmann  Mattern's Four Counter Method to detect
 * termination detection.
 * "Algorithms for Distributed Termination Detection." Distributed
 * Computing, vol 2, pp 161-175, 1987.
 */
tbx_bool_t mpir_test_termination(MPI_Comm comm);

/**
 * Increases by one the counter of incoming messages. The counter is
 * used for termination detection.
 */
void mpir_inc_nb_incoming_msg(void);

/**
 * Increases by one the counter of outgoing messages. The counter is
 * used for termination detection.
 */
void mpir_inc_nb_outgoing_msg(void);

#endif /* MPI_NMAD_PRIVATE_H */
