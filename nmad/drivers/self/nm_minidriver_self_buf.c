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
#include <semaphore.h>

#include <nm_private.h>

#include <Padico/Module.h>

#define NM_SELFBUF_SIZE 8192

static void*nm_selfbuf_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_selfbuf_destroy(void*status);

static const struct puk_component_driver_s nm_selfbuf_component =
  {
    .instantiate = &nm_selfbuf_instantiate,
    .destroy     = &nm_selfbuf_destroy
  };

static void nm_selfbuf_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_selfbuf_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_selfbuf_close(puk_context_t context);
static void nm_selfbuf_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_selfbuf_buf_send_get(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_selfbuf_buf_send_post(void*_status, nm_len_t len);
static int  nm_selfbuf_send_poll(void*_status);
static int  nm_selfbuf_buf_recv_poll(void*_status, void**p_buffer, nm_len_t*p_len);
static int  nm_selfbuf_recv_poll_any(puk_context_t p_context, void**pp_status, void**p_buffer, nm_len_t*p_len);
static int  nm_selfbuf_recv_wait_any(puk_context_t p_context, void**pp_status, void**p_buffer, nm_len_t*p_len);
static void nm_selfbuf_buf_recv_release(void*_status);

static const struct nm_minidriver_iface_s nm_selfbuf_minidriver =
  {
    .getprops          = &nm_selfbuf_getprops,
    .init              = &nm_selfbuf_init,
    .close             = &nm_selfbuf_close,
    .connect           = &nm_selfbuf_connect,
    .send_post         = NULL,
    .send_data         = NULL,
    .buf_send_get      = &nm_selfbuf_buf_send_get,
    .buf_send_post     = &nm_selfbuf_buf_send_post,
    .send_poll         = &nm_selfbuf_send_poll,
    .recv_init         = NULL,
    .recv_data         = NULL,
    .poll_one          = NULL,
    .buf_recv_poll     = &nm_selfbuf_buf_recv_poll,
    .buf_recv_poll_any = &nm_selfbuf_recv_poll_any,
    .buf_recv_wait_any = &nm_selfbuf_recv_wait_any,
    .buf_recv_release  = &nm_selfbuf_buf_recv_release,
    .cancel_recv       = NULL
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_self_buf,
  puk_component_declare("Minidriver_self_buf",
			puk_component_provides("PadicoComponent", "component", &nm_selfbuf_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_selfbuf_minidriver)));

/* ********************************************************* */

/** 'self' driver per-context data. */
struct nm_selfbuf_context_s
{
  char*url;
  struct nm_selfbuf_s*p_status; /**< assumed to be a singleton */
  sem_t sem;
};

/** 'self' per-instance status (singleton, actually). */
struct nm_selfbuf_s
{
  struct nm_selfbuf_context_s*p_selfbuf_context;
  nm_len_t len;
  volatile int posted;
  volatile int consumed;
  char buffer[NM_SELFBUF_SIZE];
};

/* ********************************************************* */

static void*nm_selfbuf_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_selfbuf_s*p_status = malloc(sizeof(struct nm_selfbuf_s));
  struct nm_selfbuf_context_s*p_selfbuf_context = puk_context_get_status(context);
  p_status->p_selfbuf_context = p_selfbuf_context;
  p_status->len      = NM_LEN_UNDEFINED;
  p_status->posted   = 0;
  p_status->consumed = 0;
  assert(p_selfbuf_context->p_status == NULL);
  p_selfbuf_context->p_status = p_status;
  return p_status;
}

static void nm_selfbuf_destroy(void*_status)
{
  struct nm_selfbuf_s*p_status = _status;
  assert(p_status->p_selfbuf_context->p_status == p_status);
  p_status->p_selfbuf_context->p_status = NULL;
  free(p_status);
}

/* ********************************************************* */

static void nm_selfbuf_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  props->profile.latency = 200;
  props->profile.bandwidth = 20000;
  props->capabilities.max_msg_size = NM_SELFBUF_SIZE;
  props->capabilities.supports_buf_send = 1;
  props->capabilities.supports_buf_recv = 1;
}

static void nm_selfbuf_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_selfbuf_context_s*p_selfbuf_context = malloc(sizeof(struct nm_selfbuf_context_s));
  p_selfbuf_context->url = strdup("-");
  puk_context_set_status(context, p_selfbuf_context);
  sem_init(&p_selfbuf_context->sem, 0, 0);
  *drv_url = p_selfbuf_context->url;
  *url_size = strlen(p_selfbuf_context->url) + 1;
}

static void nm_selfbuf_close(puk_context_t context)
{
  struct nm_selfbuf_context_s*p_selfbuf_context = puk_context_get_status(context);
  puk_context_set_status(context, NULL);
  free(p_selfbuf_context->url);
  free(p_selfbuf_context);
}

static void nm_selfbuf_connect(void*_status, const void*remote_url, size_t url_size)
{
  /* empty */
}

static void nm_selfbuf_buf_send_get(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_selfbuf_s*p_status = _status;
  assert(p_status->posted == 0);
  *p_buffer = &p_status->buffer[0];
  *p_len = NM_SELFBUF_SIZE;
}

static void nm_selfbuf_buf_send_post(void*_status, nm_len_t len)
{
  struct nm_selfbuf_s*p_status = _status;
  p_status->len = len;
  p_status->consumed = 0;
  nm_mem_fence();
  p_status->posted = 1;
  sem_post(&p_status->p_selfbuf_context->sem);
}

static int nm_selfbuf_send_poll(void*_status)
{
  struct nm_selfbuf_s*p_status = _status;
  assert((p_status->posted == 1) || (p_status->consumed == 1));
  if(p_status->consumed)
    {
      p_status->consumed = 0;
      /* send.posted was reset in receiver to prevent race condition
       * (receive same packet twice) */
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static int nm_selfbuf_buf_recv_poll(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_selfbuf_s*p_status = _status;
  if(p_status->posted)
    {
      *p_buffer = &p_status->buffer[0];
      *p_len = p_status->len;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static int nm_selfbuf_recv_poll_any(puk_context_t p_context, void**pp_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_selfbuf_context_s*p_selfbuf_context = puk_context_get_status(p_context);
  int rc = nm_selfbuf_buf_recv_poll(p_selfbuf_context->p_status, p_buffer, p_len);
  if(rc == NM_ESUCCESS)
    *pp_status = p_selfbuf_context->p_status;
  return rc;
}

static int nm_selfbuf_recv_wait_any(puk_context_t p_context, void**pp_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_selfbuf_context_s*p_selfbuf_context = puk_context_get_status(p_context);
  int rc = nm_selfbuf_recv_poll_any(p_context, pp_status, p_buffer, p_len);
  while(rc == -NM_EAGAIN)
    {
      sem_wait(&p_selfbuf_context->sem);
      rc = nm_selfbuf_recv_poll_any(p_context, pp_status, p_buffer, p_len);
    }
  return rc;
}

static void nm_selfbuf_buf_recv_release(void*_status)
{
  struct nm_selfbuf_s*p_status = _status;
  p_status->posted = 0;
  nm_mem_fence();
  p_status->consumed = 1;
}

