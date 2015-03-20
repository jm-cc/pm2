/*
 * NewMadeleine
 * Copyright (C) 2010 (see AUTHORS file)
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
#include <fcntl.h>
#include <limits.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <nm_private.h>

#include <Padico/Module.h>


PUK_LFQUEUE_TYPE(nm_pw, struct nm_pkt_wrap*, NULL, 1024);


/** 'self' per-gate data (singleton, actually)
 */
struct nm_self
{
  struct nm_pw_lfqueue_s queues[2]; /**< pw queues (2 trks) */
};


/** self NewMad Driver */
static const char*nm_self_get_driver_url(struct nm_drv *p_drv);
static int nm_self_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_self_init(struct nm_drv* p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_self_exit(struct nm_drv* p_drv);
static int nm_self_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url);
static int nm_self_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id);
static int nm_self_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_self_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_self_poll_send(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_self_poll_recv(void*_status, struct nm_pkt_wrap *p_pw);
#ifdef PIOM_BLOCKING_CALLS
static int nm_self_wait_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_self_wait_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
#endif

static const struct nm_drv_iface_s nm_self_driver =
  {
    .name               = "self",
    
    .query              = &nm_self_query,
    .init               = &nm_self_init,
    .close              = &nm_self_exit,

    .connect		= &nm_self_connect,
    .disconnect         = &nm_self_disconnect,

    .post_send_iov	= &nm_self_send_iov,
    .post_recv_iov      = &nm_self_recv_iov,

    .poll_send_iov	= &nm_self_poll_send,
    .poll_recv_iov	= &nm_self_poll_recv,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .get_driver_url     = &nm_self_get_driver_url,

#ifdef PIOM_BLOCKING_CALLS
    .wait_send_iov	= &nm_self_wait_send_iov,
    .wait_recv_iov	= &nm_self_wait_recv_iov,

    .wait_recv_any_iov  = NULL,
    .wait_send_any_iov  = NULL,
#endif
    .capabilities.min_period    = 0,
    .capabilities.is_exportable = 0
  };

/** 'PadicoComponent' facet for self driver */
static void*nm_self_instantiate(puk_instance_t, puk_context_t);
static void nm_self_destroy(void*);

static const struct puk_component_driver_s nm_self_component_driver =
  {
    .instantiate = &nm_self_instantiate,
    .destroy     = &nm_self_destroy
  };


/** Component declaration */
PADICO_MODULE_COMPONENT(NewMad_Driver_self,
			puk_component_declare("NewMad_Driver_self",
					      puk_component_provides("PadicoComponent", "component", &nm_self_component_driver),
					      puk_component_provides("NewMad_Driver", "driver", &nm_self_driver)));


/** Instanciate functions */
static void*nm_self_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_self*status = TBX_MALLOC(sizeof(struct nm_self));
  static int init_done = 0;
  if(init_done)
    {
      fprintf(stderr, "nmad: self- cannot create more than 1 instance for 'self'.\n");
      abort();
    }
  init_done = 1;
  nm_pw_lfqueue_init(&status->queues[0]);
  nm_pw_lfqueue_init(&status->queues[1]);
  return status;
}

static void nm_self_destroy(void*_status)
{
  TBX_FREE(_status);
}

/** Url function */
static const char*nm_self_get_driver_url(struct nm_drv *p_drv)
{
  static const char*url = NULL;
  if(url == NULL)
    url = strdup("-");
  return url;
}

static int nm_self_query(struct nm_drv *p_drv,
			 struct nm_driver_query_param *params TBX_UNUSED,
			 int nparam TBX_UNUSED)
{
  p_drv->profile.latency = INT_MAX;
  p_drv->profile.bandwidth = 0;
  p_drv->priv = NULL;
  return NM_ESUCCESS;
}

static int nm_self_init(struct nm_drv* p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  /* open the requested number of tracks */
  int i;
  for(i = 0; i < nb_trks; i++)
    {
      /* track capabilities encoding */
      trk_caps[i].rq_type	= nm_trk_rq_dgram;
      trk_caps[i].iov_type	= nm_trk_iov_both_assym;
      trk_caps[i].max_pending_send_request  = 1;
      trk_caps[i].max_pending_recv_request  = 1;
      trk_caps[i].max_single_request_length = SSIZE_MAX;
      trk_caps[i].max_iovec_request_length  = 0;
#ifdef IOV_MAX
      trk_caps[i].max_iovec_size	    = IOV_MAX;
#else /* IOV_MAX */
      trk_caps[i].max_iovec_size	    = sysconf(_SC_IOV_MAX);
#endif /* IOV_MAX */
    }
  return NM_ESUCCESS;
}

static int nm_self_exit(struct nm_drv*p_drv)
{
  return NM_ESUCCESS;
}

static int nm_self_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url)
{
  return NM_ESUCCESS;
}

static int nm_self_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id)
{
  return NM_ESUCCESS;
}

static int nm_self_send_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_self*status = (struct nm_self*)_status;
  p_pw->drv_priv = (void*)0x01;
  int err = nm_pw_lfqueue_enqueue(&status->queues[p_pw->trk_id], p_pw);
  if(err != 0)
    {
      fprintf(stderr, "nmad: self- queue full while sending data.\n");
    }
  return -NM_EAGAIN;
}

static int nm_self_poll_send(void*_status, struct nm_pkt_wrap *p_pw)
{
  if(p_pw->drv_priv == NULL)
    return NM_ESUCCESS;
  else
    return -NM_EAGAIN;
}

static int nm_self_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_self*status = (struct nm_self*)_status;
  if(p_pw->v_nb != 1)
    {
      fprintf(stderr, "nmad: self- iovec not supported on recv side yet.\n");
      abort();
    }
  return nm_self_poll_recv(_status, p_pw);
}

static int nm_self_poll_recv(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_self*status = (struct nm_self*)_status;
  struct nm_pkt_wrap*p_send_pw = nm_pw_lfqueue_dequeue(&status->queues[p_pw->trk_id]);
  if(p_send_pw == NULL)
    return -NM_EAGAIN;
  int done = 0;
  int i;
  for(i = 0; i < p_send_pw->v_nb; i++)
    {
      if(done + p_send_pw->v[i].iov_len > p_pw->v[0].iov_len)
	{
	  fprintf(stderr, "nmad: self- trying to send more data than receive posted.\n");
	  abort();
	}
      memcpy(p_pw->v[0].iov_base + done, p_send_pw->v[i].iov_base, p_send_pw->v[i].iov_len);
      done += p_send_pw->v[i].iov_len;
    }
  p_send_pw->drv_priv = NULL;
  return NM_ESUCCESS;
}

