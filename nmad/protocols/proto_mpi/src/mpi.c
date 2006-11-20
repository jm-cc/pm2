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
 * mpi.c
 * =====
 */

#include <stdint.h>
#include <madeleine.h>
#include <nm_public.h>
#include <nm_so_public.h>
#include <nm_so_sendrecv_interface.h>
#include <nm_mad3_private.h>

#include "mpi.h"
#include "mpi_nmad_private.h"

static p_mad_madeleine_t  madeleine       = NULL;
static int                global_size     = -1;
static int                process_rank    = -1;
static int               *sizeof_datatype = NULL;
static nm_so_sr_interface p_so_sr_if;
static long              *out_gate_id;
static long              *in_gate_id;

int not_implemented(char *s)
{
  printf("%s: Not implemented yet\n", s);
  abort();
}

int MPI_Init(int *argc,
             char ***argv) {

  p_mad_session_t                  session    = NULL;
  struct nm_core                  *p_core     = NULL;
  int                              err;
  int                              dest;
  int                              source;
  p_mad_channel_t                  channel    = NULL;
  p_mad_connection_t               connection = NULL;
  p_mad_nmad_connection_specific_t cs	      = NULL;

  /*
   * Initialization of various libraries.
   * Reference to the Madeleine object.
   */
  madeleine    = mad_init(argc, *argv);

  /*
   * Reference to the session information object
   */
  session      = madeleine->session;

  /*
   * Globally unique process rank.
   */
  process_rank = session->process_rank;
  //printf("My global rank is %d\n", process_rank);

  /*
   * How to obtain the configuration size.
   */
  global_size = tbx_slist_get_length(madeleine->dir->process_slist);
  //printf("The configuration size is %d\n", global_size);

  /*
   * Reference to the NewMadeleine core object
   */
  p_core = mad_nmad_get_core();
  err = nm_so_sr_interface_init(p_core, &p_so_sr_if);
  CHECK_RETURN_CODE(err, "nm_so_sr_interface_init");

  /*
   * Init the sizeof_datatype array
   */
  sizeof_datatype = malloc((MPI_LONG_LONG + 1) * sizeof(int));
  sizeof_datatype[0] = 0;
  sizeof_datatype[MPI_CHAR] = sizeof(signed char);
  sizeof_datatype[MPI_UNSIGNED_CHAR] = sizeof(unsigned char);
  sizeof_datatype[MPI_BYTE] = 1;
  sizeof_datatype[MPI_SHORT] = sizeof(signed short);
  sizeof_datatype[MPI_UNSIGNED_SHORT] = sizeof(unsigned short);
  sizeof_datatype[MPI_INT] = sizeof(signed int);
  sizeof_datatype[MPI_UNSIGNED] = sizeof(unsigned int);
  sizeof_datatype[MPI_LONG] = sizeof(signed long);
  sizeof_datatype[MPI_UNSIGNED_LONG] = sizeof(unsigned long);
  sizeof_datatype[MPI_FLOAT] = sizeof(float);
  sizeof_datatype[MPI_DOUBLE] = sizeof(double);
  sizeof_datatype[MPI_LONG_DOUBLE] = sizeof(long double);
  sizeof_datatype[MPI_LONG_LONG_INT] = sizeof(long long int);
  sizeof_datatype[MPI_LONG_LONG] = sizeof(long long);

  /*
   * Store the gate id of all the other processes
   */
  out_gate_id = malloc(global_size * sizeof(long));
  in_gate_id = malloc(global_size * sizeof(long));

  /* Get a reference to the channel structure */
  channel = tbx_htable_get(madeleine->channel_htable, "channel_comm_world");

  /* If that fails, it means that our process does not belong to the channel */
  if (!channel) {
    fprintf(stderr, "I don't belong to this channel");
    return 1;
  }

  for(dest=0 ; dest<global_size ; dest++) {
    out_gate_id[dest] = -1;
    if (dest == process_rank) continue;

    connection = tbx_darray_get(channel->out_connection_darray, dest);
    if (!connection) {
      fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    }
    else {
      MPI_NMAD_TRACE("Connection out: %p\n", out);
      cs = connection->specific;
      out_gate_id[dest] = cs->gate_id;
    }
  }

  for(source=0 ; source<global_size ; source++) {
    in_gate_id[source] = -1;
    if (source == process_rank) continue;

    connection =  tbx_darray_get(channel->in_connection_darray, source);
    if (!connection) {
      fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
    }
    else {
      MPI_NMAD_TRACE("Connection in: %p\n", in);
      cs = connection->specific;
      in_gate_id[source] = cs->gate_id;
    }
  }

  return 0;
}

int MPI_Finalize(void) {
  mad_exit(madeleine);
  return 0;
}

int MPI_Comm_size(MPI_Comm comm,
                  int *size) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  *size = global_size;
  return 0;
}

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) {
  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  *rank = process_rank;
  return 0;
}

int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm) {
  MPI_Request request;
  int         err = 0;

  MPI_Isend(buffer, count, datatype, dest, tag, comm, &request);

  err = nm_so_sr_swait(p_so_sr_if, request);

  return err;
}

int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status) {
  MPI_Request request;
  int         err = 0;

  MPI_Irecv(buffer, count, datatype, source, tag, comm, &request);

  err = nm_so_sr_rwait(p_so_sr_if, request);

  if (status != NULL) {
    status->count = count;
    status->MPI_SOURCE = source;
    status->MPI_TAG = tag;
    status->MPI_ERROR = err;
  }

  return err;
}

int MPI_Isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  int  err     = 0;
  long gate_id;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  if (dest >= global_size || out_gate_id[dest] == -1) {
    fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    return 1;
  }

  gate_id = out_gate_id[dest];
  err = nm_so_sr_isend(p_so_sr_if, gate_id, tag, buffer, count * sizeof_datatype[datatype], request);

  inc_nb_outgoing_msg();
  return err;
}

int MPI_Irecv(void* buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) {
  int err      = 0;
  long gate_id;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  if (source >= global_size || in_gate_id[source] == -1) {
    fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
    return 1;
  }

  gate_id = in_gate_id[source];
  err = nm_so_sr_irecv(p_so_sr_if, gate_id, tag, buffer, count * sizeof_datatype[datatype], request);

  inc_nb_incoming_msg();
  return err;
}

int MPI_Barrier(MPI_Comm comm) {

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  tbx_bool_t termination = test_termination(comm);
  MPI_NMAD_TRACE("Result %d\n", termination);
  while (termination == tbx_false) {
    sleep(1);
    termination = test_termination(comm);
  }
  return 0;
}

double MPI_Wtime(void) {
  tbx_tick_t time;
  TBX_GET_TICK(time);
  double usec = tbx_tick2usec(time);
  return usec / 1000000;
}


