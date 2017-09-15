/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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

#define MAXMSG 1000

int main(int argc, char**argv)
{ 
  nm_examples_init(&argc, argv);

  int i;
  if(is_server)
    {
      for(i = 0; i < MAXMSG; i++)
	{
	  int value = -1;
	  nm_sr_request_t request;
	  nm_sr_recv_init(p_session, &request);
	  nm_sr_recv_unpack_contiguous(p_session, &request, &value, sizeof(int));
	  nm_sr_recv_irecv(p_session, &request, p_gate, 0, NM_TAG_MASK_NONE); /* any tag */
	  nm_sr_rwait(p_session, &request);
	  printf("# %d: %d\n", i, value);
	}
    }
  else
    {
      int*values = malloc(MAXMSG * sizeof(int));
      nm_sr_request_t*requests = malloc(MAXMSG * sizeof(nm_sr_request_t));
      for(i = 0; i < MAXMSG; i++)
	{
	  const int tag = i;
	  values[i] = i;
	  nm_sr_send_init(p_session, &requests[i]);
	  nm_sr_send_pack_contiguous(p_session, &requests[i], &values[i], sizeof(int));
	  nm_sr_send_set_priority(p_session, &requests[i], i);
	  nm_sr_send_isend(p_session, &requests[i], p_gate, tag);
	}
      for(i = 0; i < MAXMSG; i++)
	{
	  nm_sr_swait(p_session, &requests[i]);
	}
      free(values);
      free(requests);
    }
  
  nm_examples_exit();
  exit(0);
}
