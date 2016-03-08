/*
 * NewMadeleine
 * Copyright (C) 2006-2015 (see AUTHORS file)
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

/** global header at the beginning of pw */
struct nm_header_global_s
{
  uint16_t skip; /**< size of v0 actually used ( == skip value to reach v[1] ) */
} __attribute__((packed));

static inline void nm_header_global_finalize(struct nm_pkt_wrap*p_pw)
{
  struct nm_header_global_s*h = p_pw->v[0].iov_base;
  const int v0len = p_pw->v[0].iov_len;
  assert(v0len <= UINT16_MAX);
  h->skip = v0len;
}

static inline uint16_t nm_header_global_skip(const struct nm_pkt_wrap*p_pw)
{
  assert(p_pw->flags & (NM_PW_GLOBAL_HEADER | NM_PW_BUFFER));
  const struct nm_header_global_s*h = p_pw->v[0].iov_base;
  return h->skip;
}

typedef uint8_t nm_proto_t;

/** mask for proto ID part */
#define NM_PROTO_ID_MASK        0x0F
/** mask for proto flags */
#define NM_PROTO_FLAG_MASK      0xF0

/** a chunk of data */
#define NM_PROTO_DATA           0x01
/** a simplified (short header) chunk of data */
#define NM_PROTO_SHORT_DATA     0x02
/** new optimized packet format */
#define NM_PROTO_PKT_DATA       0x03
/** rendez-vous request */
#define NM_PROTO_RDV            0x04
/** ready-to-receive, replay to rdv */
#define NM_PROTO_RTR            0x05
/** an ack for a ssend (sent when receiving first chunk) */
#define NM_PROTO_ACK            0x06
/** ctrl chunk for strategy (don't decode in nm core) */
#define NM_PROTO_STRAT          0x07

/* flag for last proto in packet */
#define NM_PROTO_LAST           0x80
/** last chunk of data for the given pack */
#define NM_PROTO_FLAG_LASTCHUNK 0x10
/** data is 32 bit-aligned in packet */
#define NM_PROTO_FLAG_ALIGNED   0x20
/** data sent as synchronous send- please send an ack on first chunk */
#define NM_PROTO_FLAG_ACKREQ    0x40

// Warning : All header structs (except the global one) _MUST_ begin
// with the 'proto_id' field

struct nm_header_pkt_data_s
{
  nm_proto_t    proto_id;  /**< proto ID- should be NM_PROTO_PKT_DATA */
  nm_core_tag_t tag_id;    /**< tag for the message */
  nm_seq_t      seq;       /**< sequence number */
  nm_len_t      data_len;  /**< length of data enclosed */
  nm_len_t      chunk_offset;
  uint16_t      hlen;      /**< length in header (header + data in header) */
} __attribute__((packed));

struct nm_header_data_s
{
  nm_proto_t proto_id;  /**< proto ID- should be NM_PROTO_DATA */
  nm_core_tag_t tag_id;
  nm_seq_t seq;
  nm_len_t len;
  nm_len_t chunk_offset;
  uint16_t skip;
} __attribute__((packed));

struct nm_header_short_data_s
{
  nm_proto_t proto_id;  /**< proto ID- should be NM_PROTO_SHORT_DATA */
  nm_core_tag_t tag_id;
  nm_seq_t seq;
  uint8_t len;
} __attribute__((packed));

struct nm_header_ctrl_rdv_s
{
  nm_proto_t proto_id;  /**< proto ID- should be NM_PROTO_RDV */
  nm_core_tag_t tag_id;
  nm_seq_t seq;
  nm_len_t len;
  nm_len_t chunk_offset;
} __attribute__((packed));

struct nm_header_ctrl_rtr_s
{
  nm_proto_t proto_id;  /**< proto ID- should be NM_PROTO_RTR */
  nm_core_tag_t tag_id; /**< tag of the acknowledged data */
  nm_seq_t seq;
  nm_trk_id_t trk_id;
  uint16_t drv_index;   /**< index of the driver relative to the gate */
  nm_len_t chunk_offset;
  nm_len_t chunk_len;
} __attribute__((packed));

struct nm_header_ctrl_ack_s
{
  nm_proto_t proto_id;  /**< proto ID- should be NM_PROTO_ACK */
  nm_core_tag_t tag_id;
  nm_seq_t seq;
} __attribute__((packed));

/** a unified control header type (except strat)
 */
union nm_header_ctrl_generic_s
{
  struct nm_header_ctrl_rdv_s rdv;
  struct nm_header_ctrl_rtr_s rtr;
  struct nm_header_ctrl_ack_s ack;
  struct
  {
    nm_proto_t proto_id;
    nm_core_tag_t tag_id;
    nm_seq_t seq;
  } generic;
};

