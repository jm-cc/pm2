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

#include "sr_examples_helper.h"

int main(int argc, char**argv)
{
  const char msg[] = "hello, world";
  const size_t len = 1 + strlen(msg);
  char *buf = malloc(len);
  char *ref = "ref";
      
  nm_examples_init(&argc, argv);
  
  if (is_server)
    {
      nm_sr_request_t request;
      nm_sr_request_t*req2 = NULL;
      
      /* server
       */
      memset(buf, 0, len);
      
      nm_sr_irecv_with_ref(p_session, NM_ANY_GATE, 0, buf, len, &request, ref);
      do
	{
	  nm_sr_recv_success(p_session, &req2);
	}
      while(!req2);
      
      printf("received buffer: %s\n", buf);

      char*ref2;
      nm_sr_request_get_ref(req2, (void**)&ref2);
      printf("ref = %s; ref2 = %s\n", ref, ref2);

      size_t size = 0;
      nm_sr_request_get_size(req2, &size);
      printf("size = %zu\n", size);
      
      nm_tag_t tag = 0;
      nm_sr_request_get_tag(req2, &tag);
      printf("tag = %lu", (unsigned long)tag);
    }
  else
    {
      nm_sr_request_t request;
      nm_sr_request_t*req2 = NULL;
      /* client
       */
      strcpy(buf, msg);
      
      nm_sr_isend_with_ref(p_session, p_gate, 0, buf, len, &request, ref);
      do
	{
	  nm_sr_send_success(p_session, &req2);
	}
      while(!req2);

      char*ref2;
      nm_sr_request_get_ref(req2, (void**)&ref2);
      printf("ref = %s; ref2 = %s\n", ref, ref2);
    }
  
  nm_examples_exit();
  exit(0);
}
