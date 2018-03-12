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

const char *msg	= "hello, world";

int main(int argc, char**argv)
{ 
  nm_examples_init(&argc, argv);

  nm_len_t len = 1 + strlen(msg);
  char*buf = malloc((size_t)len);
  nm_examples_barrier(1);
  nm_group_t p_group = nm_comm_group(p_comm);
  const int root = 0;
  const int self = nm_group_rank(p_group);
  const nm_tag_t tag = 0x42;

  if(self == root)
    {
      strcpy(buf, msg);
    }
  else
    {
      memset(buf, 0, len);
    }
  struct nm_data_s data;
  nm_data_contiguous_build(&data, (void*)buf, len);
  struct nm_coll_bcast_s*p_bcast = nm_coll_group_data_ibcast(p_session, p_group, root, self, &data, tag);
  nm_coll_ibcast_wait(p_bcast);
  
  printf("buffer contents: %s\n", buf);

  free(buf);
  nm_examples_exit();
  exit(0);
}
