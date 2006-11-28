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

static mpir_datatype_t   **datatypes = NULL;
static p_tbx_slist_t       available_datatypes;
static mpir_function_t   **functions = NULL;
static p_tbx_slist_t       available_functions;
static int 		   nb_incoming_msg  = 0;
static int 		   nb_outgoing_msg  = 0;

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
  datatypes = malloc((NUMBER_OF_DATATYPES+1) * sizeof(mpir_datatype_t));

  for(i=0 ; i<=MPI_LONG_LONG ; i++) {
    datatypes[i] = malloc(sizeof(mpir_datatype_t));
  }
  for(i=1 ; i<=MPI_LONG_LONG ; i++) {
    datatypes[i]->basic = 1;
    datatypes[i]->committed = 1;
    datatypes[i]->is_contig = 1;
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

  available_datatypes = tbx_slist_nil();
  for(i=MPI_LONG_LONG+1 ; i<=NUMBER_OF_DATATYPES ; i++) {
    int *ptr;
    ptr = malloc(sizeof(int));
    *ptr = i;
    tbx_slist_push(available_datatypes, ptr);
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

int sizeof_datatype(MPI_Datatype datatype) {
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

int mpir_type_commit(MPI_Datatype *datatype) {
  if (*datatype > NUMBER_OF_DATATYPES || datatypes[*datatype] == NULL) {
    ERROR("Unknown datatype %d\n", *datatype);
    return -1;
  }
  datatypes[*datatype]->committed = 1;
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
      free(datatypes[*datatype]->old_types);
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
  datatypes[*newtype]->old_type = get_datatype(oldtype);
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 1;
  datatypes[*newtype]->size = datatypes[*newtype]->old_type->size * count;
  datatypes[*newtype]->elements = count;

  MPI_NMAD_TRACE("Creating new contiguous type (%d) with size=%d based on type %d with a size %d\n", *newtype, datatypes[*newtype]->size, oldtype, sizeof_datatype(oldtype));
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
  datatypes[*newtype]->old_type = get_datatype(oldtype);
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->size = datatypes[*newtype]->old_type->size * count * blocklength;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->blocklen = blocklength;
  datatypes[*newtype]->block_size = blocklength * datatypes[*newtype]->old_type->size;
  datatypes[*newtype]->stride = stride;

  MPI_NMAD_TRACE("Creating new (h)vector type (%d) with size=%d based on type %d with a size %d\n", *newtype, datatypes[*newtype]->size, oldtype, sizeof_datatype(oldtype));
  return MPI_SUCCESS;
}

int mpir_type_indexed(int count,
                      int *array_of_blocklengths,
                      int *array_of_displacements,
                      mpir_nodetype_t type,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype) {
  int i;

  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = type;
  datatypes[*newtype]->old_type = get_datatype(oldtype);
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->elements = count;
  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(int));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->indices[i] = array_of_displacements[i];
  }
  if (type == MPIR_HINDEXED) {
    for(i=0 ; i<count ; i++) {
      datatypes[*newtype]->indices[i] *= datatypes[*newtype]->old_type->size;
    }
  }
  datatypes[*newtype]->size = datatypes[*newtype]->indices[count-1] + datatypes[*newtype]->old_type->size * datatypes[*newtype]->blocklens[count-1];

  MPI_NMAD_TRACE("Creating new index type (%d) with size=%d based on type %d with a size %d\n", *newtype, datatypes[*newtype]->size, oldtype, sizeof_datatype(oldtype));
  return MPI_SUCCESS;
}

int mpir_type_struct(int count,
                     int *array_of_blocklengths,
                     MPI_Aint *array_of_displacements,
                     MPI_Datatype *array_of_types,
                     MPI_Datatype *newtype) {
  int i;

  *newtype = get_available_datatype();
  datatypes[*newtype] = malloc(sizeof(mpir_datatype_t));

  datatypes[*newtype]->dte_type = MPIR_STRUCT;
  datatypes[*newtype]->committed = 0;
  datatypes[*newtype]->is_contig = 0;
  datatypes[*newtype]->elements = count;

  datatypes[*newtype]->blocklens = malloc(count * sizeof(int));
  datatypes[*newtype]->indices = malloc(count * sizeof(int));
  datatypes[*newtype]->old_types = malloc(count * sizeof(struct mpir_datatype_s *));
  for(i=0 ; i<count ; i++) {
    datatypes[*newtype]->blocklens[i] = array_of_blocklengths[i];
    datatypes[*newtype]->old_types[i] = get_datatype(array_of_types[i]);
    datatypes[*newtype]->indices[i] = array_of_displacements[i];
  }
  datatypes[*newtype]->size = datatypes[*newtype]->indices[count-1] + datatypes[*newtype]->old_types[count-1]->size * datatypes[*newtype]->blocklens[count-1];

  MPI_NMAD_TRACE("Creating new struct type (%d) with size=%d\n", *newtype, datatypes[*newtype]->size);
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
  free(functions[*op]);
  int *ptr;
  ptr = malloc(sizeof(int));
  *ptr = *op;
  tbx_slist_enqueue(available_functions, ptr);
  return MPI_SUCCESS; 
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
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MAX */
    default : {
      not_implemented("Datatype for MAX Reduce operation");
      break;
    }
  }
}

