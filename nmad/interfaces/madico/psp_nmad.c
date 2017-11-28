/** @file psp_nmad.c
 *  @brief PadicoTM PSP interface over nmad
 */

#include <Padico/Module.h>
#include <Padico/Puk.h>
#include <Padico/Topology.h>
#include <Padico/PSP.h>
#include <Padico/NetSelector.h>
#include <Padico/AddrDB.h>
#include <Padico/PadicoControl.h>
#include <Padico/PadicoTM.h>

PADICO_MODULE_DECLARE(PSP_NMad);
PADICO_MODULE_INIT(padico_psp_nmad_init);

#include <nm_public.h>
#include <nm_private.h>
#include <nm_sendrecv_interface.h>

/* ********************************************************* */
/* *** PadicoSimplePackets for NewMad Verbs */

static void*psp_nmad_instantiate(puk_instance_t ai, puk_context_t context);
static void psp_nmad_destroy(void*_instance);
static void psp_nmad_init(void*_instance, uint32_t tag, const char*label, size_t*h_size,
			  padico_psp_handler_t handler, void*key);
static void psp_nmad_connect(void*_instance, padico_na_node_t node);
static padico_psp_connection_t psp_nmad_new_message(void*_instance, 
							      padico_na_node_t node, void**_sbuf);
static void psp_nmad_pack(void*_instance, padico_psp_connection_t _conn,
			  const char*bytes, size_t size);
static void psp_nmad_end_message(void*_instance, padico_psp_connection_t _conn);

/** instanciation facet for PSP-NMAD
 * @ingroup PSP-NMAD
 */
const static struct puk_component_driver_s psp_nmad_component_driver =
  {
    .instantiate = &psp_nmad_instantiate,
    .destroy     = &psp_nmad_destroy
  };

/** 'SimplePackets' facet for PSP-NMAD
* @ingroup PSP-NMAD
*/
const static struct padico_psp_driver_s psp_nmad_driver =
  {
    .init        = &psp_nmad_init,
    .connect     = &psp_nmad_connect,
    .listen      = NULL,
    .new_message = &psp_nmad_new_message,
    .pack        = &psp_nmad_pack,
    .end_message = &psp_nmad_end_message
  };

/* ********************************************************* */
/* *** Structures */

enum padico_nmad_conn_state_e
  {
    PSP_NMAD_CONN_NONE = 0,   /**< connection is created- unconnected, no pending request */
    PSP_NMAD_CONN_INPROGRESS, /**< connection in progress- address published, session connecting */
    PSP_NMAD_CONN_ESTABLISHED /**< connection successfully connected */
  };

#define PSP_NMAD_URL_SIZE 64

PUK_LIST_TYPE(padico_nmad_connection,
	      padico_na_node_t remote_node;        /**< remote padico node */
	      nm_gate_t gate;                      /**< newmad gate for given node */
	      enum padico_nmad_conn_state_e state;
	      char url[PSP_NMAD_URL_SIZE];         /**< newmad url for remote node */
	      marcel_mutex_t mutex;                /**< connection mutex */
	      char*sbuf;                           /**< pre allocated header buffer */
	      int header_sent;                     /**< state of the header */
	      nm_sr_request_t request;             /**< nmad requests for the tag */
	      );

static struct padico_nmad_s
{
  struct padico_psp_directory_s slots;         /**< PSP demultiplexing */
  puk_hashtable_t connections_by_node;         /**< [hash: node -> nmad_connection] */
  puk_hashtable_t connections_by_gate;         /**< [tab: gate -> nmad_connection] */
  nm_session_t p_session;                      /**< NMad session object for PSP */
  puk_component_t assembly;                      /**< assembly used by NMad */    
  char*local_url;                              /**< my url */
  int polling_thread_launched;                 /**< if polling thread was started */
  marcel_mutex_t lock;
} padico_nmad = { .p_session = NULL, .assembly = NULL, .local_url = NULL };

