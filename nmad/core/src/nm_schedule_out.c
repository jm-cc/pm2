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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include "nm_private.h"
#include "nm_pkt_wrap.h"
#include "nm_sched.h"
#include "nm_gate.h"
#include "nm_drv.h"
#include "nm_trk.h"
#include "nm_errno.h"
#include "nm_log.h"

/** Handle failed incoming requests.
 *
 * requests are handed to the implementation dependent scheduler
 */
static
__inline__
int
nm_process_failed_send_rq(struct nm_gate	*p_gate,
                          struct nm_pkt_wrap	*p_pw,
                          int		 	_err) {
        struct nm_sched	*p_sched	= p_gate->p_sched;
        int	err;

        /* Call implementation dependent request handler */
        err	= p_sched->ops.out_process_failed_rq(p_sched, p_pw, _err);

        err = NM_ESUCCESS;

        return err;
}


/** Handle successful incoming requests.
 *
 * requests are handed to the implementation dependent scheduler
 */
static
__inline__
int
nm_process_successful_send_rq(struct nm_gate		*p_gate,
                              struct nm_pkt_wrap	*p_pw) {
        struct nm_sched	*p_sched	= p_gate->p_sched;
        int	err;

        /* Call implementation dependent request handler */
        err	= p_sched->ops.out_process_success_rq(p_sched, p_pw);

        err = NM_ESUCCESS;

        return err;
}

/** Handle outgoing requests that have been processed by the driver-dependent
 * code.
 *
 * - requests may be successful or failed, and should be handled appropriately
 * --> this function is responsible for the processing common to both cases
 */
static
__inline__
int
nm_process_complete_send_rq(struct nm_gate	*p_gate,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err) {
        int err;

        NM_TRACEF("send request complete: gate %d, drv %d, trk %d, proto %d, seq %d",
             p_pw->p_gate->id,
             p_pw->p_drv->id,
             p_pw->p_trk->id,
             p_pw->proto_id,
             p_pw->seq);

        p_pw->p_drv ->out_req_nb--;
        p_pw->p_trk ->out_req_nb--;
        p_pw->p_gdrv->out_req_nb--;
        p_gate->out_req_nb--;

        if (_err == NM_ESUCCESS) {
		FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_SEND_PACKET, p_pw, p_pw->p_drv->id, p_pw->p_trk->id);
                err = nm_process_successful_send_rq(p_gate, p_pw);
                if (err < 0) {
                        NM_DISPF("process_successful_send_rq returned %d", err);
                }
        } else {
                /* error or connection closed, do something */
                err	= nm_process_failed_send_rq(p_gate, p_pw, _err);
                if (err < 0) {
                        NM_DISPF("process_failed_send_rq returned %d", err);
                }
        }

        return err;
}

/** Poll active outgoing requests.
 */
static
__inline__
int
nm_poll_send	(struct nm_gate *p_gate) {
        struct nm_pkt_wrap	*p_pw	= NULL;
        const int		 req_nb = p_gate->out_req_nb;
        int i = 0;
        int j;
        int err;

        /* Fast path */
        for (i = 0; i < req_nb; i++) {
                p_pw	= p_gate->out_req_list[i];
#ifdef XPAULETTE
                err	= p_pw->p_drv->ops.wait_iov(p_pw);
#else
		err	= p_pw->p_drv->ops.poll_send_iov(p_pw);
#endif
                if (err != -NM_EAGAIN)
                        goto update_needed;
        }

        err = -NM_EAGAIN;
        goto out;

        /* Slow path */
 update_needed:
        j = i++;
        nm_process_complete_send_rq(p_gate, p_pw, err);

        for (;i < req_nb; i++) {
                p_pw	= p_gate->out_req_list[i];
#ifdef XPAULETTE
                err	= p_pw->p_drv->ops.wait_iov(p_pw);
#else
		err	= p_pw->p_drv->ops.poll_send_iov(p_pw);
#endif

                if (err == -NM_EAGAIN) {
                        p_gate->out_req_list[j] = p_gate->out_req_list[i];
                        j++;
                } else {
                        nm_process_complete_send_rq(p_gate, p_pw, err);
                }
        }

 out:
        return err;
}

