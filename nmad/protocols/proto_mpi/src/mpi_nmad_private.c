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
 * mpi_nmad_private.c
 * ==================
 */

#include <stdint.h>
#include "mpi.h"
#include "mpi_nmad_private.h"

static mpir_datatype_t     **datatypes = NULL;
static p_tbx_slist_t         available_datatypes;
static mpir_function_t     **functions = NULL;
static p_tbx_slist_t         available_functions;
static mpir_communicator_t **communicators = NULL;
static p_tbx_slist_t         available_communicators;
static int 		     nb_incoming_msg  = 0;
static int 		     nb_outgoing_msg  = 0;

debug_type_t debug_mpi_nmad_trace=NEW_DEBUG_TYPE("MPI_NMAD: ", "mpi_nmad_trace");
debug_type_t debug_mpi_nmad_transfer=NEW_DEBUG_TYPE("MPI_NMAD_TRANSFER: ", "mpi_nmad_transfer");
debug_type_t debug_mpi_nmad_log=NEW_DEBUG_TYPE("MPI_NMAD_LOG: ", "mpi_nmad_log");

int not_implemented(char *s) {
  fprintf(stderr, "*************** ERROR: %s: Not implemented yet\n", s);
  fflush(stderr);
  return MPI_Abort(MPI_COMM_WORLD, 1);
}

void internal_init() {
  int i;

  /*
   * Initialise the basic datatypes
   */
  datatypes = malloc((NUMBER_OF_DATATYPES+1) * sizeof(mpir_datatype_t *));

  for(i=0 ; i<=MPI_INTEGER ; i++) {
    datatypes[i] = malloc(sizeof(mpir_datatype_t));
  }
  for(i=0 ; i<=MPI_INTEGER ; i++) {
    datatypes[i]->basic = 1;
    datatypes[i]->committed = 1;
    datatypes[i]->is_contig = 1;
    datatypes[i]->dte_type = MPIR_BASIC;
  }
  datatypes[0]->size = 0;
  datatypes[MPI_CHAR]->size = sizeof(signed char);
  datatypes[MPI_UNSIGNED_CHAR]->size = sizeof(unsigned char);
  datatypes[MPI_BYTE]->size = 1;
  datatypes[MPI_SHORT]->size = sizeof(signed short);
  datatypes[MPI_UNSIGNED_SHORT]->size = sizeof(unsigned short);
  datatypes[MPI_INT]->size = sizeof(signed int);
  datatypes[MPI_UNSIGNED]->size = sizeof(unsigned int);
  datatypes[MPI_LONG]->size = sizeof(signed long);
  datatypes[MPI_UNSIGNED_LONG]->size = sizeof(unsigned long);
  datatypes[MPI_FLOAT]->size = sizeof(float);
  datatypes[MPI_DOUBLE]->size = sizeof(double);
  datatypes[MPI_LONG_DOUBLE]->size = sizeof(long double);
  datatypes[MPI_LONG_LONG_INT]->size = sizeof(long long int);
  datatypes[MPI_LONG_LONG]->size = sizeof(long long);
  datatypes[MPI_LOGICAL]->size = sizeof(float);
  datatypes[MPI_REAL]->size = sizeof(float);
  datatypes[MPI_DOUBLE_PRECISION]->size = sizeof(double);
  datatypes[MPI_INTEGER]->size = sizeof(float);

  datatypes[0]->extent = datatypes[0]->size;
  datatypes[MPI_CHAR]->extent =  datatypes[MPI_CHAR]->size;
  datatypes[MPI_UNSIGNED_CHAR]->extent = datatypes[MPI_UNSIGNED_CHAR]->size;
  datatypes[MPI_BYTE]->extent = datatypes[MPI_BYTE]->size;
  datatypes[MPI_SHORT]->extent = datatypes[MPI_SHORT]->size;
  datatypes[MPI_UNSIGNED_SHORT]->extent = datatypes[MPI_UNSIGNED_SHORT]->size;
  datatypes[MPI_INT]->extent =   datatypes[MPI_INT]->size; 
  datatypes[MPI_UNSIGNED]->extent = datatypes[MPI_UNSIGNED]->size;
  datatypes[MPI_LONG]->extent = datatypes[MPI_LONG]->size;
  datatypes[MPI_UNSIGNED_LONG]->extent = datatypes[MPI_UNSIGNED_LONG]->size;
  datatypes[MPI_FLOAT]->extent = datatypes[MPI_FLOAT]->size;
  datatypes[MPI_DOUBLE]->extent = datatypes[MPI_DOUBLE]->size;
  datatypes[MPI_LONG_DOUBLE]->extent = datatypes[MPI_LONG_DOUBLE]->size;
  datatypes[MPI_LONG_LONG_INT]->extent = datatypes[MPI_LONG_LONG_INT]->size;
  datatypes[MPI_LONG_LONG]->extent = datatypes[MPI_LONG_LONG]->size;
  datatypes[MPI_REAL]->extent = datatypes[MPI_REAL]->size;
  datatypes[MPI_DOUBLE_PRECISION]->extent = datatypes[MPI_DOUBLE_PRECISION]->size;
  datatypes[MPI_INTEGER]->extent = datatypes[MPI_INTEGER]->size;

  available_datatypes = tbx_slist_nil();
  for(i=MPI_LONG_LONG+1 ; i<=NUMBER_OF_DATATYPES ; i++) {
    int *ptr;
    ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(available_datatypes, ptr);
  }

  /* Initialise data for communicators */
  communicators = malloc(NUMBER_OF_COMMUNICATORS * sizeof(mpir_communicator_t));
  communicators[0] = malloc(sizeof(mpir_communicator_t));
  communicators[0]->is_global = 1;
  communicators[0]->communicator_id = MPI_COMM_WORLD;
  available_communicators = tbx_slist_nil();
  for(i=MPI_COMM_WORLD+1 ; i<=NUMBER_OF_COMMUNICATORS ; i++) {
    int *ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(available_communicators, ptr);
  }

  /* Initialise the collective functions */
  functions = malloc((NUMBER_OF_FUNCTIONS+1) * sizeof(mpir_function_t));
  for(i=MPI_MAX ; i<=MPI_MAXLOC ; i++) {
    functions[i] = malloc(sizeof(mpir_function_t));
    functions[i]->commute = 1;
  }
  functions[MPI_MAX]->function = &mpir_op_max;
  functions[MPI_MIN]->function = &mpir_op_min;
  functions[MPI_SUM]->function = &mpir_op_sum;
  functions[MPI_PROD]->function = &mpir_op_prod;

  available_functions = tbx_slist_nil();
  for(i=1 ; i<MPI_MAX ; i++) {
    int *ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(available_functions, ptr);
  }
}

