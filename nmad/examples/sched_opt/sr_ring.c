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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"

int rank;

#define PRINT(str, ...)		fprintf(stderr, "[%d] " str "\n", rank, ## __VA_ARGS__)

int main(int argc, char **argv) {
  int numtasks, dest, source;
  nm_gate_id_t gate;
  int tag = 1;
  float buffer[2], r_buffer[2];
  nm_so_request out_request;
  nm_so_request in_request;
  struct nm_so_interface *sr_if            = NULL;

  nm_so_init(&argc, argv);
  nm_so_get_sr_if(&sr_if);
  nm_so_get_rank(&rank);
  nm_so_get_size(&numtasks);

  if (rank == 0) {
    dest = 1;
    source = numtasks-1;
    buffer[0] = 12.45;
    buffer[1] = 3.14;

    nm_so_get_gate_out_id(dest, &gate);
    PRINT("sending to node %d, gate %d", dest, gate);
    nm_so_sr_isend(sr_if, gate, tag, buffer, 2*sizeof(float), &out_request);
    nm_so_sr_swait(sr_if, out_request);

    nm_so_get_gate_in_id(source, &gate);
    PRINT("receiving from node %d, gate %d", source, gate);
    nm_so_sr_irecv(sr_if, gate, tag, r_buffer, 2*sizeof(float), &in_request);
    nm_so_sr_rwait(sr_if, in_request);

    PRINT("Message [%f,%f]\n", r_buffer[0], r_buffer[1]);
  }
  else {
    source = rank - 1;
    if (rank == numtasks-1) {
      dest = 0;
    }
    else {
      dest = rank + 1;
    }
    
    nm_so_get_gate_in_id(source, &gate);
    PRINT("receiving from node %d, gate %d", source, gate);
    nm_so_sr_irecv(sr_if, gate, tag, buffer, 2*sizeof(float), &in_request);
    nm_so_sr_rwait(sr_if, in_request);

    nm_so_get_gate_out_id(dest, &gate);
    PRINT("sending to node %d, gate %d", dest, gate);
    nm_so_sr_isend(sr_if, gate, tag, buffer, 2*sizeof(float), &out_request);
    nm_so_sr_swait(sr_if, out_request);
  }

  nmad_exit();
  exit(0);
}
