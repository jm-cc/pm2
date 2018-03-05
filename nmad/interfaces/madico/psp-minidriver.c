/** @file psp-minidriver.c
 *  @brief PSP interface for nmad minidriver
 */

#include <Padico/Module.h>
#include <Padico/Puk.h>
#include <Padico/Topology.h>
#include <Padico/PSP.h>
#include <Padico/NetSelector.h>
#include <Padico/AddrDB.h>
#include <Padico/PadicoControl.h>

PADICO_MODULE_DECLARE(PSP_minidriver);
PADICO_MODULE_INIT(padico_psp_minidriver_init);

#if defined(NMAD) && defined(PIOMAN)

#include <nm_minidriver.h>
#include <pioman.h>


/* ********************************************************* */
/* *** PadicoSimplePackets for minidriver */

static void*psp_minidriver_instantiate(puk_instance_t instance, puk_context_t context);
static void psp_minidriver_destroy(void*_status);
static void psp_minidriver_init(void*_status, uint32_t tag, const char*label, size_t*h_size,
				padico_psp_handler_t handler, void*key);
static void psp_minidriver_listen(void*_status, padico_na_node_t node);
static void psp_minidriver_connect(void*_status, padico_na_node_t node);
static padico_psp_connection_t psp_minidriver_new_message(void*_status, 
								    padico_na_node_t node, void**_sbuf);
static void psp_minidriver_pack(void*_status, padico_psp_connection_t _conn,
			  const char*bytes, size_t size);
static void psp_minidriver_end_message(void*_status, padico_psp_connection_t _conn);

/** instanciation facet for PSP-minidriver
 * @ingroup PSP-minidriver
 */
const static struct puk_component_driver_s psp_minidriver_component_driver =
  {
    .instantiate = &psp_minidriver_instantiate,
    .destroy     = &psp_minidriver_destroy
  };

/** 'SimplePackets' facet for PSP-minidriver
* @ingroup PSP-minidriver
*/
const static struct padico_psp_driver_s psp_minidriver_driver =
  {
    .init        = &psp_minidriver_init,
    .listen      = &psp_minidriver_listen,
    .connect     = &psp_minidriver_connect,
    .new_message = &psp_minidriver_new_message,
    .pack        = &psp_minidriver_pack,
    .end_message = &psp_minidriver_end_message
  };

/* ********************************************************* */
/* *** Structures */


struct psp_minidriver_connreq_s
{
  padico_na_node_t node;
  uint32_t tag;
};

PUK_VECT_TYPE(psp_minidriver_conns, struct psp_minidriver_status_s*);
PUK_VECT_TYPE(psp_minidriver_iov, struct iovec);

#define PSP_MINIDRIVER_SBUF_MAX 256

struct psp_minidriver_sreq_s
{
  struct psp_minidriver_status_s*status;
  struct psp_minidriver_iov_vect_s chunks;
  char sbuf[PSP_MINIDRIVER_SBUF_MAX];
};

PUK_ALLOCATOR_TYPE(psp_minidriver_sreq, struct psp_minidriver_sreq_s);

static struct padico_minidriver_s
{
  struct psp_minidriver_conns_vect_s conns;    /**< established connections */
  struct padico_psp_directory_s slots;         /**< PSP demultiplexing */
  int polling_thread_launched;                 /**< if polling thread was started */
  marcel_mutex_t lock;                         /**< local access to global info (conns & slots) */
  marcel_cond_t cond;
  struct piom_ltask ltask;
  psp_minidriver_sreq_allocator_t sreq_alloc;
} padico_minidriver;


