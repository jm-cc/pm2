/** @file
 * @brief NewMad Minidriver over PadicoTM PSP.
 * @ingroup NewMadico
 */

#include <Padico/Puk.h>
#include <Padico/Topology.h>
#include <Padico/PSP.h>
#include <Padico/PadicoTM.h>

PADICO_MODULE_DECLARE(Minidriver_PSP);
PADICO_MODULE_INIT(padico_minipsp_module_init);

#ifdef MARCEL
#ifndef PIOMAN
#  error "PIOMan is required for NewMadico with Marcel"
#endif
#endif

#include <nm_public.h>
#include <nm_private.h>

/* ********************************************************* */

static void*padico_minipsp_instantiate(puk_instance_t instance, puk_context_t context);
static void padico_minipsp_destroy(void*);

static const struct puk_component_driver_s padico_minipsp_component =
  {
    .instantiate = &padico_minipsp_instantiate,
    .destroy     = &padico_minipsp_destroy
  };

static void padico_minipsp_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void padico_minipsp_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void padico_minipsp_connect(void*_status, const void*remote_url, size_t url_size);
static void padico_minipsp_send_post(void*_status, const struct iovec*v, int n);
static int  padico_minipsp_send_poll(void*_status);
static void padico_minipsp_recv_iov_post(void*_status,  struct iovec*v, int n);
static int  padico_minipsp_recv_poll_one(void*_status);
static int  padico_minipsp_recv_cancel(void*_status);

static const struct nm_minidriver_iface_s padico_minipsp_minidriver =
  {
    .getprops    = &padico_minipsp_getprops,
    .init        = &padico_minipsp_init,
    .connect     = &padico_minipsp_connect,
    .send_post   = &padico_minipsp_send_post,
    .send_poll   = &padico_minipsp_send_poll,
    .recv_iov_post = &padico_minipsp_recv_iov_post,
    .recv_poll_one = &padico_minipsp_recv_poll_one,
    .recv_cancel   = &padico_minipsp_recv_cancel
  };

struct padico_minipsp_url_s
{
  char uuid[PADICO_TOPO_UUID_SIZE];
  uint32_t tag;
} __attribute__((packed));

/** 'minipsp' per-context data.
 */
struct padico_minipsp_context_s
{
  struct padico_minipsp_url_s url;  /**< local url */
  puk_hashtable_t instance_by_node; /**< hash: node -> minipsp status */
};

struct minipsp_unexpected_s
{
  nm_len_t len;
  char _bytes[NM_SO_MAX_UNEXPECTED];
};
PUK_ALLOCATOR_TYPE(minipsp_unexpected, struct minipsp_unexpected_s);

PUK_LFQUEUE_TYPE(minipsp_unexpected, struct minipsp_unexpected_s*, NULL, 256);

/** 'minipsp' per-instance status.
 */
struct padico_minipsp_status_s
{
  struct puk_receptacle_PadicoSimplePackets_s psp; /**< receptacle for the used PSP */
  uint32_t remote_tag;                             /**< instance tag at destination */
  padico_na_node_t remote_node;                    /**< node for the gate */
  size_t h_size;                                   /**< PSP header size */
  struct
  {
    struct iovec posted;
    struct minipsp_unexpected_lfqueue_s unexpected;
  } recv;
  struct padico_minipsp_context_s*minipsp_context; /**< assembly context for this instance */
};

/** maximum number of contexts for minidriver_PSP */
#define PADICO_MINIPSP_MAX_CONTEXT 32

/** block of static data- table of contexts indexed by tag */
static struct
{
  struct padico_minipsp_context_s*contexts[PADICO_MINIPSP_MAX_CONTEXT];
  uint32_t context_count;
  minipsp_unexpected_allocator_t allocator;
} minipsp =
  {
    .contexts = { NULL },
    .context_count = 0,
    .allocator = NULL
  };

/* ********************************************************* */

static void*padico_minipsp_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct padico_minipsp_status_s*status = padico_malloc(sizeof(struct padico_minipsp_status_s));
  status->remote_tag = PADICO_PSP_TAG_NONE;
  assert(context != NULL);
  puk_context_indirect_PadicoSimplePackets(instance, "psp", &status->psp);
  status->remote_node = NULL;
  status->h_size = 0;
  status->recv.posted.iov_base = NULL;
  minipsp_unexpected_lfqueue_init(&status->recv.unexpected);
  status->minipsp_context = puk_context_get_status(context);
  assert(status->minipsp_context != NULL);
  return status;
}

static void padico_minipsp_destroy(void*_status)
{
  struct padico_minipsp_status_s*status = _status;
  status->remote_node = NULL;
  padico_free(_status);
}

/* ********************************************************* */


/** the PSP header for Minidriver_PSP
 */
struct padico_minipsp_hdr_s
{
  uint32_t tag;         /**< instance tag at destination */
  uint64_t length;      /**< payload length */
  uint16_t h_size;      /**< length of data in header */
  char _data;           /**< placeholder for first data byte in header */
} __attribute__((packed));


