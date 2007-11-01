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


struct nm_pkt_wrap;
struct nm_gate;
struct nm_sched;

/** Control pkt.
 */
struct nm_mini_alt_ctrl {
        uint8_t	proto_id;
        uint8_t seq;
};

/** Output state.
 */
struct nm_mini_alt_gate {

        /* state flag
           - 0: send control packet on track 0
           - 1: send data packet on track !0
         */
        uint8_t			 state;

        /* next seqnum to use for control pkt
         */
        uint8_t			 seq;
        struct nm_pkt_wrap	*p_next_pw;
};

/** Input state.
 */
struct nm_mini_alt_sched_gate {

        /* list of pkt submitted by external code and waiting for a matching
           control pkt
         */
        p_tbx_slist_t	pkt_list;

        /* list of already received tokens
         */
        p_tbx_slist_t	ctrl_list;

        /* number of unprocessed tokens per protocol
         */
        uint32_t	token_per_proto[255];
};

/** Alternate implementation of mini scheduler.
 */
struct nm_mini_alt_sched {
        struct nm_mini_alt_sched_gate	sched_gates[255];

        /* state flag
           0: no receive request posted on track 0 yet or request completed
           1: receive request posted on track 0
         */
        uint8_t				state;

        p_tbx_memory_t			ctrl_mem;

        /* list of perm pkt submitted by external code and waiting for a matching
           control pkt
         */
        p_tbx_slist_t	pkt_list;

        /* number of unprocessed tokens per protocol
         */
        uint32_t	token_per_proto[255];
};

/* methods
 */
int
nm_mini_alt_out_schedule_gate(struct nm_gate *p_gate);

int
nm_mini_alt_out_process_success_rq(struct nm_sched	*p_sched,
                              struct nm_pkt_wrap	*p_pw);

int
nm_mini_alt_out_process_failed_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw,
                             int		 _err);

int
nm_mini_alt_in_schedule(struct nm_sched *p_sched);

int
nm_mini_alt_in_process_success_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw);

int
nm_mini_alt_in_process_failed_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw,
                             int		 _err);

