/*
 * NewMadeleine
 * Copyright (C) 2017 (see AUTHORS file)
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
#include <poll.h>
#include <limits.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/utsname.h>


#include <nm_minidriver.h>
#include <nm_private.h>
#include <Padico/Module.h>

static void*nm_tcp_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_tcp_destroy(void*status);

static const struct puk_component_driver_s nm_tcp_component =
  {
    .instantiate = &nm_tcp_instantiate,
    .destroy     = &nm_tcp_destroy
  };

static void nm_tcp_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_tcp_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_tcp_close(puk_context_t context);
static void nm_tcp_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_tcp_send_post(void*_status, const struct iovec*v, int n);
static int  nm_tcp_send_poll(void*_status);
static void nm_tcp_recv_iov_post(void*_status,  struct iovec*v, int n);
static int  nm_tcp_recv_poll_one(void*_status);
static int  nm_tcp_recv_poll_any(puk_context_t p_context, void**_status);
static int  nm_tcp_recv_wait_any(puk_context_t p_context, void**_status);

static const struct nm_minidriver_iface_s nm_tcp_minidriver =
  {
    .getprops       = &nm_tcp_getprops,
    .init           = &nm_tcp_init,
    .close          = &nm_tcp_close,
    .connect        = &nm_tcp_connect,
    .send_post      = &nm_tcp_send_post,
    .send_data      = NULL,
    .send_poll      = &nm_tcp_send_poll,
    .recv_iov_post  = &nm_tcp_recv_iov_post,
    .recv_data_post = NULL,
    .recv_poll_one  = &nm_tcp_recv_poll_one,
    .recv_poll_any  = &nm_tcp_recv_poll_any,
    .recv_wait_any  = &nm_tcp_recv_wait_any,
    .recv_cancel    = NULL
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_tcp,
  puk_component_declare("Minidriver_tcp",
			puk_component_provides("PadicoComponent", "component", &nm_tcp_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_tcp_minidriver)));

/* ********************************************************* */

#define NM_TCP_URL_SIZE_MAX 64
/** identity of peer node */
struct nm_tcp_peer_id_s
{
  char url[NM_TCP_URL_SIZE_MAX];
};
/** pending connection */
struct nm_tcp_pending_s
{
  int fd;                       /**< socket of the pending connection */
  struct nm_tcp_peer_id_s peer; /**< url of the peer node */
};
PUK_VECT_TYPE(nm_tcp_pending, struct nm_tcp_pending_s);

PUK_VECT_TYPE(nm_tcp_status, struct nm_tcp_s*);
              
/** 'tcp' driver per-context data. */
struct nm_tcp_context_s
{
  struct nm_tcp_status_vect_s p_statuses;
  struct pollfd*fds;        /**< pollfd used for recv_poll|wait_any */
  int nfds;                 /**< size of above fds */
  int round_robin;          /**< first fd to poll for fairness */
  struct sockaddr_in addr;  /**< server socket address */
  int server_fd;            /**< server socket fd */
  char*url;                 /**< server url */
  nm_tcp_pending_vect_t pending_fds;
};

/** 'tcp' per-instance status. */
struct nm_tcp_s
{
  int fd;                   /**< tcp socket */
  int error;
  struct nm_tcp_context_s*p_tcp_context;
  struct
  {
    struct iovec*v;
    int n;
  } recv;
};


/* ********************************************************* */

static void*nm_tcp_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_tcp_s*p_status = padico_malloc(sizeof(struct nm_tcp_s));
  struct nm_tcp_context_s*p_tcp_context = puk_context_get_status(context);
  p_status->fd = -1;
  p_status->error = 0;
  p_status->p_tcp_context = p_tcp_context;
  nm_tcp_status_vect_push_back(&p_tcp_context->p_statuses, p_status);
  return p_status;
}

