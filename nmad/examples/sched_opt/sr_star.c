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
  int numtasks;
  int tag = 1;

  init(&argc, argv);
  rank = get_rank();
  numtasks = get_size();

  if (rank == 0) {
    int child, gate;
    nm_so_request *out_requests;
    nm_so_request *in_requests;
    float buffer[2], r_buffer[2];

    buffer[0] = 12.45;
    buffer[1] = 3.14;

    out_requests = malloc(numtasks * sizeof(nm_so_request));
    in_requests = malloc(numtasks * sizeof(nm_so_request));

    for(child=1 ; child<numtasks ; child++) {
      gate = get_gate_out_id(child);
      nm_so_sr_isend(sr_if, gate, child, buffer, 2*sizeof(float), &out_requests[child-1]);
      PRINT("Isending to child %d completed", child);
    }

    for(child=1 ; child<numtasks ; child++) {
      nm_so_sr_swait(sr_if, out_requests[child-1]);
      PRINT("Waiting to child %d completed", child);
    }

    for(child=1 ; child<numtasks ; child++) {
      gate = get_gate_in_id(child);
      PRINT("Waiting from child %d", child);
      nm_so_sr_irecv(sr_if, gate, child, r_buffer, 2*sizeof(float), &in_requests[child-1]);
      nm_so_sr_rwait(sr_if, in_requests[child-1]);

      if (r_buffer[0] != buffer[0] && r_buffer[1] != buffer[1]) {
	PRINT("Expected [%f,%f] - Received [%f,%f]", buffer[0], buffer[1], r_buffer[0], r_buffer[1]);
      }
      else {
	PRINT("Message from child %d [%f,%f]", child, r_buffer[0], r_buffer[1]);
      }
    }

    free(out_requests);
    free(in_requests);
  }
  else {
    int father = 0;
    float r_buffer[2];
    int gate;
    nm_so_request out_request;
    nm_so_request in_request;

    gate = get_gate_in_id(0);
    nm_so_sr_irecv(sr_if, gate, rank, r_buffer, 2*sizeof(float), &in_request);
    nm_so_sr_rwait(sr_if, in_request);

    PRINT("Received from father and sending back");

    gate = get_gate_out_id(0);
    nm_so_sr_isend(sr_if, gate, rank, r_buffer, 2*sizeof(float), &out_request);
    nm_so_sr_swait(sr_if, out_request);
    PRINT("Waiting completed");
  }

  nmad_exit();
  exit(0);
}
