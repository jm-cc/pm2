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
#define _NM_DATA_GENERATOR_SIZE 32

struct nm_data_s;
/** function apply to each data chunk upon traversal */
typedef void (*nm_data_apply_t)(void*ptr, nm_len_t len, void*_ref);

/** funtion to traverse data with app layout, i.e. map op
 * @param p_data data descriptor
 * @param apply function to apply to all chunks
 * @param _context context pointer given to apply function
 */
typedef void (*nm_data_traversal_t)(const void*_content, nm_data_apply_t apply, void*_context);

struct nm_data_generator_s
{
  char _bytes[_NM_DATA_GENERATOR_SIZE];
};

/** initializes a generator (i.e. semi-coroutine) for the given data type */
typedef void (*nm_data_generator_init_t)(const struct nm_data_s*p_data, void*_generator);
/** chunk of data returned by generators */
struct nm_data_chunk_s
{
  void*ptr;
  nm_len_t len;
};
#define NM_DATA_CHUNK_NULL ((struct nm_data_chunk_s){ .ptr = NULL, .len = 0 })
static inline int nm_data_chunk_isnull(const struct nm_data_chunk_s*p_chunk)
{
  return (p_chunk->ptr == NULL);
}

/** returns next data chunk for the given generator.
 * _content and _generator must be consistent accross calls
 * no error checking is done
 */
typedef struct nm_data_chunk_s (*nm_data_generator_next_t)(const struct nm_data_s*p_data, void*_generator);

/** destroys resources allocated by generator */
typedef void (*nm_data_generator_destroy_t)(const struct nm_data_s*p_data, void*_generator);

/* forward declarations, functions used in macro below */
/** @internal */
void                   nm_data_generic_generator(const struct nm_data_s*p_data, void*_generator);
/** @internal */
struct nm_data_chunk_s nm_data_generic_next(const struct nm_data_s*p_data, void*_generator);
/** @internal */
void                   nm_data_coroutine_generator(const struct nm_data_s*p_data, void*_generator);
/** @internal */
struct nm_data_chunk_s nm_data_coroutine_next(const struct nm_data_s*p_data, void*_generator);
/** @internal */
void                   nm_data_coroutine_generator_destroy(const struct nm_data_s*p_data, void*_generator);

#define NM_DATA_USE_COROUTINE
#ifdef NM_DATA_USE_COROUTINE
#define nm_data_default_generator         &nm_data_coroutine_generator
#define nm_data_default_next              &nm_data_coroutine_next
#define nm_data_default_generator_destroy &nm_data_coroutine_generator_destroy
#else
#define nm_data_default_generator         &nm_data_generic_generator
#define nm_data_default_next              &nm_data_generic_next
#define nm_data_default_generator_destroy NULL
#endif

/** set of operations available on data type.
 * either p_traversal or p_generator may be NULL (not both)
 */
