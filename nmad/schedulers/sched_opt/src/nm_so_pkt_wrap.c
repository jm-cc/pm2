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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_public.h>

#include "nm_rdv_public.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
/* Per-'iovec entry' allocation flags (exclusive) */
#define NM_SO_ALLOC_STATIC       0
#define NM_SO_ALLOC_DYNAMIC      1
#endif

/* Global priv_flags */
#define NM_SO_NO_HEADER          1
#define NM_SO_IOV_ALLOC_DYNAMIC  2
#define NM_SO_RECV_PW            4

/* fast packet allocator */
#define INITIAL_PKT_NUM  16
static p_tbx_memory_t nm_so_pw_mem = NULL;
static p_tbx_memory_t nm_so_pw_recv_mem = NULL;

/* Some ugly macros (for convenience only) */
#define nm_so_iov_flags(p_so_pw, i)  ((p_so_pw)->pw.nm_v[i].priv_flags)

int
nm_so_pw_init(struct nm_core *p_core)
{
  tbx_malloc_init(&nm_so_pw_mem,
		  sizeof(struct nm_so_pkt_wrap) + NM_SO_PREALLOC_BUF_LEN,
		  INITIAL_PKT_NUM, "nmad/.../sched_opt/nm_so_pkt_wrap");

  tbx_malloc_init(&nm_so_pw_recv_mem,
		  sizeof(struct nm_so_pkt_wrap) + NM_SO_MAX_UNEXPECTED +
		  NM_SO_GLOBAL_HEADER_SIZE + NM_SO_DATA_HEADER_SIZE,
		  INITIAL_PKT_NUM, "nmad/.../sched_opt/nm_so_pkt_wrap");

  return NM_ESUCCESS;
}

int
nm_so_pw_alloc(int flags, struct nm_so_pkt_wrap **pp_so_pw)
{
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err = NM_ESUCCESS;

  if(flags & NM_SO_DATA_PREPARE_RECV)
    p_so_pw = tbx_malloc(nm_so_pw_recv_mem);
  else
    p_so_pw = tbx_malloc(nm_so_pw_mem);

  if (!p_so_pw) {
    err = -NM_ENOMEM;
    goto out;
  }

  p_so_pw->pw.v = p_so_pw->v;
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
  p_so_pw->pw.nm_v = p_so_pw->nm_v;
#endif
  p_so_pw->pw.v_first = 0;
  p_so_pw->pw.v_size = NM_SO_PREALLOC_IOV_LEN;

  if(tbx_unlikely(flags & NM_SO_DATA_DONT_USE_HEADER)) {

    p_so_pw->pw.pkt_priv_flags |= NM_SO_NO_HEADER;

    if(flags & NM_SO_DATA_PREPARE_RECV) {

      p_so_pw->pw.v_nb = 1;
      p_so_pw->v->iov_base = p_so_pw->buf;
      p_so_pw->v->iov_len = (p_so_pw->pw.length = NM_SO_MAX_UNEXPECTED +
			     NM_SO_GLOBAL_HEADER_SIZE + NM_SO_DATA_HEADER_SIZE);

      p_so_pw->pw.pkt_priv_flags = NM_SO_RECV_PW;

      p_so_pw->optimistic_recv = 0;

    } else {

      p_so_pw->pw.pkt_priv_flags = NM_SO_NO_HEADER;

      p_so_pw->pw.v_nb = 0;
      p_so_pw->pw.length = 0;
    }

  } else {

    p_so_pw->pw.pkt_priv_flags = 0;

    p_so_pw->pw.v_nb = 1;
    p_so_pw->v->iov_base = p_so_pw->buf;
    p_so_pw->v->iov_len = NM_SO_GLOBAL_HEADER_SIZE;

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
    nm_so_iov_flags(p_so_pw, 0) = NM_SO_ALLOC_STATIC;
#endif

    p_so_pw->pw.length = NM_SO_GLOBAL_HEADER_SIZE;

    p_so_pw->header_index = 0;
    p_so_pw->uncompleted_header = NULL;
    p_so_pw->pending_skips = 0;
  }

  *pp_so_pw = p_so_pw;

 out:
    return err;
}

