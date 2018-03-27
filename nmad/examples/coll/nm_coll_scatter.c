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

#define MAXLEN (1024*1024)

int main(int argc, char**argv)
{ 
  nm_examples_init(&argc, argv);

  const int self = nm_comm_rank(p_comm);
  const int size = nm_comm_size(p_comm);
  const int root = size - 1;
  const nm_tag_t tag = 0x42;
  void*sbuf = NULL;
  if(self == root)
    {
      sbuf = malloc(size * MAXLEN);
    }
  nm_len_t len;
  for(len = 1; len < MAXLEN; len *= 2)
    {
      char*msg = malloc(len);
      fill_buffer(msg, len);
      char*buf = malloc(len);
      memset(buf, 0, len);
      if(self == root)
        {
          printf("len = %ld\n", len);
          int i;
          for(i = 0; i < size; i++)
            {
              fill_buffer(sbuf + i * len, len);
            }
        }
      nm_examples_barrier(1);

      nm_coll_scatter(p_comm, root, sbuf, len, buf, len, tag);

      printf("  checking data.\n");
      control_buffer(buf, len);
      printf("ok\n");
      free(msg);
      free(buf);
    }
  if(sbuf)
    free(sbuf);
  nm_examples_exit();
  exit(0);
}
