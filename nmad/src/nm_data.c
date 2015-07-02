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

void nm_data_traversal_contiguous(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_contiguous_s*p_contiguous = _content;
  void*ptr = p_contiguous->ptr;
  const nm_len_t len = p_contiguous->len;
  (*apply)(ptr, len, _context);
}

/* ********************************************************* */

void nm_data_traversal_iov(const void*_content, nm_data_apply_t apply, void*_context)
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
      if((p_context->chunk_ptr != NULL) || (ptr == NULL))
	{
	  (*p_context->apply)(p_context->chunk_ptr, p_context->chunk_len, p_context->_context);
	}
      p_context->chunk_ptr = ptr;
      p_context->chunk_len = len;
    }
}
void nm_data_aggregator_traversal(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context)
{
  struct nm_data_aggregator_s context = { .chunk_ptr = NULL, .chunk_len = 0, .apply = apply, ._context = _context };
  nm_data_traversal_apply(p_data, &nm_data_aggregator_apply, &context);
  if(context.chunk_ptr != NULL)
    {
      /* flush last pending chunk */
      (*context.apply)(context.chunk_ptr, context.chunk_len, context._context);
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
      struct nm_data_chunk_extractor_s chunk_extractor = 
	{ .chunk_offset = chunk_offset, .chunk_len = chunk_len, .done = 0,
	  .apply = apply, ._context = _context };
      nm_data_aggregator_traversal(p_data, &nm_data_chunk_extractor_apply, &chunk_extractor);
    }
  else
    {
      (*apply)(NULL, 0, _context);
    }
}

/* ********************************************************* */

/** compute data size
 */
struct nm_data_size_s
{
  nm_len_t size; /**< size accumulator */
};
static void nm_data_size_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_size_s*p_context = _context;
  p_context->size += len;
}
nm_len_t nm_data_size(const struct nm_data_s*p_data)
{
  struct nm_data_size_s size = { .size = 0 };
  nm_data_traversal_apply(p_data, &nm_data_size_apply, &size);
  return size.size;
}

/* ********************************************************* */

/** compute various data properties
 */
struct nm_data_properties_context_s
{
  nm_len_t size; /**< total size in bytes (accumulator) */
  int blocks;    /**< number of blocks */
  int is_contig; /**< is contiguous, up to the current position */
  void*blockend; /**< end of previous block*/
};
static void nm_data_properties_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_properties_context_s*p_context = _context;
  p_context->size += len;
  p_context->blocks += 1;
  if(p_context->is_contig)
    {
      if((p_context->blockend != NULL) && (ptr != p_context->blockend))
	p_context->is_contig = 0;
      p_context->blockend = ptr + len;
    }
}
void nm_data_properties_compute(const struct nm_data_s*p_data, nm_len_t*p_len, int*p_blocks, int*p_is_contig)
{
  struct nm_data_properties_context_s properties = { .size = 0, .blocks = 0, .is_contig = 1, .blockend = NULL };
  nm_data_traversal_apply(p_data, &nm_data_properties_apply, &properties);
  if(p_len)
    *p_len = properties.size;
  if(p_blocks)
    *p_blocks = properties.blocks;
  if(p_is_contig)
    *p_is_contig = properties.is_contig;
}

/* ********************************************************* */

/** copy data from network buffer (contiguous) to user layout
 */
struct nm_data_copy_s
{
  const void*ptr; /**< source buffer (contiguous) */
};
static void nm_data_copy_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_copy_s*p_context = _context;
  memcpy(ptr, p_context->ptr, len);
  p_context->ptr += len;
}
/** copy chunk of data to user layout */
void nm_data_copy(const struct nm_data_s*p_data, nm_len_t chunk_offset, const void *ptr, nm_len_t len)
{
  if(len > 0)
    {
      struct nm_data_copy_s copy = { .ptr = ptr };
      struct nm_data_chunk_extractor_s chunk_extractor = 
	{ .chunk_offset = chunk_offset, .chunk_len = len, .done = 0, 
	  .apply = &nm_data_copy_apply, ._context = &copy };
      nm_data_traversal_apply(p_data, &nm_data_chunk_extractor_apply, &chunk_extractor);
    }
}


