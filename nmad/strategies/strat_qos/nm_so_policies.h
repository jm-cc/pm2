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

#ifndef NM_SO_POLICIES_H
#define NM_SO_POLICIES_H

#ifdef NMAD_QOS

typedef struct nm_so_policy_struct nm_so_policy;

typedef int (*nm_so_policy_pack_ctrl_func)(void *private,
					   union nm_so_generic_ctrl_header *p_ctrl);

typedef int (*nm_so_policy_pack_func)(struct nm_gate *p_gate,
				      void *private,
				      nm_tag_t tag, nm_seq_t seq,
				      void *data, uint32_t len);

typedef int (*nm_so_policy_try_and_commit_func)(struct nm_gate *p_gate,
						void *private);

typedef int (*nm_so_policy_init_gate_func)(void **addr_private);

typedef int (*nm_so_policy_exit_gate_func)(void **addr_private);

typedef int (*nm_so_policy_busy_func)(void *private);


struct nm_so_policy_struct {
  nm_so_policy_pack_ctrl_func pack_ctrl;
  nm_so_policy_pack_func pack;
  nm_so_policy_try_and_commit_func try_and_commit;
  nm_so_policy_init_gate_func init_gate;
  nm_so_policy_exit_gate_func exit_gate;
  nm_so_policy_busy_func busy;
  void *priv;
};

#endif /* NMAD_QOS */

#endif
