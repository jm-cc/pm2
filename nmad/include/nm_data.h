/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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

/** @file nm_data.h High-level data manipulation through iterators
 */

#ifndef NM_DATA_H
#define NM_DATA_H

#include <assert.h>

/** maximum size of content descriptor for nm_data */
#define _NM_DATA_CONTENT_SIZE 32

struct nm_data_s;
/** function apply to each data chunk upon traversal */
typedef void (*nm_data_apply_t)(void*ptr, nm_len_t len, void*_ref);

/** funtion to traverse data with app layout, i.e. map op
 * @param p_data data descriptor
 * @param apply function to apply to all chunks
 * @param _context context pointer given to apply function
 */
typedef void (*nm_data_traversal_t)(const void*_content, nm_data_apply_t apply, void*_context);

/** memcpy function for data descriptors- copy from descriptor to given buffer */
typedef void (*nm_data_copy_from_t)(const void*_content, nm_len_t offset, nm_len_t len, void*destbuf);

/** memcpy function for data descriptors- copy to descriptor from buffer */
typedef void (*nm_data_copy_to_t)(const void*_content, nm_len_t offset, nm_len_t len, const void*srcbuf);

/** set of operations available on data type.
 * May be NULL except p_traversal
 */
struct nm_data_ops_s
{
  nm_data_traversal_t p_traversal;
  nm_data_copy_from_t p_copyfrom;
  nm_data_copy_to_t   p_copyto;
};

/** block of static properties for a given data descriptor */
struct nm_data_properties_s
{
  int blocks;    /**< number of blocks; -1 if properties ar not initialized */
  nm_len_t size; /**< total size in bytes (accumulator) */
  int is_contig; /**< is data contiguous */
  void*base_ptr; /**< base pointer, in case data is contiguous */
};

/** a data descriptor, used to pack/unpack data from app layout to/from contiguous buffers */
struct nm_data_s
{
  struct nm_data_ops_s ops;
  struct nm_data_properties_s props;
  /** placeholder for type-dependant content */
  char _content[_NM_DATA_CONTENT_SIZE];
};
#define NM_DATA_TYPE(ENAME, TYPE, OPS)					\
  static inline void nm_data_##ENAME##_set(struct nm_data_s*p_data, TYPE value) \
  {									\
    p_data->ops = *(OPS);						\
    p_data->props.blocks = -1;						\
    assert(sizeof(TYPE) <= _NM_DATA_CONTENT_SIZE);			\
    TYPE*p_content = (TYPE*)&p_data->_content[0];			\
    *p_content = value;							\
  }

/** data descriptor for contiguous data 
 */
struct nm_data_contiguous_s
{
  void*ptr;
  nm_len_t len;
};
const struct nm_data_ops_s nm_data_ops_contiguous;
NM_DATA_TYPE(contiguous, struct nm_data_contiguous_s, &nm_data_ops_contiguous);
/** data descriptor for iov data
 */
struct nm_data_iov_s
{
  struct iovec*v;
  int n;
};
const struct nm_data_ops_s nm_data_ops_iov;
NM_DATA_TYPE(iov, struct nm_data_iov_s, &nm_data_ops_iov);

/** helper function to apply iterator to data
 */
static inline void nm_data_traversal_apply(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context)
{
  (*p_data->ops.p_traversal)((void*)p_data->_content, apply, _context);
}

void nm_data_chunk_extractor_traversal(const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len,
				       nm_data_apply_t apply, void*_context);

void nm_data_aggregator_traversal(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context);

const struct nm_data_properties_s*nm_data_properties_get(struct nm_data_s*p_data);

static inline nm_len_t nm_data_size(const struct nm_data_s*p_data)
{
  const struct nm_data_properties_s*p_props = nm_data_properties_get((struct nm_data_s*)p_data);
  return p_props->size;
}

/** copy chunk of data from user layout to contiguous buffer */
void nm_data_copy_from(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, void*destbuf);

/** copy chunk of data from contiguous buffer to user layout */
void nm_data_copy_to(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, const void*srcbuf);

#endif /* NM_DATA_H */

