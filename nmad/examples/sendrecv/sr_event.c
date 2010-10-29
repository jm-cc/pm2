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

static const char short_msg[] = "hello, world";
static const char small_msg[] = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
#define long_len (1024*128)
static const char long_msg[long_len] = { '.' };
static char *buf = NULL;

static void request_notifier(nm_sr_event_t event, const nm_sr_event_info_t*info)
{
  if(event &  NM_SR_EVENT_RECV_COMPLETED)
    {
      const nm_gate_t from = info->recv_completed.p_gate;
      nm_sr_request_t*p_request = info->recv_completed.p_request;
      size_t size;
      nm_tag_t tag;
      void*ref;
      nm_sr_get_rtag(p_core, p_request, &tag);
      nm_sr_get_size(p_core, p_request, &size);
      nm_sr_get_ref(p_core, p_request, &ref);
      printf(":: event NM_SR_EVENT_RECV_COMPLETED- tag = %d; size = %d; ref = %p; from gate = %s\n",
	     tag, size, ref, from?(from == gate_id ? "peer":"unknown"):"(nil)");
      if(size < 1024)
	printf("   buffer contents: %s\n", buf);
    }
  if(event &  NM_SR_EVENT_RECV_UNEXPECTED)
    {
      const nm_gate_t from = info->recv_unexpected.p_gate;
      const nm_tag_t tag = info->recv_unexpected.tag;
      const size_t len = info->recv_unexpected.len;
      printf(":: event NM_SR_EVENT_RECV_UNEXPECTED- tag = %d; len = %d; from gate = %s\n",
	     tag, len, from?(from == gate_id ? "peer":"unknown"):"(nil)");
      if(tag == 1)
	{
	  printf("   receiving tag = %d in event handler...\n", tag);
	  nm_sr_request_t request;
	  nm_sr_irecv(p_core, from, tag, buf, len, &request);
	  nm_sr_rwait(p_core, &request);
	  printf("   unexpected recv done %d bytes.\n", len);
	}
    }
  printf("\n");
}

int main(int argc, char **argv)
{
  const size_t short_len = 1 + strlen(short_msg);
  const size_t small_len = 1 + strlen(small_msg);

  buf = malloc(2 * long_len); /* large enough to receive iovecs */
  
  init(&argc, argv);
  
  if(is_server)
    {
      /* ** server */
      nm_sr_request_t request0, request1;

      /* per-request event. */
      memset(buf, 0, long_len);
      nm_sr_recv_init(p_core, &request0);
      nm_sr_recv_unpack_data(p_core, &request0, buf, short_len);
      nm_sr_request_monitor(p_core, &request0, NM_SR_EVENT_RECV_COMPLETED, &request_notifier);
      nm_sr_recv_irecv(p_core, &request0, NM_ANY_GATE, 0, NM_TAG_MASK_FULL);
      nm_sr_rwait(p_core, &request0);

      /* set global event handlers */
      memset(buf, 0, long_len);
      nm_sr_monitor(p_core, NM_SR_EVENT_RECV_UNEXPECTED, &request_notifier);
      nm_sr_monitor(p_core, NM_SR_EVENT_RECV_COMPLETED, &request_notifier);

      /* receive the last packet to force all others to be unexpected */
      nm_sr_irecv(p_core, NM_ANY_GATE, 2, buf, short_len, &request1);
      nm_sr_rwait(p_core, &request1);

      /* test the *_with_ref feature */
      nm_sr_irecv_with_ref(p_core, NM_ANY_GATE, 0, buf, short_len, &request0, (void*)0xDEADBEEF);
      nm_sr_rwait(p_core, &request0);

    }
  else
    {
      /* ** client */
      nm_sr_request_t request;
      /* test EVENT_RECV_COMPLETED */
      nm_sr_isend(p_core, gate_id, 0, short_msg, short_len, &request);
      nm_sr_swait(p_core, &request);
      /* test EVENT_RECV_COMPLETED with ref */
      nm_sr_isend(p_core, gate_id, 0, short_msg, short_len, &request);
      nm_sr_swait(p_core, &request);
      /* short unexpected (12 bytes) */
      nm_sr_isend(p_core, gate_id, 1, short_msg, short_len, &request);
      nm_sr_swait(p_core, &request);
      /* small unexpected (300 bytes) */
      nm_sr_isend(p_core, gate_id, 1, small_msg, small_len, &request);
      nm_sr_swait(p_core, &request);
      /* iovec unexpected (12+12 bytes) */
      struct iovec iov1[2] = 
	{ [0] = { .iov_base = (void*)short_msg, .iov_len = short_len }, 
	  [1] = { .iov_base = (void*)short_msg, .iov_len = short_len } };
      nm_sr_isend_iov(p_core, gate_id, 1, iov1, 2, &request);
      nm_sr_swait(p_core, &request);
      /* large unexpected (128 kB) */
      nm_sr_isend(p_core, gate_id, 1, long_msg, long_len, &request);
      nm_sr_swait(p_core, &request);
      /* iovec unexpected (300+128kB) */
      struct iovec iov2[2] = 
	{ [0] = { .iov_base = (void*)small_msg, .iov_len = small_len }, 
	  [1] = { .iov_base = (void*)long_msg,  .iov_len = long_len } };
      nm_sr_isend_iov(p_core, gate_id, 1, iov2, 2, &request);
      nm_sr_swait(p_core, &request);
      /* sync message */
      nm_sr_isend(p_core, gate_id, 2, short_msg, short_len, &request);
      nm_sr_swait(p_core, &request);
    }
  
  free(buf);
  nmad_exit();
  exit(0);
}
