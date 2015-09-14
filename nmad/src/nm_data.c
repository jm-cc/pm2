/*
 * NewMadeleine
 * Copyright (C) 2014 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULA R PURPOSE.  See the GNU
 * General Public License for more details.
 */

/** @file nm_data.c High-level data manipulation through iterators
 */

#include <nm_private.h>

PADICO_MODULE_HOOK(NewMad_Core);


/* ********************************************************* */
/* ** contiguous data */

struct nm_data_contiguous_generator_s
{
  nm_len_t done; /**< amount of data processed so far */
};
static void nm_data_contiguous_generator(struct nm_data_s*p_data, void*_generator)
{
  const struct nm_data_contiguous_s*p_contiguous = nm_data_contiguous_content(p_data);
  struct nm_data_contiguous_generator_s*p_generator = _generator;
  p_generator->done = 0;
}
static struct nm_data_chunk_s nm_data_contiguous_next(struct nm_data_s*p_data, void*_generator)
{
  const struct nm_data_contiguous_s*p_contiguous = nm_data_contiguous_content(p_data);
  struct nm_data_contiguous_generator_s*p_generator = _generator;
  struct nm_data_chunk_s chunk = { .ptr = p_contiguous->ptr + p_generator->done, .len = p_contiguous->len - p_generator->done };
  p_generator->done += chunk.len;
  assert(p_generator->done <= p_contiguous->len);
  return chunk;
}
static void nm_data_contiguous_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_contiguous_s*p_contiguous = _content;
  void*ptr = p_contiguous->ptr;
  const nm_len_t len = p_contiguous->len;
  (*apply)(ptr, len, _context);
}
const struct nm_data_ops_s nm_data_ops_contiguous =
  {
    .p_traversal = &nm_data_contiguous_traversal,
    .p_generator = &nm_data_contiguous_generator,
    .p_next      = &nm_data_contiguous_next
  };

/* ********************************************************* */
/* ** iovec data */

struct nm_data_iov_generator_s
{
  int i; /**< current index in v */
};
static void nm_data_iov_generator(struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_iov_generator_s*p_generator = _generator;
  p_generator->i = 0;
}
static struct nm_data_chunk_s nm_data_iov_next(struct nm_data_s*p_data, void*_generator)
{
  const struct nm_data_iov_s*p_iov = nm_data_iov_content(p_data);
  struct nm_data_iov_generator_s*p_generator = _generator;
  const struct iovec*v = &p_iov->v[p_generator->i];
  struct nm_data_chunk_s chunk = { .ptr = v->iov_base, .len = v->iov_len };
  assert(p_generator->i < p_iov->n);
  p_generator->i++;
  return chunk;
}
static void nm_data_iov_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_iov_s*p_iov = _content;
  const struct iovec*const v = p_iov->v;
  const int n = p_iov->n;
  int i;
  for(i = 0; i < n; i++)
    {
      (*apply)(v[i].iov_base, v[i].iov_len, _context);
    }
}
const struct nm_data_ops_s nm_data_ops_iov =
  {
    .p_traversal = &nm_data_iov_traversal,
    .p_generator = &nm_data_iov_generator,
    .p_next      = &nm_data_iov_next
  };

/* ********************************************************* */

/** filter function application on aggregated contiguous chunks
 */
