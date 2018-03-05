/*
 * NewMadeleine
 * Copyright (C) 2010-2017 (see AUTHORS file)
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
#include <sys/uio.h>
#include <sys/poll.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <nm_private.h>

#include <Padico/Module.h>


static void*nm_self_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_self_destroy(void*status);

static const struct puk_component_driver_s nm_self_component =
  {
    .instantiate = &nm_self_instantiate,
    .destroy     = &nm_self_destroy
  };

static void nm_self_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_self_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_self_close(puk_context_t context);
static void nm_self_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_self_send_iov_post(void*_status, const struct iovec*v, int n);
static int  nm_self_send_poll(void*_status);
static void nm_self_recv_iov_post(void*_status,  struct iovec*v, int n);
static int  nm_self_recv_poll_one(void*_status);
static int  nm_self_recv_cancel(void*_status);

static const struct nm_minidriver_iface_s nm_self_minidriver =
  {
    .getprops       = &nm_self_getprops,
    .init           = &nm_self_init,
    .close          = &nm_self_close,
    .connect        = &nm_self_connect,
    .send_iov_post      = &nm_self_send_iov_post,
    .send_poll      = &nm_self_send_poll,
    .recv_iov_post  = &nm_self_recv_iov_post,
    .recv_poll_one  = &nm_self_recv_poll_one,
    .recv_cancel    = &nm_self_recv_cancel
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_self,
  puk_component_declare("Minidriver_self",
			puk_component_provides("PadicoComponent", "component", &nm_self_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_self_minidriver)));

/* ********************************************************* */

/** 'self' driver per-context data. */
struct nm_self_context_s
{
  char*url;
};

/** 'self' per-instance status (singleton, actually). */
struct nm_self_s
{
  struct nm_self_context_s*p_self_context;
  struct
  {
    struct iovec*v;
    int n;
    struct nm_data_s*p_data;
  } recv;
  struct
  {
    const struct iovec*v;
    int n;
    struct nm_data_s*p_data;
    volatile int consumed;
    volatile int posted;
  } send;
};

/* ********************************************************* */

static void*nm_self_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_self_s*p_status = malloc(sizeof(struct nm_self_s));
  struct nm_self_context_s*p_self_context = puk_context_get_status(context);
  p_status->p_self_context = p_self_context;
  p_status->send.v        = NULL;
  p_status->send.p_data   = NULL;
  p_status->send.consumed = 0;
  p_status->send.posted   = 0;
  p_status->recv.v        = NULL;
  p_status->recv.p_data   = NULL;
  return p_status;
}

static void nm_self_destroy(void*_status)
{
  struct nm_self_s*p_status = _status;
  free(p_status);
}

/* ********************************************************* */

static void nm_self_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  props->profile.latency = 200;
  props->profile.bandwidth = 20000;
  /*   props->capabilities.supports_data = 1; */
}

static void nm_self_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_self_context_s*p_self_context = malloc(sizeof(struct nm_self_context_s));
  p_self_context->url = strdup("-");
  puk_context_set_status(context, p_self_context);
  *drv_url = p_self_context->url;
  *url_size = strlen(p_self_context->url) + 1;
}

static void nm_self_close(puk_context_t context)
{
  struct nm_self_context_s*p_self_context = puk_context_get_status(context);
  puk_context_set_status(context, NULL);
  free(p_self_context->url);
  free(p_self_context);
}

static void nm_self_connect(void*_status, const void*remote_url, size_t url_size)
{
  /* empty */
}

static void nm_self_send_iov_post(void*_status, const struct iovec*v, int n)
{
  struct nm_self_s*p_status = _status;
  p_status->send.consumed = 0;
  p_status->send.v = v;
  p_status->send.n = n;
  nm_mem_fence();
  p_status->send.posted = 1;
}

static int nm_self_send_poll(void*_status)
{
  struct nm_self_s*p_status = _status;
  if(p_status->send.consumed)
    {
      p_status->send.v = NULL;
      p_status->send.n = 0;
      p_status->send.consumed = 0;
      /* send.posted was reset in receiver to prevent race condition
       * (receive same packet twice) */
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static void nm_self_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct nm_self_s*p_status = _status;
  if(n != 1)
    {
      fprintf(stderr, "nmad: self- iovec not supported on recv side yet.\n");
      abort();
    }
  p_status->recv.v = v;
  p_status->recv.n = n;
}

static int nm_self_recv_poll_one(void*_status)
{
  struct nm_self_s*p_status = _status;
  if(p_status->send.posted)
    {
      int done = 0;
      int i;
      for(i = 0; i < p_status->send.n; i++)
	{
	  if(done + p_status->send.v[i].iov_len > p_status->recv.v[0].iov_len)
	    {
	      fprintf(stderr, "nmad: self- trying to send more data than receive posted.\n");
	      abort();
	    }
	  memcpy(p_status->recv.v[0].iov_base + done, p_status->send.v[i].iov_base, p_status->send.v[i].iov_len);
	  done += p_status->send.v[i].iov_len;
	}
      p_status->send.posted = 0;
      nm_mem_fence();
      p_status->send.consumed = 1;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}


static int nm_self_recv_cancel(void*_status)
{ 
  return -NM_ENOTIMPL;
}
