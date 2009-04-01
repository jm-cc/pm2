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

#include <pm2_common.h>

#include <nm_private.h>

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
/* Per-'iovec entry' allocation flags (exclusive) */
/** @name IOV entry allocation

Flag constants to indicate whether a given iovec entry has been
dynamically or statically allocated.
*/
/*@{*/
/** Entry is has been allocated statically and should not be freed explicitely.
 */
#define NM_SO_ALLOC_STATIC       0
/** Entry is has been allocated dynamically and should be freed after use.
 */
#define NM_SO_ALLOC_DYNAMIC      1
/*@}*/
#endif

/* Global priv_flags */
/** @name Global private flags

Flag constants for the flag field of pw.
*/
/*@{*/
/** Headerless pkt, if set.
 */
#define NM_SO_NO_HEADER          1

/** Pkt with a dynamically allocated IOV, if set.
 */
#define NM_SO_IOV_ALLOC_DYNAMIC  2

/** Pkt has been allocated for reception, if set.
 */
#define NM_SO_RECV_PW            4
/*@}*/

/** Fast packet allocator constant for initial number of entries. */
#if defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
#  define INITIAL_PKT_NUM  4
#else
#  define INITIAL_PKT_NUM  16
#endif

/** Allocator struct for headerless SO pkt wrapper.
 */
static p_tbx_memory_t nm_so_pw_nohd_mem = NULL;

/** Allocator struct for send SO pkt wrapper.
 */
static p_tbx_memory_t nm_so_pw_send_mem = NULL;

/** Allocator struct for recv SO pkt wrapper.
 */
static p_tbx_memory_t nm_so_pw_recv_mem = NULL;

/* Some ugly macros (for convenience only) */
#define nm_so_iov_flags(p_pw, i)  ((p_pw)->nm_v[i].priv_flags)

/** Initialize the fast allocator structs for SO pkt wrapper.
 *
 *  @param p_core a pointer to the NM core object.
 *  @return The NM status.
 */
int nm_so_pw_init(struct nm_core *p_core TBX_UNUSED)
{
  tbx_malloc_init(&nm_so_pw_nohd_mem,
		  sizeof(struct nm_pkt_wrap),
		  INITIAL_PKT_NUM, "nmad/.../sched_opt/nm_pkt_wrap/nohd");

  tbx_malloc_init(&nm_so_pw_send_mem,
		  sizeof(struct nm_pkt_wrap) + NM_SO_MAX_UNEXPECTED,
		  INITIAL_PKT_NUM, "nmad/.../sched_opt/nm_pkt_wrap/send");

  /* There's no need to use a separate allocator for send and recv ! */
  nm_so_pw_recv_mem = nm_so_pw_send_mem;

  return NM_ESUCCESS;
}

/** Cleanup the fast allocator structs for SO pkt wrapper.
 *
 *  @return The NM status.
 */
int nm_so_pw_exit(void)
{
  tbx_malloc_clean(nm_so_pw_nohd_mem);
  tbx_malloc_clean(nm_so_pw_send_mem);

  return NM_ESUCCESS;
}

static void nm_so_pw_reset(struct nm_pkt_wrap * __restrict__ p_pw)
{
  p_pw->drv_priv	= NULL;
}


static void nm_so_pw_raz(struct nm_pkt_wrap *p_pw)
{
  int i;

  p_pw->p_drv  = NULL;
  p_pw->trk_id = NM_TRK_NONE;
  p_pw->p_gate = NULL;
  p_pw->p_gdrv = NULL;
  p_pw->proto_id = 0;
  p_pw->seq = 0;
  p_pw->drv_priv   = NULL;

  p_pw->pkt_priv_flags = 0;
  p_pw->length = 0;
  p_pw->iov_flags = 0;

  p_pw->data = NULL;
  p_pw->len_v = NULL;

  p_pw->v_size          = 0;
  p_pw->v_first         = 0;
  p_pw->v_nb            = 0;

  p_pw->v = NULL;
  p_pw->nm_v = NULL;

#ifdef PIO_OFFLOAD
  p_pw->data_to_offload = tbx_false;
#endif

  p_pw->header_ref_count = 0;
  p_pw->pending_skips = 0;

  for(i = 0; i < NM_SO_PREALLOC_IOV_LEN; i++){
    p_pw->prealloc_v[i].iov_len  = 0;
    p_pw->prealloc_v[i].iov_base = NULL;
  }

  p_pw->chunk_offset = 0;
  p_pw->is_completed = tbx_false;
  p_pw->datatype_copied_buf = tbx_false;
}