struct nm_data_aggregator_s
{
  void*chunk_ptr;         /**< pointer on current chunk begin */
  nm_len_t chunk_len;     /**< length of current chunk beeing processed */
  nm_data_apply_t apply;  /**< composed function to apply to chunk */
  void*_context;          /**< context for composed apply function */
};
static void nm_data_aggregator_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_aggregator_s*p_context = _context;
  if((ptr != NULL) && (ptr == p_context->chunk_ptr + p_context->chunk_len))
    {
      /* contiguous with current chunk */
      p_context->chunk_len += len;
    }
  else
    {
      /* not contiguous; flush prev chunk, set current block as current chunk */
      if(p_context->chunk_len != NM_LEN_UNDEFINED)
	{
	  (*p_context->apply)(p_context->chunk_ptr, p_context->chunk_len, p_context->_context);
	}
      p_context->chunk_ptr = ptr;
      p_context->chunk_len = len;
    }
}
void nm_data_aggregator_traversal(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_properties_s*p_props = nm_data_properties_get(p_data);
  if(p_props->is_contig)
    {
      (*apply)(p_props->base_ptr, p_props->size, _context);
    }
  else
    {
      struct nm_data_aggregator_s context = { .chunk_ptr = NULL, .chunk_len = NM_LEN_UNDEFINED, .apply = apply, ._context = _context };
      nm_data_traversal_apply(p_data, &nm_data_aggregator_apply, &context);
      if(context.chunk_len != NM_LEN_UNDEFINED)
	{
	  /* flush last pending chunk */
	  (*context.apply)(context.chunk_ptr, context.chunk_len, context._context);
	}
      else
	{
	  /* zero-length data */
	  (*context.apply)(NULL, 0, context._context);
	}
    }
 }

/* ********************************************************* */

/** filter function application to a delimited sub-set of data
 */
struct nm_data_chunk_extractor_s
{
  nm_len_t chunk_offset; /**< offset for begin of copy at destination */
  nm_len_t chunk_len;    /**< length to copy */
  nm_len_t done;         /**< offset done so far at destination */
  nm_data_apply_t apply; /**< composed function to apply to chunk */
  void*_context;         /**< context for composed apply function */
};
static void nm_data_chunk_extractor_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_chunk_extractor_s*p_context = _context;
  const nm_len_t chunk_offset = p_context->chunk_offset;
  const nm_len_t chunk_len = p_context->chunk_len;
  if( (!(p_context->done + len <= p_context->chunk_offset))                   /* data before chunk- do nothing */
      &&
      (!(p_context->done >= p_context->chunk_offset + p_context->chunk_len))) /* data after chunk- do nothing */
    {
      /* data in chunk */
      const nm_len_t block_offset = (p_context->done < chunk_offset) ? (chunk_offset - p_context->done) : 0;
      const nm_len_t block_len = (chunk_offset + chunk_len > p_context->done + len) ? 
	(len - block_offset) : (chunk_offset + chunk_len - p_context->done - block_offset);
      (*p_context->apply)(ptr + block_offset, block_len, p_context->_context);
    }
  p_context->done += len;
}
/** apply function to only a given chunk of data */
void nm_data_chunk_extractor_traversal(const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len,
				       nm_data_apply_t apply, void*_context)
{
  if(chunk_len != 0)
    {
      if((chunk_offset == 0) && (chunk_len == nm_data_size(p_data)))
	{
	  nm_data_traversal_apply(p_data, apply, _context);
	}
	else
	  {
	    struct nm_data_chunk_extractor_s chunk_extractor = 
	      { .chunk_offset = chunk_offset, .chunk_len = chunk_len, .done = 0,
		.apply = apply, ._context = _context };
	    nm_data_traversal_apply(p_data, &nm_data_chunk_extractor_apply, &chunk_extractor);
	  }
    }
  else
    {
      (*apply)(NULL, 0, _context);
    }
}

/* ********************************************************* */
/* ** generic generator, using traversal */
/* use with care, complexity is n^2 */

