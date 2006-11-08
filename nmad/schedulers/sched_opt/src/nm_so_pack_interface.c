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

/*
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_so_private.h>

#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"
#include "nm_so_interfaces.h"
#include "nm_so_pack_interface.h"

struct nm_so_cnx {
  struct nm_so_interface *p_interface;
};


int
nm_so_pack_interface_init(struct nm_core *p_core
			  nm_so_pack_api *p_interface);
{
  struct nm_so_interface *p_api;
  int err;

  err = nm_so_ri_init(p_core, &p_api);

  return err;
}


int
nm_so_begin_packing(nm_so_pack_api interface,
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

  err = NM_ESUCCESS;
 out:
  return err;
}

int
nm_so_begin_unpacking(nm_so_pack_api interface,
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


int nm_so_pi_init_gate(struct nm_gate *p_gate){
  return nm_so_raw_interface.init_gate(p_gate);
}



int nm_so_pi_pack_success(struct nm_gate *p_gate,
                          uint8_t tag, uint8_t seq){

  return nm_so_raw_interface.pack_success(p_gate, tag, seq);
}


int nm_so_pi_unpack_success(struct nm_gate *p_gate,
                            uint8_t tag, uint8_t seq){
  return nm_so_raw_interface.unpack_success(p_gate, tag, seq);
}

nm_so_interface nm_so_pack_interface = {
  .init = nm_so_pack_interface_init,
  .init_gate = nm_so_pi_init_gate,
  .pack_success = nm_so_pi_pack_success,
  .unpack_success = nm_so_pi_unpack_success,

  .priv = NULL,
};

*/
