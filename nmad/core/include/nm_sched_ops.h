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

#ifndef NM_SCHED_OPS_H
#define NM_SCHED_OPS_H

struct nm_drv;
struct nm_gate;
struct nm_pkt_wrap;
struct nm_sched;
struct nm_trk;

/** Scheduler commands. */
struct nm_sched_ops {

        /** Initialize the scheduler module.
         */
        int (*init)			(struct nm_sched *p_sched);

        /** Shutdown the scheduler module.
         */
        int (*exit)			(struct nm_sched *p_sched);

        /** Initialize the scheduler structures for a new track.
         */
        int (*init_trks)		(struct nm_sched	 *p_sched,
                                         struct nm_drv		 *p_drv);

        /** Initialize the scheduler structures for a new gate.
         */
        int (*init_gate)		(struct nm_sched	 *p_sched,
                                         struct nm_gate		 *p_gate);

        /** Clean-up the scheduler structures for a track.
         */
        int (*close_trks)		(struct nm_sched	 *p_sched,
                                         struct nm_drv		 *p_drv);

        /** Clean-up the scheduler structures for a gate.
         */
        int (*close_gate)		(struct nm_sched	 *p_sched,
                                         struct nm_gate		 *p_gate);


        /** Schedule and post new outgoing buffers */
        int (*out_schedule_gate)	(struct nm_gate *p_gate);

        /** Process complete successful outgoing request */
        int (*out_process_success_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw);

        /** Process complete failed outgoing request */
        int (*out_process_failed_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw,
                                         int			 _err);


        /** Schedule and post new incoming buffers.
         */
        int (*in_schedule)		(struct nm_sched *p_sched);

        /** Process complete successful incoming request.
         */
        int (*in_process_success_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw);

        /** Process complete failed incoming request.
         */
        int (*in_process_failed_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw,
                                         int			 _err);

};

#endif /* NM_SCHED_OPS_H */