int
nm_so_pw_free(struct nm_so_pkt_wrap *p_so_pw)
{
  int err;
  int flags = p_so_pw->pw.pkt_priv_flags;

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
  /* Clean iov entries first */
  {
    int i;

    for(i = 0; i < p_so_pw->pw.v_nb; i++)
      if(nm_so_iov_flags(p_so_pw, i) == NM_SO_ALLOC_DYNAMIC)
	TBX_FREE(p_so_pw->pw.v[i].iov_base);
  }
#endif

  /* Then clean whole iov */
  if(flags & NM_SO_IOV_ALLOC_DYNAMIC) {
    TBX_FREE(p_so_pw->pw.v);
    TBX_FREE(p_so_pw->pw.nm_v);
  }

  /* Finally clean packet wrapper itself */
  if(flags & NM_SO_RECV_PW)
    tbx_free(nm_so_pw_recv_mem, p_so_pw);
  else
    tbx_free(nm_so_pw_mem, p_so_pw);

  err = NM_ESUCCESS;
  return err;
}

int
nm_so_pw_add_data(struct nm_so_pkt_wrap *p_so_pw,
		  uint8_t proto_id, uint8_t seq,
		  void *data, uint32_t len, int flags)
{
  int err;
  struct iovec *vec;

  if(!(flags & NM_SO_DATA_DONT_USE_HEADER)) {
    /* Add data with header */

    if(p_so_pw->uncompleted_header) {

      struct nm_so_data_header *h = p_so_pw->uncompleted_header;
      uint32_t gap = nm_so_aligned(h->len) - h->len;

      p_so_pw->uncompleted_header = NULL;

      // TODO: realloc iov dynamically!
      if(p_so_pw->pw.v_nb == NM_SO_PREALLOC_IOV_LEN) {
	err = -NM_ENOMEM;
	goto out;
      }

      /* Use a new iovec entry for upcoming headers */
      p_so_pw->header_index = p_so_pw->pw.v_nb++;

      vec = p_so_pw->pw.v + p_so_pw->header_index;

      if(gap) {

	/* Add a small gap to correct alignment on the receiving side */
	vec->iov_base = (char *)h + NM_SO_DATA_HEADER_SIZE +
	                NM_SO_ALIGN_FRONTIER - gap;
	vec->iov_len = gap;

	/* Important: Correct the size reserved for the previous data
	   chunk! */
	h->len = nm_so_aligned(h->len);

	p_so_pw->pw.length += gap;

      } else {

	vec->iov_base = (char *)h + NM_SO_DATA_HEADER_SIZE;
	vec->iov_len = 0;
      }

    } else {

      vec = p_so_pw->pw.v + p_so_pw->header_index;
    }

    if(!(flags & NM_SO_DATA_IS_CTRL_HEADER)) {

      /* Small data case */
      struct nm_so_data_header *h;

      /* Add header */
      h = vec->iov_base + vec->iov_len;

      h->proto_id = proto_id;
      h->seq = seq;

      vec->iov_len += NM_SO_DATA_HEADER_SIZE;

      if(flags & NM_SO_DATA_USE_COPY) {

	/* Data immediately follows its header */
	uint32_t size;

	memcpy(vec->iov_base + vec->iov_len, data, len);

	h->skip = 0;
	size = nm_so_aligned(len);
	h->len = size;
	vec->iov_len += size;
	p_so_pw->pw.length += NM_SO_DATA_HEADER_SIZE + size;

      } else {
	/* Data are handled by a separate iovec entry */

	struct iovec *dvec;

	// TODO: realloc iov dynamically!
	if(p_so_pw->pw.v_nb == NM_SO_PREALLOC_IOV_LEN) {
	  err = -NM_ENOMEM;
	  goto out;
	}

	dvec = p_so_pw->pw.v + p_so_pw->pw.v_nb;
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
	nm_so_iov_flags(p_so_pw, p_so_pw->pw.v_nb) = NM_SO_ALLOC_STATIC;
#endif

	dvec->iov_base = data;
	dvec->iov_len = len;

	/* We don't know yet the gap between header and data, so we
	   temporary store the iovec index as the 'skip' value */
	h->skip = p_so_pw->pw.v_nb;
	p_so_pw->pending_skips++;
	h->len = len;

	if(flags & NM_SO_DATA_FORCE_CONTIGUOUS) {

	  /* Actually we know that skip == 0 at this point. But to
	     simplify the nm_so_finalize routine, we store the iovec
	     index, as in the general case. Also note that the len is
	     _temporary_ set to 'len' and may be changed later to
	     aligned(len) if another header is added.*/

	  /* We must continue the header area on a new iovec entry if
	     needed, and add a small gap to correct alignment. We
	     postpone the processing to the next add_data operation. */
	  p_so_pw->uncompleted_header = h;

	}

	p_so_pw->pw.length += NM_SO_DATA_HEADER_SIZE + len;

	p_so_pw->pw.v_nb++;

      }

    } else {
	
      /* Data actually references a ctrl header. We simply append it
	 to the previous other headers. */
      union nm_so_generic_ctrl_header *src, *dst;

      src = data;
      dst = vec->iov_base + vec->iov_len;

      /* Copy ctrl header */
      *dst = *src;

      vec->iov_len += NM_SO_CTRL_HEADER_SIZE;

      p_so_pw->pw.length += NM_SO_CTRL_HEADER_SIZE;

    }

  } else {
    /* Data chunk added 'as is': simply use a new iovec entry */

    // TODO: realloc iov dynamically!
    if(p_so_pw->pw.v_nb == NM_SO_PREALLOC_IOV_LEN) {
      err = -NM_ENOMEM;
      goto out;
    }

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
    nm_so_iov_flags(p_so_pw, p_so_pw->pw.v_nb) = NM_SO_ALLOC_STATIC;
#endif
    vec = p_so_pw->pw.v + p_so_pw->pw.v_nb++;
    vec->iov_base = data;
    vec->iov_len = len;

    p_so_pw->pw.proto_id = proto_id;
    p_so_pw->pw.seq = seq;

    p_so_pw->pw.length += len;

  }

  err = NM_ESUCCESS;

 out:
  return err;
}