/** Allocate a suitable pkt wrapper for various SO purposes.
 *
 *  @param flags the flags indicating what pkt wrapper is needed.
 *    - @c NM_SO_DATA_DONT_USE_HEADER:  whether to prepare a global header or not.
 *    - @c NM_SO_DATA_PREPARE_RECV:  whether to prepare a send or receive pkt wrapper.
 *  @param pp_pw a pointer to the pkt wrapper pointer where to store the result.
 *  @return The NM status.
 */
int nm_so_pw_alloc(int flags, struct nm_pkt_wrap **pp_pw)
{
  struct nm_pkt_wrap *p_pw;
  int err = NM_ESUCCESS;

  if(flags & NM_SO_DATA_DONT_USE_HEADER)
    {
      /* dont use header -> no dedicated buffer for the headers */
      if(flags & NM_SO_DATA_PREPARE_RECV) 
	{
	  /* receive, no header */
	  p_pw = tbx_malloc(nm_so_pw_recv_mem);
	  if (!p_pw)
	    {
	      err = -NM_ENOMEM;
	      goto out;
	    }
	  
	  nm_so_pw_reset(p_pw);
	  nm_so_pw_raz(p_pw);
	  
	  /* io vector */
	  p_pw->v = p_pw->prealloc_v;
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
	  p_pw->nm_v = p_pw->prealloc_nm_v;
#endif
	  p_pw->v_first = 0;
	  p_pw->v_size = NM_SO_PREALLOC_IOV_LEN;
	  p_pw->v_nb = 1;
	  
	  /* pw flags */
	  p_pw->pkt_priv_flags = NM_SO_RECV_PW;
	  
	  p_pw->is_completed = tbx_true;
	  
	  /* first entry: pkt header */
	  p_pw->prealloc_v->iov_base = p_pw->buf;
	  p_pw->prealloc_v->iov_len = (p_pw->length = NM_SO_MAX_UNEXPECTED);
	  
	} 
      else
	{
	  /* send, no header -> for the large messages */
	  
	  p_pw = tbx_malloc(nm_so_pw_nohd_mem);
	  if (!p_pw)
	    {
	      err = -NM_ENOMEM;
	      goto out;
	    }
	  
	  nm_so_pw_reset(p_pw);
	  nm_so_pw_raz(p_pw);
	  
	  /* io vector */
	  p_pw->v = p_pw->prealloc_v;
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
	  p_pw->nm_v = p_pw->prealloc_nm_v;
#endif
	  p_pw->v_first = 0;
	  p_pw->v_size = NM_SO_PREALLOC_IOV_LEN;
	  p_pw->v_nb = 0;
	  
	  p_pw->is_completed = tbx_true;
	  
	  /* pw flags */
	  p_pw->pkt_priv_flags = NM_SO_NO_HEADER;
	  
	  /* pkt is empty for now */
	  p_pw->length = 0;
	}
    }
  else 
    {
      /* send, with header -> one dedicated buffer for the headers */
      p_pw = tbx_malloc(nm_so_pw_send_mem);
      if (!p_pw)
	{
	  err = -NM_ENOMEM;
	  goto out;
	}
      
      nm_so_pw_reset(p_pw);
      nm_so_pw_raz(p_pw);
      
      /* io vector */
      p_pw->v = p_pw->prealloc_v;
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
      p_pw->nm_v = p_pw->prealloc_nm_v;
#endif
      p_pw->v_first = 0;
      p_pw->v_size = NM_SO_PREALLOC_IOV_LEN;
      p_pw->v_nb = 1;
      
      /* first entry: global header */
      p_pw->prealloc_v->iov_base = p_pw->buf;
      p_pw->prealloc_v->iov_len = NM_SO_GLOBAL_HEADER_SIZE;
      
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
      nm_so_iov_flags(p_pw, 0) = NM_SO_ALLOC_STATIC;
#endif
      
      p_pw->is_completed = tbx_true;
      
      /* cumulated length: global header length) */
      p_pw->length = NM_SO_GLOBAL_HEADER_SIZE;
      
      /* pw flags */
      p_pw->pkt_priv_flags = 0;
      
      p_pw->pending_skips = 0;
    }
  
  INIT_LIST_HEAD(&p_pw->link);

  NM_SO_TRACE_LEVEL(3,"creating a pw %p (%d bytes)\n", p_pw, p_pw->length);

  *pp_pw = p_pw;

 out:
  return err;
}

