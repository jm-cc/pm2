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
#include <nm_launcher.h>

//#define STAR_DEBUG
#define PRINT(str, ...)		fprintf(stderr, "[%d] " str "\n", rank, ## __VA_ARGS__)

int main(int argc, char **argv) {
  int numtasks;
  int rank;

  init(&argc, argv);
  nm_launcher_get_rank(&rank);
  nm_launcher_get_size(&numtasks);

  if (rank == 0) {
    int child;
    nm_gate_t gate;
    nm_sr_request_t *out_requests;
    nm_sr_request_t *in_requests;
    float buffer[2], r_buffer[2];

    buffer[0] = 12.45;
    buffer[1] = 3.14;

    out_requests = malloc(numtasks * sizeof(nm_sr_request_t));
    in_requests = malloc(numtasks * sizeof(nm_sr_request_t));

    for(child=1 ; child<numtasks ; child++) {
      nm_launcher_get_gate(child, &gate);
      nm_sr_isend(p_core, gate, child, buffer, 2*sizeof(float), &out_requests[child-1]);
#ifdef STAR_DEBUG
      PRINT("Isending to child %d completed", child);
#endif
    }

    for(child=1 ; child<numtasks ; child++) {
      nm_sr_swait(p_core, &out_requests[child-1]);
#ifdef STAR_DEBUG
      PRINT("Waiting to child %d completed", child);
#endif
    }

    for(child=1 ; child<numtasks ; child++) {
      nm_launcher_get_gate(child, &gate);
#ifdef STAR_DEBUG
      PRINT("Waiting from child %d", child);
#endif
      nm_sr_irecv(p_core, gate, child, r_buffer, 2*sizeof(float), &in_requests[child-1]);
      nm_sr_rwait(p_core, &in_requests[child-1]);

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
    float r_buffer[2];
    nm_gate_t gate;
    nm_sr_request_t out_request;
    nm_sr_request_t in_request;

    nm_launcher_get_gate(0, &gate);
    nm_sr_irecv(p_core, gate, rank, r_buffer, 2*sizeof(float), &in_request);
    nm_sr_rwait(p_core, &in_request);

#ifdef STAR_DEBUG
    PRINT("Received from father and sending back");
#endif

    nm_launcher_get_gate(0, &gate);
    nm_sr_isend(p_core, gate, rank, r_buffer, 2*sizeof(float), &out_request);
    nm_sr_swait(p_core, &out_request);
#ifdef STAR_DEBUG
    PRINT("Waiting completed");
#endif
  }

  nmad_exit();
  exit(0);
}
