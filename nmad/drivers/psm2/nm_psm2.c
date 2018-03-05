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
static void nm_psm2_send_iov_post(void*_status, const struct iovec*v, int n);
static int  nm_psm2_send_poll(void*_status);
static void nm_psm2_recv_iov_post(void*_status,  struct iovec*v, int n);
static int  nm_psm2_recv_poll_one(void*_status);
static int  nm_psm2_recv_poll_any(puk_context_t p_context, void**_status);
static int  nm_psm2_recv_cancel_any(puk_context_t p_context);

static const struct nm_minidriver_iface_s nm_psm2_minidriver =
  {
    .getprops        = &nm_psm2_getprops,
    .init            = &nm_psm2_init,
    .close           = &nm_psm2_close,
    .connect         = &nm_psm2_connect,
    .send_iov_post       = &nm_psm2_send_iov_post,
    .send_data_post       = NULL,
    .send_poll       = &nm_psm2_send_poll,
    .recv_iov_post   = &nm_psm2_recv_iov_post,
    .recv_data_post  = NULL,
    .recv_poll_one   = &nm_psm2_recv_poll_one,
    .recv_poll_any   = &nm_psm2_recv_poll_any,
    .recv_wait_any   = NULL,
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

#define NM_PSM2_TAG_BUILD(CONTEXT_ID, PEER_ID)                          \
  ( ( ((uint64_t)(CONTEXT_ID)) << 32 ) | ((uint64_t)(PEER_ID)) )

#define NM_PSM2_TAG_INIT         NM_PSM2_TAG_BUILD(0xFFFF, 0)
#define NM_PSM2_TAG_MASK_CONTEXT NM_PSM2_TAG_BUILD(-1, 0)
#define NM_PSM2_TAG_MASK_FULL    NM_PSM2_TAG_BUILD(-1, -1)
#define NM_PSM2_TAG_GET_PEER_ID(TAG)            \
  ((TAG) & 0xFFFFFFFF)
#define NM_PSM2_TAG_GET_CONTEXT_ID(TAG)         \
  (((TAG) >> 32) & 0xFFFFFFFF)

struct nm_psm2_url_s
{
  psm2_epid_t epid;
  uint32_t context_id;
};

struct nm_psm2_peer_s
{
  struct nm_psm2_url_s url;
  uint32_t remote_peer_id;
  psm2_epaddr_t epaddr;
  struct nm_psm2_s*p_status;
};

PUK_VECT_TYPE(nm_psm2_peer, struct nm_psm2_peer_s*);

/** global per-process data (since PSM2 supports only **one endpoint per process**) */
struct nm_psm2_process_s
{
  struct nm_psm2_peer_vect_s peers;
  psm2_uuid_t uuid;
  psm2_ep_t myep;
  psm2_epid_t myepid;
  int next_context_id;
};

/** 'psm2' driver per-context data. */
struct nm_psm2_context_s
{
  struct nm_psm2_process_s*p_process;
  psm2_mq_t mq;
  struct nm_psm2_url_s url;
};

/** 'psm2' per-instance status. */
struct nm_psm2_s
{
  uint32_t local_id;
  struct nm_psm2_peer_s*p_peer;
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
  p_status->p_peer = NULL;
  return p_status;
}

static void nm_psm2_destroy(void*_status)
{
  struct nm_psm2_s*p_status = _status;
  free(p_status);
}

/* ********************************************************* */

static inline void nm_psm2_check_error(int rc, const char*function)
{
  if(rc != PSM2_OK)
    {
      NM_FATAL("psm2: error %d in %s (%s).\n", rc, function, psm2_error_get_string(rc));
    }
}

static void nm_psm2_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  p_props->capabilities.max_msg_size = UINT32_MAX;
  p_props->capabilities.has_recv_any = 1;
  p_props->capabilities.no_iovec = 1;
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
      nm_psm2_peer_vect_init(&p_psm2_process->peers);
      p_psm2_process->next_context_id = 1;
      const char*s_uuid = getenv("PADICO_BOOT_UUID");
      if(s_uuid)
        {
          int len = strlen(s_uuid);
          assert(len == 32);
          void*uuid = puk_base16_decode(s_uuid, &len, NULL);
          memcpy(p_psm2_process->uuid, uuid, 16);
        }
      else
        {
          NM_WARN("psm2: no session UUID; generating default UUID.\n");
          int i;
          for(i = 0; i < 16; i++)
            {
              p_psm2_process->uuid[i] = 0x02;
            }
        }
      int ver_major = PSM2_VERNO_MAJOR;
      int ver_minor = PSM2_VERNO_MINOR;
      rc = psm2_init(&ver_major, &ver_minor);
      nm_psm2_check_error(rc, "psm2_init");
      uint32_t num_units = -1;
      psm2_ep_num_devunits(&num_units);
      NM_DISPF("psm2- detected %u psm2 units\n", (unsigned)num_units);
      struct psm2_ep_open_opts options;
      rc = psm2_ep_open_opts_get_defaults(&options);
      nm_psm2_check_error(rc, "psm2_ep_open_opts_get_defaults [ get default options ]");
      /* force options */
      options.affinity = 0;
      rc = psm2_ep_open(p_psm2_process->uuid, &options, &p_psm2_process->myep, &p_psm2_process->myepid);
      nm_psm2_check_error(rc, "psm2_ep_open [open endpoint ]");
    }
  p_psm2_context->p_process = p_psm2_process;
  p_psm2_context->url.context_id = p_psm2_process->next_context_id++;
  p_psm2_context->url.epid = p_psm2_process->myepid;
  rc = psm2_mq_init(p_psm2_process->myep, NM_PSM2_TAG_MASK_CONTEXT, NULL, 0, &p_psm2_context->mq);
  nm_psm2_check_error(rc, "psm2_mq_init");
  puk_context_set_status(context, p_psm2_context);
  *p_url = &p_psm2_context->url;
  *p_url_size = sizeof(struct nm_psm2_url_s);
}

