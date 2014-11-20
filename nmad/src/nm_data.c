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

void nm_data_traversal_contiguous(void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_contiguous_s*p_contiguous = _content;
  void*ptr = p_contiguous->ptr;
  const nm_len_t len = p_contiguous->len;
  (*apply)(ptr, len, _context);
}

/* ********************************************************* */

void nm_data_traversal_iov(void*_content, nm_data_apply_t apply, void*_context)
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

/** filter function application to a delimited sub-set of data
 */
struct nm_data_chunk_extractor_s
{
  nm_len_t chunk_offset; /**< offset for begin of copy at destination */
  nm_len_t chunk_len;    /**< length to copy */
  nm_len_t done;         /**< offset done so far at destination */
  nm_data_apply_t apply; /**< composed function to apply to chunk */
  void*_context;         /**< context for composaed apply function */
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
	(len - block_offset) : (chunk_offset + chunk_len - p_context->done);
      (*p_context->apply)(ptr + block_offset, block_len, p_context->_context);
    }
  p_context->done += len;
}
/** apply function to only a given chunk of data */
void nm_data_chunk_extractor_traversal(const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len,
				       nm_data_apply_t apply, void*_context)
{
  struct nm_data_chunk_extractor_s chunk_extractor = 
    { .chunk_offset = chunk_offset, .chunk_len = chunk_len, .done = 0,
      .apply = apply, ._context = _context };
  nm_data_traversal_apply(p_data, &nm_data_chunk_extractor_apply, &chunk_extractor);
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

