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

const char *msg	= "hello, world";

int main(int argc, char**argv)
{
  int len = 1 + strlen(msg);
  char*buf = malloc((size_t)len);

  nm_examples_init(&argc, argv);
  
  if (is_server)
    {
      nm_sr_request_t request;
      nm_sr_request_t request0;
      /* server
       */
      memset(buf, 0, len);
      
      nm_sr_irecv(p_session, p_gate, 1, buf, len, &request0);
      
      int i = 0;
      while(i++ < 10000)
	nm_sr_rtest(p_session, &request0);
      
      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
      nm_sr_rwait(p_session, &request);
      
      nm_sr_rwait(p_session, &request0);
      
    }
  else
    {
      nm_sr_request_t request;
      /* client
       */
      strcpy(buf, msg);
      
      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
      nm_sr_swait(p_session, &request);
      
      usleep(100*1000);
      nm_sr_isend(p_session, p_gate, 1, buf, len, &request);
      nm_sr_swait(p_session, &request);
    }
  
  if (is_server) 
    {
      printf("buffer contents: %s\n", buf);
    }
  
  free(buf);
  nm_examples_exit();
  exit(0);
}
