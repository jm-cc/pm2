/*
 * NewMadeleine
 * Copyright (C) 2017 (see AUTHORS file)
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

static const char short_msg[] = "hello, world";
static const char small_msg[] = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
#define LONG_LEN (1024*128)
static const char long_msg[LONG_LEN] = { '.' };

static char *buf = NULL;
static nm_len_t recv_len = NM_LEN_UNDEFINED;
static struct iovec recv_iov[3];

static void request_notifier(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  if(event &  NM_SR_EVENT_FINALIZED)
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
      printf(":: event NM_SR_EVENT_FINALIZEDD- tag = %d; size = %d; ref = %p; from gate = %s\n",
	     (int)tag, (int)size, ref, from?(from == p_gate ? "peer":"unknown"):"(nil)");
      if(size > sizeof(nm_len_t) && size < 1024)
	printf("   buffer contents: %s\n", buf);
      if(size > sizeof(nm_len_t))
	{
	  /* re-arm */
	  nm_sr_recv_init(p_session, p_request);
	  nm_sr_request_monitor(p_session, p_request, NM_SR_EVENT_FINALIZED | NM_SR_EVENT_RECV_DATA, &request_notifier);
	  nm_sr_recv_irecv(p_session, p_request, NM_ANY_GATE, 0, NM_TAG_MASK_FULL);
	}
      else
	{
	  printf(":: received EOS; don't rearm req.\n");
	}
    }
  if(event &  NM_SR_EVENT_RECV_DATA)
    {
      nm_sr_request_t*p_request = p_info->req.p_request;
      nm_gate_t from = NM_GATE_NONE;
      nm_tag_t tag = -1;
      nm_sr_request_get_gate(p_request, &from);
      nm_sr_request_get_tag(p_request, &tag);
      recv_len = NM_LEN_UNDEFINED;
      struct nm_data_s header;
      nm_data_contiguous_build(&header, &recv_len, sizeof(nm_len_t));
      int rc = nm_sr_recv_peek(p_session, p_request, &header);
      assert(rc == NM_ESUCCESS);
      printf(":: event NM_SR_EVENT_RECV_DATA- tag = %d; len = %d; from gate = %s\n",
	     (int)tag, (int)recv_len, from?(from == p_gate ? "peer":"unknown"):"(nil)");
      recv_iov[0].iov_base = &recv_len;
      recv_iov[0].iov_len  = sizeof(nm_len_t);
      recv_iov[1].iov_base = buf;
      recv_iov[1].iov_len  = recv_len;
      recv_iov[2].iov_base = NULL;
      nm_sr_recv_unpack_iov(p_session, p_request, recv_iov, 2);
    }
}


int main(int argc, char **argv)
{
  const size_t short_len = 1 + strlen(short_msg);
  const size_t small_len = 1 + strlen(small_msg);

  buf = malloc(2 * LONG_LEN + sizeof(nm_len_t)); /* large enough to receive iovecs */
  
  nm_examples_init(&argc, argv);
  
  if(is_server)
    {
      /* ** server */
      nm_sr_request_t request0, request1;

      memset(buf, 0, LONG_LEN);

      fprintf(stderr, "# server: post recv tag = 0\n");
      nm_sr_recv_init(p_session, &request0);
      nm_sr_request_monitor(p_session, &request0, NM_SR_EVENT_FINALIZED | NM_SR_EVENT_RECV_DATA, &request_notifier);
      nm_sr_recv_irecv(p_session, &request0, NM_ANY_GATE, 0, NM_TAG_MASK_FULL);
      fprintf(stderr, "# server: recv posted.\n");

      fprintf(stderr, "# server: force recv- wait on tag = 2.\n");
      /* receive the last packet to force all others to be unexpected */
      nm_sr_irecv(p_session, NM_ANY_GATE, 2, buf, short_len, &request1);
      nm_sr_rwait(p_session, &request1);
      fprintf(stderr, "# server: done.\n");
    }
  else
    {
      const nm_tag_t tag = 0;
      nm_len_t msg_len = NM_LEN_UNDEFINED;
      /* ** client */
      nm_sr_request_t request;
      /* header + short */
      msg_len = short_len;
      fprintf(stderr, "# send tag 0- short = %d\n", msg_len);
      struct iovec iov[2] = 
	{ [0] = { .iov_base = (void*)&msg_len,   .iov_len = sizeof(nm_len_t) }, 
	  [1] = { .iov_base = (void*)short_msg,  .iov_len = short_len } };
      nm_sr_send_init(p_session, &request);
      nm_sr_send_pack_iov(p_session, &request, iov, 2);
      nm_sr_send_header(p_session, &request, sizeof(nm_len_t));
      nm_sr_send_isend(p_session, &request, p_gate, tag);
      nm_sr_swait(p_session, &request);

      /* header + small */
      fprintf(stderr, "# send tag 0- small = %d\n", small_len);
      msg_len = small_len;
      struct iovec iov2[2] = 
	{ [0] = { .iov_base = (void*)&msg_len,   .iov_len = sizeof(nm_len_t) }, 
	  [1] = { .iov_base = (void*)small_msg,  .iov_len = small_len } };
      nm_sr_send_init(p_session, &request);
      nm_sr_send_pack_iov(p_session, &request, iov2, 2);
      nm_sr_send_header(p_session, &request, sizeof(nm_len_t));
      nm_sr_send_isend(p_session, &request, p_gate, tag);
      nm_sr_swait(p_session, &request);

      /* header + large */
      fprintf(stderr, "# send tag 0- large = %d\n", LONG_LEN);
      msg_len = LONG_LEN;
      struct iovec iov3[2] = 
	{ [0] = { .iov_base = (void*)&msg_len,  .iov_len = sizeof(nm_len_t) }, 
	  [1] = { .iov_base = (void*)long_msg,  .iov_len = LONG_LEN } };
      nm_sr_send_init(p_session, &request);
      nm_sr_send_pack_iov(p_session, &request, iov3, 2);
      nm_sr_send_header(p_session, &request, sizeof(nm_len_t));
      nm_sr_send_isend(p_session, &request, p_gate, tag);
      nm_sr_swait(p_session, &request);

      /* stop signal */
      msg_len = 0;
      nm_sr_send(p_session, p_gate, 0, &msg_len, sizeof(nm_len_t));
      
      /* sync message */
      fprintf(stderr, "# send tag 2- short\n");
      nm_sr_isend(p_session, p_gate, 2, short_msg, short_len, &request);
      nm_sr_swait(p_session, &request);
    }

  free(buf);
  nm_examples_exit();
  exit(0);
}
