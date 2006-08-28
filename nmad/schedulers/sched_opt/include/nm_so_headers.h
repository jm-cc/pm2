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

#ifndef NM_SO_HEADERS_H
#define NM_SO_HEADERS_H

#include <tbx_macros.h>

#define NM_SO_PROTO_DATA_FIRST   128
#define NM_SO_PROTO_RDV          11
#define NM_SO_PROTO_ACK          12

#define NM_SO_PROTO_DATA_UNUSED 0
#define NM_SO_PROTO_CTRL_UNUSED 1

struct nm_so_global_header {
  uint32_t len;
};

// Warning : All header structs (except the global one) _MUST_ begin 
// with the 'proto_id' field

struct nm_so_data_header {
  uint8_t  proto_id;
  uint8_t  seq;
  uint16_t skip;
  uint32_t len;
};

struct nm_so_ctrl_rdv_header {
  uint8_t proto_id;
  uint8_t tag_id;
  uint8_t seq;
};

struct nm_so_ctrl_ack_header {
  uint8_t proto_id;
  uint8_t tag_id;
  uint8_t seq;
  uint8_t track_id;
};

// The following definition is useful for handling a uniform ctrl header size
union nm_so_generic_ctrl_header {
  struct nm_so_ctrl_rdv_header r;
  struct nm_so_ctrl_ack_header a;
};

#define NM_SO_ALIGN_TYPE      uint32_t
#define NM_SO_ALIGN_FRONTIER  sizeof(NM_SO_ALIGN_TYPE)
#define nm_so_aligned(x)      tbx_aligned(x, NM_SO_ALIGN_FRONTIER)

#define NM_SO_GLOBAL_HEADER_SIZE \
  nm_so_aligned(sizeof(struct nm_so_global_header))

#define NM_SO_DATA_HEADER_SIZE \
  nm_so_aligned(sizeof(struct nm_so_data_header))

#define NM_SO_CTRL_HEADER_SIZE \
  nm_so_aligned(sizeof(union nm_so_generic_ctrl_header))

#endif
