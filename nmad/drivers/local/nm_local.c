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

static int nm_local_load(void);

PADICO_MODULE_BUILTIN(NewMad_Driver_local, &nm_local_load, NULL, NULL);

#ifdef CONFIG_PUK_PUKABI
#include <Padico/Puk-ABI.h>
#endif /* CONFIG_PUK_PUKABI */

#if defined(CONFIG_PUK_PUKABI) && defined(PADICO_ENABLE_PUKABI_FSYS)
#define NM_SYS(SYMBOL) PUK_ABI_WRAP(SYMBOL)
#else  /* PADICO_ENABLE_PUKABI_FSYS */
#define NM_SYS(SYMBOL) SYMBOL
#endif /* PADICO_ENABLE_PUKABI_FSYS */

/** 'local' driver per-instance data.
 */
struct nm_local_drv
{
  /** server socket	*/
  int server_fd;
  /** url */
  char*url;
  /** capabilities */
  struct nm_drv_cap caps;
  int nb_trks;
};

/** 'local' per-gate data.
 */
struct nm_local
{
  /** one socket per track.
   */
  int	fd[2];
};


/** local NewMad Driver */
static struct nm_drv_cap*nm_local_get_capabilities(struct nm_drv *p_drv);
static const char*nm_local_get_driver_url(struct nm_drv *p_drv);
static int nm_local_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_local_init(struct nm_drv* p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_local_exit(struct nm_drv* p_drv);
static int nm_local_connect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_local_accept(void*_status, struct nm_cnx_rq *p_crq);
static int nm_local_disconnect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_local_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_local_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_local_poll_send(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_local_poll_recv(void*_status, struct nm_pkt_wrap *p_pw);
#ifdef PIOM_BLOCKING_CALLS
static int nm_local_wait_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_local_wait_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
#endif

static const struct nm_drv_iface_s nm_local_driver =
  {
    .name               = "local",
    
    .query              = &nm_local_query,
    .init               = &nm_local_init,
    .close              = &nm_local_exit,

    .connect		= &nm_local_connect,
    .accept		= &nm_local_accept,
    .disconnect         = &nm_local_disconnect,

    .post_send_iov	= &nm_local_send_iov,
    .post_recv_iov      = &nm_local_recv_iov,

    .poll_send_iov	= &nm_local_poll_send,
    .poll_recv_iov	= &nm_local_poll_recv,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .get_driver_url     = &nm_local_get_driver_url,
    .get_capabilities   = &nm_local_get_capabilities,

#ifdef PIOM_BLOCKING_CALLS
    .wait_send_iov	= &nm_local_wait_send_iov,
    .wait_recv_iov	= &nm_local_wait_recv_iov,

    .wait_recv_any_iov  = NULL,
    .wait_send_any_iov  = NULL
#endif
  };

/** 'PadicoAdapter' facet for local driver */
static void*nm_local_instanciate(puk_instance_t, puk_context_t);
static void nm_local_destroy(void*);

static const struct puk_adapter_driver_s nm_local_adapter_driver =
  {
    .instanciate = &nm_local_instanciate,
    .destroy     = &nm_local_destroy
  };


/** Component declaration */
static int nm_local_load(void)
{
  puk_component_declare("NewMad_Driver_local",
			puk_component_provides("PadicoAdapter", "adapter", &nm_local_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_local_driver));
  return 0;
}


/** Instanciate functions */
static void*nm_local_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_local*status = TBX_MALLOC(sizeof(struct nm_local));
  memset(status, 0, sizeof(struct nm_local));
  return status;
}

static void nm_local_destroy(void*_status)
{
  TBX_FREE(_status);
}

/** Return capabilities */
static struct nm_drv_cap*nm_local_get_capabilities(struct nm_drv *p_drv)
{
  struct nm_local_drv *p_local_drv = p_drv->priv;
  return &p_local_drv->caps;
}

/** Url function */
static const char*nm_local_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_local_drv *p_local_drv = p_drv->priv;
  return p_local_drv->url;
}


static int nm_local_query(struct nm_drv *p_drv,
			  struct nm_driver_query_param *params TBX_UNUSED,
			  int nparam TBX_UNUSED)
{
  struct nm_local_drv* p_local_drv = TBX_MALLOC(sizeof(struct nm_local_drv));
  memset(p_local_drv, 0, sizeof(struct nm_local_drv));
  
  p_local_drv->caps.has_trk_rq_dgram = 1;
  p_local_drv->caps.has_selective_receive = 1;
  p_local_drv->caps.has_concurrent_selective_receive = 1;
#ifdef PIOM_BLOCKING_CALLS
  p_local_drv->caps.is_exportable = 0;
#endif
#ifdef PM2_NUIOA
  p_local_drv->caps.numa_node = PM2_NUIOA_ANY_NODE;
#endif
  p_local_drv->caps.latency = 10000;
  p_local_drv->caps.bandwidth = 1000;
  p_local_drv->caps.min_period = 0;
  p_drv->priv = p_local_drv;
  return NM_ESUCCESS;
}