static void nm_psm2_close(puk_context_t context)
{
  struct nm_psm2_context_s*p_psm2_context = puk_context_get_status(context);
  puk_context_set_status(context, NULL);
  free(p_psm2_context);
}

static void nm_psm2_connect(void*_status, const void*_remote_url, size_t url_size)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  assert(url_size == sizeof(struct nm_psm2_url_s));
  const struct nm_psm2_url_s*p_remote_url = _remote_url;
  assert(p_status->p_peer == NULL);
  nm_psm2_peer_vect_itor_t i;
  puk_vect_foreach(i, nm_psm2_peer, &p_psm2_process->peers)
    {
      if( ((*i)->url.epid == p_remote_url->epid) &&
          ((*i)->url.context_id == p_remote_url->context_id) )
        {
          /* peer is already known */
          p_status->p_peer = *i;
          assert((*i)->p_status == NULL);
          (*i)->p_status = p_status;
        }
    }
  if(p_status->p_peer == NULL)
    {
      struct nm_psm2_peer_s*p_peer = malloc(sizeof(struct nm_psm2_peer_s));
      p_peer->url = *p_remote_url;
      p_peer->remote_peer_id = -1;
      p_peer->p_status = NULL; /* will be set upon connection success */
      p_status->p_peer = p_peer;
      i = nm_psm2_peer_vect_push_back(&p_psm2_process->peers, p_peer);
      p_status->local_id = nm_psm2_peer_vect_rank(&p_psm2_process->peers, i);
      assert(p_status->local_id >= 0);
    }
  psm2_error_t error = 0;
  int rc = psm2_ep_connect(p_psm2_process->myep, 1, &p_remote_url->epid, NULL, &error,
                           &p_status->p_peer->epaddr, 0);
  nm_psm2_check_error(rc, "psm2_ep_connect");
  struct nm_psm2_peer_id_s
  {
    struct nm_psm2_url_s url;
    uint32_t local_id;
  } local_peer_id = { .url = p_psm2_context->url, .local_id = p_status->local_id };
  psm2_mq_req_t sreq;
  rc = psm2_mq_isend(p_psm2_context->mq, p_status->p_peer->epaddr, 0 /* flags */,
                     NM_PSM2_TAG_INIT, &local_peer_id, sizeof(struct nm_psm2_peer_id_s), NULL, &sreq);
  nm_psm2_check_error(rc, "psm2_mq_isend");
  rc = psm2_mq_wait(&sreq, NULL);
  nm_psm2_check_error(rc, "psm2_mq_wait");
  while(p_status->p_peer->p_status == NULL)
    {
      psm2_mq_req_t rreq;
      struct nm_psm2_peer_id_s remote_peer_id = { 0 };
      rc = psm2_mq_irecv(p_psm2_context->mq, NM_PSM2_TAG_INIT, NM_PSM2_TAG_MASK_CONTEXT, 0 /* flags */,
                         &remote_peer_id, sizeof(struct nm_psm2_peer_id_s), NULL, &rreq);
      nm_psm2_check_error(rc, "psm2_mq_irecv");
      rc = psm2_mq_wait(&rreq, NULL);
      nm_psm2_check_error(rc, "psm2_mq_wait");
      if((remote_peer_id.url.epid == p_remote_url->epid) &&
         (remote_peer_id.url.context_id == p_remote_url->context_id))
        {
          p_status->p_peer->remote_peer_id = remote_peer_id.local_id;
          p_status->p_peer->p_status = p_status;
        }
      else
        {
          struct nm_psm2_peer_s*p_peer = NULL;
          puk_vect_foreach(i, nm_psm2_peer, &p_psm2_process->peers)
            {
              if( ((*i)->url.epid == remote_peer_id.url.epid) &&
                  ((*i)->url.context_id == remote_peer_id.url.context_id) )
                {
                  p_peer = *i;
                  assert(p_peer->p_status == NULL);
                  p_peer->remote_peer_id = remote_peer_id.local_id;
                }
            }
          if(p_peer == NULL)
            {
              struct nm_psm2_peer_s*p_peer = malloc(sizeof(struct nm_psm2_peer_s));
              p_peer->url = remote_peer_id.url;
              p_peer->remote_peer_id = remote_peer_id.local_id;
              p_peer->p_status = NULL;
              nm_psm2_peer_vect_push_back(&p_psm2_process->peers, p_peer);
            }
        }
    }
}