/** Free a pkt wrapper and related structs.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @return The NM status.
 */
int nm_so_pw_free(struct nm_pkt_wrap *p_pw)
{
  int err;
  int flags = p_pw->pkt_priv_flags;

  NM_SO_TRACE_LEVEL(3,"destructing the pw %p\n", p_pw);

#ifdef PIOMAN
  piom_req_free(&p_pw->inst);
#endif

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
  /* Clean iov entries first */
  {
    int i;

    for(i = 0; i < p_pw->v_nb; i++)
      if(nm_so_iov_flags(p_pw, i) == NM_SO_ALLOC_DYNAMIC)
	TBX_FREE(p_pw->v[i].iov_base);
  }
#endif

  /* Then clean whole iov */
  if(flags & NM_SO_IOV_ALLOC_DYNAMIC) {
    TBX_FREE(p_pw->v);
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
    TBX_FREE(p_pw->nm_v);
#endif
  }

  /* Finally clean packet wrapper itself */
  if(flags & NM_SO_RECV_PW){
    tbx_free(nm_so_pw_recv_mem, p_pw);
    NM_SO_TRACE_LEVEL(3,"pw %p is removed from nm_so_pw_recv_mem\n", p_pw);
  }
  else if(flags & NM_SO_NO_HEADER) {
    NM_SO_TRACE_LEVEL(3,"pw %p is removed from nm_so_pw_nohd_mem\n", p_pw);
    tbx_free(nm_so_pw_nohd_mem, p_pw);
  }
  else {
    NM_SO_TRACE_LEVEL(3,"pw %p is removed from nm_so_pw_send_mem\n", p_pw);
    tbx_free(nm_so_pw_send_mem, p_pw);
  }

  err = NM_ESUCCESS;
  return err;
}

// Attention!! split only enable when the p_pw is finished
int nm_so_pw_split(struct nm_pkt_wrap *p_pw,
                   struct nm_pkt_wrap **pp_pw2,
                   uint32_t offset)
{
  struct nm_pkt_wrap *p_pw2 = NULL;
  int idx_pw, len, iov_offset, nb_entries;
  int idx_pw2 = 0;
  int err;

  nm_so_pw_finalize(p_pw);

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_pw2);
  if(err != NM_ESUCCESS)
    goto err;

  /* Copy the pw part */
  p_pw2->p_drv = p_pw->p_drv;
  p_pw2->trk_id = p_pw->trk_id;
  p_pw2->p_gate = p_pw->p_gate;
  p_pw2->p_gdrv = p_pw->p_gdrv;
  p_pw2->proto_id = p_pw->proto_id;
  p_pw2->seq = p_pw->seq;
  p_pw2->length = p_pw->length - offset;

  p_pw->length = offset;

  nb_entries = p_pw->v_nb;

  for(idx_pw = 0, len = 0;
      idx_pw < nb_entries;
      len += p_pw->v[idx_pw].iov_len, idx_pw++){

    if(len + p_pw->v[idx_pw].iov_len == offset){
      idx_pw++;
      goto next;
    }


    if(len + p_pw->v[idx_pw].iov_len > offset){
      iov_offset = offset - len;

      p_pw2->v_first = idx_pw;
      p_pw2->v[idx_pw].iov_len  = p_pw->v[idx_pw].iov_len  - iov_offset;
      p_pw2->v[idx_pw].iov_base = p_pw->v[idx_pw].iov_base + iov_offset;
      p_pw2->v_nb = nb_entries - idx_pw;

      p_pw->v[idx_pw].iov_len = iov_offset;
      p_pw->v_nb = idx_pw + 1;

      idx_pw++;
      goto next2;
    }
  }

 next:
  p_pw2->v_nb = nb_entries - idx_pw;
  p_pw->v_nb = idx_pw;


  while(idx_pw < nb_entries){
    p_pw2->v[idx_pw2].iov_len  = p_pw->v[idx_pw].iov_len;
    p_pw2->v[idx_pw2].iov_base = p_pw->v[idx_pw].iov_base;

    p_pw->v[idx_pw].iov_len = 0;
    p_pw->v[idx_pw].iov_base = NULL;

    idx_pw++;
    idx_pw2++;
  }
  goto out;

 next2:
  while(idx_pw < nb_entries){
    p_pw2->v[idx_pw].iov_len = p_pw->v[idx_pw].iov_len;
    p_pw2->v[idx_pw].iov_base = p_pw->v[idx_pw].iov_base;

    p_pw->v[idx_pw].iov_len = 0;
    p_pw->v[idx_pw].iov_base = NULL;

    idx_pw++;
  }

 out:
  p_pw2->chunk_offset = p_pw->chunk_offset+offset;

  // shortcut on the first iov entry
  p_pw2->data   = p_pw2->v[p_pw2->v_first].iov_base;
  p_pw2->v_size = p_pw2->v[p_pw2->v_first].iov_len;

  *pp_pw2 = p_pw2;

 err:
  return err;
}

