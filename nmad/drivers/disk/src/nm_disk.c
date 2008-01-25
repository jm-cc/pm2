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
#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "pm2_common.h"
#include "nm_public.h"
#include "nm_pkt_wrap.h"
#include "nm_drv.h"
#include "nm_drv_cap.h"
#include "nm_trk.h"
#include "nm_gate.h"
#include "nm_core.h"
#include "nm_trk_rq.h"
#include "nm_cnx_rq.h"
#include "nm_errno.h"
#include "nm_log.h"

enum nm_disk_mode {
        ndm_R,
        ndm_W,
        ndm_A,
};

struct nm_disk_drv {
        /** url */
        char*url;
        /** capabilities */
        struct nm_drv_cap caps;
};

struct nm_disk_trk {
        int _empty;
};

struct nm_disk_cnx {
        int _empty;
};

struct nm_disk_gate {
        int			fd[255];
        enum nm_disk_mode	mode;
};

struct nm_disk_pkt_wrap {
        struct nm_iovec_iter vi;
};

/** Disk NewMad Driver */
static int nm_disk_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_disk_init(struct nm_drv* p_drv);
static int nm_disk_open_trk(struct nm_trk_rq*p_trk_rq);
static int nm_disk_close_trk(struct nm_trk*p_trk);
static int nm_disk_connect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_disk_accept(void*_status, struct nm_cnx_rq *p_crq);
static int nm_disk_disconnect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_disk_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_disk_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static struct nm_drv_cap*nm_disk_get_capabilities(struct nm_drv *p_drv);
static int nm_disk_set_capabilities(struct nm_drv *p_drv, struct nm_drv_cap*);
static const char*nm_disk_get_driver_url(struct nm_drv *p_drv);
#ifdef PIOM_BLOCKING_CALLS
static int nm_disk_wait_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_disk_wait_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
#endif

static const struct nm_drv_iface_s nm_disk_driver =
  {
    .query              = &nm_disk_query,
    .init               = &nm_disk_init,

    .open_trk		= &nm_disk_open_trk,
    .close_trk	        = &nm_disk_close_trk,

    .connect		= &nm_disk_connect,
    .accept		= &nm_disk_accept,
    .disconnect         = &nm_disk_disconnect,

    .post_send_iov	= &nm_disk_send_iov,
    .post_recv_iov      = &nm_disk_recv_iov,

    .poll_send_iov	= &nm_disk_send_iov,
    .poll_recv_iov	= &nm_disk_recv_iov,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .get_driver_url     = &nm_disk_get_driver_url,
    .get_track_url      = NULL,
    .get_capabilities   = &nm_disk_get_capabilities,
    .set_capabilities   = &nm_disk_set_capabilities,

#ifdef PIOM_BLOCKING_CALLS
    .wait_send_iov	= &nm_disk_wait_send_iov,
    .wait_recv_iov	= &nm_disk_wait_recv_iov,

    .wait_recv_any_iov  = NULL,
    .wait_send_any_iov  = NULL
#endif
  };

/** 'PadicoAdapter' facet for Disk driver */
static void*nm_disk_instanciate(puk_instance_t, puk_context_t);
static void nm_disk_destroy(void*);

static const struct puk_adapter_driver_s nm_disk_adapter_driver =
  {
    .instanciate = &nm_disk_instanciate,
    .destroy     = &nm_disk_destroy
  };

/** Instanciate functions */
static void*nm_disk_instanciate(puk_instance_t instance, puk_context_t context){
  struct nm_disk_gate*status = TBX_MALLOC(sizeof (struct nm_disk_gate));
  memset(status, 0, sizeof(struct nm_disk_gate));

  return status;
}

static void nm_disk_destroy(void*_status){
  TBX_FREE(_status);
}

