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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_so_private.h>

#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"
#include "nm_so_interface.h"

struct nm_so_cnx out_cnx[NM_SO_MAX_TAGS], in_cnx[NM_SO_MAX_TAGS];

int
nm_so_interface_init(void)
{
  return NM_ESUCCESS;
}

int
nm_so_begin_packing(struct nm_core *p_core,
		    uint16_t gate_id, uint8_t tag,
		    struct nm_so_cnx **cnx)
{
  int err;

  if(gate_id >= p_core->nb_gates) {
    err	= -NM_EINVAL;
    goto out;
  }

  if(tag >= NM_SO_MAX_TAGS) {
    err = -NM_EINVAL;
    goto out;
  }

  out_cnx[tag].p_gate = p_core->gate_array + gate_id;
  out_cnx[tag].seq_number = 0;
  *cnx = out_cnx + tag;

  err = NM_ESUCCESS;
 out:
  return err;
}

int
nm_so_begin_unpacking(struct nm_core *p_core,
		      uint16_t gate_id, uint8_t tag,
		      struct nm_so_cnx **cnx)
{
  int err;

  if(gate_id >= p_core->nb_gates) {
    err	= -NM_EINVAL;
    goto out;
  }

  if(tag >= NM_SO_MAX_TAGS) {
    err = -NM_EINVAL;
    goto out;
  }

  in_cnx[tag].p_gate = p_core->gate_array + gate_id;
  in_cnx[tag].seq_number = 0;
  *cnx = in_cnx + tag;

  err = NM_ESUCCESS;
 out:
  return err;
}
