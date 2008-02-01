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

#include <pm2_common.h>

#include "nm_private.h"
#include "nm_pkt_wrap.h"
#include "nm_sched.h"
#include "nm_gate.h"
#include "nm_drv.h"
#include "nm_trk.h"
#include "nm_core.h"
#include "nm_errno.h"
#include "nm_log.h"

/** Handle failed incoming requests.
 *
 * requests are handed to the implementation dependent scheduler
 */
static
__inline__
int
nm_process_failed_recv_rq(struct nm_sched	*p_sched,
                          struct nm_pkt_wrap	*p_pw,
                          int		 	_err) {
        int err;

        /* Call implementation dependent request handler */
        err	= p_sched->ops.in_process_failed_rq(p_sched, p_pw, _err);
        if (err	!= NM_ESUCCESS) {
                NM_DISPF("sched.in_process_failed_rq returned %d", err);
        }

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
nm_process_successful_recv_rq(struct nm_sched		*p_sched,
                              struct nm_pkt_wrap	*p_pw) {
        int err;

        /* Call implementation dependent request handler */
        err	= p_sched->ops.in_process_success_rq(p_sched, p_pw);
        if (err	!= NM_ESUCCESS) {
                NM_DISPF("sched.in_process_success_rq returned %d", err);
        }

        err = NM_ESUCCESS;

        return err;
}

/** Handle incoming requests that have been processed by the driver-dependent
   code.
   - requests may be successful or failed, and should be handled appropriately
    --> this function is responsible for the processing common to both cases
*/
__inline__
int
nm_process_complete_recv_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err) {
        int err;

        NM_TRACEF("recv request complete: gate %d, drv %d, trk %d, proto %d, seq %d",
             p_pw->p_gate->id,
             p_pw->p_drv->id,
             p_pw->p_trk->id,
             p_pw->proto_id,
             p_pw->seq);

        /* clear the input request field in gate track */
        if (p_pw->p_gtrk) {
                p_pw->p_gtrk->p_in_rq = NULL;
        }

        if (_err == NM_ESUCCESS) {
                err	= nm_process_successful_recv_rq(p_sched, p_pw);
                if (err	!= NM_ESUCCESS) {
                        NM_DISPF("process_successful_recv_rq returned %d", err);
                }
        } else {
                err	= nm_process_failed_recv_rq(p_sched, p_pw, _err);
                if (err	!= NM_ESUCCESS) {
                        NM_DISPF("process_failed_recv_rq returned %d", err);
                }
        }

        err	= NM_ESUCCESS;

        return err;
}

/** Poll active incoming requests in poll_slist.
 */
static
__inline__
int
nm_poll_recv(struct nm_sched	*p_sched,
             p_tbx_slist_t	 poll_slist) {
        int err;

        tbx_slist_ref_to_head(poll_slist);

        do {
                struct nm_pkt_wrap	*p_pw = tbx_slist_ref_get(poll_slist);


                NM_TRACEF("polling inbound request: gate %d, drv %d, trk %d, proto %d, seq %d",
                     p_pw->p_gate?p_pw->p_gate->id:-1,
                     p_pw->p_drv->id,
                     p_pw->p_trk->id,
                     p_pw->proto_id,
                     p_pw->seq);

                /* poll request 					*/
                if(p_pw->p_gate)
		  {
		    err = p_pw->p_gdrv->receptacle.driver->poll_recv_iov(p_pw->p_gdrv->receptacle._status, 
									 p_pw);
		  } else {
		    err = p_pw->p_gdrv->receptacle.driver->poll_recv_iov_all(p_pw);
		  }

                /* process poll command status				*/
                if (err == -NM_EAGAIN) {
                        /* not complete, try again later
                           - leave the request in the list and go to next
                        */
                        goto next;
                }

                if (err != NM_ESUCCESS) {
                        NM_DISPF("drv->poll_recv returned %d", err);
                }

                /* process complete request */
                err = nm_process_complete_recv_rq(p_sched, p_pw, err);
                if (err < 0) {
                        NM_DISPF("nm_process_complete_rq returned %d", err);
                }

                /* remove request from list */
                if (tbx_slist_ref_extract_and_forward(poll_slist, NULL))
                        continue;
                else
                        break;

                next:
                        ;
        } while (tbx_slist_ref_forward(poll_slist));

        err = NM_ESUCCESS;

        return err;
}

