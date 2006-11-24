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

#include <tbx.h>

#define MPI_NMAD_SO_DEBUG

#define CHECK_RETURN_CODE(err, message) { if (err != NM_ESUCCESS) { printf("%s return err = %d\n", message, err); return 1; }}

#if defined(MPI_NMAD_SO_DEBUG)
#  define MPI_NMAD_TRACE(...) { fprintf(stderr, __VA_ARGS__) ; }
#else
#  define MPI_NMAD_TRACE(...) { }
#endif /* MPI_NMAD_SO_DEBUG */

#define ERROR(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); MPI_Abort(MPI_COMM_WORLD, 1); }

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
  int align; /* alignment needed for start of datatype */
  int stride; /* stride, for VECTOR and HVECTOR types */
  int *indices; /* array of indices, for (H)INDEXED, STRUCT */
  int blocklen; /* blocklen, for VECTOR and HVECTOR types */
  int block_size; /* blocklen, for VECTOR and HVECTOR types */
  int *blocklens; /* array of blocklens for (H)INDEXED, STRUCT */
  struct mpir_datatype_s *old_type;
  int nb_elements;   /* number of elements for STRUCT */
} mpir_datatype_t;

int not_implemented(char *s);

void internal_init();

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
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype);

tbx_bool_t test_termination(MPI_Comm comm);

void inc_nb_incoming_msg(void);

void inc_nb_outgoing_msg(void);

#endif /* MPI_NMAD_PRIVATE_H */
