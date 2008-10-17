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

#define MAX_PENDING 2048

int main(int argc, char	**argv)
{
  const int len = MAX_PENDING + 1;
  char buf[MAX_PENDING + 1];
  nm_sr_request_t requests[MAX_PENDING];
  int i;

  init(&argc, argv);
  
  if (is_server) 
    {
      /* server */
      memset(buf, 0, len);
      for(i = 0; i < MAX_PENDING; i++)
	{
	  nm_sr_irecv(p_core, NM_ANY_GATE, 0, &buf[i], 1, &requests[i]);
	}
      for(i = 0; i < MAX_PENDING; i++)
	{
	  nm_sr_rwait(p_core, &requests[i]);
	}
      printf("buffer contents: %s\n", buf);
    }
  else
    {
      /* client */
      for(i = 0; i < MAX_PENDING; i++)
	{
	  buf[i] = 'A' + i % ('Z' - 'A');
	}
      buf[MAX_PENDING - 1] = 0;
      for(i = 0; i < MAX_PENDING; i++)
	{
	  nm_sr_isend(p_core, gate_id, 0, &buf[i], 1, &requests[i]);
	}
      for(i = 0; i < MAX_PENDING; i++)
	{
	  nm_sr_swait(p_core, &requests[i]);
	}
    }
  
  nmad_exit();
  return 0;
}