void internal_exit() {
  int i;

  for(i=0 ; i<=MPI_INTEGER ; i++) {
    free(datatypes[i]);
  }
  free(datatypes);
  while (tbx_slist_is_nil(available_datatypes) == tbx_false) {
    int *ptr = tbx_slist_extract(available_datatypes);
    free(ptr);
  }
  tbx_slist_clear(available_datatypes);
  tbx_slist_free(available_datatypes);

  free(communicators[0]);
  free(communicators);
  while (tbx_slist_is_nil(available_communicators) == tbx_false) {
    int *ptr = tbx_slist_extract(available_communicators);
    free(ptr);
  }
  tbx_slist_clear(available_communicators);
  tbx_slist_free(available_communicators);

  for(i=MPI_MAX ; i<=MPI_MAXLOC ; i++) {
    free(functions[i]);
  }
  free(functions);
  while (tbx_slist_is_nil(available_functions) == tbx_false) {
    int *ptr = tbx_slist_extract(available_functions);
    free(ptr);
  }
  tbx_slist_clear(available_functions);
  tbx_slist_free(available_functions);
}

int get_available_datatype() {
  if (tbx_slist_is_nil(available_datatypes) == tbx_true) {
    ERROR("Maximum number of datatypes created");
    return -1;
  }
  else {
    int datatype;
    int *ptr = tbx_slist_extract(available_datatypes);
    datatype = *ptr;
    free(ptr);
    return datatype;
  }
}

size_t sizeof_datatype(MPI_Datatype datatype) {
  mpir_datatype_t *mpir_datatype = get_datatype(datatype);
  return mpir_datatype->size;
}