struct nm_data_generic_traversal_s
{
  const int target;
  int done;
  struct nm_data_chunk_s chunk;
};
static void nm_data_generic_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_generic_traversal_s*p_generic = _context;
  if(p_generic->target == p_generic->done)
    {
      p_generic->chunk.ptr = ptr;
      p_generic->chunk.len = len;
    }
  p_generic->done++;
}
struct nm_data_generic_generator_s
{
  int i;
};
void nm_data_generic_generator(struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_generic_generator_s*p_generator = _generator;
  p_generator->i = 0;
}
struct nm_data_chunk_s nm_data_generic_next(struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_generic_generator_s*p_generator = _generator;
  struct nm_data_generic_traversal_s generic_traversal =
    {
      .target = p_generator->i,
      .done   = 0,
      .chunk  = (struct nm_data_chunk_s){.ptr = NULL, .len = 0 }
    };
  nm_data_traversal_apply(p_data, &nm_data_generic_apply, &generic_traversal);
  p_generator->i++;
  return generic_traversal.chunk;
}


/* ********************************************************* */

/* ** sliced generator */

void nm_data_slicer_init(nm_data_slicer_t*p_slicer, struct nm_data_s*p_data)
{
  p_slicer->p_data = p_data;
  p_slicer->pending_chunk = (struct nm_data_chunk_s){ .ptr = NULL, .len = 0 };
  (*p_data->ops.p_generator)(p_data, &p_slicer->generator);
}

void nm_data_slicer_forward(nm_data_slicer_t*p_slicer, nm_len_t offset)
{
  struct nm_data_chunk_s chunk = p_slicer->pending_chunk;
  struct nm_data_s*const p_data = p_slicer->p_data;
  while(offset > 0)
    {
      if(chunk.len == 0)
	chunk = (*p_data->ops.p_next)(p_data, &p_slicer->generator);
      const nm_len_t chunk_len = (chunk.len > offset) ? offset : chunk.len;
      chunk.ptr += chunk_len;
      chunk.len -= chunk_len;
      offset    -= chunk_len;
    }
  p_slicer->pending_chunk = chunk;
}

void nm_data_slicer_copy_from(nm_data_slicer_t*p_slicer, void*dest_ptr, nm_len_t slice_len)
{
  struct nm_data_chunk_s chunk = p_slicer->pending_chunk;
  struct nm_data_s*const p_data = p_slicer->p_data;
  while(slice_len > 0)
    {
      if(chunk.len == 0)
	chunk = (*p_data->ops.p_next)(p_data, &p_slicer->generator);
      const nm_len_t chunk_len = (chunk.len > slice_len) ? slice_len : chunk.len;
      memcpy(dest_ptr, chunk.ptr, chunk_len);
      dest_ptr += chunk_len;
      chunk.ptr += chunk_len;
      chunk.len -= chunk_len;
      slice_len -= chunk_len;
    }
  p_slicer->pending_chunk = chunk;
}

void nm_data_slicer_copy_to(nm_data_slicer_t*p_slicer, const void*src_ptr, nm_len_t slice_len)
{
  struct nm_data_chunk_s chunk = p_slicer->pending_chunk;
  struct nm_data_s*const p_data = p_slicer->p_data;
  while(slice_len > 0)
    {
      if(chunk.len == 0)
	chunk = (*p_data->ops.p_next)(p_data, &p_slicer->generator);
      const nm_len_t chunk_len = (chunk.len > slice_len) ? slice_len : chunk.len;
      memcpy(chunk.ptr, src_ptr, chunk_len);
      src_ptr   += chunk_len;
      chunk.ptr += chunk_len;
      chunk.len -= chunk_len;
      slice_len -= chunk_len;
    }
  p_slicer->pending_chunk = chunk;
}



/* ********************************************************* */


/** compute various data properties
 */
struct nm_data_properties_context_s
{
  void*blockend; /**< end of previous block*/
  struct nm_data_properties_s props;
};
static void nm_data_properties_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_properties_context_s*p_context = _context;
  if(p_context->props.base_ptr == NULL)
    p_context->props.base_ptr = ptr;
  p_context->props.size += len;
  p_context->props.blocks += 1;
  if(p_context->props.is_contig)
    {
      if((p_context->blockend != NULL) && (ptr != p_context->blockend))
	p_context->props.is_contig = 0;
      p_context->blockend = ptr + len;
    }
}

