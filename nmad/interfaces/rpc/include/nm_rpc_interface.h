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

#ifndef NM_RPC_INTERFACE_H
#define NM_RPC_INTERFACE_H

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include <nm_session_interface.h>

typedef struct nm_rpc_token_s*nm_rpc_token_t;

typedef struct nm_rpc_service_s*nm_rpc_service_t;

typedef void (*nm_rpc_handler_t)(nm_rpc_token_t p_token);

typedef void (*nm_rpc_finalizer_t)(nm_rpc_token_t p_token);

struct nm_rpc_token_s
{
  nm_sr_request_t request;
  struct nm_data_s rpc_data, body;
  struct nm_rpc_service_s*p_service;
};

struct nm_rpc_service_s
{
  struct nm_sr_monitor_s monitor;
  nm_rpc_handler_t p_handler;
  nm_rpc_finalizer_t p_finalizer;
  nm_session_t p_session;
  void*header_ptr;
  nm_len_t hlen;
  void*ref;
  struct nm_rpc_token_s token;
};


void nm_rpc_data_build(struct nm_data_s*p_rpc_data, void*hptr, nm_len_t hlen, const struct nm_data_s*p_body);

void nm_rpc_isend(nm_session_t p_session, nm_sr_request_t*p_request,
		  nm_gate_t p_gate, nm_tag_t tag,
		  void*hptr, nm_len_t hlen, struct nm_data_s*p_body);

static inline void nm_rpc_send(nm_session_t p_session, nm_gate_t p_gate, nm_tag_t tag,
			       void*hptr, nm_len_t hlen, struct nm_data_s*p_body)
{
  nm_sr_request_t request;
  nm_rpc_isend(p_session, &request, p_gate, tag, hptr, hlen, p_body);
  nm_sr_swait(p_session, &request);
}

static inline void*nm_rpc_get_header(struct nm_rpc_token_s*p_token)
{
  return p_token->p_service->header_ptr;
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
  return size;
}

static inline void nm_rpc_recv_data(struct nm_rpc_token_s*p_token, struct nm_data_s*p_body)
{
  p_token->body = *p_body;
  nm_rpc_data_build(&p_token->rpc_data, p_token->p_service->header_ptr, p_token->p_service->hlen, &p_token->body);
  nm_sr_recv_unpack_data(p_token->p_service->p_session, &p_token->request, &p_token->rpc_data);
  nm_sr_recv_post(p_token->p_service->p_session, &p_token->request);
}

nm_rpc_service_t nm_rpc_register(nm_session_t p_session, nm_tag_t tag, nm_tag_t tag_mask, nm_len_t hlen,
				 nm_rpc_handler_t p_handler, nm_rpc_finalizer_t p_finalizer,
				 void*ref);

void nm_rpc_unregister(nm_rpc_service_t p_service);


#endif /* NM_RPC_INTERFACE_H */
