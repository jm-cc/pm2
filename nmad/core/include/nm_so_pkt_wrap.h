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

#ifndef NM_SO_PKT_WRAP_H
#define NM_SO_PKT_WRAP_H


//#define _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES

/* Data flags */
#define NM_SO_DATA_USE_COPY            1
#define NM_SO_DATA_IS_CTRL_HEADER      2
#define NM_SO_DATA_DONT_USE_HEADER     4
#define NM_SO_DATA_PREPARE_RECV        8

#define nm_l2so(l) \
        ((struct nm_pkt_wrap *)((char *)(l) -\
         (unsigned long)(&((struct nm_pkt_wrap *)0)->link)))

static __inline__
uint32_t nm_so_pw_remaining_header_area(struct nm_pkt_wrap *p_so_pw)
{
  register struct iovec *vec = p_so_pw->prealloc_v;

  return NM_SO_MAX_UNEXPECTED -
           ((vec->iov_base + vec->iov_len) - (void *)p_so_pw->buf);
}

static __inline__
uint32_t nm_so_pw_remaining_data(struct nm_pkt_wrap *p_so_pw)
{
  return NM_SO_MAX_UNEXPECTED - p_so_pw->length;
}

int nm_so_pw_init(struct nm_core *p_core);

int nm_so_pw_exit(void);

int nm_so_pw_alloc(int flags, struct nm_pkt_wrap **pp_pw);

int nm_so_pw_free(struct nm_pkt_wrap *p_pw);

int nm_so_pw_split(struct nm_pkt_wrap *p_pw,
                   struct nm_pkt_wrap **pp_pw2,
                   uint32_t offset);

int
nm_so_pw_add_data(struct nm_pkt_wrap *p_pw,
		  nm_tag_t proto_id, uint8_t seq,
		  const void *data, uint32_t len,
                  uint32_t offset, uint8_t is_last_chunk, int flags);

int
nm_so_pw_add_datatype(struct nm_pkt_wrap *p_pw,
                      nm_tag_t proto_id, uint8_t seq,
                      uint32_t len, const struct DLOOP_Segment *segp);


int
nm_so_pw_store_datatype(struct nm_pkt_wrap *p_so_pw,
                        nm_tag_t proto_id, uint8_t seq,
                        uint32_t len, const struct DLOOP_Segment *segp);

int
nm_so_pw_copy_contiguously_datatype(struct nm_pkt_wrap *p_so_pw,
                                    nm_tag_t proto_id, uint8_t seq,
                                    uint32_t len, struct DLOOP_Segment *segp);

#define nm_so_pw_add_control(p_so_pw, p_ctrl) \
  nm_so_pw_add_data((p_so_pw), \
		    0, 0, \
		    (p_ctrl), sizeof(*(p_ctrl)), \
		    0, \
                    0, \
                    NM_SO_DATA_IS_CTRL_HEADER)


static __inline__
int
nm_so_pw_alloc_and_fill_with_data(nm_tag_t proto_id, uint8_t seq,
				  const void *data, uint32_t len,
                                  uint32_t chunk_offset,
                                  uint8_t is_last_chunk,
                                  int flags,
				  struct nm_pkt_wrap **pp_so_pw)
{
  struct nm_pkt_wrap *p_so_pw;
  int err = nm_so_pw_alloc(flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;
  nm_so_pw_add_data(p_so_pw, proto_id, seq, data, len, chunk_offset, is_last_chunk, flags);
  p_so_pw->chunk_offset = chunk_offset;
  *pp_so_pw = p_so_pw;
 out:
    return err;
}

static __inline__
int
nm_so_pw_alloc_and_fill_with_control(const union nm_so_generic_ctrl_header *ctrl,
				     struct nm_pkt_wrap **pp_so_pw)
{
  int err;
  struct nm_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc(0, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  err = nm_so_pw_add_control(p_so_pw, ctrl);
  if(err != NM_ESUCCESS)
    goto free;

  *pp_so_pw = p_so_pw;

 out:
    return err;
 free:
    nm_so_pw_free(p_so_pw);
    goto out;
}

#ifdef PIO_OFFLOAD
int
nm_so_pw_offloaded_finalize(struct nm_pkt_wrap *p_so_pw);
#endif

int
nm_so_pw_finalize(struct nm_pkt_wrap *p_so_pw);


/* Iterators */

typedef void nm_so_pw_data_handler(struct nm_pkt_wrap *p_so_pw,
				   void *ptr,
				   nm_so_data_header_t*header, uint32_t len,
				   nm_tag_t proto_id, uint8_t seq,
				   uint32_t chunk_offset, uint8_t is_last_chunk);
typedef void nm_so_pw_rdv_handler(struct nm_pkt_wrap *p_so_pw,
				  nm_so_generic_ctrl_header_t*rdv,
				  nm_tag_t tag, uint8_t seq,
				  uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk);
typedef void nm_so_pw_ack_handler(struct nm_pkt_wrap *p_so_pw,
				  struct nm_so_ctrl_ack_header*header);
typedef void nm_so_pw_ack_chunk_handler(struct nm_pkt_wrap *p_so_pw,
					nm_tag_t tag, uint8_t seq, uint32_t chunk_offset,
					struct nm_so_ctrl_ack_chunk_header*header);
int
nm_so_pw_iterate_over_headers(struct nm_pkt_wrap *p_so_pw,
			      nm_so_pw_data_handler data_handler,
			      nm_so_pw_rdv_handler rdv_handler,
			      nm_so_pw_ack_handler ack_handler,
                              nm_so_pw_ack_chunk_handler ack_chunk_handler);

static __inline__
int
nm_so_pw_dec_header_ref_count(struct nm_pkt_wrap *p_so_pw)
{
  if(!(--p_so_pw->header_ref_count)){
    nm_so_pw_free(p_so_pw);
  }
  return NM_ESUCCESS;
}

#endif /* NM_SO_PKT_WRAP_H */