/* ********************************************************* */

static void nm_psm2_send_iov_post(void*_status, const struct iovec*v, int n)
{
  struct nm_psm2_s*p_status = _status;
  struct nm_psm2_context_s*p_psm2_context = p_status->p_psm2_context;
  assert(n == 1);
  int rc = psm2_mq_isend(p_psm2_context->mq, p_status->p_peer->epaddr, 0 /* flags */,
                         NM_PSM2_TAG_BUILD(p_status->p_peer->url.context_id, p_status->p_peer->remote_peer_id),
                         v[0].iov_base, v[0].iov_len, NULL, &p_status->sreq);
  nm_psm2_check_error(rc, "psm2_mq_isend");
}

static int nm_psm2_send_poll(void*_status)
{
  struct nm_psm2_s*p_status = _status;
  int rc = psm2_mq_test(&p_status->sreq, NULL);
  if(rc == PSM2_OK)
    {
      return NM_ESUCCESS;
    }
  else if(rc == PSM2_MQ_INCOMPLETE)
    {
      psm2_poll(p_psm2_process->myep);
      rc = psm2_mq_test(&p_status->sreq, NULL);
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
  int rc = psm2_mq_irecv(p_psm2_context->mq,
                         NM_PSM2_TAG_BUILD(p_status->p_peer->url.context_id, p_status->local_id),
                         NM_PSM2_TAG_MASK_FULL, 0 /* flags */,
                         v[0].iov_base, v[0].iov_len, NULL, &p_status->rreq);
  nm_psm2_check_error(rc, "psm2_mq_irecv");
}

static int nm_psm2_recv_poll_one(void*_status)
{
  struct nm_psm2_s*p_status = _status;
  int rc = psm2_mq_test(&p_status->rreq, NULL);
  if(rc == PSM2_OK)
    {
      return NM_ESUCCESS;
    }
  else if(rc == PSM2_MQ_INCOMPLETE)
    {
      psm2_poll(p_psm2_process->myep);
      rc = psm2_mq_test(&p_status->rreq, NULL);
      if(rc == PSM2_OK)
        {
          return NM_ESUCCESS;
        }
      else
        {
          return -NM_EAGAIN;
        }
    }
  else
    {
      NM_FATAL("psm2: error in psm2_mq_test; rc = %d\n", rc);
      return -NM_EUNKNOWN;
    }
}

static int nm_psm2_recv_poll_any(puk_context_t p_context, void**_status)
{
  struct nm_psm2_context_s*p_psm2_context = puk_context_get_status(p_context);
  psm2_mq_status_t req_status;
  int rc = psm2_mq_iprobe(p_psm2_context->mq,
                           NM_PSM2_TAG_BUILD(p_psm2_context->url.context_id, 0),
                           NM_PSM2_TAG_MASK_CONTEXT,
                           &req_status);
  if(rc == PSM2_OK)
    {
      const uint32_t peer_id = NM_PSM2_TAG_GET_PEER_ID(req_status.msg_tag);
      assert(peer_id >= 0);
      assert(peer_id < nm_psm2_peer_vect_size(&p_psm2_process->peers));
      const struct nm_psm2_peer_s*p_peer = nm_psm2_peer_vect_at(&p_psm2_process->peers, peer_id);
      struct nm_psm2_s*p_status = p_peer->p_status;
      *_status = p_status;
      return NM_ESUCCESS;
    }
  else if(rc == PSM2_MQ_INCOMPLETE)
    {
      psm2_poll(p_psm2_process->myep);
      *_status = NULL;
      return -NM_EAGAIN;
    }
  else
    {
      nm_psm2_check_error(rc, "psm2_mq_improbe");
      return -NM_EUNKNOWN;
    }
}

static int nm_psm2_recv_cancel_any(puk_context_t p_context)
{
  /* nothing to do */
  return NM_ESUCCESS;
}