/** Post new outgoing requests.
   - ideally, this function shouldn't have to check track availability since
   it is the job of the implementation-dependent scheduling code, but one never
   knows...
   - this function must handle immediately completed requests properly
 */
static
__inline__
int
nm_post_send	(struct nm_gate *p_gate) {
        p_tbx_slist_t 	 const post_slist	= p_gate->post_sched_out_list;
        int err;

        tbx_slist_ref_to_head(post_slist);
        do {
                struct nm_pkt_wrap	*p_pw	= NULL;

        again:
                p_pw	= tbx_slist_ref_get(post_slist);

                /* check track availability				*/
                /* TODO */
                if (0/* track always available for now */)
                        continue;

                if (p_gate->out_req_nb == 256)
                        continue;

                p_gate->out_req_nb++;
                p_pw->p_gdrv->out_req_nb++;
                p_pw->p_drv->out_req_nb++;
                p_pw->p_trk->out_req_nb++;

                /* ready to send					*/
		FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_TRACK_TO_DRIVER, p_pw, p_pw->p_drv->id, p_pw->p_trk->id);
                NM_TRACEF("posting new send request: gate %d, drv %d, trk %d, proto %d, seq %d",
                     p_pw->p_gate->id,
                     p_pw->p_drv->id,
                     p_pw->p_trk->id,
                     p_pw->proto_id,
                     p_pw->seq);
                /* post request */
                err = p_pw->p_gdrv->p_drv->ops.post_send_iov(p_pw);

                /* process post command status				*/

                /* driver immediately succeeded in sending the packet? */
                if (err == -NM_EAGAIN) {
                        /* No, put the request in the list */

                        p_gate->out_req_list[p_gate->out_req_nb-1]	= p_pw;
                        NM_TRACEF("new request %d pending: gate %d, drv %d, trk %d, proto %d, seq %d",
                             p_gate->out_req_nb - 1,
                             p_pw->p_gate->id,
                             p_pw->p_drv->id,
                             p_pw->p_trk->id,
                             p_pw->proto_id,
                             p_pw->seq);
#ifdef XPAULETTE
			err = p_pw->p_gdrv->p_drv->ops.wait_iov(p_pw);
#endif /* XPAULETTE */
 
                } else {
                        /* Yes, request complete, process it */
                        NM_TRACEF("request completed immediately");

                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv->post_send returned %d", err);
                        }

                        err = nm_process_complete_send_rq(p_gate, p_pw, err);
                        if (err < 0) {
                                NM_DISPF("nm_process_complete send_rq returned %d", err);
                        }
                }

                /* remove request from post list */
                if (tbx_slist_ref_extract_and_forward(post_slist, NULL))
                        goto again;
                else
                        break;
        } while (tbx_slist_ref_forward(post_slist));

        err = NM_ESUCCESS;

        return err;
}

/** Main scheduler func for outgoing requests.
   - this function must be called once for each gate on a regular basis
 */
int
nm_sched_out_gate	(struct nm_gate *p_gate) {
        int	err;

        /* schedule new requests	*/
        err	= p_gate->p_sched->ops.out_schedule_gate(p_gate);
        if (err < 0) {
                NM_DISPF("sched.schedule_out returned %d", err);
        }

        /* post new requests	*/
        if (!tbx_slist_is_nil(p_gate->post_sched_out_list)) {
             NM_TRACEF("posting outbound requests");
	     err	= nm_post_send(p_gate);

            if (err < 0 && err != -NM_EAGAIN) {
                NM_DISPF("post_send returned %d", err);
            }
        }

        /* check pending out requests	*/
        if (p_gate->out_req_nb) {
                NM_TRACEF("polling outbound requests");
                err = nm_poll_send(p_gate);

                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("poll_send returned %d", err);
                }
        }

        err = NM_ESUCCESS;

        return err;
}


