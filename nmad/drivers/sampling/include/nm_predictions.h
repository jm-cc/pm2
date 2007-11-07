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

struct nm_core;

extern int nm_ns_init(struct nm_core *p_core);
extern int nm_ns_exit(struct nm_core *p_core);

extern int nm_ns_dec_bws(struct nm_core *p_core, uint8_t **drv_ids);
extern int nm_ns_inc_lats(struct nm_core *p_core, uint8_t **drv_ids);

extern int nm_ns_split_ratio(uint32_t len_to_send, struct nm_core *p_core, int drv1_id, int drv2_id, uint32_t *offset);
extern int nm_ns_multiple_split_ratio(uint32_t len_to_send, struct nm_core *p_core, int nb_drv, uint8_t *drv_ids, uint32_t *chunk_lens, int *final_nb_drv);

#endif
