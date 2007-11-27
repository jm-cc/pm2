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

#ifndef _NM_SO_PKT_WRAP_H_
#define _NM_SO_PKT_WRAP_H_

#include <tbx.h>
#include <pm2_list.h>

#include "nm_core.h"
#include "nm_so_parameters.h"
#include "nm_pkt_wrap.h"
#include "nm_so_headers.h"

#include <ccs_public.h>

//#define _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES

/* Data flags */
#define NM_SO_DATA_USE_COPY            1
#define NM_SO_DATA_IS_CTRL_HEADER      2
#define NM_SO_DATA_DONT_USE_HEADER     4
#define NM_SO_DATA_PREPARE_RECV        8

struct nm_so_pkt_wrap {

  struct nm_pkt_wrap pw;

  int                header_ref_count;

  /* Used on the sending side */
  int                pending_skips;

  /* Used on the receiving side */
  struct list_head   link;

  struct iovec       v[NM_SO_PREALLOC_IOV_LEN];

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
  struct nm_iovec    nm_v[NM_SO_PREALLOC_IOV_LEN];
#endif

  uint32_t chunk_offset;
  tbx_bool_t is_completed;
  struct DLOOP_Segment *segp;

  tbx_bool_t datatype_copied_buf;
  
  /* The following field MUST be the LAST within the structure */
  NM_SO_ALIGN_TYPE   buf[1];
};

#define nm_pw2so(p_pw) \
	((struct nm_so_pkt_wrap *)((char *)(p_pw) - \
         (unsigned long)(&((struct nm_so_pkt_wrap *)0)->pw)))

#define nm_l2so(l) \
        ((struct nm_so_pkt_wrap *)((char *)(l) -\
         (unsigned long)(&((struct nm_so_pkt_wrap *)0)->link)))

static __inline__
uint32_t nm_so_pw_remaining_header_area(struct nm_so_pkt_wrap *p_so_pw)
{
  //**register struct iovec *vec = p_so_pw->header_index;
  register struct iovec *vec = p_so_pw->v;

  return NM_SO_PREALLOC_BUF_LEN -
           ((vec->iov_base + vec->iov_len) - (void *)p_so_pw->buf);
}

static __inline__
uint32_t nm_so_pw_remaining_data(struct nm_so_pkt_wrap *p_so_pw)
{
  return NM_SO_MAX_UNEXPECTED - p_so_pw->pw.length;
}

int
nm_so_pw_init(struct nm_core *p_core);

int
nm_so_pw_exit(void);

int
nm_so_pw_alloc(int flags, struct nm_so_pkt_wrap **pp_so_pw);

int
nm_so_pw_free(struct nm_so_pkt_wrap *p_so_pw);

int nm_so_pw_split(struct nm_so_pkt_wrap *p_so_pw,
                   struct nm_so_pkt_wrap **pp_so_pw2,
                   uint32_t offset);

int
nm_so_pw_add_data(struct nm_so_pkt_wrap *p_so_pw,
		  uint8_t proto_id, uint8_t seq,
		  void *data, uint32_t len,
                  uint32_t offset, uint8_t is_last_chunk, int flags);

int
nm_so_pw_add_datatype(struct nm_so_pkt_wrap *p_so_pw,
                      uint8_t proto_id, uint8_t seq,
                      uint32_t len, struct DLOOP_Segment *segp);


int
nm_so_pw_store_datatype(struct nm_so_pkt_wrap *p_so_pw,
                        uint8_t proto_id, uint8_t seq,
                        uint32_t len, struct DLOOP_Segment *segp);

int
nm_so_pw_add_large_datatype(struct nm_so_pkt_wrap *p_so_pw,
                            uint8_t proto_id, uint8_t seq,
                            uint32_t len, struct DLOOP_Segment *segp);

int
nm_so_pw_copy_contiguously_datatype(struct nm_so_pkt_wrap *p_so_pw,
                                    uint8_t proto_id, uint8_t seq,
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
nm_so_pw_alloc_and_fill_with_data(uint8_t proto_id, uint8_t seq,
				  void *data, uint32_t len,
                                  uint32_t chunk_offset,
                                  uint8_t is_last_chunk,
                                  int flags,
				  struct nm_so_pkt_wrap **pp_so_pw)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc(flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  err = nm_so_pw_add_data(p_so_pw, proto_id, seq, data, len, chunk_offset, is_last_chunk, flags);
  if(err != NM_ESUCCESS)
    goto free;

  p_so_pw->chunk_offset = chunk_offset;

  *pp_so_pw = p_so_pw;

 out:
    return err;
 free:
    nm_so_pw_free(p_so_pw);
    goto out;
}

static __inline__
int
nm_so_pw_alloc_and_fill_with_control(union nm_so_generic_ctrl_header *ctrl,
				     struct nm_so_pkt_wrap **pp_so_pw)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;

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

int
nm_so_pw_finalize(struct nm_so_pkt_wrap *p_so_pw);


/* Iterators */

#define NM_SO_HEADER_MARK_READ   0
#define NM_SO_HEADER_MARK_UNREAD 1

typedef int nm_so_pw_data_handler(struct nm_so_pkt_wrap *p_so_pw,
				  void *ptr,
                                  void *header, uint32_t len,
				  uint8_t proto_id, uint8_t seq,
                                  uint32_t chunk_offset, uint8_t is_last_chunk);
typedef int nm_so_pw_rdv_handler(struct nm_so_pkt_wrap *p_so_pw,
                                 void *rdv,
				 uint8_t tag_id, uint8_t seq,
                                 uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk);
typedef int nm_so_pw_ack_handler(struct nm_so_pkt_wrap *p_so_pw,
                                 uint8_t tag_id, uint8_t seq,
                                 uint8_t track_id, uint32_t chunk_offset);
typedef int nm_so_pw_ack_chunk_handler(struct nm_so_pkt_wrap *p_so_pw,
                                       uint8_t tag_id, uint8_t seq, uint32_t chunk_offset,
                                       uint8_t track_id, uint32_t chunk_len);
int
nm_so_pw_iterate_over_headers(struct nm_so_pkt_wrap *p_so_pw,
			      nm_so_pw_data_handler data_handler,
			      nm_so_pw_rdv_handler rdv_handler,
			      nm_so_pw_ack_handler ack_handler,
                              nm_so_pw_ack_chunk_handler ack_chunk_handler);

static __inline__
int
nm_so_pw_dec_header_ref_count(struct nm_so_pkt_wrap *p_so_pw)
{
  if(!(--p_so_pw->header_ref_count)){
    nm_so_pw_free(p_so_pw);
  }
  return NM_ESUCCESS;
}

#endif