struct psp_nmad_instance_s
{
  padico_psp_slot_t slot;                      /**< PSP slot */
  uint32_t tag;                                /**< Corresponding PSP tag */
  size_t h_size;                               /**< user's requeted header size */
};


/* ********************************************************* */
/* *** Instances Functions */

static void*psp_nmad_instantiate(puk_instance_t ai, puk_context_t context)
{
  struct psp_nmad_instance_s*instance = padico_malloc(sizeof(struct psp_nmad_instance_s));
  instance->slot = NULL;
  return instance;
}

static void psp_nmad_destroy(void*_instance)
{
  struct psp_nmad_instance_s*instance = (struct psp_nmad_instance_s*)_instance;
  if(instance->slot)
    padico_psp_slot_remove(&padico_nmad.slots, instance->slot);
  padico_free(instance);
}

/* ********************************************************* */
/* *** Polling function */
static void psp_nmad_pump(void*token, void*bytes, size_t size)
{
  nm_sr_request_t request;

  /* token = nmad connection */
  const nm_gate_t gate = *(nm_gate_t*)token;

  /* pump size message in bytes ptr */
  nm_sr_irecv(padico_nmad.p_session, gate, 0, bytes, size, &request);
  nm_sr_rwait(padico_nmad.p_session, &request);
}

static void*psp_nmad_worker(void*dummy)
{
  void*buf = NULL;
  size_t buf_size = 0;
  for(;;)
    {
      nm_gate_t gate;
      nm_sr_request_t request;
      uint32_t tag;
      /* Get tag when a message comes */
      nm_sr_irecv(padico_nmad.p_session, NM_ANY_GATE, 0, &tag, sizeof(uint32_t), &request);
      nm_sr_rwait(padico_nmad.p_session, &request);

      /* Get psp_slot */
      const padico_psp_slot_t slot = padico_psp_slot_lookup(&padico_nmad.slots, tag);
      if(slot->h_size > buf_size)
	{
	  buf = padico_realloc(buf, slot->h_size);
	  memset(buf, 0, slot->h_size);
	  buf_size = slot->h_size;
	}
      /* get the node */
      nm_sr_recv_source(padico_nmad.p_session, &request, &gate);

      /* Callback: pre "unpack" header */
      nm_sr_irecv(padico_nmad.p_session, gate, 0, buf, slot->h_size, &request);
      nm_sr_rwait(padico_nmad.p_session, &request); 
      struct padico_nmad_connection_s*conn = puk_hashtable_lookup(padico_nmad.connections_by_gate, gate);
      (*(slot->handler))(buf, conn->remote_node, slot->key, &psp_nmad_pump, &gate);
    }
  return NULL;
}

/* ********************************************************* */
/* *** helper functions */