struct psp_minidriver_status_s
{
  struct
  {
    struct puk_receptacle_NewMad_minidriver_s r;
    puk_context_t context;                     /**< sub-contexts for driver */
    struct nm_minidriver_properties_s props;   /**< driver properties */
    const void*url;                            /**< driver remote url */
    padico_string_t addrdb_name;               /**< ref for this url in AddrDB */
  } trk[2];
  padico_na_node_t remote_node;                /**< remote padico node */
  enum { PSP_MINIDRIVER_STATE_NONE, PSP_MINIDRIVER_STATE_INPROGRESS, PSP_MINIDRIVER_STATE_ESTABLISHED } state;
  padico_psp_slot_t slot;                      /**< PSP slot */
  size_t h_size;                               /**< user's requeted header size */
  char*rbuf;                                   /**< header buffer for receive */
  struct iovec v;                              /**< iovec for rbuf  */
  size_t url_size;
  marcel_mutex_t mutex;                        /**< connection mutex */
  int threshold;
  volatile int pending;                        /**< whether a received packet is pending */
  volatile int posted;                         /**< whether a recv is posted */
};


/* ********************************************************* */

static void*psp_minidriver_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct psp_minidriver_status_s*status = padico_malloc(sizeof(struct psp_minidriver_status_s));
  puk_context_indirect_NewMad_minidriver(instance, "trk0", &status->trk[0].r);
  puk_context_indirect_NewMad_minidriver(instance, "trk1", &status->trk[1].r);
  puk_component_conn_t trk0 = puk_context_conn_lookup(context, NULL, "trk0");
  puk_component_conn_t trk1 = puk_context_conn_lookup(context, NULL, "trk1");
  status->slot = NULL;
  status->trk[0].context = trk0->context;
  status->trk[1].context = trk1->context;

  (*status->trk[0].r.driver->getprops)(trk0->context, &status->trk[0].props);
  size_t url0_size = 0, url1_size = 0;
  (*status->trk[0].r.driver->init)(status->trk[0].context, &status->trk[0].url, &url0_size);
  (*status->trk[1].r.driver->init)(status->trk[1].context, &status->trk[1].url, &url1_size);
  assert(url0_size == url1_size);
  status->url_size = url0_size;
  padico_out(40, "instantiate- status = %p; url_size = %d\n", status, (int)status->url_size);
  status->threshold = 8*1024;
  status->pending = 0;
  status->posted = 0;
  marcel_mutex_init(&status->mutex, NULL);
  return status;
}

static void psp_minidriver_destroy(void*_status)
{
  struct psp_minidriver_status_s*status = (struct psp_minidriver_status_s*)_status;
  if(status->slot)
    padico_psp_slot_remove(&padico_minidriver.slots, status->slot);
  padico_free(status);
}

/* ********************************************************* */


static void psp_minidriver_pump(void*_token, void*bytes, size_t size)
{
  struct psp_minidriver_status_s*status = _token;
  struct iovec v = { .iov_base = bytes, .iov_len = size };
  const int trk = (size > status->threshold) ? 1 : 0;
  /* pump size message in bytes ptr */
  (*status->trk[trk].r.driver->recv_iov_post)(status->trk[trk].r._status, &v, 1);
  int rc = -1;
  do
    {
      rc = (*status->trk[trk].r.driver->recv_poll_one)(status->trk[trk].r._status);
    }
  while(rc != 0);

}


static void psp_minidriver_refill(struct psp_minidriver_status_s*status)
{
  status->pending = 0;
  *(uint32_t*)status->rbuf = 0xDEADBEEF;
  status->v.iov_base = status->rbuf;
  status->v.iov_len = status->slot->h_size;
  (*status->trk[0].r.driver->recv_iov_post)(status->trk[0].r._status, &status->v, 1);
  status->posted = 1;
}

static int psp_minidriver_task_send(void*_sreq)
{
  struct psp_minidriver_sreq_s*sreq = _sreq;
  struct psp_minidriver_status_s*status = sreq->status;
  struct iovec v;
  v.iov_base = sreq->sbuf;
  v.iov_len = status->slot->h_size;
  (*status->trk[0].r.driver->send_iov_post)(status->trk[0].r._status, &v, 1);
  int rc = -1;
  do
    {
      rc = (*status->trk[0].r.driver->send_poll)(status->trk[0].r._status);
    }
  while(rc != 0);
  psp_minidriver_iov_vect_itor_t i;
  puk_vect_foreach(i, psp_minidriver_iov, &sreq->chunks)
    {
      const int trk = (i->iov_len > status->threshold) ? 1 : 0;
      (*status->trk[trk].r.driver->send_iov_post)(status->trk[trk].r._status, i, 1);
      int rc = -1;
      do
	{
	  rc = (*status->trk[trk].r.driver->send_poll)(status->trk[trk].r._status);
	}
      while(rc != 0);
    }
  return 0;
}