/** Transfer request from the post_slist to the pending_slist if the
   state of the drv/trk targeted by a request allows this request to
   be posted.
 */
static
__inline__
int
nm_post_recv(struct nm_sched	*p_sched,
             p_tbx_slist_t 	 post_slist,
             p_tbx_slist_t 	 pending_slist) {
        int err;

        tbx_slist_ref_to_head(post_slist);
        do {
                struct nm_pkt_wrap	*p_pw = NULL;

        again:
                p_pw = tbx_slist_ref_get(post_slist);

                /* check track availability				*/
                /* a recv is already pending on this gate/track */
                if (p_pw->p_gate && p_pw->p_gtrk->p_in_rq)
                        continue;


                /* ready to post request				*/

                /* update incoming request field in gate track if
                   the request specifies a gate */
                if (p_pw->p_gate) {
                        p_pw->p_gtrk->p_in_rq	= p_pw;
                }

                NM_TRACEF("posting new recv request: gate %d, drv %d, trk %d, proto %d, seq %d",
                     p_pw->p_gate?p_pw->p_gate->id:-1,
                     p_pw->p_drv->id,
                     p_pw->p_trk->id,
                     p_pw->proto_id,
                     p_pw->seq);

                /* post request */
                if(p_pw->p_gate)
		  {
		    err = p_pw->p_gdrv->receptacle.driver->post_recv_iov(p_pw->p_gdrv->receptacle._status, 
									 p_pw);
		  } else {
		    err = p_pw->p_gdrv->receptacle.driver->post_recv_iov_all(p_pw);
		  }


                /* process post command status				*/

                /* driver immediately succeeded in receiving the packet? */
                if (err == -NM_EAGAIN) {
                        NM_TRACEF("new recv request pending: gate %d, drv %d, trk %d, proto %d, seq %d",
                             p_pw->p_gate?p_pw->p_gate->id:-1,
                             p_pw->p_drv->id,
                             p_pw->p_trk->id,
                             p_pw->proto_id,
                             p_pw->seq);

                        /* No, put it in pending list */
                        tbx_slist_append(pending_slist, p_pw);
                } else {
                        NM_TRACEF("request completed immediately");

                        /* Yes, request complete, process it */
                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv->post_recv returned %d", err);
                        }

                        err = nm_process_complete_recv_rq(p_sched, p_pw, err);
                        if (err < 0) {
                                NM_DISPF("nm_process_complete_rq returned %d", err);
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

/** Main scheduler func for incoming requests.
   - this function must be called once for each gate on a regular basis
 */
int
nm_sched_in(struct nm_core *p_core) {
        struct nm_sched *p_sched = p_core->p_sched;
        int err;


        /* poll pending requests 					*/

        /* aux requests */
        if (!tbx_slist_is_nil(p_sched->pending_aux_recv_req)) {
                NM_TRACEF("polling inbound aux requests");
                err = nm_poll_recv(p_sched,
                                   p_sched->pending_aux_recv_req);
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("poll_recv (aux) returned %d", err);
                }
        }

        /* permissive requests */
        if (!tbx_slist_is_nil(p_sched->pending_perm_recv_req)) {
                NM_TRACEF("polling inbound perm requests");
                err = nm_poll_recv(p_sched,
                                   p_sched->pending_perm_recv_req);
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("poll_recv (perm) returned %d", err);
                }
        }


        /* schedule new reception requests				*/
        err	= p_core->p_sched->ops.in_schedule(p_sched);
        if (err < 0) {
                NM_DISPF("sched.in_schedule returned %d", err);
        }


        /* post new requests 						*/

        /* aux requests */
        if (!tbx_slist_is_nil(p_sched->post_aux_recv_req)) {
                NM_TRACEF("posting inbound aux requests");
                err = nm_post_recv(p_sched,
                                   p_sched->post_aux_recv_req,
                                   p_sched->pending_aux_recv_req);
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("post_recv (aux) returned %d", err);
                }
        }

        /* permissive requests */
        if (!tbx_slist_is_nil(p_sched->post_perm_recv_req)) {
                NM_TRACEF("posting inbound perm requests");
                err = nm_post_recv(p_sched,
                                   p_sched->post_perm_recv_req,
                                   p_sched->pending_perm_recv_req);
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("post_recv (perm) returned %d", err);
                }
        }

        err = NM_ESUCCESS;

        return err;
}
