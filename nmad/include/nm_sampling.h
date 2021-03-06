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

#ifndef NM_PREDICTIONS_H
#define NM_PREDICTIONS_H

/** @internal used by nm core to update performance profiles when a new driver is loaded */
extern int nm_ns_update(struct nm_core *p_core, nm_drv_t p_drv);
extern int nm_ns_exit(struct nm_core *p_core);

extern int nm_ns_dec_bws(struct nm_core *p_core, nm_drv_t const**p_drvs, int*nb_drvs);
extern int nm_ns_inc_lats(struct nm_core *p_core, nm_drv_t const**p_drvs, int*nb_drvs);

extern int nm_ns_multiple_split_ratio(nm_len_t len, struct nm_core *p_core, struct nm_gate_s*p_gate,
				      int*nb_chunks, struct nm_rdv_chunk*chunks);

#endif
