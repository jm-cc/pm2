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

struct nm_gate;
struct nm_sched;
struct nm_pkt_wrap;

/* output structs
 */
struct nm_null_gate {
        int _empty;
};

/* scheduler instance data
 */
struct nm_null_sched {
        int _empty;
};

/* methods
 */
int
nm_null_out_schedule_gate(struct nm_gate *p_gate);

int
nm_null_out_process_success_rq(struct nm_sched	*p_sched,
                              struct nm_pkt_wrap	*p_pw);

int
nm_null_out_process_failed_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw,
                             int		 _err);

int
nm_null_in_schedule(struct nm_sched *p_sched);

int
nm_null_in_process_success_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw);

int
nm_null_in_process_failed_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw,
                             int		 _err);