static void psp_nmad_do_connect(padico_na_node_t node)
{
  int err = NM_ESUCCESS;
  marcel_mutex_lock(&padico_nmad.lock);
  if(!padico_nmad.assembly)
    {
      padico_nmad.assembly = padico_ns_serial_selector(node, NULL, puk_iface_NewMad_Driver());
      padico_setenv("NMAD_ASSEMBLY", padico_nmad.assembly->name);
    }
  struct padico_nmad_connection_s*conn = puk_hashtable_lookup(padico_nmad.connections_by_node, node);
  if(!conn)
    {
      padico_out(30, "creating connection\n");
      conn = padico_malloc(sizeof(struct padico_nmad_connection_s));
      conn->state       = PSP_NMAD_CONN_NONE;
      conn->remote_node = node;
      conn->sbuf        = NULL;
      conn->header_sent = 0;
      marcel_mutex_init(&conn->mutex, NULL);
      puk_hashtable_insert(padico_nmad.connections_by_node, node, conn);

      conn->state = PSP_NMAD_CONN_INPROGRESS;
      marcel_mutex_unlock(&padico_nmad.lock);
      padico_out(30, "connection in progress \n");
      padico_addrdb_publish(node, padico_module_self_name(),
			    "psp-nmad", 8, 
			    padico_nmad.local_url, strlen(padico_nmad.local_url)+1);
      padico_req_t req = padico_tm_req_new(NULL, NULL);
      padico_addrdb_get(node, padico_module_self_name(),
			"psp-nmad" , 8, &conn->url[0], PSP_NMAD_URL_SIZE, req);
      padico_tm_req_wait(req);
      err = nm_session_connect(padico_nmad.p_session, &conn->gate, &conn->url[0]);
      if (err != NM_ESUCCESS)
	{
	  padico_fatal("NewMad: nm_session_connect returned err = %d\n", err);
	}
      padico_out(30, "connection established \n");
      marcel_mutex_lock(&padico_nmad.lock);
      conn->state = PSP_NMAD_CONN_ESTABLISHED;
      puk_hashtable_insert(padico_nmad.connections_by_gate, conn->gate, conn);
    }
  if(conn->state == PSP_NMAD_CONN_ESTABLISHED && !padico_nmad.polling_thread_launched)
    {
      padico_rttask_t t = padico_rttask_new();
      t->kind = PADICO_RTTASK_RTTHREAD;
      t->background_fun = &psp_nmad_worker;
      t->background_arg = NULL;
      padico_tm_rttask_submit(t);
      padico_nmad.polling_thread_launched = 1;
    }
  marcel_mutex_unlock(&padico_nmad.lock);
}

/* ********************************************************* */
/* *** Exchange Url Callback */

static void*psp_nmad_connection_worker(void*_node)
{
  padico_na_node_t node = _node;
  psp_nmad_do_connect(node);
  return NULL;
}

static void psp_nmad_connect_handler(puk_parse_entity_t e)
{
  const char*uuid = puk_parse_getattr(e, "uuid");
  assert(uuid);
  padico_na_node_t node = padico_na_getnodebyuuid((padico_topo_uuid_t)uuid);
  padico_tm_invoke_later(&psp_nmad_connection_worker, node);
}

static struct puk_tag_action_s psp_nmad_action_connect =
{
  .xml_tag        = "PSP-NMad:Connect",
  .start_handler  = &psp_nmad_connect_handler,
  .end_handler    = NULL,
  .required_level = PUK_TRUST_CONTROL
};


/* ********************************************************* */
/* *** Module initialisation function */

extern int padico_psp_nmad_init(void)
{
  marcel_mutex_init(&padico_nmad.lock, NULL);
  puk_component_declare("PSP-NMad",
			puk_component_provides("PadicoComponent", "component", &psp_nmad_component_driver),
			puk_component_provides("PadicoSimplePackets", "psp", &psp_nmad_driver));
  
  /* Add action to exchange urls */
  puk_xml_add_action(psp_nmad_action_connect);
  
  /* Initialize static structure */
  padico_nmad.p_session = NULL;
  padico_nmad.assembly = NULL;
  padico_psp_directory_init(&padico_nmad.slots);
  padico_nmad.connections_by_node = puk_hashtable_new_ptr();
  padico_nmad.connections_by_gate = puk_hashtable_new_ptr();
  padico_nmad.polling_thread_launched = 0;
  
  /* session init */
  int err = nm_session_create(&padico_nmad.p_session, "psp-nmad");
  if (err != NM_ESUCCESS)
    {
      padico_warning("NewMad: nm_session_create returned err = %d\n", err);
      return -1;
    }
  int argc = 1;
  char* argv[] = { "psp-nmad", NULL };
  const char*local_url = NULL;
  nm_session_init(padico_nmad.p_session, &argc, argv, &local_url);
  padico_nmad.local_url = padico_strdup(local_url);
	  
  return 0;
}

/* ********************************************************* */
/* *** Driver Functions */

