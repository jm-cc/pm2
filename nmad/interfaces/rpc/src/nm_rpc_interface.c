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

#include <stdint.h>

#include <Padico/Puk.h>
#include <nm_private.h>
#include <nm_sendrecv_interface.h>
#include <nm_rpc_interface.h>

/* ********************************************************* */

struct nm_rpc_content_s
{
  void*header_ptr;
  nm_len_t header_len;
  const struct nm_data_s*p_body;
};

static void nm_rpc_traversal(const void*_content, nm_data_apply_t apply, void*_context);
const struct nm_data_ops_s nm_rpc_ops =
  {
    .p_traversal = &nm_rpc_traversal
  };
NM_DATA_TYPE(rpc, struct nm_rpc_content_s, &nm_rpc_ops);

static void nm_rpc_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_rpc_content_s*p_content = _content;
  if(p_content->header_len > 0)
    {
      (*apply)(p_content->header_ptr, p_content->header_len, _context);
    }
  nm_data_traversal_apply(p_content->p_body, apply, _context);
}

void nm_rpc_data_build(struct nm_data_s*p_rpc_data, void*hptr, nm_len_t hlen, const struct nm_data_s*p_body)
{
  nm_data_rpc_set(p_rpc_data, (struct nm_rpc_content_s)
		  {
		    .header_ptr = hptr,
		    .header_len = hlen,
		    .p_body     = p_body
		  });
}


/* ********************************************************* */

void nm_rpc_isend(nm_session_t p_session, nm_sr_request_t*p_request,
		  nm_gate_t p_gate, nm_tag_t tag,
		  void*hptr, nm_len_t hlen, struct nm_data_s*p_body)
{
  struct nm_data_s rpc_data;
  nm_rpc_data_build(&rpc_data, hptr, hlen, p_body);
  nm_sr_send_init(p_session, p_request);
  nm_sr_send_pack_data(p_session, p_request, &rpc_data);
  nm_sr_send_dest(p_session, p_request, p_gate, tag);
  nm_sr_send_header(p_session, p_request, hlen);
  nm_sr_send_submit(p_session, p_request);
}

static void nm_rpc_finalizer(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  struct nm_rpc_token_s*p_token = _ref;
  (*p_token->p_service->p_finalizer)(p_token);
  free(p_token);
}

static void nm_rpc_handler(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  assert(event & NM_SR_EVENT_RECV_UNEXPECTED);
  const nm_gate_t from   = p_info->recv_unexpected.p_gate;
  const nm_tag_t tag     = p_info->recv_unexpected.tag;
  const size_t len       = p_info->recv_unexpected.len;
  nm_session_t p_session = p_info->recv_unexpected.p_session;
  struct nm_rpc_service_s*p_service = _ref;
  struct nm_rpc_token_s*p_token = malloc(sizeof(struct nm_rpc_token_s));
  p_token->p_service = p_service;
  nm_sr_recv_init(p_session, &p_token->request);
  nm_sr_recv_match_event(p_session, &p_token->request, p_info);
  struct nm_data_s header;
  nm_data_contiguous_build(&header, p_service->header_ptr, p_service->hlen);
  int rc = nm_sr_recv_peek(p_session, &p_token->request, &header);
  if(rc != NM_ESUCCESS)
    {
      fprintf(stderr, "# nm_rpc: rc = %d in nm_sr_recv_peek()\n", rc);
      abort();
    }
  nm_sr_request_set_ref(&p_token->request, p_token);
  nm_sr_request_monitor(p_session, &p_token->request, NM_STATUS_FINALIZED, &nm_rpc_finalizer);
  (*p_service->p_handler)(p_token, &p_token->request);
}

nm_rpc_service_t nm_rpc_register(nm_session_t p_session, nm_tag_t tag, nm_tag_t tag_mask, nm_len_t hlen,
				 nm_rpc_handler_t p_handler, nm_rpc_finalizer_t p_finalizer,
				 void*ref)
{
  struct nm_rpc_service_s*p_service = malloc(sizeof(struct nm_rpc_service_s));
  p_service->p_session   = p_session;
  p_service->p_handler   = p_handler;
  p_service->p_finalizer = p_finalizer;
  p_service->ref         = ref;
  p_service->hlen        = hlen;
  p_service->header_ptr  = malloc(hlen);
  p_service->monitor     = (struct nm_sr_monitor_s)
    {
      .p_notifier = &nm_rpc_handler,
      .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
      .p_gate     = NM_GATE_NONE,
      .tag        = tag,
      .tag_mask   = tag_mask,
      .ref        = p_service
    };
  nm_sr_session_monitor_set(p_session, &p_service->monitor);
  return p_service;
}


