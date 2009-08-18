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
extern int nm_ns_update(struct nm_core *p_core);
extern int nm_ns_exit(struct nm_core *p_core);

extern int nm_ns_dec_bws(struct nm_core *p_core, const nm_drv_id_t **drv_ids, int*nb_drvs);
extern int nm_ns_inc_lats(struct nm_core *p_core, const nm_drv_id_t **drv_ids, int*nb_drvs);

extern int nm_ns_split_ratio(uint32_t len_to_send, struct nm_core *p_core,
			     nm_drv_id_t drv1_id, nm_drv_id_t drv2_id, uint32_t *offset);
extern int nm_ns_multiple_split_ratio(uint32_t len, struct nm_core *p_core,
				      int*nb_chunks, struct nm_rdv_chunk*chunks);

#endif
