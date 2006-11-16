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

#undef MPI_NMAD_SO_DEBUG

#include <stdint.h>
#include <madeleine.h>
#include "mpi.h"

static p_mad_madeleine_t madeleine       = NULL;
static int               global_size     = -1;
static int               process_rank    = -1;
static int              *sizeof_datatype = NULL;

int not_implemented(char *s)
{
  printf("%s: Not implemented yet\n", s);
  abort();
}

int MPI_Init(int *argc,
             char ***argv) {

  p_mad_session_t   session   = NULL;

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
  p_mad_channel_t    channel = NULL;
  p_mad_connection_t out     = NULL;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  /* Get a reference to the channel structure */
  channel = tbx_htable_get(madeleine->channel_htable, "channel_comm_world");

  /* If that fails, it means that our process does not belong to the channel */
  if (!channel) {
    fprintf(stderr, "I don't belong to this channel");
    return 1;
  }

  out = mad_nmad_begin_packing(channel, dest);
  if (!out) {
    fprintf(stderr, "Cannot find a connection between %d and %d\n", process_rank, dest);
    return 1;
  }
#if defined(MPI_NMAD_SO_DEBUG)
  fprintf(stderr, "Connection out: %p\n", out);
#endif /* MPI_NMAD_SO_DEBUG */

  mad_nmad_pack(out, buffer, count * sizeof_datatype[datatype], mad_send_SAFER, mad_receive_EXPRESS);
  mad_nmad_end_packing(out);
  return 0;
}

int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status) {
  p_mad_channel_t    channel = NULL;
  p_mad_connection_t in      = NULL;

  if (comm != MPI_COMM_WORLD) return not_implemented("Not using MPI_COMM_WORLD");

  /* Get a reference to the channel structure */
  channel = tbx_htable_get(madeleine->channel_htable, "channel_comm_world");

  /* If that fails, it means that our process does not belong to the channel */
  if (!channel) {
    fprintf(stderr, "I don't belong to this channel");
    return 1;
  }

  in = mad_nmad_begin_unpacking_from(channel, source);
  if (!in) {
    fprintf(stderr, "Cannot find a in connection between %d and %d\n", process_rank, source);
    return 1;
  }
#if defined(MPI_NMAD_SO_DEBUG)
  fprintf(stderr, "Connection in: %p\n", in);
#endif /* MPI_NMAD_SO_DEBUG */

  mad_nmad_unpack(in, buffer, count * sizeof_datatype[datatype], mad_send_SAFER, mad_receive_EXPRESS);
  mad_nmad_end_unpacking(in);

  status->count = count;
  status->MPI_SOURCE = source;
  status->MPI_TAG = tag;
  status->MPI_ERROR = 0;
  return 0;
}