/** Append a fragment of data to the pkt wrapper being built.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @param proto_id the protocol id which generated the fragment.
 *  @param seq the sequence number of the fragment.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param flags the flags controlling the way the fragment is appended.
 *  @return The NM status.
 */
int nm_so_pw_add_data(struct nm_pkt_wrap *p_pw,
		      nm_tag_t proto_id, uint8_t seq,
		      const void *data, uint32_t len,
		      uint32_t offset,
		      uint8_t is_last_chunk,
		      int flags)
{
  int err;
  struct iovec *vec;

  if(!(flags & NM_SO_DATA_DONT_USE_HEADER)) {
    /* Add data with header */

    /* the headers are always in the first entries of the struct iovec tab */
    vec = p_pw->prealloc_v;

    if(tbx_likely(!(flags & NM_SO_DATA_IS_CTRL_HEADER))) {
      /* Small data case */
      struct nm_so_data_header *h;

      /* Add header */
      h = vec->iov_base + vec->iov_len;

      h->proto_id = proto_id;
      h->seq = seq;
      h->chunk_offset = offset;
      h->is_last_chunk = is_last_chunk;

      vec->iov_len += NM_SO_DATA_HEADER_SIZE;

      if(tbx_likely(flags & NM_SO_DATA_USE_COPY)) {

        /* Data immediately follows its header */
	uint32_t size;
        size = nm_so_aligned(len);

	h->skip = 0;
        h->len = len;

        if(len) {
#ifdef PIO_OFFLOAD
          p_pw->data_to_offload = tbx_true;

          p_pw->v[p_pw->v_nb].iov_base = data;
          p_pw->v[p_pw->v_nb].iov_len  = len;
          p_pw->v_nb ++;

#else
	  memcpy(vec->iov_base + vec->iov_len, data, len);
#endif
          vec->iov_len += size;
	}

	p_pw->length += NM_SO_DATA_HEADER_SIZE + size;

      } else {
	/* Data are handled by a separate iovec entry */

	struct iovec *dvec;

	dvec = p_pw->v + p_pw->v_nb;
#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
	nm_so_iov_flags(p_pw, p_pw->v_nb) = NM_SO_ALLOC_STATIC;
#endif

	dvec->iov_base = (void*)data;
	dvec->iov_len = len;

	/* We don't know yet the gap between header and data, so we
	   temporary store the iovec index as the 'skip' value */
	h->skip = p_pw->v_nb;
	p_pw->pending_skips++;
	h->len = len;
        h->chunk_offset = offset;
        h->is_last_chunk = is_last_chunk;

        p_pw->length += NM_SO_DATA_HEADER_SIZE + len;

	p_pw->v_nb++;

      }

    } else {

      /* Data actually references a ctrl header. We simply append it
	 to the previous other headers. */
      union nm_so_generic_ctrl_header *src, *dst;

      src = (void*)data;
      dst = vec->iov_base + vec->iov_len;

      /* Copy ctrl header */
      *dst = *src;

      vec->iov_len += NM_SO_CTRL_HEADER_SIZE;

      p_pw->length += NM_SO_CTRL_HEADER_SIZE;

    }

  } else {
    /* Data chunk added 'as is': simply use a new iovec entry */

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
    nm_so_iov_flags(p_pw, p_pw->v_nb) = NM_SO_ALLOC_STATIC;
#endif

    vec = p_pw->v + p_pw->v_nb++;
    vec->iov_base = (void*)data;
    vec->iov_len = len;

    p_pw->proto_id = proto_id;
    p_pw->seq = seq;

    p_pw->length += len;

  }

  err = NM_ESUCCESS;

  return err;
}

