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

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */

#include "nm_ibverbs.h"

#include <Padico/Module.h>

static int nm_ibverbs_auto_load(void);

PADICO_MODULE_BUILTIN(NewMad_ibverbs_auto, &nm_ibverbs_auto_load, NULL, NULL);


/* *** method: 'auto' *********************************** */

/** threshold to switch from adaptrdma to regrdma */
#define NM_IBVERBS_AUTO_THRESHOLD (2560 * 1024) /* 2.5MB */

struct nm_ibverbs_auto_submethod_s
{
  puk_instance_t instance;
  struct puk_receptacle_NewMad_ibverbs_method_s r;
};

struct nm_ibverbs_auto
{
  struct nm_ibverbs_auto_submethod_s adaptrdma, regrdma;
  struct puk_receptacle_NewMad_ibverbs_method_s *r_send, *r_recv;
};

static void nm_ibverbs_auto_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv);
static void nm_ibverbs_auto_addr_pack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_auto_addr_unpack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_auto_send_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_auto_send_poll(void*_status);
static void nm_ibverbs_auto_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_auto_poll_one(void*_status);

static const struct nm_ibverbs_method_iface_s nm_ibverbs_auto_method =
  {
    .cnx_create  = &nm_ibverbs_auto_cnx_create,
    .addr_pack   = &nm_ibverbs_auto_addr_pack,
    .addr_unpack = &nm_ibverbs_auto_addr_unpack,
    .send_post   = &nm_ibverbs_auto_send_post,
    .send_poll   = &nm_ibverbs_auto_send_poll,
    .recv_init   = &nm_ibverbs_auto_recv_init,
    .poll_one    = &nm_ibverbs_auto_poll_one,
    .poll_any    = NULL,
    .cancel_recv = NULL
  };

static void*nm_ibverbs_auto_instanciate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_auto_destroy(void*);

static const struct puk_adapter_driver_s nm_ibverbs_auto_adapter =
  {
    .instanciate = &nm_ibverbs_auto_instanciate,
    .destroy = &nm_ibverbs_auto_destroy
  };

static int nm_ibverbs_auto_load(void)
{
  puk_component_declare("NewMad_ibverbs_auto",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_auto_adapter),
			puk_component_provides("NewMad_ibverbs_method", "method", &nm_ibverbs_auto_method));
  return 0;
}

static void*nm_ibverbs_auto_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_auto*status = TBX_MALLOC(sizeof(struct nm_ibverbs_auto));
  puk_component_t adaptrdma = puk_adapter_resolve("NewMad_ibverbs_adaptrdma");
  puk_component_t regrdma   = puk_adapter_resolve("NewMad_ibverbs_regrdma");
  status->adaptrdma.instance = puk_adapter_instanciate(adaptrdma);
  status->regrdma.instance   = puk_adapter_instanciate(regrdma);
  puk_instance_indirect_NewMad_ibverbs_method(status->adaptrdma.instance, NULL, &status->adaptrdma.r);
  puk_instance_indirect_NewMad_ibverbs_method(status->regrdma.instance, NULL, &status->regrdma.r);
  status->r_send = NULL;
  status->r_recv = NULL;
  return status;
}

static void nm_ibverbs_auto_destroy(void*_status)
{
  /* TODO */
}

static void nm_ibverbs_auto_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv)
{
  struct nm_ibverbs_auto*status = _status;
  (*status->adaptrdma.r.driver->cnx_create)(status->adaptrdma.r._status, p_ibverbs_cnx, p_ibverbs_drv);
  (*status->regrdma.r.driver->cnx_create)(status->regrdma.r._status, p_ibverbs_cnx, p_ibverbs_drv);
}

static void nm_ibverbs_auto_addr_pack(void*_status, struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_auto*status = _status;
  (*status->adaptrdma.r.driver->addr_pack)(status->adaptrdma.r._status, addr);
  (*status->regrdma.r.driver->addr_pack)(status->regrdma.r._status, addr);
}
static void nm_ibverbs_auto_addr_unpack(void*_status, struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_auto*status = _status;
  (*status->adaptrdma.r.driver->addr_unpack)(status->adaptrdma.r._status, addr);
  (*status->regrdma.r.driver->addr_unpack)(status->regrdma.r._status, addr);
}
static void nm_ibverbs_auto_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_auto*status = _status;
  status->r_send = (v[0].iov_len < NM_IBVERBS_AUTO_THRESHOLD) ? &status->adaptrdma.r : &status->regrdma.r;
  (*status->r_send->driver->send_post)(status->r_send->_status, v, n);
}
static int  nm_ibverbs_auto_send_poll(void*_status)
{
  struct nm_ibverbs_auto*status = _status;
  int err = (*status->r_send->driver->send_poll)(status->r_send->_status);
  if(err == NM_ESUCCESS)
    status->r_send = NULL;
  return err;
}
static void nm_ibverbs_auto_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_auto*status = _status;
  status->r_recv = (v->iov_len < NM_IBVERBS_AUTO_THRESHOLD) ? &status->adaptrdma.r : &status->regrdma.r;
  (*status->r_recv->driver->recv_init)(status->r_recv->_status, v, n);
}
static int  nm_ibverbs_auto_poll_one(void*_status)
{
  struct nm_ibverbs_auto*status = _status;
  int err = (*status->r_recv->driver->poll_one)(status->r_recv->_status);
  if(err == NM_ESUCCESS)
    status->r_recv = NULL;
  return err;
}