const struct nm_data_properties_s*nm_data_properties_get(struct nm_data_s*p_data)
{
  if(p_data->props.blocks == -1)
    {
      struct nm_data_properties_context_s context = { .blockend = NULL,
						      .props = { .size = 0, .blocks = 0, .is_contig = 1, .base_ptr = NULL }  };
      nm_data_traversal_apply(p_data, &nm_data_properties_apply, &context);
      p_data->props = context.props;
    }
  return &p_data->props;
}

/* ********************************************************* */

/** copy data from network buffer (contiguous) to user layout
 */
struct nm_data_copy_to_s
{
  const void*ptr; /**< source buffer (contiguous) */
};
static void nm_data_copy_to_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_copy_to_s*p_context = _context;
  memcpy(ptr, p_context->ptr, len);
  p_context->ptr += len;
}
/** copy chunk of data to user layout */
void nm_data_copy_to(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, const void*srcbuf)
{
  if(len > 0)
    {
      struct nm_data_copy_to_s copy = { .ptr = srcbuf };
      nm_data_chunk_extractor_traversal(p_data, offset, len, &nm_data_copy_to_apply, &copy);
    }
}

/* ********************************************************* */

/** copy data from network buffer (contiguous) to user layout
 */
struct nm_data_copy_from_s
{
  void*ptr; /**< dest buffer (contiguous) */
};
static void nm_data_copy_from_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_copy_from_s*p_context = _context;
  memcpy(p_context->ptr, ptr, len);
  p_context->ptr += len;
}

void nm_data_copy_from(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, void*destbuf)
{
  if(len > 0)
    {
      struct nm_data_copy_from_s copy = { .ptr = destbuf };
      nm_data_chunk_extractor_traversal(p_data, offset, len, &nm_data_copy_from_apply, &copy);
    }
}

/* ********************************************************* */

#define NM_DATA_IOV_THRESHOLD 64

/** pack iterator-based data to pw with global header
 */
struct nm_data_pkt_packer_s
{
  struct nm_pkt_wrap*p_pw; /**< the pw to fill */
  nm_len_t skip;
  uint16_t*p_prev_len;     /**< previous block */
};
static void nm_data_pkt_pack_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_pkt_packer_s*p_context = _context;
  struct nm_pkt_wrap*p_pw = p_context->p_pw;
  struct iovec*v0 = &p_pw->v[0];
  if(len < NM_DATA_IOV_THRESHOLD)
    {
#warning TODO- remove nm_so_pw_finalize (save pointer to last header to set PROTO_LAST)
      
      /* small data in header */
      assert(len <= UINT16_MAX);
      if((p_context->p_prev_len != NULL) && (*p_context->p_prev_len + len < NM_DATA_IOV_THRESHOLD))
	{
	  *p_context->p_prev_len += len;
	  memcpy(v0->iov_base + v0->iov_len, ptr, len);
	  v0->iov_len += len;
	  p_pw->length += len;
	}
      else
	{
	  uint16_t*p_len = v0->iov_base + v0->iov_len;
	  *p_len = len;
	  memcpy(v0->iov_base + v0->iov_len + sizeof(uint16_t), ptr, len);
	  v0->iov_len += len + sizeof(uint16_t);
	  p_pw->length += len + sizeof(uint16_t);
	  p_context->p_prev_len = p_len;
	}
    }
  else
    {
      /* data in iovec */
      uint16_t*p_len = v0->iov_base + v0->iov_len;
      uint16_t*p_skip = p_len + 1;
      assert(len <= UINT16_MAX);
      *p_len = len;
      if(p_context->skip == 0)
	{
	  nm_len_t skip = 0;
	  int i;
	  for(i = 1; i < p_pw->v_nb; i++)
	    {
	      skip += p_pw->v[i].iov_len;
	    }
	  p_context->skip = skip;
	}
      struct iovec*v = nm_pw_grow_iovec(p_context->p_pw);
      v->iov_base = ptr;
      v->iov_len = len;
      *p_skip = p_context->skip;
      p_pw->v[0].iov_len += 2 * sizeof(uint16_t);
      p_pw->length += len + 2 * sizeof(uint16_t);
      p_context->skip += len;
      p_context->p_prev_len = NULL;
    }
}