int nm_so_pw_store_datatype(struct nm_pkt_wrap *p_pw,
			    nm_tag_t proto_id, uint8_t seq,
			    uint32_t len, const struct DLOOP_Segment *segp)
{
  p_pw->proto_id = proto_id;
  p_pw->seq = seq;

  p_pw->length += len;

  p_pw->segp = (struct DLOOP_Segment*)segp;
  p_pw->datatype_offset = 0;

  return NM_ESUCCESS;
}

// function dedicated to the datatypes which do not require a rendezvous
int nm_so_pw_add_datatype(struct nm_pkt_wrap *p_pw,
			  nm_tag_t proto_id, uint8_t seq,
			  uint32_t len, const struct DLOOP_Segment *segp)
{
  uint32_t size = 0;

  if(len) {
    size = nm_so_aligned(len);

    DLOOP_Offset first = 0;
    DLOOP_Offset last  = len;

    {
      struct nm_so_data_header *h;
      struct iovec *vec =  p_pw->prealloc_v;
      /* Add header */
      h = vec->iov_base + vec->iov_len;

      h->proto_id = proto_id;
      h->seq = seq;
      h->is_last_chunk = 1;
      h->len = len;
      h->skip = 0;
      h->chunk_offset = 0;

      vec->iov_len += NM_SO_DATA_HEADER_SIZE;
      p_pw->length += NM_SO_DATA_HEADER_SIZE;
    }

    /* unfold datatype into contiguous memory */
    CCSI_Segment_pack(segp, first, &last, p_pw->v[0].iov_base + p_pw->v[0].iov_len);

    p_pw->v[0].iov_len  += size;
    p_pw->length += size;
  }

  return NM_ESUCCESS;
}

int nm_so_pw_copy_contiguously_datatype(struct nm_pkt_wrap *p_pw,
                                    nm_tag_t proto_id, uint8_t seq,
                                    uint32_t len, struct DLOOP_Segment *segp)
{
  void *buf = NULL;
  DLOOP_Offset first = 0;
  DLOOP_Offset last  = len;
  struct iovec *vec = NULL;
  int err;

  buf = TBX_MALLOC(len);

  err = nm_so_pw_add_data(p_pw,
                          proto_id, seq,
                          buf, len,
                          0, 1,
                          NM_SO_DATA_DONT_USE_HEADER);

  vec = p_pw->v;

  /* unfold datatype into contiguous memory */
  CCSI_Segment_pack(segp, first, &last, vec->iov_base);

  vec->iov_len = len;

  err = NM_ESUCCESS;
  return err;
}

