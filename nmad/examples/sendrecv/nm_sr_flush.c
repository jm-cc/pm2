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

#include "../common/nm_examples_helper.h"

/* test up to 2M pending isend/irecv at the same time */

#define MAX_PENDING (2*1024*1024)

int main(int argc, char	**argv)
{
  const int len = MAX_PENDING + 1;
  char*buf = malloc(MAX_PENDING + 1);
  nm_sr_request_t*requests = malloc(MAX_PENDING * sizeof(nm_sr_request_t));
  int i;
  int max_pending;

  nm_examples_init(&argc, argv);
  
  for(max_pending = 4; max_pending <= MAX_PENDING; max_pending *= 2)
    {
      if (is_server) 
	{
	  /* server */
	  printf("starting- max_pending = %d\n", max_pending);
	  memset(buf, 0, len);
	  for(i = 0; i < max_pending; i++)
	    {
	      nm_sr_irecv(p_session, NM_ANY_GATE, 0, &buf[i], 1, &requests[i]);
	    }
	  for(i = 0; i < max_pending; i++)
	    {
	      nm_sr_rwait(p_session, &requests[i]);
	    }
	  for(i = 0; i < max_pending - 1; i++)
	    {
	      if(buf[i] != (char)('A' + i % ('Z' - 'A')))
		{
		  printf("data corruption detected i = %d.\n", i);
		  abort();
		}
	    }
	  printf("  ok.\n");
	}
      else
	{
	  /* client */
	  fprintf(stderr, "# sending: max_pending = %d\n", max_pending);
	  for(i = 0; i < max_pending; i++)
	    {
	      buf[i] = 'A' + i % ('Z' - 'A');
	    }
	  buf[max_pending - 1] = 0;
	  for(i = 0; i < max_pending; i++)
	    {
	      nm_sr_isend(p_session, p_gate, 0, &buf[i], 1, &requests[i]);
	    }
	  for(i = 0; i < max_pending; i++)
	    {
	      nm_sr_swait(p_session, &requests[i]);
	    }
	}
      nm_examples_barrier(max_pending);
    }
  free(requests);
  free(buf);
  nm_examples_exit();
  return 0;
}
