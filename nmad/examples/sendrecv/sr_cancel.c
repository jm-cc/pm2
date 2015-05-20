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

int main(int argc, char	**argv)
{
  size_t len = 1 + strlen(msg);
  char *buf = malloc(len);;
  
  nm_examples_init(&argc, argv);
  
  if (is_server)
    {
      nm_sr_request_t request, request2;
      /* server
       */
      memset(buf, 0, len);
      
      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
      nm_sr_rwait(p_session, &request);
      printf("buffer contents: %s\n", buf);

      int err = nm_sr_rcancel(p_session, &request);
      printf("nm_sr_cancel rc = %3d (expected: -NM_EALREADY = %d)\n", err, -NM_EALREADY);

      nm_sr_irecv(p_session, NM_GATE_NONE, 0, buf, len, &request);
      err = nm_sr_rcancel(p_session, &request);
      printf("nm_sr_cancel rc = %3d (expected:  NM_ESUCCESS = %d)\n", err, NM_ESUCCESS);

      err = nm_sr_rtest(p_session, &request);
      printf("nm_sr_test   rc = %3d (expected: -NM_ECANCELED = %d)\n", err, -NM_ECANCELED);

      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request2);
      err = nm_sr_rcancel(p_session, &request);
      printf("nm_sr_cancel rc = %3d (expected:  NM_ESUCCESS = %d)\n", err, NM_ESUCCESS);

      err = nm_sr_rcancel(p_session, &request2);
      printf("nm_sr_cancel rc = %3d (expected:  NM_ESUCCESS = %d)\n", err, NM_ESUCCESS);

      err = nm_sr_rcancel(p_session, &request);
      printf("nm_sr_cancel rc = %3d (expected: -NM_ECANCELED = %d)\n", err, -NM_ECANCELED);

      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
      nm_sr_swait(p_session, &request);

      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
      nm_sr_rwait(p_session, &request);
    }
  else
    {
      nm_sr_request_t request;
      /* client
       */
      strcpy(buf, msg);
      
      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
      nm_sr_swait(p_session, &request);

      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
      nm_sr_rwait(p_session, &request);

      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
      nm_sr_swait(p_session, &request);
    }
 
  free(buf);
  nm_examples_exit();
  return 0;
}
