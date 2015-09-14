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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/** @file flypack- primitives for packing data on the fly.
 */

#include <nm_core_interface.h>

#define FLYPACK_CHUNK_SIZE 16

static void*flypack_buf = NULL;
static nm_len_t flypack_len = 0;

struct flypack_data_s
{
  void*buf;
  nm_len_t len;
};

static void flypack_traversal(const void*_content, nm_data_apply_t apply, void*_context);
static void flypack_generator_init(struct nm_data_s*p_data, void*_generator);
static struct nm_data_chunk_s flypack_generator_next(struct nm_data_s*p_data, void*_generator);
static void flypack_copy_from(const void*_content, nm_len_t offset, nm_len_t len, void*destbuf);
static void flypack_copy_to(const void*_content, nm_len_t offset, nm_len_t len, const void*srcbuf);

const static struct nm_data_ops_s flypack_ops =
  {
    .p_traversal = &flypack_traversal,
    .p_generator = &flypack_generator_init,
    .p_next      = &flypack_generator_next
  };
NM_DATA_TYPE(flypack, struct flypack_data_s, &flypack_ops);


static nm_len_t flypack_chunk_size(void)
{
  static nm_len_t chunk_size = 0;
  if(chunk_size == 0)
    {
      const char*s_chunk_size = getenv("FLYPACK_CHUNK_SIZE");
      if(s_chunk_size != NULL)
	{
	  chunk_size = atoi(s_chunk_size);
	}
      else
	{
	  chunk_size = FLYPACK_CHUNK_SIZE;
	}
      fprintf(stderr, "# nmad: flypack chunk size = %d\n", (int)chunk_size);
    }
  return chunk_size;
}

static void flypack_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct flypack_data_s*content = _content;
  const nm_len_t chunk_size = flypack_chunk_size();
  if(content->len >= chunk_size)
    {
      const int chunk_count = content->len / chunk_size;
      int i;
      for(i = 0; i < chunk_count; i++)
	{
	  (*apply)(content->buf + 2 * chunk_size * i, chunk_size, _context);
	}
    }
  else
    {
      (*apply)(content->buf, content->len, _context);
    }
}

struct flypack_generator_s
{
  nm_len_t chunk_size;
  nm_len_t i;
};
static void flypack_generator_init(struct nm_data_s*p_data, void*_generator)
{
  const struct flypack_data_s*p_content = nm_data_flypack_content(p_data);
  struct flypack_generator_s*p_generator = _generator;
  p_generator->chunk_size = flypack_chunk_size();
  p_generator->i = 0;
}
static struct nm_data_chunk_s flypack_generator_next(struct nm_data_s*p_data, void*_generator)
{
  const struct flypack_data_s*p_content = nm_data_flypack_content(p_data);
  struct flypack_generator_s*p_generator = _generator;
  struct nm_data_chunk_s chunk =
    {
      .ptr = p_content->buf + 2 * p_generator->chunk_size * p_generator->i,
      .len = (p_generator->chunk_size > p_content->len) ? p_content->len : p_generator->chunk_size
    };
  p_generator->i++;
  return chunk;
}



