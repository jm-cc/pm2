
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

#ifndef ISOADDR_ATTR_IS_DEF
#define ISOADDR_ATTR_IS_DEF

#define isoaddr_attr_set_status(attr, _status) (attr)->status = _status

#define isoaddr_attr_set_protocol(attr, _protocol) (attr)->protocol = _protocol

#define isoaddr_attr_set_alignment(attr, _align) (attr)->align = _align

#define isoaddr_attr_set_atomic(attr) (attr)->atomic = 1;

#define isoaddr_attr_set_non_atomic(attr) (attr)->atomic = 0;

#define isoaddr_attr_set_special(attr) (attr)->special = 1

#define isoaddr_attr_get_status(attr) ((attr)->status)

#define isoaddr_attr_get_protocol(attr) ((attr)->protocol)

#define isoaddr_attr_get_alignment(attr) ((attr)->align)

#define isoaddr_attr_atomic(attr) ((attr)->atomic)

#define isoaddr_attr_special(attr) ((attr)->special)

#endif