struct nm_data_ops_s
{
  nm_data_traversal_t         p_traversal;
  nm_data_generator_init_t    p_generator;
  nm_data_generator_next_t    p_next;
  nm_data_generator_destroy_t p_generator_destroy;
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
  struct nm_data_ops_s ops;              /**< collection of iterators */
  struct nm_data_properties_s props;     /**< cache for properties */
  char _content[_NM_DATA_CONTENT_SIZE];  /**< placeholder for type-dependant content */
};
/** macro to generate typed functions to init/access data fields */
#define NM_DATA_TYPE(ENAME, CONTENT_TYPE, OPS)				\
  static inline void nm_data_##ENAME##_set(struct nm_data_s*p_data, CONTENT_TYPE value) \
  {									\
    p_data->ops = *(OPS);						\
    if(p_data->ops.p_generator == NULL)					\
      {									\
	p_data->ops.p_generator = nm_data_default_generator;		\
	p_data->ops.p_next      = nm_data_default_next;			\
	p_data->ops.p_generator_destroy = nm_data_default_generator_destroy; \
      }									\
    p_data->props.blocks = -1;						\
    assert(sizeof(CONTENT_TYPE) <= _NM_DATA_CONTENT_SIZE);		\
    CONTENT_TYPE*p_content = (CONTENT_TYPE*)&p_data->_content[0];	\
    *p_content = value;							\
  }									\
  static inline CONTENT_TYPE*nm_data_##ENAME##_content(const struct nm_data_s*p_data) \
  {									\
    return (CONTENT_TYPE*)p_data->_content;				\
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

/* ** helper functions for generator */

/** build a new generator for the given data type */
static inline void nm_data_generator_init(const struct nm_data_s*p_data, struct nm_data_generator_s*p_generator)
{
  assert(p_data->ops.p_generator != NULL);
  (*p_data->ops.p_generator)(p_data, p_generator);
}

/** get the next chunk of data */
static inline struct nm_data_chunk_s nm_data_generator_next(const struct nm_data_s*p_data, struct nm_data_generator_s*p_generator)
{
  assert(p_data->ops.p_next != NULL);
  const struct nm_data_chunk_s chunk = (*p_data->ops.p_next)(p_data, p_generator);
  return chunk;
}

/** destroy the generator after use */
static inline void nm_data_generator_destroy(const struct nm_data_s*p_data, struct nm_data_generator_s*p_generator)
{
  if(p_data->ops.p_generator_destroy)
    (*p_data->ops.p_generator_destroy)(p_data, p_generator);
}

void nm_data_chunk_extractor_traversal(const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len,
				       nm_data_apply_t apply, void*_context);

void nm_data_aggregator_traversal(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context);

/** returns the properties for the data */
const struct nm_data_properties_s*nm_data_properties_get(const struct nm_data_s*p_data);

static inline nm_len_t nm_data_size(const struct nm_data_s*p_data)
{
  const struct nm_data_properties_s*p_props = nm_data_properties_get((struct nm_data_s*)p_data);
  return p_props->size;
}

/** copy chunk of data from user layout to contiguous buffer */
void nm_data_copy_from(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, void*destbuf);

/** copy chunk of data from contiguous buffer to user layout */
void nm_data_copy_to(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, const void*srcbuf);

typedef struct nm_data_slicer_s
{
  const struct nm_data_s*p_data;
  struct nm_data_chunk_s pending_chunk;
  struct nm_data_generator_s generator;
  nm_len_t done;
  struct nm_data_slicer_coroutine_s*p_coroutine;
} nm_data_slicer_t;
#define NM_DATA_SLICER_NULL ((struct nm_data_slicer_s){ .p_data = NULL })

static inline int nm_data_slicer_isnull(const nm_data_slicer_t*p_slicer)
{
  return (p_slicer->p_data == NULL);
}


#define USE_COROUTINE_SLICER

#ifdef USE_COROUTINE_SLICER
#define nm_data_slicer_init      nm_data_slicer_coroutine_init
#define nm_data_slicer_forward   nm_data_slicer_coroutine_forward
#define nm_data_slicer_copy_from nm_data_slicer_coroutine_copy_from
#define nm_data_slicer_copy_to   nm_data_slicer_coroutine_copy_to
#define nm_data_slicer_destroy   nm_data_slicer_coroutine_destroy
#else
#define nm_data_slicer_init      nm_data_slicer_generator_init
#define nm_data_slicer_forward   nm_data_slicer_generator_forward
#define nm_data_slicer_copy_from nm_data_slicer_generator_copy_from
#define nm_data_slicer_copy_to   nm_data_slicer_generator_copy_to
#define nm_data_slicer_destroy   nm_data_slicer_generator_destroy
#endif

void nm_data_slicer_init(nm_data_slicer_t*p_slicer, const struct nm_data_s*p_data);

void nm_data_slicer_forward(nm_data_slicer_t*p_slicer, nm_len_t offset);

void nm_data_slicer_copy_from(nm_data_slicer_t*p_slicer, void*dest_ptr, nm_len_t slice_len);

void nm_data_slicer_copy_to(nm_data_slicer_t*p_slicer, const void*src_ptr, nm_len_t slice_len);

void nm_data_slicer_destroy(nm_data_slicer_t*p_slicer);

#endif /* NM_DATA_H */