void nm_data_pkt_pack(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
		      const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len, uint8_t flags)
{
  struct iovec*v0 = &p_pw->v[0];
  struct nm_header_pkt_data_s*h = v0->iov_base + v0->iov_len;
  nm_header_init_pkt_data(h, tag, seq, flags, chunk_len, chunk_offset);
  v0->iov_len  += NM_HEADER_PKT_DATA_SIZE;
  p_pw->length += NM_HEADER_PKT_DATA_SIZE;
  struct nm_data_pkt_packer_s packer = { .p_pw = p_pw, .skip = 0, .p_prev_len = NULL };
  nm_data_chunk_extractor_traversal(p_data, chunk_offset, chunk_len, &nm_data_pkt_pack_apply, &packer);
  v0 = &p_pw->v[0]; /* pw->v may have moved- update */
  assert(v0->iov_len <= UINT16_MAX);
  h->hlen = (v0->iov_base + v0->iov_len) - (void*)h;
}


/* ********************************************************* */

/** Unpack data from packed pkt to nm_data
*/
struct nm_data_pkt_unpacker_s
{
  const struct nm_header_pkt_data_s*const h;
  const void*ptr;     /**< current source chunk in buffer */
  const void*rem_buf; /**< pointer to remainder from previous block */
  uint16_t rem_len;   /**< length of remainder */
  const void*v1_base; /**< v0 + skip, base of iov-based fragments */
};
static void nm_data_pkt_unpack_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_pkt_unpacker_s*p_context = _context;
  const uint16_t hlen = p_context->h->hlen;
  const void*rbuffer  = p_context->rem_buf;
  uint16_t rlen       = p_context->rem_len;
  const void*rptr     = p_context->ptr;
  while(len > 0)
    {
      if(rbuffer == NULL)
	{
	  /* load next block */
	  const uint16_t*p_len = rptr;
	  rlen = *p_len;
	  if((rptr - (void*)p_context->h) >= hlen)
	    {
	      p_context->rem_buf = NULL;
	      return;
	    }
	  else if(rlen < NM_DATA_IOV_THRESHOLD)
	    {
	      rbuffer = rptr + 2;
	      rptr += rlen + sizeof(uint16_t);
	    }	  
	  else
	    {
	      const uint16_t*p_skip = rptr + 2;
	      rbuffer = p_context->v1_base + *p_skip;
	      rptr += 2 * sizeof(uint16_t);
	    }
	}
      uint16_t blen = rlen;
      const void*const bbuffer = rbuffer;
      if(blen > len)
	{
	  /* consume len bytes, and truncate block */
	  blen = len;
	  rbuffer += len;
	  rlen -= len;
	}
      else
	{
	  /* consume block */
	  rbuffer = NULL;
	}
      memcpy(ptr, bbuffer, blen);
      ptr += blen;
      len -= blen;
    }
  p_context->rem_buf = rbuffer;
  p_context->rem_len = rlen;
  p_context->ptr = rptr;
}

/** unpack from pkt format to data format */
void nm_data_pkt_unpack(const struct nm_data_s*p_data, const struct nm_header_pkt_data_s*h, const struct nm_pkt_wrap*p_pw,
			nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_data_pkt_unpacker_s data_unpacker =
    { .h = h, .ptr = h + 1, .v1_base = p_pw->v[0].iov_base + nm_header_global_skip(p_pw), .rem_buf = NULL };
  nm_data_chunk_extractor_traversal(p_data, chunk_offset, chunk_len, &nm_data_pkt_unpack_apply, &data_unpacker);
}
