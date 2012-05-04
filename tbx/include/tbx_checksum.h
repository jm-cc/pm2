/** @file tbx_checksum.h
 *  @brief Various checksum algorithms.
 *  Code gathered from various places. See comments for licenses
 *  and authors.
 */

/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010-2012 "the PM2 team" (see AUTHORS file)
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

#ifndef TBX_CHECKSUM_H
#define TBX_CHECKSUM_H

#include <stdint.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#include "tbx_compiler.h"

typedef uint32_t (*tbx_checksum_func_t)(const void*_data, size_t _len);
typedef uint32_t (*tbx_checksum_copy_func_t)(void*_dest, const void*_src, size_t _len);

/** a checksum algorithm */
struct tbx_checksum_s
{
  const char*short_name;
  const char*name;
  tbx_checksum_func_t func;
  tbx_checksum_copy_func_t checksum_and_copy;
};
typedef const struct tbx_checksum_s*tbx_checksum_t;

extern tbx_checksum_t tbx_checksum_get(const char*short_name);


#endif /* TBX_CHECKSUM_H */

