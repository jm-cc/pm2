/*
 * NewMadeleine
 * Copyright (C) 2016-2017 (see AUTHORS file)
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

#ifndef NM_RPC_PRIVATE_H
#define NM_RPC_PRIVATE_H

/** @file nm_rpc_private.h
 * Private header for the RPC interface; this is not part of the 
 * public API and is not supposed to be used by end-users
 */

#include <assert.h>

/** an outgoing rpc request */
struct nm_rpc_req_s
{
  nm_sr_request_t request;            /**< the sendrecv request, actually allocated here */
  struct nm_data_s body;              /**< the user-supplied body data descriptor, copied to allow asynchronicity */
  nm_rpc_req_notifier_t p_notifier;   /**< notification function to call uppon req completion */
  void*p_notifier_ref;                /**< user-supplied parameter for the notifier */
};

/** an incoming rpc request */
struct nm_rpc_token_s
{
  nm_sr_request_t request;            /**< the sr request used for the full rpc_data */
  struct nm_data_s rpc_data, body;    /**< data descriptors for rpc request and user-supplied body */
  struct nm_rpc_service_s*p_service;  /**< service this token belongs to */
  void*ref;                           /**< user ref for the token */
};

/** descriptor for a registered RPC service */
struct nm_rpc_service_s
{
  struct nm_sr_monitor_s monitor;     /**< the monitor triggered by UNEXPECTED events */
  nm_rpc_handler_t p_handler;         /**< user-supplied function, called upon header arrival */
  nm_rpc_finalizer_t p_finalizer;     /**< user-supplied function, called upon data body arrival */
  nm_session_t p_session;             /**< session used to send/recv requests */
  struct nm_data_s header;
  void*header_ptr;
  nm_len_t hlen;
  void*ref;
};

/** @internal data constructor for the compound rpc data type (header + body) */
void nm_rpc_data_build(struct nm_data_s*p_rpc_data, void*hptr, nm_len_t hlen, const struct nm_data_s*p_body);

/** @internal private, exposed for inlining */
void nm_rpc_req_delete(nm_rpc_req_t p_rpc_req);

static inline void nm_rpc_send(nm_session_t p_session, nm_gate_t p_gate, nm_tag_t tag,
			       void*hptr, nm_len_t hlen, struct nm_data_s*p_body)
{
  nm_rpc_req_t p_req = nm_rpc_isend(p_session, p_gate, tag, hptr, hlen, p_body);
  nm_rpc_req_wait(p_req);
}

static inline void nm_rpc_req_wait(nm_rpc_req_t p_req)
{
  nm_sr_swait(p_req->request.p_session, &p_req->request);
  nm_rpc_req_delete(p_req);
}

static inline void*nm_rpc_get_header(struct nm_rpc_token_s*p_token)
{
  assert(p_token != NULL);
  assert(p_token->p_service != NULL);
  return p_token->p_service->header_ptr;
}

static inline void*nm_rpc_service_get_ref(struct nm_rpc_token_s*p_token)
{
  return p_token->p_service->ref;
}

static inline nm_gate_t nm_rpc_get_source(struct nm_rpc_token_s*p_token)
{
  nm_gate_t p_gate = NULL;
  nm_sr_request_get_gate(&p_token->request, &p_gate);
  return p_gate;
}

static inline nm_tag_t nm_rpc_get_tag(struct nm_rpc_token_s*p_token)
{
  nm_tag_t tag = 0;
  nm_sr_request_get_tag(&p_token->request, &tag);
  return tag;
}

static inline nm_len_t nm_rpc_get_size(struct nm_rpc_token_s*p_token)
{
  nm_len_t size = NM_LEN_UNDEFINED;
  nm_sr_request_get_size(&p_token->request, &size);
  assert(size >= p_token->p_service->hlen);
  return size - p_token->p_service->hlen;
}

static inline nm_session_t nm_rpc_get_session(struct nm_rpc_token_s*p_token)
{
  return p_token->p_service->p_session;
}

static inline void*nm_rpc_token_get_ref(struct nm_rpc_token_s*p_token)
{
  return p_token->ref;
}

static inline void nm_rpc_token_set_ref(struct nm_rpc_token_s*p_token, void*ref)
{
  p_token->ref = ref;
}

static inline void nm_rpc_irecv_data(struct nm_rpc_token_s*p_token, struct nm_data_s*p_body)
{
  assert(nm_data_isnull(&p_token->body));
  p_token->body = *p_body;
  nm_rpc_data_build(&p_token->rpc_data, p_token->p_service->header_ptr, p_token->p_service->hlen, &p_token->body);
  nm_sr_recv_unpack_data(p_token->p_service->p_session, &p_token->request, &p_token->rpc_data);
  nm_sr_recv_post(p_token->p_service->p_session, &p_token->request);
}



#endif /* NM_RPC_PRIVATE_H */