int
nm_so_pw_finalize(struct nm_so_pkt_wrap *p_so_pw)
{
  int err = NM_ESUCCESS;
  uint32_t remaining_bytes;
  uint32_t to_skip;
  void *ptr;
  struct iovec *vec;

  if(p_so_pw->pw.pkt_priv_flags & NM_SO_NO_HEADER)
    /* We're done */
    goto out;

  ((struct nm_so_global_header *)(p_so_pw->pw.v->iov_base))->len = 
    p_so_pw->pw.length;

  /* Fix the 'skip' fields */
  if(!p_so_pw->pending_skips)
    /* Were're done */
    goto out;

  vec = p_so_pw->pw.v;
  ptr = vec->iov_base + NM_SO_GLOBAL_HEADER_SIZE;
  remaining_bytes = vec->iov_len - NM_SO_GLOBAL_HEADER_SIZE;
  to_skip = 0;

  do {

    if(!remaining_bytes) {
      /* Headers area continues on a separate iovec entry */
      vec++;
      ptr = (void *)nm_so_aligned((uint32_t)vec->iov_base);
      remaining_bytes = vec->iov_len
	                - ((uint32_t)ptr - (uint32_t)vec->iov_base);
      to_skip = 0;
    }

    if (*(uint8_t *)ptr >= NM_SO_PROTO_DATA_FIRST) {

      struct nm_so_data_header *h = ptr;

      if(h->skip == 0) {
	/* Data immediately follows */
	ptr += NM_SO_DATA_HEADER_SIZE + h->len;
	remaining_bytes -= NM_SO_DATA_HEADER_SIZE + h->len;
      } else {
	/* Data occupy a separate iovec entry */
	ptr += NM_SO_DATA_HEADER_SIZE;
	remaining_bytes -= NM_SO_DATA_HEADER_SIZE;
	h->skip = remaining_bytes + to_skip;
	vec++;
	to_skip += vec->iov_len;
	p_so_pw->pending_skips--;
      }

    } else {
      /* Ctrl header */

      ptr += NM_SO_CTRL_HEADER_SIZE;
      remaining_bytes -= NM_SO_CTRL_HEADER_SIZE;
    }

  } while (p_so_pw->pending_skips);

 out:
  return err;
}

