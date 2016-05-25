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

static const char short_msg[] = "hello, world";
static const char small_msg[] = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
#define long_len (1024*128)
static const char long_msg[long_len] = { '.' };
static char *buf = NULL;
#define MAX_UNEXPECTED 10
static int n_unexpected = 0;
static nm_sr_request_t unexpected_requests[MAX_UNEXPECTED];

static void request_notifier(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  if(event &  NM_SR_EVENT_RECV_COMPLETED)
    {
      nm_sr_request_t*p_request = p_info->req.p_request;
      nm_gate_t from = NM_GATE_NONE;
      nm_sr_request_get_gate(p_request, &from);
      size_t size;
      nm_tag_t tag;
      void*ref;
      nm_sr_request_get_tag(p_request, &tag);
      nm_sr_request_get_size(p_request, &size);
      nm_sr_request_get_ref(p_request, &ref);
      printf(":: event NM_SR_EVENT_RECV_COMPLETED- tag = %d; size = %d; ref = %p; from gate = %s\n",
	     (int)tag, (int)size, ref, from?(from == p_gate ? "peer":"unknown"):"(nil)");
      if(size < 1024)
	printf("   buffer contents: %s\n", buf);
    }
  if(event &  NM_SR_EVENT_RECV_UNEXPECTED)
    {
      const nm_gate_t from = p_info->recv_unexpected.p_gate;
      const nm_tag_t tag = p_info->recv_unexpected.tag;
      const size_t len = p_info->recv_unexpected.len;
      printf("# event NM_SR_EVENT_RECV_UNEXPECTED- tag = %d; len = %d; from gate = %s\n",
	     (int)tag, (int)len, from?(from == p_gate ? "peer":"unknown"):"(nil)");
      if(tag == 1)
	{
	  printf(":: event NM_SR_EVENT_RECV_UNEXPECTED- tag = %d; len = %d; from gate = %s\n",
		 (int)tag, (int)len, from?(from == p_gate ? "peer":"unknown"):"(nil)");
	  printf("   receiving tag = %d; len = %d in event handler...\n", (int)tag, (int)len);
	  nm_sr_request_t*p_request = &unexpected_requests[n_unexpected++];
	  nm_sr_recv_init(p_session, p_request);
	  nm_sr_recv_unpack_contiguous(p_session, p_request, buf, len);
	  nm_sr_recv_irecv_event(p_session, p_request, p_info);
	}
    }
  if(event & NM_SR_EVENT_FINALIZED)
    {
      printf(":: event NM_SR_EVENT_FINALIZED.\n");
    }
}

static const struct nm_sr_monitor_s monitor =
  {
    .p_notifier = &request_notifier,
    .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
    .p_gate     = NM_GATE_NONE,
    .tag        = 1,
    .tag_mask   = NM_TAG_MASK_FULL
  };

int main(int argc, char **argv)
{
  const size_t short_len = 1 + strlen(short_msg);
  const size_t small_len = 1 + strlen(small_msg);

  buf = malloc(2 * long_len); /* large enough to receive iovecs */
  
  nm_examples_init(&argc, argv);
  
  if(is_server)
    {
      /* ** server */
      nm_sr_request_t request0, request1;

      /* set global handler for unexpected */
      memset(buf, 0, long_len);
      nm_sr_session_monitor_set(p_session, &monitor);

      fprintf(stderr, "# ## per-request event...\n");
      /* per-request event. */
      memset(buf, 0, long_len);
      nm_sr_recv_init(p_session, &request0);
      nm_sr_recv_unpack_contiguous(p_session, &request0, buf, short_len);
      nm_sr_request_monitor(p_session, &request0, NM_SR_EVENT_RECV_COMPLETED, &request_notifier);
      nm_sr_recv_irecv(p_session, &request0, NM_ANY_GATE, 0, NM_TAG_MASK_FULL);
      nm_sr_rwait(p_session, &request0);
      fprintf(stderr, "# done.\n");

      fprintf(stderr, "# ## force recv.\n");
      /* receive the last packet to force all others to be unexpected */
      nm_sr_irecv(p_session, NM_ANY_GATE, 2, buf, short_len, &request1);
      nm_sr_rwait(p_session, &request1);
      fprintf(stderr, "# done.\n");

      fprintf(stderr, "# ## recv with ref.\n");
      /* test the *_with_ref feature */
      nm_sr_irecv_with_ref(p_session, NM_ANY_GATE, 0, buf, short_len, &request0, (void*)0xDEADBEEF);
      nm_sr_rwait(p_session, &request0);
      fprintf(stderr, "# done.\n");

      fprintf(stderr, "# ## flushing pending requests.\n");
      /* flush pending requests */
      int i;
      for(i = 0; i < n_unexpected; i++)
	{
	  nm_sr_rwait(p_session, &unexpected_requests[i]);
	}
      fprintf(stderr, "# done.\n");
      nm_sr_session_monitor_remove(p_session, &monitor);
    }
  else
    {
      /* ** client */
      nm_sr_request_t request;
      /* test EVENT_RECV_COMPLETED */
      fprintf(stderr, "# send tag 0- short\n");
      nm_sr_isend(p_session, p_gate, 0, short_msg, short_len, &request);
      nm_sr_swait(p_session, &request);
      /* test EVENT_RECV_COMPLETED with ref */
      fprintf(stderr, "# send tag 0- short\n");
      nm_sr_isend(p_session, p_gate, 0, short_msg, short_len, &request);
      nm_sr_swait(p_session, &request);
      /* short unexpected (12 bytes) */
      fprintf(stderr, "# send tag 1- short\n");
      nm_sr_isend(p_session, p_gate, 1, short_msg, short_len, &request);
      nm_sr_swait(p_session, &request);
      /* small unexpected (300 bytes) */
      fprintf(stderr, "# send tag 1- small+small\n");
      nm_sr_isend(p_session, p_gate, 1, small_msg, small_len, &request);
      nm_sr_swait(p_session, &request);
      /* iovec unexpected (12+12 bytes) */
      struct iovec iov1[2] = 
	{ [0] = { .iov_base = (void*)short_msg, .iov_len = short_len }, 
	  [1] = { .iov_base = (void*)short_msg, .iov_len = short_len } };
      fprintf(stderr, "# send tag 1- short+short\n");
      nm_sr_isend_iov(p_session, p_gate, 1, iov1, 2, &request);
      nm_sr_swait(p_session, &request);
      /* large unexpected (128 kB) */
      fprintf(stderr, "# send tag 1- long\n");
      nm_sr_isend(p_session, p_gate, 1, long_msg, long_len, &request);
      nm_sr_swait(p_session, &request);
      /* iovec unexpected (300+128kB) */
      struct iovec iov2[2] = 
	{ [0] = { .iov_base = (void*)small_msg, .iov_len = small_len }, 
	  [1] = { .iov_base = (void*)long_msg,  .iov_len = long_len } };
      fprintf(stderr, "# send tag 2- small+long\n");
      nm_sr_isend_iov(p_session, p_gate, 1, iov2, 2, &request);
      nm_sr_swait(p_session, &request);
      /* sync message */
      fprintf(stderr, "# send tag 2- short\n");
      nm_sr_isend(p_session, p_gate, 2, short_msg, short_len, &request);
      nm_sr_swait(p_session, &request);
    }

  free(buf);
  nm_examples_exit();
  exit(0);
}
