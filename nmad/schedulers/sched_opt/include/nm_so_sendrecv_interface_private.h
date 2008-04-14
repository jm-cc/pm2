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

#ifndef _NM_SO_SENDRECV_INTERFACE_PRIVATE_H_
#define _NM_SO_SENDRECV_INTERFACE_PRIVATE_H_

#ifdef PIOMAN
#include <pioman.h>
typedef piom_cond_t nm_so_status_t;
#else /* PIOMAN */
typedef volatile uint8_t nm_so_status_t;
#endif /* PIOMAN */

struct nm_so_interface {
  struct nm_core *p_core;
  struct nm_so_sched *p_so_sched;
};

struct nm_so_request_s {
  nm_so_status_t*status;
  uint8_t seq;
  long gate_id;
};

#endif
