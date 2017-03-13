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

#ifndef NM_RPC_INTERFACE_H
#define NM_RPC_INTERFACE_H

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include <nm_session_interface.h>

/** a RPC token given to handlers */
typedef struct nm_rpc_token_s*nm_rpc_token_t;

/** a RPC registered service */
typedef struct nm_rpc_service_s*nm_rpc_service_t;

/** a RPC handler called upon request invocation */
typedef void (*nm_rpc_handler_t)(nm_rpc_token_t p_token);

/** a RPC finalizer, called when all data has been received */
typedef void (*nm_rpc_finalizer_t)(nm_rpc_token_t p_token);

/** register a new RPC service, listenning on the given tag/tag_mask */
nm_rpc_service_t nm_rpc_register(nm_session_t p_session, nm_tag_t tag, nm_tag_t tag_mask, nm_len_t hlen,
				 nm_rpc_handler_t p_handler, nm_rpc_finalizer_t p_finalizer,
				 void*ref);

/** stop listenning fro RPC requests on given service */
void nm_rpc_unregister(nm_rpc_service_t p_service);

/** send a RPC request; non-blocking, wait on request for completion */
void nm_rpc_isend(nm_session_t p_session, nm_sr_request_t*p_request,
		  nm_gate_t p_gate, nm_tag_t tag,
		  void*hptr, nm_len_t hlen, struct nm_data_s*p_body);

/** send a RPC request; blocking */
static inline void nm_rpc_send(nm_session_t p_session, nm_gate_t p_gate, nm_tag_t tag,
			       void*hptr, nm_len_t hlen, struct nm_data_s*p_body);

/** get pointer to the received header; to be called from a handler */
static inline void*nm_rpc_get_header(struct nm_rpc_token_s*p_token);

/** get the source for the received request; to be called from a handler */
static inline nm_gate_t nm_rpc_get_source(struct nm_rpc_token_s*p_token);

/** get the tag for the received request; to be called from a handler */
static inline nm_tag_t nm_rpc_get_tag(struct nm_rpc_token_s*p_token);

/** get the size of the body for the received request; to be called from a handler */
static inline nm_len_t nm_rpc_get_size(struct nm_rpc_token_s*p_token);

/** attach a user reference to a token */
static inline void nm_rpc_token_set_ref(struct nm_rpc_token_s*p_token, void*ref);

/** get a user reference previously attached to the token */
static inline void*nm_rpc_token_get_ref(struct nm_rpc_token_s*p_token);

/** get a user reference given upon service registration */
static inline void*nm_rpc_service_get_ref(struct nm_rpc_token_s*p_token);

/** asynchronously posts the recv for the body of a received request; to be called from a handler;
 * there is no guarantee that data is available when this function completes.
 * data will be actually received in 'finalizer' */
static inline void nm_rpc_irecv_data(struct nm_rpc_token_s*p_token, struct nm_data_s*p_body);



/* ********************************************************* */

/** @internal include private header for inlining */
#include <nm_rpc_private.h>

#endif /* NM_RPC_INTERFACE_H */