static int psp_minidriver_task_poll(void*_status)
{
  struct psp_minidriver_status_s*status = NULL;
  psp_minidriver_conns_vect_itor_t i;
  puk_vect_foreach(i, psp_minidriver_conns, &padico_minidriver.conns)
    {
      status = *i;
      if(status->posted && !status->pending)
	{
	  assert(status->state == PSP_MINIDRIVER_STATE_ESTABLISHED);
	  int rc = (*status->trk[0].r.driver->recv_poll_one)(status->trk[0].r._status);
	  if(rc == 0)
	    {
	      status->pending = 1;
	      status->posted = 0;
	      break;
	    }
	}
      else if(status->pending)
	{
	  break;
	}
    }
  if(status != NULL && status->pending)
    {
      *(struct psp_minidriver_status_s**)_status = status;
      piom_ltask_completed(&padico_minidriver.ltask);
    }
  return 0;
}

static void*psp_minidriver_worker(void*dummy)
{
  volatile struct psp_minidriver_status_s*_status = NULL;
  piom_ltask_create(&padico_minidriver.ltask, &psp_minidriver_task_poll, &_status, PIOM_LTASK_OPTION_REPEAT);
  for(;;)
    {
      piom_ltask_submit(&padico_minidriver.ltask);
      piom_ltask_wait(&padico_minidriver.ltask);
      struct psp_minidriver_status_s*status = (void*)_status;
      if(status)
	{
	  padico_out(40, "worker- dispatching...\n");
	  /*	  marcel_mutex_lock(&status->mutex); */
	  const uint32_t*p_tag = (void*)status->rbuf;
	  assert(*p_tag == status->slot->tag);
	  const padico_psp_slot_t slot = padico_psp_slot_lookup(&padico_minidriver.slots, *p_tag);
	  (*(slot->handler))(p_tag + 1, status->remote_node, slot->key, &psp_minidriver_pump, status);
	  psp_minidriver_refill(status);
	  /*   marcel_mutex_unlock(&status->mutex); */
	  padico_out(40, "worker- dispatching done.\n");
	}
    }
  return NULL;
}

static void psp_minidriver_do_connect(struct psp_minidriver_status_s*status)
{
  padico_out(40, "status = %p; connecting node = %s...\n", status, (const char*)padico_na_node_getuuid(status->remote_node));
  marcel_mutex_lock(&status->mutex);
  if(status->state == PSP_MINIDRIVER_STATE_NONE)
    {
      status->state = PSP_MINIDRIVER_STATE_INPROGRESS;
      int trk;
      for(trk = 0; trk <= 1; trk++)
	{
	  char remote_url[status->url_size];
	  padico_out(40, "trk#%d waiting peer address...\n", trk);
	  padico_req_t req = padico_tm_req_new(NULL, NULL);
	  padico_addrdb_get(status->remote_node, padico_module_self_name(),
			    padico_string_get(status->trk[trk].addrdb_name), padico_string_size(status->trk[trk].addrdb_name),
			    &remote_url[0], status->url_size, req);
	  padico_tm_req_wait(req);
	  struct puk_receptacle_NewMad_minidriver_s*r = &status->trk[trk].r;
	  (*r->driver->connect)(r->_status, remote_url, status->url_size);
	  padico_out(40, "trk#%d connection established \n", trk);
	}
      status->state = PSP_MINIDRIVER_STATE_ESTABLISHED;
      psp_minidriver_refill(status);
    }
  marcel_mutex_unlock(&status->mutex);
  if(!padico_minidriver.polling_thread_launched)
    {
      padico_minidriver.polling_thread_launched = 1;
      padico_rttask_t t = padico_rttask_new();
      t->kind = PADICO_RTTASK_RTTHREAD;
      t->background_fun = &psp_minidriver_worker;
      t->background_arg = NULL;
      padico_tm_rttask_submit(t);
    }
  padico_out(40, "connected.\n");
}