static void nm_tcp_destroy(void*_status)
{
  struct nm_tcp_s*p_status = _status;
  struct nm_tcp_context_s*p_tcp_context = p_status->p_tcp_context;
  nm_tcp_status_vect_itor_t itor = nm_tcp_status_vect_find(&p_tcp_context->p_statuses, p_status);
  nm_tcp_status_vect_erase(&p_tcp_context->p_statuses, itor);
  /* half-close for sending */
  NM_SYS(shutdown)(p_status->fd, SHUT_WR);
  /* flush (and throw away) remaining bytes in the pipe up to the EOS */
  int ret = -1;
  do
    {
      char dummy = 0;
      ret = NM_SYS(read)(p_status->fd, &dummy, sizeof(dummy));
    }
  while (ret > 0 || ((ret == -1 && errno == EAGAIN)));
  /* safely close fd when EOS is reached */
  NM_SYS(close)(p_status->fd);
  padico_free(p_status);
}

/* ********************************************************* */

static void nm_tcp_rebuild_fds(struct nm_tcp_context_s*p_tcp_context)
{
  if(p_tcp_context->nfds != nm_tcp_status_vect_size(&p_tcp_context->p_statuses))
    {
      p_tcp_context->nfds = nm_tcp_status_vect_size(&p_tcp_context->p_statuses);
      p_tcp_context->fds = realloc(p_tcp_context->fds, sizeof(struct pollfd) * p_tcp_context->nfds);
    }
  int i;
  for(i = 0; i < p_tcp_context->nfds; i++)
    {
      const struct nm_tcp_s*p_status = nm_tcp_status_vect_at(&p_tcp_context->p_statuses, i);
      if(!p_status->error)
        p_tcp_context->fds[i].fd = p_status->fd;
      else
        p_tcp_context->fds[i].fd = -p_status->fd;
      p_tcp_context->fds[i].events = POLLIN;
    }
  p_tcp_context->round_robin = 0;
}

static void nm_tcp_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  props->capabilities.has_recv_any = 1;
  props->capabilities.supports_wait_any = 1;
  props->capabilities.prefers_wait_any = 1;
  props->profile.latency = 500;
  props->profile.bandwidth = 8000;
}

static void nm_tcp_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  uint16_t port = 0;
  struct nm_tcp_context_s*p_tcp_context = padico_malloc(sizeof(struct nm_tcp_context_s));
  p_tcp_context->addr.sin_family = AF_INET;
  p_tcp_context->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  p_tcp_context->addr.sin_port = htons(port);
  p_tcp_context->server_fd = NM_SYS(socket)(AF_INET, SOCK_STREAM, 0);
  int rc = NM_SYS(bind)(p_tcp_context->server_fd, (struct sockaddr*)&p_tcp_context->addr, sizeof(p_tcp_context->addr));
  if(rc != 0)
    {
      NM_FATAL("tcp: bind() error: %s\n", strerror(errno));
    }
  socklen_t addr_len = sizeof(p_tcp_context->addr);
  rc = NM_SYS(getsockname)(p_tcp_context->server_fd, (struct sockaddr*)&p_tcp_context->addr, &addr_len);
  if(rc != 0)
    {
      NM_FATAL("tcp: getsockname() error: %s\n", strerror(errno));
    }
  port = ntohs(p_tcp_context->addr.sin_port);
  struct utsname utsname;
  uname(&utsname);
  padico_string_t s_url = padico_string_new();
  padico_string_catf(s_url, "%s:%d", utsname.nodename, port);
  p_tcp_context->url = strdup(padico_string_get(s_url));
  padico_string_delete(s_url);
  rc = NM_SYS(listen)(p_tcp_context->server_fd, 128);
  if(rc != 0)
    {
      NM_FATAL("tcp: listen() error: %s\n", strerror(errno));
    }
  nm_tcp_status_vect_init(&p_tcp_context->p_statuses);
  p_tcp_context->pending_fds = nm_tcp_pending_vect_new();
  p_tcp_context->fds = NULL;
  p_tcp_context->nfds = 0;
  puk_context_set_status(context, p_tcp_context);
  *drv_url = p_tcp_context->url;
  *url_size = strlen(p_tcp_context->url) + 1;
  assert(*url_size <= NM_TCP_URL_SIZE_MAX);
}