static void minipsp_callback(const void*_hdr, padico_na_node_t from, void*_key,
			     padico_psp_pump_t pump, void*token)
{
  const struct padico_minipsp_hdr_s*hdr = _hdr;
  const uint32_t tag = hdr->tag;
  assert(tag >= 0);
  assert(tag < minipsp.context_count);
  struct padico_minipsp_context_s*minipsp_context = minipsp.contexts[tag];
  struct padico_minipsp_status_s*status = puk_hashtable_lookup(minipsp_context->instance_by_node, from);
  if(status->recv.posted.iov_base != NULL)
    {
      assert(hdr->length <= status->recv.posted.iov_len);
      (*pump)(token, status->recv.posted.iov_base, status->recv.posted.iov_len);
      status->recv.posted.iov_base = NULL;
    }
  else
    {
      struct minipsp_unexpected_s*u = minipsp_unexpected_malloc(minipsp.allocator);
      u->len = hdr->length;
      (*pump)(token, &u->_bytes[0], u->len);
      int err = 0;
      do
	{
	  err = minipsp_unexpected_lfqueue_enqueue(&status->recv.unexpected, u);
	  if(err)
	    {
	      padico_warning("minidriver_PSP: unexpected queue full.\n");
	      sched_yield();
	    }
	}
      while(err);
    }
}

static void padico_minipsp_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  props->profile.latency = 5000;
  props->profile.bandwidth = 10000;
}

static void padico_minipsp_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct padico_minipsp_context_s*minipsp_context = padico_malloc(sizeof(struct padico_minipsp_context_s));
  puk_context_set_status(context, minipsp_context);
  minipsp_context->instance_by_node = puk_hashtable_new_ptr();
  const int seq = __sync_fetch_and_add(&minipsp.context_count, 1);
  padico_topo_uuid_t uuid = padico_na_node_getuuid(padico_na_getlocalnode());
  memcpy(&minipsp_context->url.uuid[0], uuid, PADICO_TOPO_UUID_SIZE);
  minipsp_context->url.tag = seq;
  *drv_url = &minipsp_context->url;
  *url_size = sizeof(struct padico_minipsp_url_s);
  minipsp.contexts[seq] = minipsp_context;
}

static void padico_minipsp_connect(void*_status, const void*_remote_url, size_t url_size)
{
  struct padico_minipsp_status_s*status = _status;
  const struct padico_minipsp_url_s*remote_url = _remote_url;
  status->remote_node = padico_na_getnodebyuuid((padico_topo_uuid_t)remote_url->uuid);
  assert(status->remote_node != NULL);
  const uint32_t remote_tag = remote_url->tag;
  status->remote_tag = remote_tag;
  status->h_size = sizeof(struct padico_minipsp_hdr_s);
  padico_psp_init(&status->psp, PADICO_PSP_MINIDRIVER, "Minidriver", &status->h_size, &minipsp_callback, NULL);
  puk_hashtable_insert(status->minipsp_context->instance_by_node, status->remote_node, status);
  padico_psp_listen(&status->psp, status->remote_node);
  padico_psp_connect(&status->psp, status->remote_node);
}

static void padico_minipsp_send_post(void*_status, const struct iovec*v, int n)
{
  struct padico_minipsp_status_s*status = _status;
  if(n == 1)
    {
      struct padico_minipsp_hdr_s*hdr = NULL;
      padico_psp_connection_t conn =
	padico_psp_new_message(&status->psp, status->remote_node, (void**)&hdr);
      hdr->tag    = status->remote_tag;
      hdr->length = v[0].iov_len;
      hdr->h_size = 0;
      padico_psp_pack(&status->psp, conn, v[0].iov_base, v[0].iov_len);
      padico_psp_end_message(&status->psp, conn);
    }
  else
    {
      padico_fatal("minipsp: send_post- iovec n > 1 not implemented yet.\n");
    }
}

static int  padico_minipsp_send_poll(void*_status)
{
  return NM_ESUCCESS;
}

static void padico_minipsp_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct padico_minipsp_status_s*status = _status;
  assert(status->recv.posted.iov_base == NULL);
  assert(n == 1);
  status->recv.posted.iov_len = v[0].iov_len;
  status->recv.posted.iov_base = v[0].iov_base;
}

static int padico_minipsp_recv_poll_one(void*_status)
{
  struct padico_minipsp_status_s*status = _status;
  if(status->recv.posted.iov_base == NULL)
    {
      return NM_ESUCCESS;
    }
  else
    {
      struct minipsp_unexpected_s*u = minipsp_unexpected_lfqueue_dequeue(&status->recv.unexpected);
      if(u != NULL)
	{
	  memcpy(status->recv.posted.iov_base, &u->_bytes[0], u->len);
	  minipsp_unexpected_free(minipsp.allocator, u);
	}
    }
  return -NM_EAGAIN;
}

static int padico_minipsp_recv_cancel(void*_status)
{
  return -NM_ENOTIMPL;
}

/* ********************************************************* */

static int padico_minipsp_module_init(void)
{
  minipsp.allocator = minipsp_unexpected_allocator_new(16);
  puk_component_declare("Minidriver_PSP",
			puk_component_provides("PadicoComponent", "component",  &padico_minipsp_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &padico_minipsp_minidriver),
			puk_component_uses("PadicoSimplePackets", "psp"));
  return 0;
}