static void*psp_minidriver_connection_worker(void*_creq)
{
  struct psp_minidriver_connreq_s*creq = _creq;
  struct psp_minidriver_status_s*status = NULL;
  psp_minidriver_conns_vect_itor_t i;
  marcel_mutex_lock(&padico_minidriver.lock);
  while(status == NULL)
    {
      puk_vect_foreach(i, psp_minidriver_conns, &padico_minidriver.conns)
	{
	  if(((*i)->remote_node == creq->node) && ((*i)->slot->tag == creq->tag))
	    {
	      status = *i;
	      break;
	    }
	}
      if(status == NULL)
	{
	  marcel_cond_wait(&padico_minidriver.cond, &padico_minidriver.lock);
	}
    }
  marcel_mutex_unlock(&padico_minidriver.lock);
  psp_minidriver_do_connect(status);
  padico_free(creq);
  return NULL;
}

static void psp_minidriver_connect_handler(puk_parse_entity_t e)
{
  const char*uuid = puk_parse_getattr(e, "uuid");
  const char*s_tag = puk_parse_getattr(e, "tag");
  assert(uuid && s_tag);
  padico_na_node_t node = padico_na_getnodebyuuid((padico_topo_uuid_t)uuid);
  uint32_t tag = strtoul(s_tag, NULL, 16);
  struct psp_minidriver_connreq_s*creq = padico_malloc(sizeof(struct psp_minidriver_connreq_s));
  creq->node = node;
  creq->tag = tag;
  padico_tm_invoke_later(&psp_minidriver_connection_worker, creq);
}

static struct puk_tag_action_s psp_minidriver_action_connect =
{
  .xml_tag        = "PSP-minidriver:connect",
  .start_handler  = &psp_minidriver_connect_handler,
  .end_handler    = NULL,
  .required_level = PUK_TRUST_CONTROL
};

/* ********************************************************* */

#endif /* NMAD */

extern int padico_psp_minidriver_init(void)
{
#if defined (NMAD) && defined(PIOMAN)
  marcel_mutex_init(&padico_minidriver.lock, NULL);
  marcel_cond_init(&padico_minidriver.cond, NULL);
  puk_component_declare("PSP-minidriver",
			puk_component_provides("PadicoComponent", "component", &psp_minidriver_component_driver),
			puk_component_provides("PadicoSimplePackets", "psp", &psp_minidriver_driver),
			puk_component_uses("NewMad_minidriver", "trk0"),
			puk_component_uses("NewMad_minidriver", "trk1"));

  puk_xml_add_action(psp_minidriver_action_connect);
 
  /* Initialize static structure */
  padico_psp_directory_init(&padico_minidriver.slots);
  psp_minidriver_conns_vect_init(&padico_minidriver.conns);
  padico_minidriver.polling_thread_launched = 0;
  padico_minidriver.sreq_alloc = psp_minidriver_sreq_allocator_new(16);
  return 0;
  
#else  /* NMAD && PIOMAN */

  padico_warning("PSP-minidriver: not initialized- NewMad not included in core.");
  return -1;

#endif /* NMAD && PIOMAN */
}

#if defined (NMAD) && defined(PIOMAN)

/* ********************************************************* */

static void psp_minidriver_init(void*_status, 
				uint32_t tag, 
				const char*label, 
				size_t*h_size,
				padico_psp_handler_t handler, 
				void*key)
{
  struct psp_minidriver_status_s*status = (struct psp_minidriver_status_s*)_status;
  /* Now we know what is the size of the buffer, we can allocate (and 1 uint32_t is kept for us) */
  *h_size += 16;
  status->h_size = *h_size;
  status->slot = padico_psp_slot_insert(&padico_minidriver.slots, handler, key, tag, status->h_size);
  status->rbuf = padico_malloc(*h_size);
  status->state = PSP_MINIDRIVER_STATE_NONE;
  assert(*h_size <= PSP_MINIDRIVER_SBUF_MAX);
}

