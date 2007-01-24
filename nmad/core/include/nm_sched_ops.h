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

struct nm_drv;
struct nm_gate;
struct nm_pkt_wrap;
struct nm_sched;
struct nm_trk;

struct nm_sched_ops {
        /* scheduler commands */
        int (*init)			(struct nm_sched *p_sched);

        int (*exit)			(struct nm_sched *p_sched);


        /* session commands */
        int (*init_trks)		(struct nm_sched	 *p_sched,
                                         struct nm_drv		 *p_drv);

        int (*init_gate)		(struct nm_sched	 *p_sched,
                                         struct nm_gate		 *p_gate);

        int (*close_trks)		(struct nm_sched	 *p_sched,
                                         struct nm_drv		 *p_drv);

        int (*close_gate)		(struct nm_sched	 *p_sched,
                                         struct nm_gate		 *p_gate);


        /* schedule and post new outgoing buffers */
        int (*out_schedule_gate)	(struct nm_gate *p_gate);

        /* process complete successful outgoing request */
        int (*out_process_success_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw);

        /* process complete failed outgoing request */
        int (*out_process_failed_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw,
                                         int			 _err);


        /* schedule and post new incoming buffers */
        int (*in_schedule)		(struct nm_sched *p_sched);

        /* process complete successful incoming request */
        int (*in_process_success_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw);

        /* process complete failed incoming request */
        int (*in_process_failed_rq)	(struct nm_sched	*p_sched,
                                         struct nm_pkt_wrap	*p_pw,
                                         int			 _err);

};

