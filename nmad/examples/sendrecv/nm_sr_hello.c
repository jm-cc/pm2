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

const char *msg	= "hello, world";

int main(int argc, char**argv)
{ 
  nm_examples_init(&argc, argv);

  uint64_t len = 1 + strlen(msg);
  char*buf = malloc((size_t)len);
  
  if(is_server)
    {
      /* server side */
      nm_sr_request_t request;
      memset(buf, 0, len);
      
      nm_sr_irecv(p_session, NM_ANY_GATE, 0, buf, len, &request);
      nm_sr_rwait(p_session, &request);

      printf("buffer contents: %s\n", buf);
    }
  else
    {
      /* client side */
      nm_sr_request_t request;
      strcpy(buf, msg);
      
      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
      nm_sr_swait(p_session, &request);
    }
 
  free(buf);
  nm_examples_exit();
  exit(0);
}