static void nm_tcp_close(puk_context_t context)
{
  struct nm_tcp_context_s*p_tcp_context = puk_context_get_status(context);
  NM_SYS(close)(p_tcp_context->server_fd);
  puk_context_set_status(context, NULL);
  padico_free(p_tcp_context->url);
  padico_free(p_tcp_context);
}

static void nm_tcp_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_tcp_s*p_status = _status;
  struct nm_tcp_context_s*p_tcp_context = p_status->p_tcp_context;

  if(strcmp(p_tcp_context->url, remote_url) > 0)
    {
      /* ** connect */
      char*url2 = strdup(remote_url);
      const char*remote_hostname = url2;
      char*remote_port = strchr(url2, ':');
      *remote_port = '\0';
      remote_port++;
      const uint16_t port = atoi(remote_port);
      struct hostent*host_entry = gethostbyname(remote_hostname);
      if(host_entry == NULL)
	{
	  NM_FATAL("tcp: cannot resolve remote host: %s.\n", remote_hostname);
	}
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      memcpy(&addr.sin_addr.s_addr, host_entry->h_addr, host_entry->h_length);
      int fd = NM_SYS(socket)(AF_INET, SOCK_STREAM, 0);
      int rc = NM_SYS(connect)(fd, (struct sockaddr*)&addr, sizeof(addr));
      if(rc != 0)
	{
	  NM_FATAL("tcp: cannot connect to %s:%d (%s)\n", remote_hostname, port, strerror(errno));
	}
      struct nm_tcp_peer_id_s id;
      memset(&id.url, 0, sizeof(id.url));
      strncpy(id.url, p_tcp_context->url, sizeof(id.url));
      rc = NM_SYS(send)(fd, &id, sizeof(id), MSG_WAITALL);
      if(rc != sizeof(id))
	{
	  NM_FATAL("tcp: error while sending id to peer node.\n");
	}
      p_status->fd = fd;
    }
  else
    {
      int fd = -1;
      nm_tcp_pending_vect_itor_t i;
      puk_vect_foreach(i, nm_tcp_pending, p_tcp_context->pending_fds)
        {
          if(strcmp(i->peer.url, remote_url) == 0)
            {
              fd = i->fd;
              nm_tcp_pending_vect_erase(p_tcp_context->pending_fds, i);
              break;
            }
        }
      while(fd == -1)
	{
	  fd = NM_SYS(accept)(p_tcp_context->server_fd, NULL, NULL);
	  if(fd < 0)
	    {
	      NM_FATAL("nmad: tcp- error while accepting\n");
	    }
	  struct nm_tcp_peer_id_s id;
	  int rc = NM_SYS(recv)(fd, &id, sizeof(id), MSG_WAITALL);
	  if(rc != sizeof(id))
	    {
	      NM_FATAL("nmad: tcp- error while receiving peer node id.\n");
	    }
	  if(strcmp(id.url, remote_url) != 0)
	    {
	      struct nm_tcp_pending_s pending =
		{
		  .fd = fd,
		  .peer = id
		};
	      nm_tcp_pending_vect_push_back(p_tcp_context->pending_fds, pending);
	      fd = -1;
	    }
	}
      p_status->fd = fd;
    }
  int val = 1;
  socklen_t len = sizeof(val);
  NM_SYS(setsockopt)(p_status->fd, IPPROTO_TCP, TCP_NODELAY, (void*)&val, len);
  nm_tcp_rebuild_fds(p_tcp_context);
}

static void nm_tcp_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_tcp_s*p_status = _status;
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
      NM_WARN("nmad: tcp- WARNING- socket closed while sending message.\n");
    }
  else if(rc < 0 || rc < size + sizeof(size))
    {
      NM_FATAL("nmad: tcp- error %d while sending message (%s).\n", err, strerror(err));
    }
}