static void psp_nmad_init(void*_instance, 
			  uint32_t tag, 
			  const char*label, 
			  size_t*h_size,
			  padico_psp_handler_t handler, 
			  void*key)
{
  struct psp_nmad_instance_s*instance = (struct psp_nmad_instance_s*)_instance;
  /* Now we know what is the size of the buffer, we can allocate (and 1 uint32_t is kept for us) */
  *h_size += 16;
  instance->h_size = *h_size;
  instance->tag = tag;
  instance->slot = padico_psp_slot_insert(&padico_nmad.slots, handler, key, tag, instance->h_size);
}

static void psp_nmad_connect(void*_instance, padico_na_node_t node)
{ 
  marcel_mutex_lock(&padico_nmad.lock);
  struct padico_nmad_connection_s*conn = puk_hashtable_lookup(padico_nmad.connections_by_node, node);
  if(!conn)
    {
      padico_topo_uuid_t uuid = padico_na_node_getuuid(padico_na_getlocalnode());
      padico_string_t s_req = padico_string_new();
      padico_string_catf(s_req, "<PSP-NMad:Connect uuid=\"%s\"/>\n", (char*)uuid);
      padico_control_send_oneway(node, padico_string_get(s_req));
      padico_string_delete(s_req);
    }
  marcel_mutex_unlock(&padico_nmad.lock);

  psp_nmad_do_connect(node);
}

static padico_psp_connection_t psp_nmad_new_message(void*_instance, 
							      padico_na_node_t node, 
							      void**_sbuf)
{
  struct psp_nmad_instance_s*const instance = (struct psp_nmad_instance_s*)_instance;
  struct padico_nmad_connection_s*conn = puk_hashtable_lookup(padico_nmad.connections_by_node, node);
  marcel_mutex_lock(&conn->mutex);
  /* New pack: no header sent, and we have to prepare the header buffer */
  if(!conn->sbuf)
    conn->sbuf = padico_malloc(instance->h_size);
  *_sbuf = conn->sbuf;
  conn->header_sent = 0;
  /* Send Tag */
  nm_sr_isend(padico_nmad.p_session, conn->gate, 0, 
	      &instance->tag, sizeof(uint32_t), &conn->request);
  return (padico_psp_connection_t)conn;
}

static void psp_nmad_pack(void*_instance, 
			  padico_psp_connection_t _conn,
			  const char*bytes, 
			  size_t size)
{
  struct psp_nmad_instance_s*const instance = (struct psp_nmad_instance_s*)_instance;
  struct padico_nmad_connection_s*conn = (struct padico_nmad_connection_s*)_conn;
  nm_sr_request_t r1, r2;
  if(!conn->header_sent)
    {
      /* Header not sent: Do it, then send the message */
      nm_sr_isend(padico_nmad.p_session, conn->gate, 0, conn->sbuf, instance->h_size, &r1);
      conn->header_sent = 1;
      nm_sr_isend(padico_nmad.p_session, conn->gate, 0, bytes, size, &r2);
      nm_sr_swait(padico_nmad.p_session, &conn->request);
      nm_sr_swait(padico_nmad.p_session, &r1);
      nm_sr_swait(padico_nmad.p_session, &r2);
    }
  else
    {
      /* Header already sent: just send the message */
      nm_sr_isend(padico_nmad.p_session, conn->gate, 0, bytes, size, &r1);
      nm_sr_swait(padico_nmad.p_session, &r1);
    }
}

static void psp_nmad_end_message(void*_instance, 
				 padico_psp_connection_t _conn)
{
  struct psp_nmad_instance_s*const instance = (struct psp_nmad_instance_s*)_instance;
  struct padico_nmad_connection_s*conn = (struct padico_nmad_connection_s*)_conn;
  if(!conn->header_sent)
    {
      nm_sr_request_t request;
      nm_sr_isend(padico_nmad.p_session, conn->gate, 0, conn->sbuf, instance->h_size, &request);
      nm_sr_swait(padico_nmad.p_session, &conn->request);
      nm_sr_swait(padico_nmad.p_session, &request);
      conn->header_sent = 1;
    }
  marcel_mutex_unlock(&conn->mutex);
}

