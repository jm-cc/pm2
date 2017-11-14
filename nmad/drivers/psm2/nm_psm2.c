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
#include <errno.h>
#include <assert.h>

#include <nm_minidriver.h>
#include <nm_private.h>
#include <Padico/Module.h>

#include <psm2.h>
#include <psm2_mq.h>

static void*nm_psm2_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_psm2_destroy(void*status);

static const struct puk_component_driver_s nm_psm2_component =
  {
    .instantiate = &nm_psm2_instantiate,
    .destroy     = &nm_psm2_destroy
  };

static void nm_psm2_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_psm2_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_psm2_close(puk_context_t context);
static void nm_psm2_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_psm2_send_post(void*_status, const struct iovec*v, int n);
static int  nm_psm2_send_poll(void*_status);
static void nm_psm2_recv_iov_post(void*_status,  struct iovec*v, int n);
static int  nm_psm2_recv_poll_one(void*_status);
static int  nm_psm2_recv_poll_any(puk_context_t p_context, void**_status);
static int  nm_psm2_recv_wait_any(puk_context_t p_context, void**_status);
static int  nm_psm2_recv_cancel_any(puk_context_t p_context);

static const struct nm_minidriver_iface_s nm_psm2_minidriver =
  {
    .getprops        = &nm_psm2_getprops,
    .init            = &nm_psm2_init,
    .close           = &nm_psm2_close,
    .connect         = &nm_psm2_connect,
    .send_post       = &nm_psm2_send_post,
    .send_data       = NULL,
    .send_poll       = &nm_psm2_send_poll,
    .recv_iov_post   = &nm_psm2_recv_iov_post,
    .recv_data_post  = NULL,
    .recv_poll_one   = &nm_psm2_recv_poll_one,
    .recv_poll_any   = &nm_psm2_recv_poll_any,
    .recv_wait_any   = &nm_psm2_recv_wait_any,
    .recv_cancel     = NULL,
    .recv_cancel_any = &nm_psm2_recv_cancel_any
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_psm2,
  puk_component_declare("Minidriver_psm2",
			puk_component_provides("PadicoComponent", "component", &nm_psm2_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_psm2_minidriver)
                        ));

/* ********************************************************* */
             
/** 'psm2' driver per-context data. */
struct nm_psm2_context_s
{
  char*url;                 /**< server url */
};

/** 'psm2' per-instance status. */
struct nm_psm2_s
{
  struct nm_psm2_context_s*p_psm2_context;
  struct
  {
    struct iovec*v;
    int n;
  } recv;
};

/* ********************************************************* */

static void*nm_psm2_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_psm2_s*p_status = malloc(sizeof(struct nm_psm2_s));
  struct nm_psm2_context_s*p_psm2_context = puk_context_get_status(context);
  p_status->p_psm2_context = p_psm2_context;
  return p_status;
}

static void nm_psm2_destroy(void*_status)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  free(p_status);
}

/* ********************************************************* */

static void nm_psm2_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  p_props->capabilities.has_recv_any = 0;
  p_props->capabilities.supports_wait_any = 0;
  p_props->capabilities.prefers_wait_any = 0;
  p_props->profile.latency = 500;
  p_props->profile.bandwidth = 8000;
}

static void nm_psm2_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{
  struct nm_psm2_context_s*p_psm2_context = malloc(sizeof(struct nm_psm2_context_s));
  const char*url = "dummy";
  int ver_major = PSM2_VERNO_MAJOR;
  int ver_minor = PSM2_VERNO_MINOR;
  int rc = psm2_init(&ver_major, &ver_minor);
  if(rc != PSM2_OK)
    {
      NM_FATAL("psm2: error in init.\n");
    }

  p_psm2_context->url = strdup(url);
  puk_context_set_status(context, p_psm2_context);
  *p_url = p_psm2_context->url;
  *p_url_size = strlen(p_psm2_context->url) + 1;
}

static void nm_psm2_close(puk_context_t context)
{
  struct nm_psm2_context_s*p_psm2_context = puk_context_get_status(context);
  puk_context_set_status(context, NULL);
  free(p_psm2_context->url);
  free(p_psm2_context);
}

static void nm_psm2_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;

  /* TODO */
  
}

static void nm_psm2_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_psm2_s*p_status = _status;

  /* TODO */
}

static int nm_psm2_send_poll(void*_status)
{
  /* TODO */
  return NM_ESUCCESS;
}

static void nm_psm2_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct nm_psm2_s*p_status = _status;
  /* TODO */
}

static int nm_psm2_recv_poll_one(void*_status)
{
  struct nm_psm2_s*p_status = _status;

  /* TODO */

  return NM_ESUCCESS;
}

static int nm_psm2_recv_any_common(puk_context_t p_context, void**_status, int timeout)
{
  struct nm_psm2_s*p_status = NULL;
  struct nm_psm2_context_s*p_psm2_context = puk_context_get_status(p_context);

  /* TODO */
  return NM_ESUCCESS;
}

static int nm_psm2_recv_poll_any(puk_context_t p_context, void**_status)
{
  return nm_psm2_recv_any_common(p_context, _status, 0);
}

static int nm_psm2_recv_wait_any(puk_context_t p_context, void**_status)
{
  return nm_psm2_recv_any_common(p_context, _status, -1);
}

static int nm_psm2_recv_cancel_any(puk_context_t p_context)
{
  struct nm_psm2_context_s*p_psm2_context = puk_context_get_status(p_context);

  /* TODO */

  return NM_ESUCCESS;
}