mpir_datatype_t* get_datatype(MPI_Datatype datatype) {
  if (datatype < NUMBER_OF_DATATYPES) {
    return datatypes[datatype];
  }
  else {
    ERROR("Datatype %d unknown", datatype);
    return NULL;
  }
}

int mpir_type_size(MPI_Datatype datatype, int *size) {
  if (datatype < NUMBER_OF_DATATYPES) {
    *size = datatypes[datatype]->size;
    return MPI_SUCCESS;
  }
  else {
    ERROR("Datatype %d unknown", datatype);
    return -1;
  }
}

int mpir_type_commit(MPI_Datatype *datatype) {
  if (*datatype > NUMBER_OF_DATATYPES || datatypes[*datatype] == NULL) {
    ERROR("Unknown datatype %d\n", *datatype);
    return -1;
  }
  datatypes[*datatype]->committed = 1;
  datatypes[*datatype]->is_optimized = 0;
  return MPI_SUCCESS;
}

int mpir_type_free(MPI_Datatype *datatype) {
  if (*datatype > NUMBER_OF_DATATYPES || datatypes[*datatype] == NULL) {
    ERROR("Unknown datatype %d\n", *datatype);
    return -1;
  }
  if (datatypes[*datatype]->dte_type == MPIR_INDEXED ||
      datatypes[*datatype]->dte_type == MPIR_HINDEXED ||
      datatypes[*datatype]->dte_type == MPIR_STRUCT) {
    free(datatypes[*datatype]->blocklens);
    free(datatypes[*datatype]->indices);
    if (datatypes[*datatype]->dte_type == MPIR_STRUCT) {
      free(datatypes[*datatype]->old_sizes);
    }
  }
  free(datatypes[*datatype]);
  int *ptr;
  ptr = malloc(sizeof(int));
  *ptr = *datatype;
  tbx_slist_enqueue(available_datatypes, ptr);
  return MPI_SUCCESS;
}

int mpir_type_contiguous(int count,
                         MPI_Datatype oldtype,
                         MPI_Datatype *newtype) {
  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = MPIR_CONTIG;
  datatypes[*newtype]->old_size = sizeof_datatype(oldtype);
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 1;
  datatypes[*newtype]->size = datatypes[*newtype]->old_size * count;
  datatypes[*newtype]->elements = count;

  MPI_NMAD_TRACE("Creating new contiguous type (%d) with size=%lu based on type %d with a size %lu\n", *newtype, (unsigned long)datatypes[*newtype]->size, oldtype, (unsigned long)datatypes[*newtype]->old_size);
  return MPI_SUCCESS;
}

int mpir_type_vector(int count,
                     int blocklength,
                     int stride,
                     mpir_nodetype_t type,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype) {
  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = type;
  datatypes[*newtype]->old_size = sizeof_datatype(oldtype);
  datatypes[*newtype]->old_sizes = NULL;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->size = datatypes[*newtype]->old_size * count * blocklength;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->blocklen = blocklength;
  datatypes[*newtype]->block_size = blocklength * datatypes[*newtype]->old_size;
  datatypes[*newtype]->stride = stride;

  MPI_NMAD_TRACE("Creating new (h)vector type (%d) with size=%lu based on type %d with a size %lu\n", *newtype, (unsigned long)datatypes[*newtype]->size, oldtype, (unsigned long)sizeof_datatype(oldtype));
  return MPI_SUCCESS;
}

int mpir_type_indexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      mpir_nodetype_t type,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  int i;

  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = type;
  datatypes[*newtype]->old_size = sizeof_datatype(oldtype);
  datatypes[*newtype]->old_sizes = NULL;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->indices[i] = array_of_displacements[i];
  }
  if (type == MPIR_HINDEXED) {
    for(i=0 ; i<count ; i++) {
      datatypes[*newtype]->indices[i] *= datatypes[*newtype]->old_size;
    }
  }
  datatypes[*newtype]->size = datatypes[*newtype]->indices[count-1] + datatypes[*newtype]->old_size * datatypes[*newtype]->blocklens[count-1];

  MPI_NMAD_TRACE("Creating new index type (%d) with size=%lu based on type %d with a size %lu\n", *newtype, (unsigned long)datatypes[*newtype]->size, oldtype, (unsigned long)sizeof_datatype(oldtype));
  return MPI_SUCCESS;
}

