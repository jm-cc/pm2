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

#include <nm_minidriver.h>
#include <nm_private.h>
#include <Padico/Module.h>

static void*nm_local_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_local_destroy(void*status);

static const struct puk_component_driver_s nm_local_component =
  {
    .instantiate = &nm_local_instantiate,
    .destroy     = &nm_local_destroy
  };

static void nm_local_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_local_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_local_close(puk_context_t context);
static void nm_local_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_local_send_post(void*_status, const struct iovec*v, int n);
static int  nm_local_send_poll(void*_status);
static void nm_local_recv_init(void*_status,  struct iovec*v, int n);
static int  nm_local_poll_one(void*_status);
static int  nm_local_cancel_recv(void*_status);

static const struct nm_minidriver_iface_s nm_local_minidriver =
  {
    .getprops    = &nm_local_getprops,
    .init        = &nm_local_init,
    .close       = &nm_local_close,
    .connect     = &nm_local_connect,
    .send_post   = &nm_local_send_post,
    .send_poll   = &nm_local_send_poll,
    .recv_init   = &nm_local_recv_init,
    .poll_one    = &nm_local_poll_one,
    .cancel_recv = &nm_local_cancel_recv
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_local,
  puk_component_declare("Minidriver_local",
			puk_component_provides("PadicoComponent", "component", &nm_local_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_local_minidriver)));

/* ********************************************************* */

/** 'local' driver per-context data. */
struct nm_local_context_s
{
  struct sockaddr_un addr;  /**< server socket address */
  int server_fd;            /**< server socket fd */
  char*url;                 /**< server url */
};

/** 'local' per-instance status. */
struct nm_local_s
{
  int fd;                   /**< Unix socket */
  struct nm_local_context_s*p_local_context;
  struct
  {
    struct iovec*v;
    int n;
  } recv;
};

#define NM_LOCAL_URL_SIZE_MAX 64
/** identity of peer node */
struct nm_local_peer_id_s
{
  char url[NM_LOCAL_URL_SIZE_MAX];
};
/** pending connection */
struct nm_local_pending_s
{
  int fd;
  struct nm_local_peer_id_s peer;
};
PUK_VECT_TYPE(nm_local_pending, struct nm_local_pending_s);

/* ********************************************************* */

static void*nm_local_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_local_s*p_status = padico_malloc(sizeof(struct nm_local_s));
  struct nm_local_context_s*p_local_context = puk_context_get_status(context);
  p_status->fd = -1;
  p_status->p_local_context = p_local_context;
  return p_status;
}

static void nm_local_destroy(void*_status)
{
  struct nm_local_s*p_status = _status;
  /* half-close for sending */
  NM_SYS(shutdown)(p_status->fd, SHUT_WR);
  /* flush (and throw away) remaining bytes in the pipe up to the EOS */
  int ret = -1;
  do
    {
      int dummy = 0;
      ret = NM_SYS(read)(p_status->fd, &dummy, sizeof(dummy));
    }
  while (ret > 0 || ((ret == -1 && errno == EAGAIN)));
  /* safely close fd when EOS is reached */
  NM_SYS(close)(p_status->fd);
  padico_free(p_status);
}

/* ********************************************************* */

static void nm_local_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  props->profile.latency = 500;
  props->profile.bandwidth = 8000;
}

static void nm_local_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_local_context_s*p_local_context = padico_malloc(sizeof(struct nm_local_context_s));
  p_local_context->addr.sun_family = AF_UNIX;
  snprintf(p_local_context->addr.sun_path, sizeof(p_local_context->addr.sun_path),
	   "/tmp/nmad_local_%s.%d.%p", getenv("LOGNAME"), getpid(), p_local_context);
  p_local_context->url = strdup(p_local_context->addr.sun_path);
  p_local_context->server_fd = NM_SYS(socket)(AF_UNIX, SOCK_STREAM, 0);
  NM_SYS(unlink)(p_local_context->addr.sun_path);
  int rc = NM_SYS(bind)(p_local_context->server_fd, (struct sockaddr*)&p_local_context->addr, sizeof(p_local_context->addr));
  if(rc != 0)
    {
      fprintf(stderr, "nmad: local- bind error: %s\n", strerror(errno));
      abort();
    }
  rc = NM_SYS(listen)(p_local_context->server_fd, 128);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: local- listen error: %s\n", strerror(errno));
      abort();
    }
  puk_context_set_status(context, p_local_context);
  *drv_url = p_local_context->url;
  *url_size = strlen(p_local_context->url) + 1;
}

static void nm_local_close(puk_context_t context)
{
  struct nm_local_context_s*p_local_context = puk_context_get_status(context);
  NM_SYS(unlink)(p_local_context->addr.sun_path);
  puk_context_set_status(context, NULL);
  padico_free(p_local_context->url);
  padico_free(p_local_context);
}

