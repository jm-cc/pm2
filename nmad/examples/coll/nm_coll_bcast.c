/*
 * NewMadeleine
 * Copyright (C) 2006-2018 (see AUTHORS file)
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

#include "../common/nm_examples_helper.h"

#define MAXLEN (16*1024*1024)

int main(int argc, char**argv)
{ 
  nm_examples_init(&argc, argv);

  const int root = 0;
  const int self = nm_comm_rank(p_comm);
  const nm_tag_t tag = 0x42;

  nm_len_t len;
  for(len = 1; len < MAXLEN; len *= 2)
    {
      char*msg = malloc(len);
      fill_buffer(msg, len);
      char*buf = malloc(len);
      if(self == root)
        {
          fprintf(stderr, "# len = %ld\n", len);
          memcpy(buf, msg, len);
        }
      else
        {
          memset(buf, 0, len);
        }
      nm_examples_barrier(1);
      struct nm_data_s data;
      nm_data_contiguous_build(&data, (void*)buf, len);
      nm_coll_data_bcast(p_comm, root, &data, tag);
      control_buffer(buf, len);
      free(msg);
      free(buf);
    }
  nm_examples_exit();
  exit(0);
}