void mpir_op_min(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
          if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MIN */
    default : {
      not_implemented("Datatype for MIN Reduce operation");
      break;
    }
  }
}

void mpir_op_sum(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        MPI_NMAD_TRACE("Summing %d and %d\n", i_inoutvec[i], i_invec[i]);
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_SUM */
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_SUM */
    default : {
      not_implemented("Datatype for SUM Reduce operation");
      break;
    }
  }
}

void mpir_op_prod(void *invec, void *inoutvec, int *len, MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_PROD */
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_PROD */
    default : {
      not_implemented("Datatype for PROD Reduce operation");
      break;
    }
  }
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
  
  if (process_rank == 0) {
    // 1st phase
    int global_nb_incoming_msg = 0;
    int global_nb_outgoing_msg = 0;
    int i, remote_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    global_nb_incoming_msg = nb_incoming_msg;
    global_nb_outgoing_msg = nb_outgoing_msg;
    for(i=1 ; i<global_size ; i++) {
      MPI_Recv(remote_counters, 2, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
      global_nb_incoming_msg += remote_counters[0];
      global_nb_outgoing_msg += remote_counters[1];
    }
    MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);

    tbx_bool_t answer = tbx_true;
    if (global_nb_incoming_msg != global_nb_outgoing_msg) {
      answer = tbx_false;
    }

    for(i=1 ; i<global_size ; i++) {
      MPI_Send(&answer, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
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
        MPI_Recv(remote_counters, 2, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
        global_nb_incoming_msg += remote_counters[0];
        global_nb_outgoing_msg += remote_counters[1];
      }

      MPI_NMAD_TRACE("Counter [incoming msg=%d, outgoing msg=%d]\n", global_nb_incoming_msg, global_nb_outgoing_msg);
      tbx_bool_t answer = tbx_true;
      if (global_nb_incoming_msg != global_nb_outgoing_msg) {
        answer = tbx_false;
      }

      for(i=1 ; i<global_size ; i++) {
        MPI_Send(&answer, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      }

      return answer;
    }
  }
  else {
    int answer, local_counters[2];

    MPI_NMAD_TRACE("Beginning of 1st phase.\n");
    local_counters[0] = nb_incoming_msg;
    local_counters[1] = nb_outgoing_msg;
    MPI_Send(local_counters, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);

    MPI_Recv(&answer, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
    if (answer == tbx_false) {
      return tbx_false;
    }
    else {
      // 2nd phase
      MPI_NMAD_TRACE("Beginning of 2nd phase.\n");
      local_counters[0] = nb_incoming_msg;
      local_counters[1] = nb_outgoing_msg;
      MPI_Send(local_counters, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);

      MPI_Recv(&answer, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
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

