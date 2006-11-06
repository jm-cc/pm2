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

#ifndef _NM_SO_RAW_INTERFACE_H_
#define _NM_SO_RAW_INTERFACE_H_

/* WARNING!!! There's a severe limitation here: concurrent
   begin_(un)pack/end_(un)pack session are not allowed on the same
   tag_id, even if they use different gates... */

struct nm_core;
struct nm_gate;

extern int
(*__nm_so_pack)(struct nm_gate *p_gate,
		uint8_t tag, uint8_t seq,
		void *data, uint32_t len);

extern int
nm_so_stest(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq);

extern int
nm_so_swait(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq);

static __inline__
int
__nm_so_swait_range(struct nm_core *p_core,
		    struct nm_gate *p_gate,
		    uint8_t tag,
		    uint8_t seq_inf, uint8_t seq_sup)
{
  uint8_t seq = seq_inf;

  do {

    nm_so_swait(p_core, p_gate, tag, seq);

  } while(seq++ != seq_sup);

  return NM_ESUCCESS;
}



extern int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len);

extern int
nm_so_rtest(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq);

extern int
nm_so_rwait(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq);

static __inline__
int
__nm_so_rwait_range(struct nm_core *p_core,
		    struct nm_gate *p_gate,
		    uint8_t tag,
		    uint8_t seq_inf, uint8_t seq_sup)
{
  uint8_t seq = seq_inf;

  do {

    nm_so_rwait(p_core, p_gate, tag, seq);

  } while(seq++ != seq_sup);

  return NM_ESUCCESS;
}

#endif