/* ********************************************************* */

#define NM_DATA_IOV_THRESHOLD 64

/** flatten iterator-based data to struct nm_pkt_wrap
 */
struct nm_data_flatten_s
{
  struct nm_pkt_wrap*p_pw; /**< the pw to fill */
};
static void nm_data_flatten_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_flatten_s*p_context = _context;
  struct nm_pkt_wrap*p_pw = p_context->p_pw;
  struct iovec*v0 = &p_pw->v[0];
  if(len < NM_DATA_IOV_THRESHOLD)
    {
#warning TODO- aggregate with previous chunk if possible ####################################

#warning TODO- remove nm_so_pw_finalize (save pointer to last header to set PROTO_LAST)
      
      /* small data in header */
      uint16_t*p_len = v0->iov_base + v0->iov_len;
      assert(len <= UINT16_MAX);
      *p_len = len;
      memcpy(v0->iov_base + v0->iov_len + sizeof(uint16_t), ptr, len);
      v0->iov_len += len + sizeof(uint16_t);
      p_pw->length += len + sizeof(uint16_t);
    }
  else
    {
      /* data in iovec */
      uint16_t*p_len =  v0->iov_base + v0->iov_len;
      uint16_t*p_skip = p_len + 1;
      assert(len <= UINT16_MAX);
      *p_len = len;
#warning TODO- cache the skip value ##################################
      nm_len_t skip = 0;
      int i;
      for(i = 1; i < p_pw->v_nb; i++)
	{
	  skip += p_pw->v[i].iov_len;
	}
      struct iovec*v = nm_pw_grow_iovec(p_context->p_pw);
      v->iov_base = ptr;
      v->iov_len = len;
      *p_skip = skip;
      v0->iov_len += 2 * sizeof(uint16_t);
      p_pw->length += len + 2 * sizeof(uint16_t);
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
  struct nm_data_flatten_s flatten = { .p_pw = p_pw };
  nm_data_chunk_extractor_traversal(p_data, chunk_offset, chunk_len, &nm_data_flatten_apply, &flatten);
  assert(v0->iov_len <= UINT16_MAX);
  h->hlen = (v0->iov_base + v0->iov_len) - (void*)h;
}


/* ********************************************************* */

/** Unpack data from packed pkt to nm_data
*/
struct nm_data_pkt_unpacker_s
{
  const struct nm_header_pkt_data_s*h;
  const void*ptr; /**< source pkt buffer */
  const void*v1_base; /**< v0 + skip, base of iov-based fragments */
};
static void nm_data_pkt_unpack_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_pkt_unpacker_s*p_context = _context;
  const uint16_t*p_len = p_context->ptr;
  const uint16_t rlen = *p_len;
#warning TODO- only symmetrical types for now
  if(rlen != len)
    {
      padico_fatal("# nmad: non-symetrical pack/unpack unsupported for data_pkt format; received len = %d; expected = %d.\n", rlen, len);
    }
  assert(rlen == len);
  if(rlen < NM_DATA_IOV_THRESHOLD)
    {
      const void*rbuffer = p_context->ptr + 2;
      memcpy(ptr, rbuffer, rlen);
      p_context->ptr += rlen + 2;
    }
  else
    {
      const uint16_t*p_skip = p_context->ptr + 2;
      const void*rbuffer = p_context->v1_base + *p_skip;
      memcpy(ptr, rbuffer, rlen);
      p_context->ptr += 4;
    }
}

/** unpack from pkt format to data format */
void nm_data_pkt_unpack(const struct nm_data_s*p_data, const struct nm_header_pkt_data_s*h, const struct nm_pkt_wrap*p_pw)
{
  struct nm_data_pkt_unpacker_s data_unpacker =
    { .h = h, .ptr = h + 1, .v1_base = p_pw->v[0].iov_base + nm_header_global_skip(p_pw) };
  nm_data_traversal_apply(p_data, &nm_data_pkt_unpack_apply, &data_unpacker);
}
