
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#ifndef POINTERS_EST_DEF
#define POINTERS_EST_DEF

typedef union {
   char zone[8];
   long bidon;
} pointer;

#define to_pointer(arg, p_addr)  *((void **)p_addr) = arg

#define to_any_t(p_addr) (*((void **)p_addr))

#endif