int
nm_so_pw_alloc_optimistic(uint8_t tag, uint8_t seq,
			  void *data, uint32_t len,
			  struct nm_so_pkt_wrap **pp_so_pw)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;
  struct iovec *vec;

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER |
		       NM_SO_DATA_PREPARE_RECV,
		       &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  p_so_pw->optimistic_recv = 1;
  p_so_pw->pw.proto_id = 128 + tag;
  p_so_pw->pw.seq = seq;

  vec = p_so_pw->pw.v;
  /* First entry will contain global header + (hopefully) the expected
     data header */
  vec[0].iov_len = NM_SO_GLOBAL_HEADER_SIZE + NM_SO_DATA_HEADER_SIZE;
  /* Second entry will (hopefully) contain our data */
  vec[1].iov_base = data;
  vec[1].iov_len = len;
  /* Third entry will contain additionnal data if any */
  vec[2].iov_base = (void *)p_so_pw->buf +
                      NM_SO_GLOBAL_HEADER_SIZE +
                      NM_SO_DATA_HEADER_SIZE +
                      len;
  vec[2].iov_len = NM_SO_MAX_UNEXPECTED - len;

  p_so_pw->pw.v_nb = 3;

  *pp_so_pw = p_so_pw;

 out:
    return err;
}

int
nm_so_pw_check_optimistic(struct nm_so_pkt_wrap *p_so_pw,
			  int *status)
{
  struct nm_so_data_header *h;

  if(p_so_pw->optimistic_recv) {

    h = p_so_pw->pw.v->iov_base + NM_SO_GLOBAL_HEADER_SIZE;

    if(h->proto_id != p_so_pw->pw.proto_id || h->seq != p_so_pw->pw.seq) {
      /* Bad luck! We have to copy the data back into the header zone !*/

      memcpy((void *)h + NM_SO_DATA_HEADER_SIZE,
	     p_so_pw->pw.v[1].iov_base,
	     p_so_pw->pw.v[1].iov_len);

      p_so_pw->pw.v[0].iov_len = NM_SO_MAX_UNEXPECTED +
	NM_SO_GLOBAL_HEADER_SIZE + NM_SO_DATA_HEADER_SIZE;

      p_so_pw->pw.v_nb = 1;

      *status = NM_SO_OPTIMISTIC_FAILED;
    } else {
      /* Cool! We guessed right! We just have to mark the header "read". */
      h->proto_id = NM_SO_PROTO_DATA_UNUSED;

      *status = NM_SO_OPTIMISTIC_SUCCESS;
    }
  } else
    *status = NM_SO_OPTIMISTIC_FAILED;

  return NM_ESUCCESS;
}

