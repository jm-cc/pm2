/*
 * NewMadeleine
 * Copyright (C) 2016-2017 (see AUTHORS file)
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
#include <assert.h>

#include "../common/nm_examples_helper.h"

const char msg[] = "Hello, world!";

int main(int argc, char**argv)
{
  nm_len_t len = sizeof(int) + strlen(msg) + 1;
  void*buf = malloc(len);
  nm_sr_request_t request;
  
  nm_examples_init(&argc, argv);
  
  if(is_server)
    {
      memset(buf, 0, len);
      nm_sr_recv_init(p_session, &request);
      nm_sr_recv_match(p_session, &request, NM_ANY_GATE, 0, NM_TAG_MASK_FULL );
      struct nm_data_s data;
      nm_data_contiguous_build(&data, buf, sizeof(int));
      while(nm_sr_recv_iprobe(p_session, &request) == -NM_EAGAIN)
	{
	  nm_sr_progress(p_session);
	}
      int rc = nm_sr_recv_peek(p_session, &request, &data);
      assert(rc == NM_ESUCCESS);
      int*p_len = buf;
      printf("SUCCESS- rc = %d; header len = %d\n", rc, *p_len);
      nm_sr_recv_unpack_contiguous(p_session, &request, buf, len);
      nm_sr_recv_post(p_session, &request);
      nm_sr_rwait(p_session, &request);
      printf("buffer contents: %s\n", (char*)(buf + sizeof(int)));
    }
  else
    {
      int*p_len = buf;
      strcpy(buf + sizeof(int), msg);
      *p_len = len;
      sleep(1);
      fprintf(stderr, "# sender- header len = %d\n", *p_len);
      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
      nm_sr_swait(p_session, &request);
    }
  free(buf);
  nm_examples_exit();
  return 0;
}
