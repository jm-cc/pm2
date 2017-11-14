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
    .recv_poll_any   = NULL, /* &nm_psm2_recv_poll_any, */
    .recv_wait_any   = NULL, /* &nm_psm2_recv_wait_any, */
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

PUK_VECT_TYPE(nm_psm2_status, struct nm_psm2_s*);

/** global per-process data (since PSM2 supports only **one endpoint per process**) */
struct nm_psm2_process_s
{
  struct nm_psm2_status_vect_s statuses;
  psm2_uuid_t uuid;
  psm2_ep_t myep;
  psm2_epid_t myepid;
  int next_id;
};

/** 'psm2' driver per-context data. */
struct nm_psm2_context_s
{
  struct nm_psm2_process_s*p_process;
  psm2_mq_t mq;
};

/** 'psm2' per-instance status. */
struct nm_psm2_s
{
  struct nm_psm2_peer_s
  {
    psm2_epid_t epid;
    psm2_epaddr_t epaddr;
    uint32_t local_id;
    uint32_t remote_id;
  } peer;
  struct nm_psm2_context_s*p_psm2_context;
  psm2_mq_req_t sreq, rreq;
};

/** PSM2 singleton */
static struct nm_psm2_process_s*p_psm2_process = NULL;

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
  int rc;
  if(p_psm2_process == NULL)
    {
      p_psm2_process = malloc(sizeof(struct nm_psm2_process_s));
      nm_psm2_status_vect_init(&p_psm2_process->statuses);
      p_psm2_process->next_id = 0;
      psm2_uuid_generate(p_psm2_process->uuid);
      fprintf(stderr, "# psm2: uuid = %x%x%x%x:%x%x%x%x:%x%x%x%x:%x%x%x%x\n",
              p_psm2_process->uuid[0],  p_psm2_process->uuid[1],  p_psm2_process->uuid[2],  p_psm2_process->uuid[3],
              p_psm2_process->uuid[4],  p_psm2_process->uuid[5],  p_psm2_process->uuid[6],  p_psm2_process->uuid[7],
              p_psm2_process->uuid[8],  p_psm2_process->uuid[9],  p_psm2_process->uuid[10], p_psm2_process->uuid[11],
              p_psm2_process->uuid[12], p_psm2_process->uuid[13], p_psm2_process->uuid[14], p_psm2_process->uuid[15]
              );
      int ver_major = PSM2_VERNO_MAJOR;
      int ver_minor = PSM2_VERNO_MINOR;
      rc = psm2_init(&ver_major, &ver_minor);
      if(rc != PSM2_OK)
        {
          NM_FATAL("psm2: error in init; rc = %d.\n", rc);
        }
      uint32_t num_units = -1;
      psm2_ep_num_devunits(&num_units);
      fprintf(stderr, "# psm2: detected %u units\n", (unsigned)num_units);
      struct psm2_ep_open_opts options;
      rc = psm2_ep_open_opts_get_defaults(&options);
      if(rc != PSM2_OK)
        {
          NM_FATAL("psm2: cannot get default options; rc = %d.\n", rc);
        }
      fprintf(stderr, "# psm2: options- unit = %d; affinity = %d; shm_mbytes = %d; sendbufs_num = %d\n",
              options.unit, options.affinity, options.shm_mbytes, options.sendbufs_num);
      rc = psm2_ep_open(p_psm2_process->uuid, &options, &p_psm2_process->myep, &p_psm2_process->myepid);
      if(rc != PSM2_OK)
        {
          NM_FATAL("psm2: cannot open endpoint; rc = %d.\n", rc);
        }
      fprintf(stderr, "# psm2: init done.\n");
    }
  p_psm2_context->p_process = p_psm2_process;
  rc = psm2_mq_init(p_psm2_process->myep, 0xFF /* mask for ordering */, NULL, 0, &p_psm2_context->mq);
  if(rc != PSM2_OK)
    {
      NM_FATAL("psm2: error in MQ init.\n");
    }
  puk_context_set_status(context, p_psm2_context);
  *p_url = &p_psm2_process->myepid;
  *p_url_size = sizeof(psm2_epid_t);
}

static void nm_psm2_close(puk_context_t context)
{
  struct nm_psm2_context_s*p_psm2_context = puk_context_get_status(context);
  puk_context_set_status(context, NULL);
  free(p_psm2_context);
}

