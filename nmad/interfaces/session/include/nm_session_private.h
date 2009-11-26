/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifndef NM_SESSION_PRIVATE_H
#define NM_SESSION_PRIVATE_H

#include <stdint.h>
#include <nm_session_interface.h>

/** Private state of a session object.
 */
struct nm_session_s
{
  struct nm_core*p_core;
  uint32_t hash_code;
  const char*label;
};



#endif /* NM_SESSION_PRIVATE_H */
