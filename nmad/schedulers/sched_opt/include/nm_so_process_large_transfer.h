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

#ifndef NM_SO_PROCESS_LARGE_PROCESS_H
#define NM_SO_PROCESS_LARGE_PROCESS_H

struct nm_gate;
struct nm_so_pkt_wrap;

int nm_so_build_multi_ack(struct nm_gate *p_gate,
                          uint8_t tag, uint8_t seq, uint32_t chunk_offset,
                          int nb_drv, uint8_t *drv_ids, uint32_t *chunk_lens);

int nm_so_rdv_success(tbx_bool_t is_any_src,
                      struct nm_gate *p_gate,
                      uint8_t tag, uint8_t seq,
                      uint32_t len,
                      uint32_t chunk_offset,
                      uint8_t is_last_chunk);

int init_large_datatype_recv_with_multi_ack(struct nm_so_pkt_wrap *p_so_pw);

#endif