int mpir_type_struct(int count,
                     int *array_of_blocklengths,
                     MPI_Aint *array_of_displacements,
                     MPI_Datatype *array_of_types,
                     MPI_Datatype *newtype) {
  int i;

  MPI_NMAD_TRACE("Creating struct derived datatype based on %d elements\n", count);
  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = MPIR_STRUCT;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->size = 0;

  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(MPI_Aint));
  datatypes[*newtype]->old_sizes = malloc(count * sizeof(size_t));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->old_sizes[i] = get_datatype(array_of_types[i])->size;
    datatypes[*newtype]->indices[i] = array_of_displacements[i];

    MPI_NMAD_TRACE("Element %d: length %d, old_type size %lu, indice %lu\n", i, datatypes[*newtype]->blocklens[i], (unsigned long)datatypes[*newtype]->old_sizes[i], (unsigned long)datatypes[*newtype]->indices[i]);
    datatypes[*newtype]->size +=  datatypes[*newtype]->blocklens[i] * datatypes[*newtype]->old_sizes[i];
  }
  /* We suppose here that the last field of the struct does not need
     an alignment. In case, one sends an array of struct, the 1st
     field of the 2nd struct immediatly follows the last field of the
     previous struct.
  */
  datatypes[*newtype]->extent = datatypes[*newtype]->indices[count-1] + datatypes[*newtype]->blocklens[count-1] * datatypes[*newtype]->old_sizes[count-1];
  MPI_NMAD_TRACE("Creating new struct type (%d) with size=%lu and extent=%lu\n", *newtype, (unsigned long)datatypes[*newtype]->size, (unsigned long)datatypes[*newtype]->extent);
  return MPI_SUCCESS;
}

int mpir_op_create(MPI_User_function *function,
                   int commute,
                   MPI_Op *op) {
  if (tbx_slist_is_nil(available_functions) == tbx_true) {
    ERROR("Maximum number of operations created");
    return -1;
  }
  else {
    int *ptr = tbx_slist_extract(available_functions);
    *op = *ptr;
    free(ptr);

    functions[*op] = malloc(sizeof(mpir_function_t));
    functions[*op]->function = function;
    functions[*op]->commute = commute;
    return MPI_SUCCESS;
  }
}

int mpir_op_free(MPI_Op *op) {
  if (*op > NUMBER_OF_FUNCTIONS || functions[*op] == NULL) {
    ERROR("Unknown operator %d\n", *op);
    return -1;
  }
  else {
    int *ptr;
    free(functions[*op]);
    ptr = malloc(sizeof(int));
    *ptr = *op;
    tbx_slist_enqueue(available_functions, ptr);
    return MPI_SUCCESS; 
  }
}

mpir_function_t *mpir_get_function(MPI_Op op) {
  if (functions[op] != NULL) {
    return functions[op];
  }
  else {
    ERROR("Operation %d unknown", op);
    return NULL;
  }
}

