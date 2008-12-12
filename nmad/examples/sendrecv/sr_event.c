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

static const char *msg	= "hello, world";
static char *buf = NULL;
static size_t len = 0;

static void request_notifier(nm_sr_event_t event, nm_sr_event_info_t*info)
{
  if(event &  NM_SR_EVENT_RECV_COMPLETED)
    {
      nm_gate_t from = info->recv_completed.p_gate;
      nm_sr_request_t*p_request = info->recv_completed.p_request;
      size_t size;
      nm_tag_t tag;
      void*ref;
      nm_sr_get_tag(p_core, p_request, &tag);
      nm_sr_get_size(p_core, p_request, &size);
      nm_sr_get_ref(p_core, p_request, &ref);
      printf("got event NM_SR_EVENT_RECV_COMPLETED- tag = %d; size = %d; ref = %p; from gate = %p\n", tag, size, ref, from);
      printf("buffer contents: %s\n", buf);
    }
  if(event &  NM_SR_EVENT_RECV_UNEXPECTED)
    {
      nm_gate_t from = info->recv_unexpected.p_gate;
      nm_tag_t tag = info->recv_unexpected.tag;
      printf("got event NM_SR_EVENT_RECV_UNEXPECTED- tag = %d; from gate = %p\n", tag, from);
      if(tag == 1)
	{
	  printf("  receiving from gate = %p; tag = %d in event handler...\n", from, tag);
	  nm_sr_request_t request;
	  nm_sr_irecv(p_core, from, tag, buf, len, &request);
	  nm_sr_rwait(p_core, &request);
	  printf("  done.\n");
	}
    }
}

int main(int argc, char **argv)
{
  len = 1 + strlen(msg);
  buf = strdup(msg);
  
  init(&argc, argv);
  
  if(is_server)
    {
      /* ** server */
      nm_sr_request_t request0, request1;

      /* per-request event. */
      memset(buf, 0, len);
      nm_sr_irecv(p_core, NM_ANY_GATE, 0, buf, len, &request0);
      /* warning: race-condition here- if packet arrives before nm_sr_request_monitor, no event is fired */
      nm_sr_request_monitor(p_core, &request0, NM_SR_EVENT_RECV_COMPLETED, &request_notifier);
      nm_sr_rwait(p_core, &request0);

      /* global event (all requests) */
      memset(buf, 0, len);
      nm_sr_monitor(p_core, NM_SR_EVENT_RECV_UNEXPECTED, &request_notifier);
      nm_sr_monitor(p_core, NM_SR_EVENT_RECV_COMPLETED, &request_notifier);

      nm_sr_irecv(p_core, NM_ANY_GATE, 2, buf, len, &request1);
      nm_sr_rwait(p_core, &request1);

      nm_sr_irecv_with_ref(p_core, NM_ANY_GATE, 0, buf, len, &request0, (void*)0xDEADBEEF);
      nm_sr_rwait(p_core, &request0);

    }
  else
    {
      /* ** client */
      nm_sr_request_t request;
      nm_sr_isend(p_core, gate_id, 0, buf, len, &request);
      nm_sr_swait(p_core, &request);
      nm_sr_isend(p_core, gate_id, 0, buf, len, &request);
      nm_sr_swait(p_core, &request);
      nm_sr_isend(p_core, gate_id, 1, buf, len, &request);
      nm_sr_swait(p_core, &request);
      nm_sr_isend(p_core, gate_id, 2, buf, len, &request);
      nm_sr_swait(p_core, &request);
    }
  
  free(buf);
  nmad_exit();
  exit(0);
}