int
nm_so_pw_iterate_over_headers(struct nm_so_pkt_wrap *p_so_pw,
			      nm_so_pw_data_handler data_handler,
			      nm_so_pw_rdv_handler rdv_handler,
			      nm_so_pw_ack_handler ack_handler,
			      void *arg)
{
  struct iovec *vec;
  void *ptr;
  uint32_t remaining_len;
  uint8_t proto_id;
  struct nm_so_data_header *dh;
  void *data = NULL;

  /* Each 'unread' header will increment this counter. When the
     counter will reach 0 again, the packet wrapper can (and will) be
     safely destroyed */
  p_so_pw->header_ref_count = 0;

  vec = p_so_pw->pw.v;
  ptr = vec->iov_base;
  remaining_len = ((struct nm_so_global_header *)ptr)->len -
    NM_SO_GLOBAL_HEADER_SIZE;
  ptr += NM_SO_GLOBAL_HEADER_SIZE;

  while(remaining_len) {

    /* Decode header */
    proto_id = *(uint8_t *)ptr;

    if(proto_id >= NM_SO_PROTO_DATA_FIRST ||
       proto_id == NM_SO_PROTO_DATA_UNUSED) {
      /* Data header */
      dh = ptr;

      ptr += NM_SO_DATA_HEADER_SIZE;

      if(proto_id != NM_SO_PROTO_DATA_UNUSED) {
	/* Retrieve data location */
	data = ptr;

	if(dh->skip) {
	  uint16_t skip = dh->skip;
	  uint32_t rlen;
	  struct iovec *v;

	  v = vec;
	  rlen = (v->iov_base + v->iov_len) - data;
	  if (skip < rlen)
	    data += skip;
	  else {
	    do {
	      skip -= rlen;
	      v++;
	      rlen = v->iov_len;
	    } while (skip >= rlen);
	    data = v->iov_base + skip;
	  }
	}
      }

      remaining_len -= NM_SO_DATA_HEADER_SIZE + dh->len;

      /* We must recall ptr if necessary */
      if(remaining_len && dh->skip == 0) {
	uint16_t skip;
	uint32_t rlen;

	skip = dh->len;
	rlen = (vec->iov_base + vec->iov_len) - ptr;
	if(skip < rlen)
	  ptr += skip;
	else {
	  do {
	    skip -= rlen;
	    vec++;
	    rlen = vec->iov_len;
	  } while (skip >= rlen);
	  ptr = vec->iov_base + skip;
	}
      }

      if(proto_id != NM_SO_PROTO_DATA_UNUSED && data_handler) {
	int r = data_handler(p_so_pw,
			     data, dh->len,
			     dh->proto_id, dh->seq,
			     arg);
	if(r == NM_SO_HEADER_MARK_READ)
	  dh->proto_id = NM_SO_PROTO_DATA_UNUSED;
	else
	  p_so_pw->header_ref_count++;
      }
    } else
      switch(proto_id) {
      case NM_SO_PROTO_RDV:
	{
	  union nm_so_generic_ctrl_header *ch = ptr;

	  ptr += NM_SO_CTRL_HEADER_SIZE;
	  remaining_len -= NM_SO_CTRL_HEADER_SIZE;

	  if(rdv_handler) {
	    int r = rdv_handler(p_so_pw,
				ch->r.tag_id, ch->r.seq,
				arg);
	    if(r == NM_SO_HEADER_MARK_READ)
	      ch->r.proto_id = NM_SO_PROTO_CTRL_UNUSED;
	    else
	      p_so_pw->header_ref_count++;
	  }
	}
	break;
      case NM_SO_PROTO_ACK:
	{
	  union nm_so_generic_ctrl_header *ch = ptr;

	  ptr += NM_SO_CTRL_HEADER_SIZE;
	  remaining_len -= NM_SO_CTRL_HEADER_SIZE;

	  if(ack_handler) {
	    int r = ack_handler(p_so_pw,
				ch->a.tag_id, ch->a.seq, ch->a.track_id,
				arg);
	    if(r == NM_SO_HEADER_MARK_READ)
	      ch->a.proto_id = NM_SO_PROTO_CTRL_UNUSED;
	    else
	      p_so_pw->header_ref_count++;
	  }
	}
	break;
      case NM_SO_PROTO_CTRL_UNUSED:

	ptr += NM_SO_CTRL_HEADER_SIZE;
	remaining_len -= NM_SO_CTRL_HEADER_SIZE;

	break;
      default:
	return -NM_EINVAL;

      } /* switch */

  } /* while */

  if(!p_so_pw->header_ref_count)
    nm_so_pw_free(p_so_pw);

  return NM_ESUCCESS;
}
