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
#include <nm_core_interface.h>

/* ********************************************************* */

PUK_ALLOCATOR_TYPE(nm_rpc_req, struct nm_rpc_req_s);

static nm_rpc_req_allocator_t nm_rpc_req_allocator = NULL;

static inline nm_rpc_req_t nm_rpc_req_new(void)
{
  nm_rpc_req_t p_rpc_req = nm_rpc_req_malloc(nm_rpc_req_allocator);
  return p_rpc_req;
}

void nm_rpc_req_delete(nm_rpc_req_t p_rpc_req)
{
  nm_rpc_req_free(nm_rpc_req_allocator, p_rpc_req);
}

static void nm_rpc_handler(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref);


/* ********************************************************* */

/** data content for an RPC request (header + body) */
struct nm_rpc_content_s
{
  void*header_ptr;               /**< pointer to a header buffer */
  nm_len_t header_len;           /**< length of header, as defined when RPC service is registered */
  const struct nm_data_s*p_body; /**< user-supplied body */
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

nm_rpc_req_t nm_rpc_isend(nm_session_t p_session,  nm_gate_t p_gate, nm_tag_t tag,
			  void*hptr, nm_len_t hlen, struct nm_data_s*p_body)
{
  nm_rpc_req_t p_req = nm_rpc_req_new();
  p_req->body = *p_body;
  struct nm_data_s rpc_data; /* safe as temp var; will be copied by nm_sr_send_pack_data() */
  nm_rpc_data_build(&rpc_data, hptr, hlen, &p_req->body);
  nm_sr_send_init(p_session, &p_req->request);
  nm_sr_send_pack_data(p_session, &p_req->request, &rpc_data);
  nm_sr_send_dest(p_session, &p_req->request, p_gate, tag);
  nm_sr_send_header(p_session, &p_req->request, hlen);
  nm_sr_send_submit(p_session, &p_req->request);
  return p_req;
}

static void nm_rpc_req_notifier(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  nm_rpc_req_t p_req = _ref;
  (*p_req->p_notifier)(p_req, p_req->p_notifier_ref);
  nm_rpc_req_delete(p_req);
}

void nm_rpc_req_set_notifier(nm_rpc_req_t p_req, nm_rpc_req_notifier_t p_notifier, void*ref)
{
  p_req->p_notifier = p_notifier;
  p_req->p_notifier_ref = ref;
  nm_sr_request_set_ref(&p_req->request, p_req);
  nm_sr_request_monitor(p_req->request.p_session, &p_req->request, NM_SR_EVENT_FINALIZED, &nm_rpc_req_notifier);
}

static inline void nm_rpc_req_reload(nm_rpc_service_t p_service)
{
  p_service->token.p_service = p_service;
  nm_sr_recv_init(p_service->p_session, &p_service->token.request);
  nm_sr_request_set_ref(&p_service->token.request, &p_service->token);
  nm_sr_request_monitor(p_service->p_session, &p_service->token.request,
                        NM_SR_EVENT_FINALIZED | NM_SR_EVENT_RECV_DATA | NM_SR_EVENT_RECV_CANCELLED, &nm_rpc_handler);
  nm_sr_recv_irecv(p_service->p_session, &p_service->token.request, NM_ANY_GATE, p_service->tag, p_service->tag_mask);
}

static void nm_rpc_handler(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  struct nm_rpc_token_s*p_token = _ref;
  nm_rpc_service_t p_service = p_token->p_service;
  assert(! ((event & NM_SR_EVENT_FINALIZED) &&
            (event & NM_SR_EVENT_RECV_DATA)));
  if(!(event & NM_SR_EVENT_RECV_CANCELLED))
    {
      if(event & NM_SR_EVENT_FINALIZED)
        {
          (*p_service->p_finalizer)(p_token);
          nm_rpc_req_reload(p_service);
        }
      if(event & NM_SR_EVENT_RECV_DATA)
        {
          p_token->ref = NULL;
          nm_data_null_build(&p_token->body);
          int rc = nm_sr_recv_peek(p_service->p_session, &p_token->request, &p_service->header);
          if(rc != NM_ESUCCESS)
            {
              NM_FATAL("# nm_rpc: rc = %d in nm_sr_recv_peek()\n", rc);
            }
          (*p_service->p_handler)(p_token);
        }
    }
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
  p_service->tag         = tag;
  p_service->tag_mask    = tag_mask;
  nm_data_contiguous_build(&p_service->header, p_service->header_ptr, p_service->hlen);
  if(nm_rpc_req_allocator == NULL)
    {
      nm_rpc_req_allocator = nm_rpc_req_allocator_new(8);
    }
  nm_rpc_req_reload(p_service);
  return p_service;
}

void nm_rpc_unregister(nm_rpc_service_t p_service)
{
  nm_sr_flush(p_service->p_session);
  nm_sr_rcancel(p_service->p_session, &p_service->token.request);
  free(p_service->header_ptr);
  free(p_service);
}
