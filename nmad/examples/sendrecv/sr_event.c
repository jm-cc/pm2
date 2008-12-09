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

const char *msg	= "hello, world";
char *buf = NULL;

static void request_notifier(nm_sr_request_t*p_request, nm_sr_status_t event, nm_gate_t p_gate)
{
  if(event &  NM_SR_EVENT_RECV_COMPLETED)
    {
      printf("got event NM_SR_EVENT_RECV_COMPLETED.\n");
    }
  printf("buffer contents: %s\n", buf);
}

int main(int argc, char **argv)
{
  uint64_t len = 1 + strlen(msg);
  buf = strdup(msg);
  
  init(&argc, argv);
  
  if(is_server)
    {
      /* ** server */
      nm_sr_request_t request;

      /* per-request event. */
      memset(buf, 0, len);
      nm_sr_irecv(p_core, NM_ANY_GATE, 0, buf, len, &request);
      /* warning: race-condition here- if packet arrives before nm_sr_request_monitor, no event is fired */
      nm_sr_request_monitor(p_core, &request, NM_SR_EVENT_RECV_COMPLETED, &request_notifier);
      nm_sr_rwait(p_core, &request);

      /* global event (all requests) */
      memset(buf, 0, len);
      nm_sr_monitor(p_core,NM_SR_EVENT_RECV_COMPLETED, &request_notifier);
      nm_sr_irecv(p_core, NM_ANY_GATE, 0, buf, len, &request);
      nm_sr_rwait(p_core, &request);

    }
  else
    {
      /* ** client */
      nm_sr_request_t request;
      nm_sr_isend(p_core, gate_id, 0, buf, len, &request);
      nm_sr_swait(p_core, &request);
      nm_sr_isend(p_core, gate_id, 0, buf, len, &request);
      nm_sr_swait(p_core, &request);
    }
  
  free(buf);
  nmad_exit();
  exit(0);
}
