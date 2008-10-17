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

int main(int argc, char**argv)
{
  const char msg[] = "hello, world";
  const size_t len = 1 + strlen(msg);
  char *buf = malloc(len);
  
  init(&argc, argv);
  
  if (is_server)
    {
      nm_sr_request_t request;
      char *ref	= "ref";
      nm_sr_request_t*req2 = NULL;
      
      /* server
       */
      memset(buf, 0, len);
      
      nm_sr_irecv_with_ref(p_core, NM_ANY_GATE, 0, buf, len, &request, ref);
      do
	{
	  nm_sr_recv_success(p_core, &req2);
	}
      while(!req2);
      
      char*ref2 = NULL;
      size_t size = 0;
      nm_sr_get_ref(p_core, req2, &ref2);
      nm_sr_get_size(p_core, req2, &size);
      
      printf("ref2 = %s\n", ref2);
      printf("size = %zu\n", size);
      printf("buffer contents: %s\n", buf);
      
    }
  else
    {
      nm_sr_request_t request;
      /* client
       */
      strcpy(buf, msg);
      
      nm_sr_isend(p_core, gate_id, 0, buf, len, &request);
      nm_sr_swait(p_core, &request);
    }
  
  nmad_exit();
  exit(0);
}
