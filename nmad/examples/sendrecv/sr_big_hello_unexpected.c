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

#define SIZE  (128 * 1024)

const char *msg_beg	= "hello", *msg_end = "world!";

int main(int argc, char	**argv)
{
  init(&argc, argv);

  /* build template message */
  char*template_buf = malloc(SIZE+1);
  memset(template_buf, ' ', SIZE);
  char*dst = template_buf;
  char*src = (char *) msg_beg;
  while(*src)
    *dst++ = *src++;
  dst = template_buf + SIZE - strlen(msg_end) - 1;
  src = (char *) msg_end;
  while(*src)
    *dst++ = *src++;
  dst = template_buf + SIZE - 1;
  *dst = '\0';

  if (is_server)
    {
      nm_sr_request_t request0;
      nm_sr_request_t request1;
      
      /* server */
      char*buf0 = malloc(SIZE+1);
      char*buf1 = malloc(SIZE+1);
      memset(buf1, 0, SIZE+1);
      memset(buf0, 0, SIZE+1);
      
      nm_sr_irecv(p_core, gate_id, 1, buf0, SIZE, &request0);
      nm_sr_irecv(p_core, gate_id, 0, buf1, SIZE, &request1);
      nm_sr_rwait(p_core, &request1);
      nm_sr_rwait(p_core, &request0);
      
      if(memcmp(buf0, template_buf, SIZE) != 0)
	{
	  printf("buffer 0: data corrupted.\n");
	  abort();
	}
      if(memcmp(buf1, template_buf, SIZE) != 0)
	{
	  printf("buffer 1: data corrupted.\n");
	  abort();
	}
      printf("received buffers, data ok.\n");
    } 
  else 
    {
      nm_sr_request_t request;
      char*buf = malloc(SIZE+1);
      memcpy(buf, template_buf, SIZE+1);
      
      nm_sr_isend(p_core, gate_id, 0, buf, SIZE, &request);
      nm_sr_swait(p_core, &request);
      
      nm_sr_isend(p_core, gate_id, 1, buf, SIZE, &request);
      nm_sr_swait(p_core, &request);
    }


  return 0;
}