void mpir_op_max(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MAX */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_MAX */
    default : {
      ERROR("Datatype %d for MAX Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_min(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
          if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MIN */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_MAX */
    default : {
      ERROR("Datatype %d for MIN Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_sum(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        MPI_NMAD_TRACE("Summing %d and %d\n", i_inoutvec[i], i_invec[i]);
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_SUM */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_SUM */
    default : {
      ERROR("Datatype %d for SUM Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_prod(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_PROD */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_PROD */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_PROD */
    default : {
      ERROR("Datatype %d for PROD Reduce operation", *type);
      break;
    }
  }
}

int mpir_comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  if (tbx_slist_is_nil(available_communicators) == tbx_true) {
    ERROR("Maximum number of communicators created");
    return -1;
  }
  else if (communicators[comm-MPI_COMM_WORLD] == NULL) {
    ERROR("Communicator %d is not valid", comm);
    return -1;
  }
  else {
    int *ptr = tbx_slist_extract(available_communicators);
    *newcomm = *ptr;
    free(ptr);

    communicators[*newcomm - MPI_COMM_WORLD] = malloc(sizeof(mpir_communicator_t));
    communicators[*newcomm - MPI_COMM_WORLD]->communicator_id = *newcomm;
    communicators[*newcomm - MPI_COMM_WORLD]->is_global = 1;

    return MPI_SUCCESS;
  }
}

int mpir_comm_free(MPI_Comm *comm) {
  if (*comm == MPI_COMM_WORLD) {
    ERROR("Cannot free communicator MPI_COMM_WORLD");
    return -1;
  }
  else if (*comm > NUMBER_OF_COMMUNICATORS || communicators[*comm-MPI_COMM_WORLD] == NULL) {
    ERROR("Unknown communicator %d\n", *comm);
    return -1;
  }
  else {
    int *ptr;
    free(communicators[*comm-MPI_COMM_WORLD]);
    communicators[*comm-MPI_COMM_WORLD] = NULL;
    ptr = malloc(sizeof(int));
    *ptr = *comm;
    tbx_slist_enqueue(available_communicators, ptr);
    return MPI_SUCCESS; 
  }
}

__inline__
int mpir_is_comm_valid(MPI_Comm comm) {
  if (comm > NUMBER_OF_COMMUNICATORS || communicators[comm-MPI_COMM_WORLD] == NULL) {
    return 0;
  }
  else {
    return (communicators[comm-MPI_COMM_WORLD]->is_global == 1);
  }
}

__inline__
int mpir_project_comm_and_tag(MPI_Comm comm, int tag) {
  /*
   * NewMadeleine only allows us 7 bits!
   * We suppose that comm is represented on 3 bits and tag on 4 bits.
   * We stick both of them into a new 7-bits representation
   */
  int newtag = (comm-MPI_COMM_WORLD) << 4;
  newtag += tag;
  return newtag;
}

/*
 * Using Friedmann  Mattern's Four Counter Method to detect
 * termination detection.
 * "Algorithms for Distributed Termination Detection." Distributed
 * Computing, vol 2, pp 161-175, 1987.
 */
tbx_bool_t test_termination(MPI_Comm comm) {
  int process_rank, global_size;
  MPI_Comm_rank(comm, &process_rank);
  MPI_Comm_size(comm, &global_size);
  int tag = 49;
  
  if (process_rank == 0) {
    // 1st phase
    int global_nb_incoming_msg = 0;
    int global_nb_outgoing_msg = 0;
    int i, remote_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    global_nb_incoming_msg = nb_incoming_msg;
    global_nb_outgoing_msg = nb_outgoing_msg;
    for(i=1 ; i<global_size ; i++) {
      MPI_Recv(remote_counters, 2, MPI_INT, i, tag, MPI_COMM_WORLD, NULL);
      global_nb_incoming_msg += remote_counters[0];
      global_nb_outgoing_msg += remote_counters[1];
    }
    MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);

    tbx_bool_t answer = tbx_true;
    if (global_nb_incoming_msg != global_nb_outgoing_msg) {
      answer = tbx_false;
    }

    for(i=1 ; i<global_size ; i++) {
      MPI_Send(&answer, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
    }

    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      global_nb_incoming_msg = nb_incoming_msg;
      global_nb_outgoing_msg = nb_outgoing_msg;
      for(i=1 ; i<global_size ; i++) {
        MPI_Recv(remote_counters, 2, MPI_INT, i, tag, MPI_COMM_WORLD, NULL);
        global_nb_incoming_msg += remote_counters[0];
        global_nb_outgoing_msg += remote_counters[1];
      }

      MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);
      tbx_bool_t answer = tbx_true;
      if (global_nb_incoming_msg != global_nb_outgoing_msg) {
        answer = tbx_false;
      }

      for(i=1 ; i<global_size ; i++) {
        MPI_Send(&answer, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
      }

      return answer;
    }
  }
  else {
    int answer, local_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    local_counters[0] = nb_incoming_msg;
    local_counters[1] = nb_outgoing_msg;
    MPI_Send(local_counters, 2, MPI_INT, 0, tag, MPI_COMM_WORLD);

    MPI_Recv(&answer, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, NULL);
    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      local_counters[0] = nb_incoming_msg;
      local_counters[1] = nb_outgoing_msg;
      MPI_Send(local_counters, 2, MPI_INT, 0, tag, MPI_COMM_WORLD);

      MPI_Recv(&answer, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, NULL);
      return answer;
    }
  }
}

void inc_nb_incoming_msg(void) {
  nb_incoming_msg ++;
}

void inc_nb_outgoing_msg(void) {
  nb_outgoing_msg ++;
}