static void nm_local_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_local_s*p_status = _status;
  struct nm_local_context_s*p_local_context = p_status->p_local_context;
  static nm_local_pending_vect_t pending_fds = NULL;

  if(strcmp(p_local_context->url, remote_url) > 0)
    {
      /* ** connect */
      struct sockaddr_un addr;
      int fd = NM_SYS(socket)(AF_UNIX, SOCK_STREAM, 0);
      addr.sun_family = AF_UNIX;
      strcpy(addr.sun_path, remote_url);
      int rc = NM_SYS(connect)(fd, (struct sockaddr*)&addr, sizeof(addr));
      if(rc != 0)
	{
	  fprintf(stderr, "nmad: local- cannot connect to %s\n", addr.sun_path);
	  abort();
	}
      struct nm_local_peer_id_s id;
      memset(&id.url, 0, sizeof(id.url));
      strncpy(id.url, p_local_context->url, sizeof(id.url));
      rc = NM_SYS(write)(fd, &id, sizeof(id));
      if(rc != sizeof(id))
	{
	  fprintf(stderr, "nmad: local- error while sending id to peer node.\n");
	  abort();
	}
      p_status->fd = fd;
    }
  else
    {
      int fd = -1;
      if(pending_fds != NULL)
	{
	  nm_local_pending_vect_itor_t i;
	  puk_vect_foreach(i, nm_local_pending, pending_fds)
	    {
	      if(strcmp(i->peer.url, remote_url) == 0)
		{
		  fd = i->fd;
		  nm_local_pending_vect_erase(pending_fds, i);
		  break;
		}
	    }
	}
      while(fd == -1)
	{
	  fd = NM_SYS(accept)(p_local_context->server_fd, NULL, NULL);
	  if(fd < 0)
	    {
	      fprintf(stderr, "nmad: local- error while accepting\n");
	      abort();
	    }
	  struct nm_local_peer_id_s id;
	  int rc = NM_SYS(recv)(fd, &id, sizeof(id), MSG_WAITALL);
	  if(rc != sizeof(id))
	    {
	      fprintf(stderr, "nmad: local- error while receiving peer node id.\n");
	      abort();
	    }
	  if(strcmp(id.url, remote_url) != 0)
	    {
	      struct nm_local_pending_s pending =
		{
		  .fd = fd,
		  .peer = id
		};
	      if(pending_fds == NULL)
		pending_fds = nm_local_pending_vect_new();
	      nm_local_pending_vect_push_back(pending_fds, pending);
	      fd = -1;
	    }
	}
      p_status->fd = fd;
    }
}

static void nm_local_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_local_s*p_status = _status;
  struct iovec send_iov[1 + n];
  int size = 0;
  int i;
  for(i = 0; i < n; i++)
    {
      send_iov[i+1] = (struct iovec){ .iov_base = v[i].iov_base, .iov_len = v[i].iov_len };
      size += v[i].iov_len;
    }
  send_iov[0] = (struct iovec){ .iov_base = &size, .iov_len = sizeof(size) };
  int rc = NM_SYS(writev)(p_status->fd, send_iov, n+1);
  const int err = errno;
  if(rc < 0 && err == EPIPE)
    {
      fprintf(stderr, "nmad: local- WARNING- socket closed while sending message.\n");
    }
  else if(rc < 0 || rc < size + sizeof(size))
    {
      fprintf(stderr, "nmad: local- error %d while sending message (%s).\n", err, strerror(err));
      abort();
    }
}

static int nm_local_send_poll(void*_status)
{
  return NM_ESUCCESS;
}

static void nm_local_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_local_s*p_status = _status;
  if(n != 1)
    {
      fprintf(stderr, "nmad: local- iovec not supported on recv side yet.\n");
      abort();
    }
  p_status->recv.v = v;
  p_status->recv.n = n;
}


static int nm_local_poll_one(void*_status)
{
  struct nm_local_s*p_status = _status;
  int size = 0;
  int rc = NM_SYS(recv)(p_status->fd, &size, sizeof(size), MSG_DONTWAIT);
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
  else if(size > p_status->recv.v->iov_len)
    {
      fprintf(stderr, "nmad: local- received more data than expected.\n");
      abort();
    }
  rc = NM_SYS(recv)(p_status->fd, p_status->recv.v->iov_base, size, MSG_WAITALL);
  if(rc < 0 || rc != size)
    {
      fprintf(stderr, "nmad: local- error %d while reading data (%s).\n", err, strerror(err));
      abort();
    }
  return NM_ESUCCESS;
}


static int nm_local_cancel_recv(void*_status)
{ 
  return -NM_ENOTIMPL;
}