#ifdef PIO_OFFLOAD
int nm_so_pw_offloaded_finalize(struct nm_pkt_wrap *p_pw)
{
  int err = NM_ESUCCESS;
  unsigned long remaining_bytes;
  unsigned long to_skip;
  void *ptr;
  struct iovec *vec;
  struct iovec *last_treated_vec = NULL;
  tbx_bool_t separate_iovec = tbx_false;

  /* update the length field of the global header
   */
  ((struct nm_so_global_header *)(p_pw->v->iov_base))->len =
    p_pw->length;

  /* Fix the 'skip' fields */
  if(!p_pw->pending_skips && !p_pw->data_to_offload){
    /* Were're done */
    goto out;
  }

  vec = p_pw->v;

  ptr = vec->iov_base + NM_SO_GLOBAL_HEADER_SIZE;
  remaining_bytes = vec->iov_len - NM_SO_GLOBAL_HEADER_SIZE;
  to_skip = 0;
  last_treated_vec = vec;

  do {
    if (*(nm_tag_t *)ptr >= NM_SO_PROTO_DATA_FIRST) {
      /* Data header */
      struct nm_so_data_header *h = ptr;

      if(h->skip == 0) {
        ptr += NM_SO_DATA_HEADER_SIZE;
        last_treated_vec++;

        memcpy(ptr, last_treated_vec->iov_base, h->len);
        last_treated_vec->iov_len = 0;

        ptr += nm_so_aligned(h->len);
        remaining_bytes -= NM_SO_DATA_HEADER_SIZE + nm_so_aligned(h->len);

      } else {
        /* Data occupy a separate iovec entry */
	ptr += NM_SO_DATA_HEADER_SIZE;
	remaining_bytes -= NM_SO_DATA_HEADER_SIZE;
	h->skip = remaining_bytes + to_skip;

        last_treated_vec++;
        to_skip += last_treated_vec->iov_len;

        p_pw->pending_skips--;
        separate_iovec = tbx_true;
      }

    } else {
      /* Ctrl header */
      ptr += NM_SO_CTRL_HEADER_SIZE;
      remaining_bytes -= NM_SO_CTRL_HEADER_SIZE;
    }

  } while (p_pw->pending_skips);

  /* all the used iov entries have been copied in contiguous */
  if(!separate_iovec){
    p_pw->v_nb = 1;
  }

 out:
  return err;
}
#endif

/** Finalize the incremental building of the packet.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @return The NM status.
 */
int nm_so_pw_finalize(struct nm_pkt_wrap *p_pw)
{
  int err = NM_ESUCCESS;
  unsigned long remaining_bytes;
  unsigned long to_skip;
  void *ptr;
  struct iovec *vec;
  struct iovec *last_treated_vec = NULL;

  /* check if the packet travels headerles
   */
  if(p_pw->pkt_priv_flags & NM_SO_NO_HEADER)
    /* We're done */
    goto out;

  /* update the length field of the global header
   */
  ((struct nm_so_global_header *)(p_pw->v->iov_base))->len =
    p_pw->length;

#ifdef PIO_OFFLOAD
  /* the finalize (actually the copy of the data) is deported in a tasklet
     it is now done in the nm_so_pw_copy_offloaded_data_and_finalize function
  */
  if(p_pw->data_to_offload){
    goto out;
  }
#endif

  /* Fix the 'skip' fields */
  if(!p_pw->pending_skips){
    /* Were're done */
    goto out;
  }

  vec = p_pw->v;

  ptr = vec->iov_base + NM_SO_GLOBAL_HEADER_SIZE;
  remaining_bytes = vec->iov_len - NM_SO_GLOBAL_HEADER_SIZE;
  to_skip = 0;
  last_treated_vec = vec;

  do {

    if (*(nm_tag_t *)ptr >= NM_SO_PROTO_DATA_FIRST) {
      /* Data header */

      struct nm_so_data_header *h = ptr;

      if(h->skip == 0) {
	/* Data immediately follows */
	ptr += NM_SO_DATA_HEADER_SIZE + nm_so_aligned(h->len);
        remaining_bytes -= NM_SO_DATA_HEADER_SIZE + nm_so_aligned(h->len);

      } else {
        /* Data occupy a separate iovec entry */
	ptr += NM_SO_DATA_HEADER_SIZE;
	remaining_bytes -= NM_SO_DATA_HEADER_SIZE;
	h->skip = remaining_bytes + to_skip;

        last_treated_vec++;
        to_skip += last_treated_vec->iov_len;

        p_pw->pending_skips--;
      }

    } else {
      /* Ctrl header */
      ptr += NM_SO_CTRL_HEADER_SIZE;
      remaining_bytes -= NM_SO_CTRL_HEADER_SIZE;
    }

  } while (p_pw->pending_skips);

 out:
  return err;
}

