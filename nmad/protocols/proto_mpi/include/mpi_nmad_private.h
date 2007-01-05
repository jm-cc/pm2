/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#undef MPI_NMAD_SO_DEBUG
#undef MPI_NMAD_SO_TRACE

#define CHECK_RETURN_CODE(err, message) { if (err != NM_ESUCCESS) { printf("%s return err = %d\n", message, err); return 1; }}

#if defined(MPI_NMAD_SO_DEBUG)
#  define MPI_NMAD_TRACE(...) { fprintf(stderr, __VA_ARGS__) ; }
#else
#  define MPI_NMAD_TRACE(...) { }
#endif /* MPI_NMAD_SO_DEBUG */

#if defined(MPI_NMAD_SO_TRACE)
#  define MPI_NMAD_TRANSFER(...) { fprintf(stderr, __VA_ARGS__) ; }
#else
#  define MPI_NMAD_TRANSFER(...) { }
#endif /* MPI_NMAD_SO_TRACE */

#define ERROR(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); MPI_Abort(MPI_COMM_WORLD, 1); }

#define NUMBER_OF_COMMUNICATORS (MPI_COMM_WORLD + 7)

typedef struct mpir_communicator_s {
  int communicator_id;
  int is_global;
} mpir_communicator_t;

typedef int MPI_Request_type;
#define MPI_REQUEST_ZERO ((MPI_Request_type)0)
#define MPI_REQUEST_SEND ((MPI_Request_type)1)
#define MPI_REQUEST_RECV ((MPI_Request_type)2)
#define MPI_REQUEST_PACK_SEND ((MPI_Request_type)3)
#define MPI_REQUEST_PACK_RECV ((MPI_Request_type)4)

struct MPI_Request_s {
  MPI_Request_type request_type;
  intptr_t request_id;
  struct nm_so_cnx request_cnx;
};

#define NUMBER_OF_FUNCTIONS MPI_MAXLOC

typedef struct mpir_function_s {
  MPI_User_function *function;
  int commute;
} mpir_function_t;

#define NUMBER_OF_DATATYPES (MPI_LONG_LONG + 20)

typedef enum {
    MPIR_INT, MPIR_FLOAT, MPIR_DOUBLE, MPIR_COMPLEX, MPIR_LONG, MPIR_SHORT,
    MPIR_CHAR, MPIR_BYTE, MPIR_UCHAR, MPIR_USHORT, MPIR_ULONG, MPIR_UINT,
    MPIR_CONTIG, MPIR_VECTOR, MPIR_HVECTOR,
    MPIR_INDEXED, MPIR_HINDEXED, MPIR_STRUCT, MPIR_DOUBLE_COMPLEX, MPIR_PACKED,
    MPIR_UB, MPIR_LB, MPIR_LONGDOUBLE, MPIR_LONGLONGINT,
    MPIR_LOGICAL, MPIR_FORT_INT
} mpir_nodetype_t;

typedef struct mpir_datatype_s {
  mpir_nodetype_t    dte_type; /* type of datatype element this is */
  int basic;
  int committed; /* whether committed or not */
  int is_contig; /* whether entirely contiguous */
  int size; /* size of type */
  int elements; /* number of basic elements */
  int stride; /* stride, for VECTOR and HVECTOR types */
  int *indices; /* array of indices, for (H)INDEXED, STRUCT */
  int blocklen; /* blocklen, for VECTOR and HVECTOR types */
  int block_size; /* blocklen, for VECTOR and HVECTOR types */
  int *blocklens; /* array of blocklens for (H)INDEXED, STRUCT */
  struct mpir_datatype_s *old_type;
  struct mpir_datatype_s **old_types;
  int is_optimized;
} mpir_datatype_t;

int not_implemented(char *s);

void internal_init();


/*
 * Datatype functionalities
 */

int sizeof_datatype(MPI_Datatype datatype);

mpir_datatype_t* get_datatype(MPI_Datatype datatype);

int mpir_type_commit(MPI_Datatype *datatype);

int mpir_type_free(MPI_Datatype *datatype);

int mpir_type_contiguous(int count,
                         MPI_Datatype oldtype,
                         MPI_Datatype *newtype);

int mpir_type_vector(int count,
                     int blocklength,
                     int stride,
                     mpir_nodetype_t type,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype);

int mpir_type_indexed(int count,
                      int *array_of_blocklengths,
                      int *array_of_displacements,
                      mpir_nodetype_t type,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype);

int mpir_type_struct(int count,
                     int *array_of_blocklengths,
                     MPI_Aint *array_of_displacements,
                     MPI_Datatype *array_of_types,
                     MPI_Datatype *newtype);


/*
 * Reduction operation functionalities
 */

int mpir_op_create(MPI_User_function *function,
                   int commute,
                   MPI_Op *op);

int mpir_op_free(MPI_Op *op);

mpir_function_t *mpir_get_function(MPI_Op op);

void mpir_op_max(void *invec, void *inoutvec, int *len, MPI_Datatype *type);
void mpir_op_min(void *invec, void *inoutvec, int *len, MPI_Datatype *type);
void mpir_op_sum(void *invec, void *inoutvec, int *len, MPI_Datatype *type);
void mpir_op_prod(void *invec, void *inoutvec, int *len, MPI_Datatype *type);

/*
 * Communicator operations
 */

int mpir_comm_dup(MPI_Comm comm, MPI_Comm *newcomm);

int mpir_comm_free(MPI_Comm *comm);

__inline__
int mpir_is_comm_valid(MPI_Comm comm);

__inline__
int mpir_project_comm_and_tag(MPI_Comm comm, int tag);

/*
 * Termination functionalities
 */

tbx_bool_t test_termination(MPI_Comm comm);

void inc_nb_incoming_msg(void);

void inc_nb_outgoing_msg(void);

#endif /* MPI_NMAD_PRIVATE_H */
