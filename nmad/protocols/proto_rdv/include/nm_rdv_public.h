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


struct nm_rdv_rdv_rq;
struct nm_rdv_ack_rq;

void
nm_rdv_generate_rdv(struct nm_proto *p_proto,
                    struct nm_pkt_wrap *p_pw,
                    struct nm_rdv_rdv_rq **pp_rdv_rq);

void
nm_rdv_treat_rdv_rq(struct nm_proto *p_proto,
                    struct nm_rdv_rdv_rq *p_rdv_rq,
                    struct nm_rdv_ack_rq **pp_ack);


void
nm_rdv_treat_ack_rq(struct nm_proto *p_proto,
                    struct nm_rdv_ack_rq *p_ack);

struct nm_gate *
nm_rdv_treat_waiting_rdv(struct nm_proto *p_proto,
                         struct nm_rdv_ack_rq **pp_ack);

void
nm_rdv_free_rdv(struct nm_proto *p_proto,
                struct nm_rdv_rdv_rq *rdv);

void
nm_rdv_free_ack(struct nm_proto *p_proto,
                struct nm_rdv_ack_rq *ack);

void
nm_rdv_free_track(struct nm_proto *p_proto,
                  uint8_t trk_id);

int
nm_rdv_rdv_rq_size(void);

int
nm_rdv_ack_rq_size(void);


int
nm_rdv_load(struct nm_proto_ops *p_ops);
