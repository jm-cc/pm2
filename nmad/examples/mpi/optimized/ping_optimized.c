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

#include <tbx.h>

#include "../../sendrecv/helper.h"

#include "ping_optimized.h"
#include "indexed_optimized.h"
#include "vector_optimized.h"
#include "struct_optimized.h"

int
main(int	  argc,
     char	**argv) {
  char		 *datatype = NULL;
  int             size = 0;
  int             number_of_blocks = 0;

  init(&argc, argv);

  if (argc == 1) {
    fprintf(stdout, "usage: argv[0] <datatype>\n");
    exit(EXIT_FAILURE);
  }
  datatype = argv[1];

  /*
    Ping-pong
  */
  if (!is_server) {
    /* client */
    if (!strcmp(datatype, "index")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_indexed(p_core, gate_id, size, number_of_blocks, 1);
        }
      }
    }

    if (!strcmp(datatype, "vector")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_vector(p_core, gate_id, size, number_of_blocks, 1);
        }
      }
    }

    if (!strcmp(datatype, "struct")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        pingpong_datatype_struct(p_core, gate_id, size, 1);
      }
    }
  }
  else {
    /* server */
    if (!strcmp(datatype, "index")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_indexed(p_core, gate_id, size, number_of_blocks, 0);
        }
      }
    }

    if (!strcmp(datatype, "vector")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_vector(p_core, gate_id, size, number_of_blocks, 0);
        }
      }
    }

    if (!strcmp(datatype, "struct")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        pingpong_datatype_struct(p_core, gate_id, size, 0);
      }
    }
  }
  nmad_exit();
  exit(0);
}
