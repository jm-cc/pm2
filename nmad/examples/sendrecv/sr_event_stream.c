/*
 * NewMadeleine
 * Copyright (C) 2016 (see AUTHORS file)
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

PUK_ALLOCATOR_TYPE(nm_sr_req, nm_sr_request_t);

static nm_sr_req_allocator_t req_allocator = NULL;

#define LEN 8200

static const int len = LEN;
static nm_tag_t tag = 0x01;
static char buf[LEN] = { 42 };
static unsigned long long done = 0;
		   
static void request_notifier_finalized(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  nm_sr_request_t*p_request = p_info->req.p_request;
  nm_sr_req_free(req_allocator, p_request);
  __sync_fetch_and_add(&done, 1);
}

static void request_notifier_unexpected(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  nm_sr_request_t*p_request = nm_sr_req_malloc(req_allocator);
  nm_sr_recv_init(p_session, p_request);
  nm_sr_recv_unpack_contiguous(p_session, p_request, &buf, len);
  nm_sr_recv_match_event(p_session, p_request, p_info);
  nm_sr_request_monitor(p_session, p_request, NM_SR_EVENT_FINALIZED, &request_notifier_finalized);
  nm_sr_recv_post(p_session, p_request);
}

static const struct nm_sr_monitor_s unexpected_monitor =
  {
    .p_notifier = &request_notifier_unexpected,
    .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
    .p_gate     = NM_GATE_NONE,
    .tag        = 1,
    .tag_mask   = NM_TAG_MASK_FULL
  };

int main(int argc, char **argv)
{
  nm_examples_init_topo(&argc, argv, NM_EXAMPLES_TOPO_STAR);
  req_allocator = nm_sr_req_allocator_new(8);
  if(is_server)
    {
      nm_sr_session_monitor_set(p_session, &unexpected_monitor);
    }
  int k;
  for(k = 0; k < 10; k++)
    {    
      if(is_server)
	{
	  /* ** server */
	  fprintf(stderr, "# receiving....\n");
	  nm_examples_barrier(sync_tag);
	  fprintf(stderr, "# done = %llu\n", done);
	}
      else
	{
	  /* ** client */
	  fprintf(stderr, "# sending....\n");
	  int i;
	  for(i = 0; i < 100000; i++)
	    {
	      nm_sr_request_t*p_request = nm_sr_req_malloc(req_allocator);
	      nm_sr_send_init(p_session, p_request);
	      nm_sr_send_pack_contiguous(p_session, p_request, &buf, len);
	      nm_sr_request_monitor(p_session, p_request, NM_SR_EVENT_FINALIZED, &request_notifier_finalized);
	      nm_sr_send_isend(p_session, p_request, p_gate, tag);
	    }
	  fprintf(stderr, "# waiting...\n");
	  nm_examples_barrier(sync_tag);
	  fprintf(stderr, "# done = %llu\n", done);
	}
    }
  if(is_server)
    {
      nm_sr_session_monitor_remove(p_session, &unexpected_monitor);
    }
  nm_examples_exit();
  exit(0);
}
