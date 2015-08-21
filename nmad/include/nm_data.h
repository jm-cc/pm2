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

/** maximum size of content descriptor for nm_data */
#define _NM_DATA_CONTENT_SIZE 32

struct nm_data_s;
/** function apply to each data chunk upon traversal */
typedef void (*nm_data_apply_t)(void*ptr, nm_len_t len, void*_ref);

/** traversal function for data descriptors */
typedef void (*nm_data_traversal_t)(const void*_content, nm_data_apply_t apply, void*_context);

/** memcpy function for data descriptors- copy from descriptor to given buffer */
typedef void (*nm_data_copy_from_t)(const void*_content, nm_len_t offset, nm_len_t len, void*destbuf);

/** memcpy function for data descriptors- copy to descriptor from buffer */
typedef void (*nm_data_copy_to_t)(const void*_content, nm_len_t offset, nm_len_t len, const void*srcbuf);

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
  /** funtion to traverse data with app layout, i.e. map
   * @param p_data data descriptor
   * @param apply function to apply to all chunks
   * @param _context context pointer given to apply function
   */
  nm_data_traversal_t p_traversal;
  struct nm_data_properties_s props;
  /** placeholder for type-dependant content */
  char _content[_NM_DATA_CONTENT_SIZE];
};
#define NM_DATA_TYPE(ENAME, TYPE, TRAVERSAL)				\
  static inline void nm_data_##ENAME##_set(struct nm_data_s*p_data, TYPE value) \
  {									\
    p_data->p_traversal = TRAVERSAL;					\
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
void nm_data_traversal_contiguous(const void*_content, nm_data_apply_t apply, void*_context);
NM_DATA_TYPE(contiguous, struct nm_data_contiguous_s, &nm_data_traversal_contiguous);
/** data descriptor for iov data
 */
struct nm_data_iov_s
{
  struct iovec*v;
  int n;
};
void nm_data_traversal_iov(const void*_content, nm_data_apply_t apply, void*_context);
NM_DATA_TYPE(iov, struct nm_data_iov_s, &nm_data_traversal_iov);

/** helper function to apply iterator to data
 */
static inline void nm_data_traversal_apply(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context)
{
  (*p_data->p_traversal)((void*)p_data->_content, apply, _context);
}

void nm_data_chunk_extractor_traversal(const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len,
				       nm_data_apply_t apply, void*_context);

void nm_data_aggregator_traversal(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context);

const struct nm_data_properties_s*nm_data_properties_get(struct nm_data_s*p_data);

nm_len_t nm_data_size(const struct nm_data_s*p_data);


#endif /* NM_DATA_H */