/** Iterate over the fields of a freshly received ctrl pkt.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @param data_handler the inline data callback.
 *  @param rdv_handler the rdv request callback.
 *  @param ack_handler the rdv ack callback.
 *  @return The NM status.
 */
int nm_so_pw_iterate_over_headers(struct nm_pkt_wrap *p_pw,
				  nm_so_pw_data_handler data_handler,
				  nm_so_pw_rdv_handler rdv_handler,
				  nm_so_pw_ack_handler ack_handler,
				  nm_so_pw_ack_chunk_handler ack_chunk_handler)
{
  struct iovec *vec;
  void *ptr;
  unsigned long remaining_len;
  nm_tag_t proto_id;
  struct nm_so_data_header *dh;
  void *data = NULL;

#ifdef NMAD_QOS
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pw->p_gate->strategy_receptacle;
  unsigned ack_received = 0;
#endif /* NMAD_QOS */

  /* Each 'unread' header will increment this counter. When the
     counter will reach 0 again, the packet wrapper can (and will) be
     safey destroyed */
  p_pw->header_ref_count = 0;

  vec = p_pw->v;
  ptr = vec->iov_base;
  remaining_len = ((struct nm_so_global_header *)ptr)->len - NM_SO_GLOBAL_HEADER_SIZE;

  ptr += NM_SO_GLOBAL_HEADER_SIZE;

  while(remaining_len) {
    /* Decode header */
    proto_id = *(nm_tag_t *)ptr;

    if(proto_id >= NM_SO_PROTO_DATA_FIRST ||
       proto_id == NM_SO_PROTO_DATA_UNUSED) {

      /* Data header */
      dh = ptr;

      ptr += NM_SO_DATA_HEADER_SIZE;

      if(proto_id != NM_SO_PROTO_DATA_UNUSED) {
	/* Retrieve data location */
	unsigned long skip = dh->skip;

	data = ptr;

	if(dh->len) {
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

      remaining_len -= NM_SO_DATA_HEADER_SIZE + nm_so_aligned(dh->len);

      /* We must recall ptr if necessary */
      if(dh->skip == 0){ // data are just after the header
        ptr += nm_so_aligned(dh->len);
      }  // else the next header is just behind

      if(proto_id != NM_SO_PROTO_DATA_UNUSED && data_handler) {
	int r = data_handler(p_pw,
			     data,
			     dh, dh->len, dh->proto_id, dh->seq, dh->chunk_offset, dh->is_last_chunk);

	if (r == NM_SO_HEADER_MARK_READ) {
	  dh->proto_id = NM_SO_PROTO_DATA_UNUSED;
	} else {
	  p_pw->header_ref_count++;
	}

      }

    } else
      switch(proto_id) {
      case NM_SO_PROTO_RDV:
	{
	  union nm_so_generic_ctrl_header *ch = ptr;

	  ptr += NM_SO_CTRL_HEADER_SIZE;
	  remaining_len -= NM_SO_CTRL_HEADER_SIZE;

	  if (rdv_handler) {
	    int r = rdv_handler(p_pw,
                                ch,
                                ch->r.tag_id, ch->r.seq,
                                ch->r.len, ch->r.chunk_offset, ch->r.is_last_chunk);
	    if (r == NM_SO_HEADER_MARK_READ) {
	      ch->r.proto_id = NM_SO_PROTO_CTRL_UNUSED;
	    } else {
	      p_pw->header_ref_count++;
	    }

	  } else {
	    NM_SO_TRACE("Sent completed of a RDV on tag = %d, seq = %d\n", ch->r.tag_id, ch->r.seq);
	  }
	}
	break;
      case NM_SO_PROTO_ACK:
	{
	  union nm_so_generic_ctrl_header *ch = ptr;

	  ptr += NM_SO_CTRL_HEADER_SIZE;
	  remaining_len -= NM_SO_CTRL_HEADER_SIZE;

	  if(ack_handler) {
	    int r;

#ifdef NMAD_QOS
	    if(strategy->driver->ack_callback != NULL)
	      {
		ack_received = 1;
		r = strategy->driver->ack_callback(strategy->_status,
						   p_pw,
						   ch->a.tag_id,
						   ch->a.seq,
						   ch->a.track_id,
						   0);
	      }
	    else
#endif /* NMAD_QOS */
              r = ack_handler(p_pw,
                              ch->a.tag_id, ch->a.seq, ch->a.track_id,
                              ch->a.chunk_offset);

	    if (r == NM_SO_HEADER_MARK_READ) {
	      ch->a.proto_id = NM_SO_PROTO_CTRL_UNUSED;
	    } else {
	      p_pw->header_ref_count++;
	    }

	  } else {
            NM_SO_TRACE("Sent completed of an ACK on tag = %d, seq = %d, offset = %d\n", ch->a.tag_id, ch->a.seq, ch->a.chunk_offset);
	  }
	}
	break;

      case NM_SO_PROTO_MULTI_ACK:
        {
          union nm_so_generic_ctrl_header *ch = ptr;

          nm_tag_t *proto_id  = &ch->ma.proto_id;
          uint8_t nb_chunks = ch->ma.nb_chunks;
          nm_tag_t tag_id = ch->ma.tag_id;
          uint8_t seq = ch->ma.seq;
          uint32_t chunk_offset = ch->ma.chunk_offset;
          tbx_bool_t header_ref_count_incremented = tbx_false;
          int i;

          ptr += NM_SO_CTRL_HEADER_SIZE;
          remaining_len -= NM_SO_CTRL_HEADER_SIZE;
          ch = ptr;

          if (!ack_chunk_handler) {
            NM_SO_TRACE("Sent completed of a multi-ack\n");

            for(i = 0; i < nb_chunks; i++) {
              ptr += NM_SO_CTRL_HEADER_SIZE;
              remaining_len -= NM_SO_CTRL_HEADER_SIZE;
            }

          } else {
            NM_SO_TRACE("NM_SO_PROTO_MULTI_ACK received - nb_acks = %u, tag_id = %u, seq = %u\n", nb_chunks, tag_id, seq);

            for(i = 0; i < nb_chunks; i++) {

              if (ch->ac.proto_id == NM_SO_PROTO_CTRL_UNUSED)
                goto next;

              if (ch->ac.proto_id != NM_SO_PROTO_ACK_CHUNK) {
                TBX_FAILURE("Invalid header - ACK_CHUNK expected");
              }

              NM_SO_TRACE("NM_SO_PROTO_ACK_CHUNK received - tag_id = %u, seq = %u - trk_id = %u, chunk_len =%u\n", tag_id, seq, ch->ac.trk_id, ch->ac.chunk_len);

              int r = ack_chunk_handler(p_pw, tag_id, seq, chunk_offset,
                                        ch->ac.trk_id, ch->ac.chunk_len);

              chunk_offset += ch->ac.chunk_len;

              if (r == NM_SO_HEADER_MARK_READ) {
                ch->a.proto_id = NM_SO_PROTO_CTRL_UNUSED;
              } else {
                header_ref_count_incremented = tbx_true;
                p_pw->header_ref_count++;
              }

            next:
              ptr += NM_SO_CTRL_HEADER_SIZE;
              ch = ptr;
              remaining_len -= NM_SO_CTRL_HEADER_SIZE;
            }

            if(header_ref_count_incremented == tbx_false){
              *proto_id = NM_SO_PROTO_CTRL_UNUSED;
            } else {
              NM_SO_TRACE("Multi-ack not completly treated\n");
            }
          }
        }
        break;

      case NM_SO_PROTO_CTRL_UNUSED:
        {
          ptr += NM_SO_CTRL_HEADER_SIZE;
          remaining_len -= NM_SO_CTRL_HEADER_SIZE;

          break;
        }

      default:
	return -NM_EINVAL;

      } /* switch */
  } /* while */

#ifdef NMAD_QOS
  if(ack_received)
    strategy->driver->ack_callback(strategy->_status,
				   p_pw, 0, 0, 128, 1);
#endif /* NMAD_QOS */

  if (!p_pw->header_ref_count){
    nm_so_pw_free(p_pw);
  }

  return NM_ESUCCESS;
}
