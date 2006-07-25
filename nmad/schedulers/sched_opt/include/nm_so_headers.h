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

#ifndef NM_SO_HEADERS_H
#define NM_SO_HEADERS_H

struct nm_so_sched_header;
struct nm_so_header;

int
nm_so_header_init(struct nm_core *p_core);

/*********************************/

int
nm_so_header_alloc_sched_header(struct nm_so_sched *so_sched,
                                uint8_t nb_seg, uint32_t len,
                                struct nm_so_sched_header **h);
int
nm_so_header_free_sched_header(struct nm_so_sched *so_sched,
                               struct nm_so_sched_header *h);
int
nm_so_header_sizeof_sched_header(int *size);

/*********************************/

int
nm_so_header_alloc_data_header(struct nm_so_sched *so_sched,
                               uint8_t proto_id,
                               uint8_t seq, uint32_t len,
                               struct nm_so_header **header);
int
nm_so_header_alloc_control_header(struct nm_so_sched *so_sched,
                                  uint8_t proto_id,
                                  struct nm_so_header **header);
int
nm_so_header_free_header(struct nm_so_sched *so_sched,
                         struct nm_so_header *header);

int
nm_so_header_sizeof_header(int *size);

/*********************************/

int
nm_so_header_get_sched_proto_id(struct nm_so_sched_header *head,
                                uint8_t *proto_id);
int
nm_so_header_get_v_nb(struct nm_so_sched_header *header,
                      uint8_t *v_nb);
int
nm_so_header_get_total_len(struct nm_so_sched_header *header,
                           uint32_t *len);
int
nm_so_header_set_sched_proto_id(struct nm_so_sched_header *head,
                                uint8_t proto_id);
int
nm_so_header_set_v_nb(struct nm_so_sched_header *header,
                      uint8_t v_nb);
int
nm_so_header_set_total_len(struct nm_so_sched_header *header,
                           uint32_t len);

/*********************************/

int
nm_so_header_get_proto_id(struct nm_so_header *head,
                          uint8_t *proto_id);
int
nm_so_header_get_seq(struct nm_so_header *header,
                     uint8_t *seq);
int
nm_so_header_get_len(struct nm_so_header *header,
                     uint32_t *len);
int
nm_so_header_set_proto_id(struct nm_so_header *header,
                          uint8_t proto_id);
int
nm_so_header_set_seq(struct nm_so_header *header,
                     uint8_t seq);
int
nm_so_header_set_len(struct nm_so_header *header,
                      uint32_t len);
/*********************************/

#endif
