/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
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

#define MAMI_CHECK_MALLOC(ptr) {if (!ptr) { fprintf(stderr, "mami_malloc failed\n"); return 1; }}
#define MAMI_CHECK_RETURN_VALUE(err, message) {if (err < 0) { perror(message); return 1; }}
#define MAMI_CHECK_RETURN_VALUE_IS(err, value, message) {if (err >= 0 || errno != value) { perror(message); return 1; }}