static void psp_minidriver_listen(void*_status, padico_na_node_t node)
{
  struct psp_minidriver_status_s*status = _status;
  padico_out(40, "listen status= %p; node = %s; tag = 0x%x\n",
	       status, (const char*)padico_na_node_getuuid(node), status->slot->tag);
  status->remote_node = node;
  int trk;
  for(trk = 0; trk <= 1; trk++)
    {
      padico_string_t s_name = padico_string_new();
      padico_string_catf(s_name, "psp-minidriver-trk%d-tag%X", trk, status->slot->tag);
      padico_addrdb_publish(node, padico_module_self_name(),
			    padico_string_get(s_name), padico_string_size(s_name), status->trk[trk].url, status->url_size);
      status->trk[trk].addrdb_name = s_name;
    }
  marcel_mutex_lock(&padico_minidriver.lock);
  psp_minidriver_conns_vect_push_back(&padico_minidriver.conns, status);
  marcel_cond_signal(&padico_minidriver.cond);
  marcel_mutex_unlock(&padico_minidriver.lock);
  padico_out(40, "listen status= %p; node = %s; done.\n", status, (const char*)padico_na_node_getuuid(node));
}

static void psp_minidriver_connect(void*_status, padico_na_node_t node)
{
  struct psp_minidriver_status_s*status = _status;
  assert(status->remote_node == node);
  if(status->state == PSP_MINIDRIVER_STATE_NONE)
    { 
      /* reverse connect */
      padico_string_t s_req = padico_string_new();
      padico_string_catf(s_req, "<PSP-minidriver:connect uuid=\"%s\" tag=\"0x%x\"/>\n", 
			 (char*)padico_na_node_getuuid(padico_na_getlocalnode()), status->slot->tag);
      padico_control_send_oneway(node, padico_string_get(s_req));
      padico_string_delete(s_req);
      /* forward connect */
      psp_minidriver_do_connect(status);
    }
}

static padico_psp_connection_t psp_minidriver_new_message(void*_status, padico_na_node_t node, void**_sbuf)
{
  struct psp_minidriver_status_s*const status = (struct psp_minidriver_status_s*)_status;
  padico_out(40, "status = %p\n", status);
  assert(node == status->remote_node);
  assert(status->state == PSP_MINIDRIVER_STATE_ESTABLISHED);
  struct psp_minidriver_sreq_s*sreq = psp_minidriver_sreq_malloc(padico_minidriver.sreq_alloc);
  psp_minidriver_iov_vect_init(&sreq->chunks);
  sreq->status = status;
  uint32_t*p_tag = (void*)&sreq->sbuf[0];
  *p_tag = status->slot->tag;
  *_sbuf = p_tag + 1;
  return sreq;
}

static void psp_minidriver_pack(void*_status, padico_psp_connection_t _conn, const char*bytes, size_t size)
{
  struct psp_minidriver_sreq_s*sreq = (struct psp_minidriver_sreq_s*)_conn;
  struct iovec v = { .iov_base = (void*)bytes, .iov_len = size };
  psp_minidriver_iov_vect_push_back(&sreq->chunks, v);
}

static void psp_minidriver_end_message(void*_status, padico_psp_connection_t _conn)
{
  struct psp_minidriver_sreq_s*sreq = (struct psp_minidriver_sreq_s*)_conn;
  struct piom_ltask ltask;
  piom_ltask_create(&ltask, &psp_minidriver_task_send, sreq, PIOM_LTASK_OPTION_ONESHOT);
  piom_ltask_submit(&ltask);
  piom_ltask_wait(&ltask);
  /* psp_minidriver_iov_vect_resize(&status->chunks, 0); */
  psp_minidriver_iov_vect_destroy(&sreq->chunks);
  psp_minidriver_sreq_free(padico_minidriver.sreq_alloc, sreq);
  padico_out(40, "done.\n");
}
#endif /* NMAD && PIOMAN */