static int nm_local_init(struct nm_drv* p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  struct nm_local_drv*p_local_drv = p_drv->priv;
  const pid_t pid = getpid();
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/nmad_local.%d", pid);
  p_local_drv->url = strdup(addr.sun_path);
  p_local_drv->server_fd = NM_SYS(socket)(AF_UNIX, SOCK_STREAM, 0);
  NM_SYS(unlink)(addr.sun_path);
  int rc = NM_SYS(bind)(p_local_drv->server_fd, (struct sockaddr*)&addr, sizeof(addr));
  if(rc != 0)
    {
      fprintf(stderr, "nmad: local- bind error: %s\n", strerror(errno));
      abort();
    }
  rc = NM_SYS(listen)(p_local_drv->server_fd, 128);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: local- listen error: %s\n", strerror(errno));
      abort();
    }

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

static int nm_local_exit(struct nm_drv*p_drv)
{
  struct nm_local_drv*p_local_drv = p_drv->priv;
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/nmad_local.%d", getpid());
  NM_SYS(unlink)(addr.sun_path);
  TBX_FREE(p_local_drv->url);
  TBX_FREE(p_local_drv);
  return NM_ESUCCESS;
}

static int nm_local_connect(void*_status, struct nm_cnx_rq *p_crq)
{
  struct sockaddr_un addr;
  struct nm_local*status = (struct nm_local*)_status;
  int fd = NM_SYS(socket)(AF_UNIX, SOCK_STREAM, 0);
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, p_crq->remote_drv_url);
  int rc = NM_SYS(connect)(fd, (struct sockaddr*)&addr, sizeof(addr));
  if(rc != 0)
    {
      fprintf(stderr, "nmad: local- cannot connect to %s\n", addr.sun_path);
      abort();
    }
  status->fd[p_crq->trk_id] = fd;
  return NM_ESUCCESS;
}

static int nm_local_accept(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_local_drv*p_local_drv = p_crq->p_drv->priv;
  struct nm_local*status = (struct nm_local*)_status;
  int fd = NM_SYS(accept)(p_local_drv->server_fd, NULL, NULL);
  if(fd < 0)
    {
      fprintf(stderr, "nmad: local- error while accepting\n");
      abort();
    }
  status->fd[p_crq->trk_id] = fd;
  return NM_ESUCCESS;
}

static int nm_local_disconnect(void*_status, struct nm_cnx_rq*p_crq)
{
  struct nm_local*status = (struct nm_local*)_status;
  int fd = status->fd[p_crq->trk_id];
  /* half-close for sending */
  NM_SYS(shutdown)(fd, SHUT_WR);
  /* flush (and throw away) remaining bytes in the pipe up to the EOS */
  int ret = -1;
  do
    {
      int dummy = 0;
      ret = NM_SYS(read)(fd, &dummy, sizeof(dummy));
    }
  while (ret > 0 || ((ret == -1 && errno == EAGAIN)));
  /* safely close fd when EOS is reached */
  NM_SYS(close)(fd);
  return NM_ESUCCESS;
}

static int nm_local_send_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_local*status = (struct nm_local*)_status;
  const int fd = status->fd[p_pw->trk_id];
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
  const int err = errno;
  if(rc < 0 && err == EPIPE)
    {
      return -NM_ECLOSED;
    }
  else if(rc < 0 || rc < size + sizeof(size))
    
    {
      fprintf(stderr, "nmad: local- error %d while sending message (%s).\n", err, strerror(err));
      abort();
    }
  return NM_ESUCCESS;
}

static int nm_local_poll_send(void*_status, struct nm_pkt_wrap *p_pw)
{
  return NM_ESUCCESS;
}

static int nm_local_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  if(p_pw->v_nb != 1)
    {
      fprintf(stderr, "nmad: local- iovec not supported on recv side yet.\n");
      abort();
    }
  return nm_local_poll_recv(_status, p_pw);
}


static int nm_local_poll_recv(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_local*status = (struct nm_local*)_status;
  const int fd = status->fd[p_pw->trk_id];
  struct iovec*iov = &p_pw->v[0];
  int size = 0;
  int rc = NM_SYS(recv)(fd, &size, sizeof(size), MSG_DONTWAIT);
  int err = errno;
  if(rc == 0)
    {
      return -NM_ECLOSED;
    }
  else if(rc < 0 && err == EAGAIN)
    {
      return -NM_EAGAIN;
    }
  else if(rc < 0)
    {
      fprintf(stderr, "nmad: local- error %d while reading header (%s).\n", err, strerror(err));
      abort();
    }
  else if(size > iov->iov_len)
    {
      fprintf(stderr, "nmad: local- received more data than expected.\n");
      abort();
    }
  rc = NM_SYS(recv)(fd, iov->iov_base, size, MSG_WAITALL);
  if(rc < 0 || rc != size)
    {
      fprintf(stderr, "nmad: local- error %d while reading data (%s).\n", err, strerror(err));
      abort();
    }
  return NM_ESUCCESS;
}

