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

struct flypack_data_s
{
  void*buf;
  nm_len_t len;
};

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
	  (*apply)(content->buf + chunk_size * i, chunk_size, _context);
	}
    }
  else
    {
      (*apply)(content->buf, content->len, _context);
    }
}

NM_DATA_TYPE(flypack, struct flypack_data_s, &flypack_traversal);
