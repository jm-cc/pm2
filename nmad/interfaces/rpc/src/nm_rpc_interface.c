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
  const struct nm_data_s*p_header;
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
  nm_data_traversal_apply(p_content->p_header, apply, _context);
  nm_data_traversal_apply(p_content->p_body, apply, _context);
}

void nm_rpc_data_build(struct nm_data_s*p_rpc_data, const struct nm_data_s*p_header, const struct nm_data_s*p_body)
{
  nm_data_rpc_set(p_rpc_data, (struct nm_rpc_content_s){ .p_header = p_header, .p_body = p_body });
}


/* ********************************************************* */

void nm_rpc_send(nm_session_t p_session, nm_gate_t p_gate, nm_tag_t tag,
		 struct nm_data_s*p_header, struct nm_data_s*p_body)
{
  nm_sr_request_t request;
  struct nm_data_s rpc_data;
  nm_data_rpc_set(&rpc_data, (struct nm_rpc_content_s){ .p_header = p_header, .p_body = p_body });
  nm_sr_send_init(p_session, &request);
  nm_sr_send_pack_data(p_session, &request, &rpc_data);
  nm_sr_send_dest(p_session, &request, p_gate, tag);
  nm_sr_send_header(p_session, &request, nm_data_size(p_header));
  nm_sr_send_submit(p_session, &request);
  nm_sr_swait(p_session, &request);
}

static void nm_rpc_finalizer(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  struct nm_rpc_token_s*p_token = _ref;
  (*p_token->p_finalizer)(p_token);
}

static void nm_rpc_handler(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  assert(event & NM_SR_EVENT_RECV_UNEXPECTED);
  const nm_gate_t from   = p_info->recv_unexpected.p_gate;
  const nm_tag_t tag     = p_info->recv_unexpected.tag;
  const size_t len       = p_info->recv_unexpected.len;
  nm_session_t p_session = p_info->recv_unexpected.p_session;
  struct nm_rpc_token_s*p_token = _ref;
  nm_sr_request_t request;
  nm_sr_recv_init(p_session, &request);
  nm_sr_recv_match_event(p_session, &request, p_info);
  int rc = nm_sr_recv_peek(p_session, &request, &p_token->header);
  if(rc != NM_ESUCCESS)
    {
      fprintf(stderr, "# nm_rpc: rc = %d in nm_sr_recv_peek()\n", rc);
      abort();
    }
  nm_sr_request_set_ref(&request, p_token);
  nm_sr_request_monitor(p_session, &request, NM_STATUS_FINALIZED, &nm_rpc_finalizer);
  p_token->p_request = &request;
  (*p_token->p_handler)(p_token, &request);
}

nm_rpc_token_t nm_rpc_register(nm_session_t p_session, nm_tag_t tag, nm_tag_t tag_mask,
			       nm_rpc_handler_t p_handler, nm_rpc_finalizer_t p_finalizer,
			       void*ref, struct nm_data_s*p_header)
{
  struct nm_rpc_token_s*p_token = malloc(sizeof(struct nm_rpc_token_s));
  p_token->p_session   = p_session;
  p_token->p_handler   = p_handler;
  p_token->p_finalizer = p_finalizer;
  p_token->ref         = ref;
  p_token->header      = *p_header;
  p_token->monitor     = (struct nm_sr_monitor_s)
    {
      .p_notifier = &nm_rpc_handler,
      .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
      .p_gate     = NM_GATE_NONE,
      .tag        = tag,
      .tag_mask   = tag_mask,
      .ref        = p_token
    };
  nm_sr_session_monitor_set(p_session, &p_token->monitor);

  return p_token;
}