static void nm_psm2_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  assert(url_size == sizeof(psm2_epid_t));
  psm2_epid_t peer_epid = *(psm2_epid_t*)remote_url;
  psm2_epaddr_t peer_epaddr;
  psm2_error_t error = 0;
  int rc = psm2_ep_connect(p_psm2_process->myep, 1, &peer_epid, NULL, &error, &peer_epaddr, 0);
  if(rc != PSM2_OK)
    {
      NM_FATAL("psm2: error in psm2_ep_connect()\n");
    }
  uint32_t local_id = p_psm2_process->next_id++;
  uint32_t remote_id = -1;
  psm2_mq_req_t sreq, rreq;
  psm2_mq_tag_t tag = { .tag0 = 0x10 /* TODO- add context ID */, .tag1 = 0, .tag2 = 0 };
  psm2_mq_tag_t tag_mask = { .tag0 = (uint32_t)-1, .tag1 = (uint32_t)-1, .tag2 = (uint32_t)-1 };
  rc = psm2_mq_isend2(p_psm2_context->mq, peer_epaddr, 0, &tag, &local_id, sizeof(local_id), NULL, &sreq);
  rc = psm2_mq_irecv2(p_psm2_context->mq, peer_epaddr, &tag, &tag_mask, 0 /* flags */,
                      &remote_id, sizeof(remote_id), NULL, &rreq);
  psm2_mq_wait2(&rreq, NULL);
  psm2_mq_wait2(&sreq, NULL);
  p_status->peer.epid = peer_epid;
  p_status->peer.epaddr = peer_epaddr;
  p_status->peer.local_id = local_id;
  p_status->peer.remote_id = remote_id;
  nm_psm2_status_vect_push_back(&p_psm2_process->statuses, p_status);
}

static void nm_psm2_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  assert(n == 1);
  psm2_mq_tag_t tag = { .tag0 = 0x01 /* TODO- context id*/, .tag1 = p_status->peer.remote_id, .tag2 = 0 };
  int rc = psm2_mq_isend2(p_psm2_context->mq, p_status->peer.epaddr, 0,
                          &tag, v[0].iov_base, v[0].iov_len, NULL, &p_status->sreq);
  if(rc != PSM2_OK)
    {
      NM_FATAL("psm2: error in psm2_mq_isend; rc = %d\n", rc)
    }
}

static int nm_psm2_send_poll(void*_status)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  psm2_mq_status2_t req_status;
  int rc = psm2_mq_test2(&p_status->sreq, &req_status);
  if(rc == PSM2_OK)
    {
      return NM_ESUCCESS;
    }
  else if(rc == PSM2_MQ_INCOMPLETE)
    {
      psm2_poll(p_psm2_process->myep);
      rc = psm2_mq_test2(&p_status->sreq, &req_status);
      if(rc == PSM2_OK)
        return NM_ESUCCESS;
      else
        return -NM_EAGAIN;
    }
  else
    {
      NM_FATAL("psm2: error in psm2_mq_test; rc = %d\n", rc);
      return -NM_EUNKNOWN;
    }
}

static void nm_psm2_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  assert(n == 1);
  psm2_mq_tag_t tag = { .tag0 = 0x01 /* TODO- context id*/, .tag1 = p_status->peer.local_id, .tag2 = 0 };
  psm2_mq_tag_t tag_mask = { .tag0 = (uint32_t)-1, .tag1 = (uint32_t)-1, .tag2 = (uint32_t)-1 };
  int rc = psm2_mq_irecv2(p_psm2_context->mq, p_status->peer.epaddr, &tag, &tag_mask, 0 /* flags */,
                          v[0].iov_base, v[0].iov_len, NULL, &p_status->rreq);
}

static int nm_psm2_recv_poll_one(void*_status)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  psm2_mq_status2_t req_status;
  int rc = psm2_mq_test2(&p_status->rreq, &req_status);
  if(rc == PSM2_OK)
    {
      return NM_ESUCCESS;
    }
  else if(rc == PSM2_MQ_INCOMPLETE)
    {
      psm2_poll(p_psm2_process->myep);
      rc = psm2_mq_test2(&p_status->rreq, &req_status);
      if(rc == PSM2_OK)
        return NM_ESUCCESS;
      else
        return -NM_EAGAIN;
    }
  else
    {
      NM_FATAL("psm2: error in psm2_mq_test; rc = %d\n", rc);
      return -NM_EUNKNOWN;
    }
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