static
int
nm_disk_query			(struct nm_drv *p_drv,
				 struct nm_driver_query_param *params,
				 int nparam) {
	struct nm_disk_drv	*p_disk_drv	= NULL;
	int err;

	/* private data							*/
	p_disk_drv	= TBX_MALLOC(sizeof (struct nm_disk_drv));
	if (!p_disk_drv) {
		err = -NM_ENOMEM;
		goto out;
	}

	memset(p_disk_drv, 0, sizeof (struct nm_disk_drv));

	/* driver capabilities encoding					*/
	p_disk_drv->caps.has_trk_rq_dgram			= 1;
	p_disk_drv->caps.has_selective_receive		= 0;
	p_disk_drv->caps.has_concurrent_selective_receive	= 0;
#ifdef PM2_NUIOA
	p_disk_drv->caps.numa_node = PM2_NUIOA_ANY_NODE;
	p_disk_drv->caps.latency = INT_MAX;
	p_disk_drv->caps.bandwidth = 0;
#endif

	p_drv->priv	= p_disk_drv;
	err = NM_ESUCCESS;

 out:
	return err;
}

static struct nm_drv_cap*nm_disk_get_capabilities(struct nm_drv *p_drv)
{
  struct nm_disk_drv *p_disk_drv = p_drv->priv;
  return &p_disk_drv->caps;
}

static int nm_disk_set_capabilities(struct nm_drv *p_drv, struct nm_drv_cap* caps)
{
  struct nm_disk_drv *p_disk_drv = p_drv->priv;
  p_disk_drv->caps = *caps;
  return 0;
}

/** Url function */
static const char*nm_disk_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_disk_drv *p_disk_drv = p_drv->priv;
  return p_disk_drv->url;
}

static
int
nm_disk_init			(struct nm_drv *p_drv) {
        struct nm_disk_drv	*p_disk_drv	= p_drv->priv;
	int err;

        /* driver url encoding						*/
        p_disk_drv->url	= tbx_strdup("-");

	err = NM_ESUCCESS;
	return err;
}

