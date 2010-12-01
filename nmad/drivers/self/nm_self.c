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

static int nm_self_load(void);

PADICO_MODULE_BUILTIN(NewMad_Driver_self, &nm_self_load, NULL, NULL);

#ifdef CONFIG_PUK_PUKABI
#include <Padico/Puk-ABI.h>
#endif /* CONFIG_PUK_PUKABI */

#if defined(CONFIG_PUK_PUKABI) && defined(PADICO_ENABLE_PUKABI_FSYS)
#define NM_SYS(SYMBOL) PUK_ABI_WRAP(SYMBOL)
#else  /* PADICO_ENABLE_PUKABI_FSYS */
#define NM_SYS(SYMBOL) SYMBOL
#endif /* PADICO_ENABLE_PUKABI_FSYS */


/** 'self' driver per-instance data.
 */
struct nm_self_drv
{
  /** 2 pipes */
  int fd[4];
  /** url */
  char*url;
  /** capabilities */
  struct nm_drv_cap caps;
  int nb_trks;
};

/** 'self' per-gate data.
 */
struct nm_self
{
  /** one fd per track.
   */
  int fd[4];
};


/** self NewMad Driver */
static struct nm_drv_cap*nm_self_get_capabilities(struct nm_drv *p_drv);
static const char*nm_self_get_driver_url(struct nm_drv *p_drv);
static int nm_self_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_self_init(struct nm_drv* p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_self_exit(struct nm_drv* p_drv);
static int nm_self_connect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_self_accept(void*_status, struct nm_cnx_rq *p_crq);
static int nm_self_disconnect(void*_status, struct nm_cnx_rq *p_crq);
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
    .accept		= &nm_self_accept,
    .disconnect         = &nm_self_disconnect,

    .post_send_iov	= &nm_self_send_iov,
    .post_recv_iov      = &nm_self_recv_iov,

    .poll_send_iov	= &nm_self_poll_send,
    .poll_recv_iov	= &nm_self_poll_recv,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .get_driver_url     = &nm_self_get_driver_url,
    .get_capabilities   = &nm_self_get_capabilities,

#ifdef PIOM_BLOCKING_CALLS
    .wait_send_iov	= &nm_self_wait_send_iov,
    .wait_recv_iov	= &nm_self_wait_recv_iov,

    .wait_recv_any_iov  = NULL,
    .wait_send_any_iov  = NULL
#endif
  };

/** 'PadicoAdapter' facet for self driver */
static void*nm_self_instanciate(puk_instance_t, puk_context_t);
static void nm_self_destroy(void*);

static const struct puk_adapter_driver_s nm_self_adapter_driver =
  {
    .instanciate = &nm_self_instanciate,
    .destroy     = &nm_self_destroy
  };


/** Component declaration */
static int nm_self_load(void)
{
  puk_component_declare("NewMad_Driver_self",
			puk_component_provides("PadicoAdapter", "adapter", &nm_self_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_self_driver));
  return 0;
}



/** Instanciate functions */
static void*nm_self_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_self*status = TBX_MALLOC(sizeof(struct nm_self));
  memset(status, 0, sizeof(struct nm_self));
  return status;
}

static void nm_self_destroy(void*_status)
{
  TBX_FREE(_status);
}

/** Return capabilities */
static struct nm_drv_cap*nm_self_get_capabilities(struct nm_drv *p_drv)
{
  struct nm_self_drv *p_self_drv = p_drv->priv;
  return &p_self_drv->caps;
}

/** Url function */
static const char*nm_self_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_self_drv *p_self_drv = p_drv->priv;
  return p_self_drv->url;
}


static int nm_self_query(struct nm_drv *p_drv,
			 struct nm_driver_query_param *params TBX_UNUSED,
			 int nparam TBX_UNUSED)
{
  struct nm_self_drv* p_self_drv = TBX_MALLOC(sizeof(struct nm_self_drv));
  memset(p_self_drv, 0, sizeof(struct nm_self_drv));
  
  p_self_drv->caps.has_trk_rq_dgram = 1;
  p_self_drv->caps.has_selective_receive = 1;
  p_self_drv->caps.has_concurrent_selective_receive = 1;
#ifdef PIOM_BLOCKING_CALLS
  p_self_drv->caps.is_exportable = 0;
#endif
#ifdef PM2_NUIOA
  p_self_drv->caps.numa_node = PM2_NUIOA_ANY_NODE;
#endif
  p_self_drv->caps.latency = INT_MAX;
  p_self_drv->caps.bandwidth = 0;
  p_drv->priv = p_self_drv;
  return NM_ESUCCESS;
}

static int nm_self_init(struct nm_drv* p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  struct nm_self_drv*p_self_drv = p_drv->priv;
  int rc = NM_SYS(pipe)(&p_self_drv->fd[0]);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: self- pipe error: %s\n", strerror(errno));
      abort();
    }
  rc = NM_SYS(pipe)(&p_self_drv->fd[2]);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: self- pipe error: %s\n", strerror(errno));
      abort();
    }
  p_self_drv->url = strdup("-");

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
  struct nm_self_drv*p_self_drv = p_drv->priv;
  TBX_FREE(p_self_drv->url);
  TBX_FREE(p_self_drv);
  return NM_ESUCCESS;
}