static int nm_tcp_send_poll(void*_status)
{
  return NM_ESUCCESS;
}

static void nm_tcp_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct nm_tcp_s*p_status = _status;
  if(n != 1)
    {
      NM_FATAL("nmad: tcp- iovec not supported on recv side yet.\n");
    }
  p_status->recv.v = v;
  p_status->recv.n = n;
}

static int nm_tcp_recv_poll_one(void*_status)
{
  struct nm_tcp_s*p_status = _status;
  int size = 0;
  int rc = NM_SYS(recv)(p_status->fd, &size, sizeof(size), MSG_DONTWAIT);
  int err = errno;
  if(rc == 0)
    {
      p_status->error = 1;
      nm_tcp_rebuild_fds(p_status->p_tcp_context);
      return -NM_ECLOSED;
    }
  else if(rc < 0 && err == EAGAIN)
    {
      return -NM_EAGAIN;
    }
  else if(rc < 0)
    {
      NM_FATAL("nmad: tcp- error %d while reading header (%s).\n", err, strerror(err));
    }
  else if(size > p_status->recv.v->iov_len)
    {
      NM_FATAL("nmad: tcp- received more data than expected.\n");
    }
  rc = NM_SYS(recv)(p_status->fd, p_status->recv.v->iov_base, size, MSG_WAITALL);
  if(rc < 0 || rc != size)
    {
      NM_FATAL("nmad: tcp- error %d while reading data (%s).\n", err, strerror(err));
    }
  return NM_ESUCCESS;
}

static int nm_tcp_recv_any_common(puk_context_t p_context, void**_status, int timeout)
{
  struct nm_tcp_s*p_status = NULL;
  struct nm_tcp_context_s*p_tcp_context = puk_context_get_status(p_context);
  int rebuild = 0;
  int rc = NM_SYS(poll)(p_tcp_context->fds, p_tcp_context->nfds, timeout);
  p_tcp_context->round_robin = (p_tcp_context->round_robin + 1) % p_tcp_context->nfds;
  if(rc > 0)
    {
      int i;
      for(i = 0; i < p_tcp_context->nfds; i++)
        {
          const int k = (p_tcp_context->round_robin + i) % p_tcp_context->nfds;
          const short e = p_tcp_context->fds[k].revents;
          if(e)
            {
              if(e & POLLIN)
                {
                  assert(nm_tcp_status_vect_at(&p_tcp_context->p_statuses, k)->fd == p_tcp_context->fds[k].fd);
                  if(p_status == NULL)
                    {
                      p_status = nm_tcp_status_vect_at(&p_tcp_context->p_statuses, k);
                    }
                }
              else if(e & (POLLERR | POLLHUP))
                {
                  NM_WARN("tcp: error for fd = %d\n", p_tcp_context->fds[k].fd);
                  nm_tcp_status_vect_at(&p_tcp_context->p_statuses, k)->error = 1;
                  p_tcp_context->fds[k].fd = -p_tcp_context->fds[k].fd;
                  rebuild = 1;
                }
              else if(e & POLLNVAL)
                {
                  NM_FATAL("tcp: invalid fd = %d in poll.\n", p_tcp_context->fds[k].fd);
                }
              else
                {
                  NM_FATAL("tcp: unexpected revent %d in poll.\n", e);
                }
            }
        }
      if(rebuild)
        nm_tcp_rebuild_fds(p_tcp_context);
      *_status = p_status;
      assert(p_status != NULL);
      return NM_ESUCCESS;
    }
  else
    {
      *_status = NULL;
      return -NM_EAGAIN;
    }
}

static int nm_tcp_recv_poll_any(puk_context_t p_context, void**_status)
{
  return nm_tcp_recv_any_common(p_context, _status, 0);
}

static int nm_tcp_recv_wait_any(puk_context_t p_context, void**_status)
{
  return nm_tcp_recv_any_common(p_context, _status, -1);
}
