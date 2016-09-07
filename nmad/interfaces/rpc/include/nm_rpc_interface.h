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

typedef void (*nm_rpc_handler_t)(nm_rpc_token_t p_token, nm_sr_request_t*p_request);

typedef void (*nm_rpc_finalizer_t)(nm_rpc_token_t p_token);

struct nm_rpc_token_s
{
  struct nm_sr_monitor_s monitor;
  struct nm_data_s header;
  void*ref;
  nm_session_t p_session;
  nm_sr_request_t*p_request;
  nm_rpc_handler_t p_handler;
  nm_rpc_finalizer_t p_finalizer;
};

void nm_rpc_data_build(struct nm_data_s*p_rpc_data, const struct nm_data_s*p_header, const struct nm_data_s*p_body);

void nm_rpc_send(nm_session_t p_session, nm_gate_t p_gate, nm_tag_t tag,
		 struct nm_data_s*p_header, struct nm_data_s*p_body);

nm_rpc_token_t nm_rpc_register(nm_session_t p_session, nm_tag_t tag, nm_tag_t tag_mask,
			       nm_rpc_handler_t p_handler, nm_rpc_finalizer_t p_finalizer,
			       void*ref, struct nm_data_s*p_header);



#endif /* NM_RPC_INTERFACE_H */