static int nm_self_connect(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_self*status = (struct nm_self*)_status;
  struct nm_self_drv*p_self_drv = p_crq->p_drv->priv;
  memcpy(&status->fd[0], &p_self_drv->fd[0], 4*sizeof(int));
  return NM_ESUCCESS;
}

static int nm_self_accept(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_self_drv*p_self_drv = p_crq->p_drv->priv;
  struct nm_self*status = (struct nm_self*)_status;
  memcpy(&status->fd[0], &p_self_drv->fd[0], 4*sizeof(int));
  return NM_ESUCCESS;
}

static int nm_self_disconnect(void*_status, struct nm_cnx_rq*p_crq)
{
  struct nm_self*status = (struct nm_self*)_status;
  const int rfd = status->fd[p_crq->trk_id];
  const int wfd = status->fd[1+p_crq->trk_id];
  NM_SYS(close)(rfd);
  NM_SYS(close)(wfd);
  status->fd[p_crq->trk_id] = -1;
  status->fd[1+p_crq->trk_id] = -1;
  return NM_ESUCCESS;
}

static int nm_self_send_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_self*status = (struct nm_self*)_status;
  if(p_pw->trk_id == 0)
    {
      const int fd = status->fd[1];
      struct iovec*iov = p_pw->v;
      const int n = p_pw->v_nb;
      struct iovec send_iov[1 + n];
      int size = 0;
      int i;
      for(i = 0; i < n; i++)
	{
	  send_iov[i+1] = (struct iovec){ .iov_base = iov[i].iov_base, .iov_len = iov[i].iov_len };
	  size += iov[i].iov_len;
	}
      send_iov[0] = (struct iovec){ .iov_base = &size, .iov_len = sizeof(size) };
      int rc = NM_SYS(writev)(fd, send_iov, n+1);
      if(rc < 0 || rc < size + sizeof(size))
	{
	  fprintf(stderr, "nmad: self- error %d while sending message.\n", errno);
	  abort();
	}
    }
  else
    {
      const int fd = status->fd[2];
      assert(p_pw->v_nb == 1);
      struct nm_pkt_wrap*p_pw_recv = NULL;
      int rc = NM_SYS(read)(fd, &p_pw_recv, sizeof(p_pw_recv));
      assert(rc == sizeof(p_pw_recv));
      assert(p_pw_recv->v[0].iov_len == p_pw->v[0].iov_len);
      memcpy(p_pw_recv->v[0].iov_base, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
      p_pw_recv->drv_priv = (void*)0x01;
    }
  return NM_ESUCCESS;
}

static int nm_self_poll_send(void*_status, struct nm_pkt_wrap *p_pw)
{
  return NM_ESUCCESS;
}

static int nm_self_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_self*status = (struct nm_self*)_status;
  if(p_pw->v_nb != 1)
    {
      fprintf(stderr, "nmad: self- iovec not supported on recv side yet.\n");
      abort();
    }
  if(p_pw->trk_id == 0)
    {
      return nm_self_poll_recv(_status, p_pw);
    }
  else
    {
      p_pw->drv_priv = NULL;
      const int fd = status->fd[3];
      int rc = NM_SYS(write)(fd, &p_pw, sizeof(p_pw));
      assert(rc == sizeof(p_pw));
      return -NM_EAGAIN;
    }
}


static int nm_self_poll_recv(void*_status, struct nm_pkt_wrap *p_pw)
{
  if(p_pw->trk_id == 0)
    {
      struct nm_self*status = (struct nm_self*)_status;
      const int fd = status->fd[2*p_pw->trk_id];
      struct iovec*iov = &p_pw->v[0];
      int size = 0;
      struct pollfd fds = { .fd = fd, .events = POLLIN };
      int rc = NM_SYS(poll)(&fds, 1, 0);
      if(rc == 0)
	{
	  return -NM_EAGAIN;
	}
      else if(rc<0 && errno == EINTR)
        {
	  return -NM_EAGAIN;
        }
      else if((rc < 0) || ((rc > 0) && (fds.revents & (POLLERR|POLLHUP|POLLNVAL))))
	{
	  return -NM_ECLOSED;
	}
      rc = NM_SYS(read)(fd, &size, sizeof(size));
      const int err = errno;
      if(rc < sizeof(size))
	{
	  fprintf(stderr, "nmad: self- error %d while reading header (%s).\n", err, strerror(err));
	  abort();
	}
      else if(size > iov->iov_len)
	{
	  fprintf(stderr, "nmad: self- received more data than expected.\n");
	  abort();
	}
      rc = NM_SYS(read)(fd, iov->iov_base, size);
      if(rc < 0 || rc != size)
	{
	  fprintf(stderr, "nmad: self- error %d while reading data.\n", errno);
	  abort();
	}
      return NM_ESUCCESS;
    }
  else
    {
      if(p_pw->drv_priv == NULL)
	return NM_ESUCCESS;
      else
	return -NM_EAGAIN;
    }
}