static
int
nm_disk_exit			(struct nm_drv *p_drv) {
        struct nm_disk_drv	*p_disk_drv	= NULL;
	int err;

        p_disk_drv	= p_drv->priv;

        TBX_FREE(p_disk_drv->url);
        TBX_FREE(p_disk_drv);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_disk_open_trk		(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_disk_trk	*p_disk_trk	= NULL;
	int err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_disk_trk	= TBX_MALLOC(sizeof (struct nm_disk_trk));
        if (!p_disk_trk) {
                err = -NM_ENOMEM;
                goto out;
        }
        memset(p_disk_trk, 0, sizeof (struct nm_disk_trk));
        p_trk->priv	= p_disk_trk;

        /* track capabilities encoding					*/
        p_trk->cap.rq_type	= nm_trk_rq_dgram;
        p_trk->cap.iov_type	= nm_trk_iov_none;
        p_trk->cap.max_pending_send_request	= 1;
        p_trk->cap.max_pending_recv_request	= 1;
        p_trk->cap.max_single_request_length	= SSIZE_MAX;
        p_trk->cap.max_iovec_request_length	= 0;
#ifdef IOV_MAX
        p_trk->cap.max_iovec_size		= IOV_MAX;
#else /* IOV_MAX */
        p_trk->cap.max_iovec_size		= sysconf(_SC_IOV_MAX);
#endif /* IOV_MAX */

	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_disk_close_trk		(struct nm_trk *p_trk) {
        struct nm_disk_trk	*p_disk_trk	= NULL;
	int err;

        p_disk_trk	= p_trk->priv;
        TBX_FREE(p_trk->url);
        TBX_FREE(p_disk_trk);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_disk_connect		(void*_status,
			 struct nm_cnx_rq *p_crq) {
        struct nm_disk_gate	*p_disk_gate	= _status;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_disk_trk	*p_disk_trk	= NULL;
        char			*drv_url	= NULL;
        char			*trk_url	= NULL;
        enum nm_disk_mode	 mode;
        int flags;
        int fd;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_disk_trk	= p_trk->priv;

        drv_url		= p_crq->remote_drv_url;
        trk_url		= p_crq->remote_trk_url;

        if (tbx_streq(drv_url, "r")) {
                mode	= ndm_R;
                flags	= O_RDONLY;
        } else if (tbx_streq(drv_url, "w")) {
                mode	= ndm_W;
                flags	= O_WRONLY|O_CREAT|O_TRUNC;
        } else if (tbx_streq(drv_url, "a")) {
                mode	= ndm_A;
                flags	= O_WRONLY|O_CREAT|O_APPEND;
        } else {
                err = -NM_EINVAL;
                goto out;
        }

        SYSCALL(fd = open(trk_url, flags, 0666));

        /* private data				*/
        if (!p_disk_gate) {
                p_disk_gate	= TBX_MALLOC(sizeof (struct nm_disk_gate));
                if (!p_disk_gate) {
                        SYSCALL(close(fd));
                        err = -NM_ENOMEM;
                        goto out;
                }

                memset(p_disk_gate, 0, sizeof(struct nm_disk_gate));

                /* update gate private data		*/
                p_disk_gate->mode	= mode;
        }

        p_disk_gate->fd[p_trk->id]	= fd;
	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_disk_accept			(void*_status,
				 struct nm_cnx_rq *p_crq) {
	return -NM_EINVAL;
}

static
int
nm_disk_disconnect		(void*_status,
				 struct nm_cnx_rq *p_crq) {
        struct nm_disk_gate	*p_disk_gate	= _status;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;

        SYSCALL(close(p_disk_gate->fd[p_trk->id]));

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_disk_outgoing_poll	(void *_status,
			 struct nm_pkt_wrap *p_pw) {
	struct pollfd		 pollfd;
        struct nm_disk_gate	*p_disk_gate	= _status;
        int			 ret;
        int			 err;

        if (!p_pw->gate_priv) {
	        p_pw->gate_priv	= p_disk_gate;
        }

        if (p_disk_gate->mode != ndm_W && p_disk_gate->mode != ndm_A) {
                err = -NM_EINVAL;
                goto out;
        }

        pollfd.fd	= p_disk_gate->fd[p_pw->p_trk->id];
        pollfd.events	= POLLOUT;
        pollfd.revents	= 0;

 poll_again:
        ret	= poll(&pollfd, 1, 0);

        if (ret < 0) {

                /* redo interrupted poll			*/
                if (errno == EINTR)
                        goto poll_again;

                perror("poll");

                /* poll syscall failed				*/
                err = -NM_ESCFAILD;
                goto out;
        }

        /* fd not ready						*/
        if (ret	== 0) {
                err = -NM_EAGAIN;
                goto out;
        }

        /* fd ready, check condition				*/
        if (pollfd.revents != POLLOUT) {
                if (pollfd.revents & POLLHUP) {
                        err = -NM_ECLOSED;
                } else if (pollfd.revents & POLLNVAL) {
                        err = -NM_EINVAL;
                } else {
                        err = -NM_EBROKEN;
                }

                goto out;
        }

        err	= NM_ESUCCESS;

 out:
        return err;
}

static
int
nm_disk_incoming_poll	(struct nm_pkt_wrap *p_pw) {
	struct pollfd		 pollfd;
        struct nm_drv		*p_drv		= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_disk_gate	*p_disk_gate	= NULL;
        struct nm_disk_trk	*p_disk_trk	= NULL;
        struct nm_core		*p_core		= NULL;

        int			 ret;
        int			 err;

        p_disk_trk	= p_pw->p_trk->priv;
        p_drv		= p_pw->p_drv;
        p_core		= p_drv->p_core;

        if (!p_pw->p_gate) {
                NM_DISPF("invalid gate-less request for disk driver");
                err = -NM_EINVAL;
                goto out;
        }

        p_gate		= p_pw->p_gate;

        if (!p_pw->gate_priv) {
	        p_pw->gate_priv = p_disk_gate;
        }


        if (p_disk_gate->mode != ndm_R) {
                NM_DISPF("invalid file mode for incoming poll");
                err = -NM_EINVAL;
                goto out;
        }

        /* check former multipoll result 			*/

        /* poll needed			 			*/
        pollfd.fd	= p_disk_gate->fd[p_pw->p_trk->id];
        pollfd.events	= POLLIN;
        pollfd.revents	= 0;

        NM_TRACE_VAL("disk incoming single poll: pollfd", pollfd.fd);

 poll_single_again:
        ret	= poll(&pollfd, 1, 0);

        if (ret < 0) {

                /* redo interrupted poll			*/
                if (errno == EINTR)
                        goto poll_single_again;

                perror("poll");

                /* poll syscall failed				*/
                err = -NM_ESCFAILD;
                goto out;
        }

        /* fd not ready						*/
        if (ret	== 0) {
                NM_TRACEF("disk incoming single poll: no hit");
                err = -NM_EAGAIN;
                goto out;
        }

        NM_TRACE_VAL("disk incoming single poll ret", ret);

        /* fd ready, check condition				*/
        if (pollfd.revents != POLLIN) {
                if (pollfd.revents & POLLHUP) {
                        NM_TRACEF("disk incoming single poll: pollhup");
                        err = -NM_ECLOSED;
                } else if (pollfd.revents & POLLNVAL) {
                        NM_TRACEF("disk incoming single poll: pollnval");
                        err = -NM_EINVAL;
                } else {
                        err = -NM_EBROKEN;
                }

                goto out;
        }

        err	= NM_ESUCCESS;

 out:
        return err;
}

/* used for post and poll */
static
int
nm_disk_send_iov	(void*_status,
			 struct nm_pkt_wrap *p_pw) {
        struct nm_disk_gate	*p_disk_gate	= NULL;
        struct nm_disk_pkt_wrap	*p_disk_pw	= NULL;
        struct nm_iovec_iter	*p_vi		= NULL;
        struct iovec		*p_cur		= NULL;
        int			 ret;
        int			 err;

        err	= nm_disk_outgoing_poll(_status, p_pw);
        if (err < 0)
                goto out_complete;

	p_disk_gate	= p_pw->gate_priv;
        p_disk_pw	= p_pw->drv_priv;
        if (!p_disk_pw) {
                p_disk_pw	= TBX_MALLOC(sizeof(struct nm_disk_pkt_wrap));
                /* current entry num is first entry num
                 */
                p_disk_pw->vi.v_cur	= p_pw->v_first;

                /* save a copy of current entry
                 */
                p_disk_pw->vi.cur_copy	= p_pw->v[p_pw->v_first];

                /* current vector size is full vector size
                 */
                p_disk_pw->vi.v_cur_size	= p_pw->v_nb;
        }

        /* get a pointer to the iterator
         */
        p_vi	= &(p_disk_pw->vi);

        /* get a pointer to the current entry
         */
        p_cur	= p_pw->v + p_disk_pw->vi.v_cur;

        NM_TRACE_VAL("disk outgoing trk id", p_pw->p_trk->id);
        NM_TRACE_VAL("disk outgoing fd", p_disk_gate->fd[p_pw->p_trk->id]);
        NM_TRACE_VAL("disk outgoing cur size", p_vi->v_cur_size);
        NM_TRACE_PTR("disk outgoing cur base", p_cur->iov_base);
        NM_TRACE_VAL("disk outgoing cur len", p_cur->iov_len);

        if (!p_vi->v_cur_size) {
                err	= NM_ESUCCESS;

                goto out_complete;
        }

 writev_again:
        ret	= writev(p_disk_gate->fd[p_pw->p_trk->id], p_cur, p_vi->v_cur_size);
        NM_TRACE_VAL("disk outgoing writev ret", ret);

        if (ret < 0) {
                if (errno == EINTR)
                        goto writev_again;

                perror("writev");
                err = -NM_ESCFAILD;
                goto out_complete;
        }

        if (ret	== 0) {
                NM_TRACEF("connection closed");
                err = -NM_ECLOSED;
                goto out_complete;
        }

        do {
                uint64_t len	= tbx_min(p_cur->iov_len, ret);
                p_cur->iov_base	+= len;
                p_cur->iov_len	-= len;

                /* still something to send for this vector entry?
                 */
                if (p_cur->iov_len)
                        break;

                /* restore vector entry
                 */
                *p_cur = p_vi->cur_copy;

                /* increment cursors
                 */
                p_vi->v_cur++;
                p_vi->v_cur_size--;

                if (!p_vi->v_cur_size) {
                        err	= NM_ESUCCESS;

                        goto out_complete;
                }

                p_cur++;
                p_vi->cur_copy = *p_cur;
                ret -= len;
        } while (ret);

        err	= -NM_EAGAIN;

 out:
        return err;

 out_complete:
        if (p_pw->drv_priv) {
                TBX_FREE(p_pw->drv_priv);
                p_pw->drv_priv	= NULL;
        }

        goto out;
}

/* used for post and poll */
static
int
nm_disk_recv_iov	(void*_status,
			 struct nm_pkt_wrap *p_pw) {
        struct nm_disk_gate	*p_disk_gate	= NULL;
        struct nm_disk_pkt_wrap	*p_disk_pw	= NULL;
        struct nm_iovec_iter	*p_vi		= NULL;
        struct iovec		*p_cur		= NULL;
        int			 ret;
        int			 err;

        err	= nm_disk_incoming_poll(p_pw);
        if (err < 0)
                goto out_complete;

        p_disk_gate	= p_pw->gate_priv;

        p_disk_pw	= p_pw->drv_priv;
        if (!p_disk_pw) {
                p_disk_pw	= TBX_MALLOC(sizeof(struct nm_disk_pkt_wrap));
                p_pw->drv_priv	= p_disk_pw;

                /* current entry num is first entry num
                 */
                p_disk_pw->vi.v_cur	= p_pw->v_first;

                /* save a copy of current entry
                 */
                p_disk_pw->vi.cur_copy	= p_pw->v[p_pw->v_first];

                /* current vector size is full vector size
                 */
                p_disk_pw->vi.v_cur_size	= p_pw->v_nb;
        }

        /* get a pointer to the iterator
         */
        p_vi	= &(p_disk_pw->vi);

        /* get a pointer to the current entry
         */
        p_cur	= p_pw->v + p_disk_pw->vi.v_cur;

        NM_TRACE_VAL("disk incoming trk id", p_pw->p_trk->id);
        NM_TRACE_VAL("disk incoming fd", p_disk_gate->fd[p_pw->p_trk->id]);
        NM_TRACE_VAL("disk incoming cur size", p_vi->v_cur_size);
        NM_TRACE_PTR("disk incoming cur base", p_cur->iov_base);
        NM_TRACE_VAL("disk incoming cur len", p_cur->iov_len);

        if (!p_vi->v_cur_size) {
                err	= NM_ESUCCESS;

                goto out_complete;
        }

 readv_again:
        ret	= readv(p_disk_gate->fd[p_pw->p_trk->id], p_cur, p_vi->v_cur_size);

        if (ret < 0) {
                if (errno == EINTR)
                        goto readv_again;

                perror("readv");

                err = -NM_ESCFAILD;
                goto out_complete;
        }

        if (ret	== 0) {
                NM_TRACEF("connection closed");
                err = -NM_ECLOSED;
                goto out_complete;
        }

        do {
                uint64_t len	= tbx_min(p_cur->iov_len, ret);
                p_cur->iov_base	+= len;
                p_cur->iov_len	-= len;

                /* still something to send for this vector entry?
                 */
                if (p_cur->iov_len)
                        break;

                /* restore vector entry
                 */
                *p_cur = p_vi->cur_copy;

                /* increment cursors
                 */
                p_vi->v_cur++;
                p_vi->v_cur_size--;

                if (!p_vi->v_cur_size) {
                        err	= NM_ESUCCESS;
                        goto out_complete;
                }

                p_cur++;
                p_vi->cur_copy = *p_cur;
                ret -= len;
        } while (ret);

        err	= -NM_EAGAIN;

 out:
        return err;

 out_complete:
        if (p_pw->drv_priv) {
                TBX_FREE(p_pw->drv_priv);
                p_pw->drv_priv	= NULL;
        }

        goto out;
}

