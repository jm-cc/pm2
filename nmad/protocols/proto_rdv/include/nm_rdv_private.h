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

#ifndef NM_RDV_PRIVATE_H
#define NM_RDV_PRIVATE_H

//#include "nm_public.h"
#define NB_EDP 4


struct nm_sched;

struct nm_rdv_rdv_rq {
    uint8_t  proto_id;
    uint8_t  seq;
    uint32_t len; // ->pour l'allocation dynamique
};

struct nm_rdv_ack_rq {
    uint8_t  proto_id;
    uint8_t  seq;
    uint8_t  track_id;
};

struct nm_rdv {
    struct nm_sched *p_sched;

    p_tbx_memory_t rdv_key;
    p_tbx_memory_t ack_key;

    p_tbx_slist_t waiting_rdv;
    p_tbx_slist_t large_waiting_ack;

    int out_req_nb[NB_EDP];
};

#endif
