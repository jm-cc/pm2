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

/* from legacy nm_proto_id.h:
 *   - Scheduler protocol flows: 1-16
 *   - General purpose protocol flows: 17-127   (obsolete)
 *   - User channels: 128-255
 */

#define NM_SO_PROTO_DATA_FIRST   128
#define NM_SO_PROTO_RDV          11
#define NM_SO_PROTO_ACK          12
#define NM_SO_PROTO_MULTI_ACK    13
#define NM_SO_PROTO_ACK_CHUNK    14

#define NM_SO_PROTO_DATA_UNUSED 2
#define NM_SO_PROTO_CTRL_UNUSED 1

#define NM_SO_DATA_FLAG_LASTCHUNK 0x01
#define NM_SO_DATA_FLAG_ALIGNED   0x02

/** Global header for every packet sent on the network */
struct nm_so_global_header
{
  uint32_t len;
};

// Warning : All header structs (except the global one) _MUST_ begin
// with the 'proto_id' field

struct nm_so_data_header {
  nm_tag_t proto_id;
  uint8_t  seq;
  uint8_t  flags;
  uint16_t skip;
  uint32_t len;
  uint32_t chunk_offset;
};

struct nm_so_ctrl_rdv_header {
  nm_tag_t proto_id;
  nm_tag_t tag_id;
  uint8_t seq;
  uint8_t is_last_chunk;
  uint32_t len;
  uint32_t chunk_offset;
};

struct nm_so_ctrl_ack_header {
  nm_tag_t proto_id;  /**< proto ID- should be NM_SO_PROTO_ACK */
  nm_tag_t tag_id;    /**< tag of the acknowledged data */
  uint8_t seq;
  nm_trk_id_t trk_id;
  nm_drv_id_t drv_id;
  uint32_t chunk_offset;
  uint32_t chunk_len;
};

struct nm_so_ctrl_multi_ack_header {
  nm_tag_t proto_id;
  uint8_t nb_chunks;
  nm_tag_t tag_id;
  uint8_t seq;
  uint32_t chunk_offset;
};

struct nm_so_ctrl_ack_chunk_header {
  nm_tag_t proto_id;
  nm_trk_id_t trk_id;
  nm_drv_id_t drv_id;
  uint32_t chunk_len;
};

/** a unified control header type
 */
union nm_so_generic_ctrl_header {
  struct nm_so_ctrl_rdv_header r;
  struct nm_so_ctrl_ack_header a;
  struct nm_so_ctrl_multi_ack_header ma;
  struct nm_so_ctrl_ack_chunk_header ac;
};

typedef union nm_so_generic_ctrl_header nm_so_generic_ctrl_header_t;
typedef struct nm_so_data_header nm_so_data_header_t;

#define NM_SO_GLOBAL_HEADER_SIZE \
  nm_so_aligned(sizeof(struct nm_so_global_header))

#define NM_SO_DATA_HEADER_SIZE \
  nm_so_aligned(sizeof(struct nm_so_data_header))

#define NM_SO_DATATYPE_HEADER_SIZE \
  nm_so_aligned(sizeof(struct nm_so_datatype_header))

#define NM_SO_CTRL_HEADER_SIZE \
  nm_so_aligned(sizeof(union nm_so_generic_ctrl_header))

static inline void nm_so_init_data(nm_so_data_header_t*p_header, nm_tag_t proto_id, uint8_t seq, uint8_t flags,
				   uint16_t skip, uint32_t len, uint32_t chunk_offset)
{ 
  p_header->proto_id = proto_id;
  p_header->seq      = seq;
  p_header->flags    = flags;
  p_header->skip     = skip;
  p_header->len      = len;
  p_header->chunk_offset = chunk_offset;
}

#define nm_so_init_rdv(p_ctrl, _tag, _seq, _len, _chunk_offset, _is_last_chunk)	\
  do { \
    (p_ctrl)->r.proto_id = NM_SO_PROTO_RDV;	\
    (p_ctrl)->r.tag_id = (_tag);		\
    (p_ctrl)->r.seq = (_seq);			\
    (p_ctrl)->r.len = (_len);			\
    (p_ctrl)->r.chunk_offset = (_chunk_offset);	\
    (p_ctrl)->r.is_last_chunk = (_is_last_chunk);	\
  } while(0)

static inline void nm_so_init_ack(union nm_so_generic_ctrl_header*p_ctrl, nm_tag_t tag, uint8_t seq,
				  nm_drv_id_t drv_id, nm_trk_id_t trk_id, uint32_t chunk_offset, uint32_t chunk_len)
{ 
  p_ctrl->a.proto_id = NM_SO_PROTO_ACK;
  p_ctrl->a.tag_id   = tag;
  p_ctrl->a.seq      = seq;
  p_ctrl->a.trk_id   = trk_id;
  p_ctrl->a.drv_id   = drv_id;
  p_ctrl->a.chunk_offset = chunk_offset;
  p_ctrl->a.chunk_len = chunk_len;
}

static inline void nm_so_init_multi_ack(union nm_so_generic_ctrl_header*p_ctrl, int nb_chunks, nm_tag_t tag,
					uint8_t seq, uint32_t chunk_offset)
{
  p_ctrl->ma.proto_id  = NM_SO_PROTO_MULTI_ACK;
  p_ctrl->ma.nb_chunks = nb_chunks;
  p_ctrl->ma.tag_id    = tag;
  p_ctrl->ma.seq       = seq;
  p_ctrl->ma.chunk_offset = chunk_offset;
}

static inline void nm_so_add_ack_chunk(union nm_so_generic_ctrl_header*p_ctrl,
				       const struct nm_rdv_chunk*chunk)
{
  p_ctrl->ac.proto_id  = NM_SO_PROTO_ACK_CHUNK;
  p_ctrl->ac.trk_id    = chunk->trk_id;
  p_ctrl->ac.drv_id    = chunk->drv_id;
  p_ctrl->ac.chunk_len = chunk->len;
}

#endif /* NM_SO_§HEADER_H */