/** header for strategy private packets */
struct nm_header_strat_s
{
  nm_proto_t proto_id; /**< should be NM_PROTO_STRAT */
  nm_len_t size;
};

typedef union nm_header_ctrl_generic_s nm_header_ctrl_generic_t;
typedef struct nm_header_data_s nm_header_data_t;
typedef struct nm_header_short_data_s nm_header_short_data_t;

#define NM_HEADER_PKT_DATA_SIZE			\
  sizeof(struct nm_header_pkt_data_s)

#define NM_HEADER_DATA_SIZE				\
  nm_so_aligned(sizeof(struct nm_header_data_s))

#define NM_HEADER_SHORT_DATA_SIZE			\
  nm_so_aligned(sizeof(struct nm_header_short_data_s))

#define NM_HEADER_CTRL_SIZE				\
  nm_so_aligned(sizeof(union nm_header_ctrl_generic_s))

static inline void nm_header_init_pkt_data(struct nm_header_pkt_data_s*p_header, nm_core_tag_t tag_id, nm_seq_t seq, uint8_t flags,
					   nm_len_t len, nm_len_t chunk_offset)
{ 
  assert((flags & NM_PROTO_ID_MASK) == 0x00);
  p_header->proto_id = NM_PROTO_PKT_DATA | flags;
  p_header->tag_id   = tag_id;
  p_header->seq      = seq;
  p_header->data_len = len;
  p_header->chunk_offset = chunk_offset;
  p_header->hlen = NM_HEADER_PKT_DATA_SIZE;
}

static inline void nm_header_init_data(nm_header_data_t*p_header, nm_core_tag_t tag_id, nm_seq_t seq, uint8_t flags,
				       uint16_t skip, nm_len_t len, nm_len_t chunk_offset)
{ 
  assert((flags & NM_PROTO_ID_MASK) == 0x00);
  p_header->proto_id = NM_PROTO_DATA | flags;
  p_header->tag_id   = tag_id;
  p_header->seq      = seq;
  p_header->skip     = skip;
  p_header->len      = len;
  p_header->chunk_offset = chunk_offset;
}

static inline void nm_header_init_short_data(nm_header_short_data_t*p_header, nm_core_tag_t tag_id, 
					     nm_seq_t seq, nm_len_t len)
{
  p_header->proto_id = NM_PROTO_SHORT_DATA | NM_PROTO_FLAG_LASTCHUNK;
  p_header->tag_id   = tag_id;
  p_header->seq      = seq;
  p_header->len      = len;
}

static inline void nm_header_init_rdv(union nm_header_ctrl_generic_s*p_ctrl, struct nm_req_s*p_pack,
				      nm_len_t len, nm_len_t chunk_offset, uint8_t rdv_flags)
{
  if(p_pack->status & NM_PACK_SYNCHRONOUS)
    rdv_flags |= NM_PROTO_FLAG_ACKREQ;
  assert((rdv_flags & NM_PROTO_ID_MASK) == 0);
  p_ctrl->rdv.proto_id     = NM_PROTO_RDV | rdv_flags;
  p_ctrl->rdv.tag_id       = p_pack->tag;
  p_ctrl->rdv.seq          = p_pack->seq;
  p_ctrl->rdv.len          = len;
  p_ctrl->rdv.chunk_offset = chunk_offset;
}

static inline void nm_header_init_rtr(union nm_header_ctrl_generic_s*p_ctrl, nm_core_tag_t tag, nm_seq_t seq,
				      uint16_t drv_index, nm_trk_id_t trk_id, nm_len_t chunk_offset, nm_len_t chunk_len)
{ 
  p_ctrl->rtr.proto_id = NM_PROTO_RTR;
  p_ctrl->rtr.tag_id   = tag;
  p_ctrl->rtr.seq      = seq;
  p_ctrl->rtr.trk_id   = trk_id;
  p_ctrl->rtr.drv_index = drv_index;
  p_ctrl->rtr.chunk_offset = chunk_offset;
  p_ctrl->rtr.chunk_len = chunk_len;
}

static inline void nm_header_init_ack(union nm_header_ctrl_generic_s*p_ctrl, nm_core_tag_t tag, nm_seq_t seq)
{ 
  p_ctrl->ack.proto_id = NM_PROTO_ACK;
  p_ctrl->ack.tag_id   = tag;
  p_ctrl->ack.seq      = seq;
}

#endif /* NM_SO_HEADER_H */
