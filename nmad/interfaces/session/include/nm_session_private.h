/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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

/** @internal Private state of a session object. */
struct nm_session_s
{
  struct nm_core*p_core;       /**< the current nmad object */
  nm_session_hash_t hash_code; /**< hash of session label, used as ID on the wire */
  const char*label;            /**< plain text session name */
  void*p_sr_session;           /**< reference to be used by 'sendrecv' per-session state */
  void (*p_sr_destructor)(struct nm_session_s*); /**< function to call before session destroy */
};

#endif /* NM_SESSION_PRIVATE_H */
