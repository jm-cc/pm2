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


#include "nm_protected.h"
#include "madeleine.h"
#include "nm_so_sendrecv_interface.h"

typedef struct s_mad_nmad_connection_specific {
#ifdef CONFIG_SCHED_OPT
        struct s_mad_nmad_connection_specific	*master_cnx;

        nm_so_request		 in_reqs[256];

        uint8_t			 in_next_seq;
        uint8_t			 in_wait_seq;
        uint8_t			 in_flow_ctrl;
#  ifdef MAD_NMAD_SO_DEBUG
        p_tbx_slist_t		 in_deferred_slist;
#  endif /* MAD_NMAD_SO_DEBUG */

        nm_so_request		 out_reqs[256];
        uint8_t			 out_next_seq;
        uint8_t			 out_wait_seq;
        uint8_t			 out_flow_ctrl;
#endif /* CONFIG_SCHED_OPT */
  	uint8_t			 gate_id;
} mad_nmad_connection_specific_t, *p_mad_nmad_connection_specific_t;
