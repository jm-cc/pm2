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
  struct nm_data_s header;
  struct nm_data_s body;
};

static void nm_rpc_traversal(const void*_content, nm_data_apply_t apply, void*_context);
const struct nm_data_ops_s nm_rpc_ops =
  {
    .p_traversal = &nm_rpc_traversal
  };
NM_DATA_TYPE(nm_rpc, struct nm_rpc_content_s, &nm_rpc_ops);

static void nm_rpc_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_rpc_content_s*p_content = _content;
  nm_data_traversal_apply(&p_content->header, apply, _context);
  nm_data_traversal_apply(&p_content->body, apply, _context);
}

/* ********************************************************* */

void nm_rpc_send(nm_session_t p_session, nm_gate_t p_gate, nm_tag_t tag,
		 struct nm_data_s*p_header, struct nm_data_s*p_body)
{
  
}

void nm_rpc_register(nm_session_t p_session, nm_tag_t tag, nm_tag_t tag_mask,
		     nm_rpc_handler_t p_handler, void*ref)
{
}

void nm_rpc_get_header(nm_rpc_token_t*p_token, struct nm_data_s*p_header)
{
}

void nm_rpc_get_body(nm_rpc_token_t*p_token, struct nm_data_s*p_header)
{
}

