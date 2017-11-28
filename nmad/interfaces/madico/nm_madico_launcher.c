/** @file launcher for NewMad using PadicoTM topology and control channel.
 */

#include <Padico/Module.h>
#include <Padico/Puk.h>
#include <Padico/Topology.h>
#include <Padico/NetSelector.h>
#include <Padico/PadicoControl.h>
#include <Padico/AddrDB.h>

#include <nm_public.h>
#include <nm_private.h>
#include <nm_session_interface.h>
#include <nm_session_private.h>
#include <nm_launcher.h>

#define NM_LAUNCHER_URL_SIZE 512

/** An instance of NewMad_Launcher_madico
 */
struct newmad_launcher_status_s
{
  nm_session_t p_session; /**< the nmad session */
  padico_group_t group;   /**< group of nodes in the session */
  const char*local_url;   /**< nmad url for local node */
  struct newmad_launcher_gate_s
  {
    nm_gate_t p_gate;
    char url[NM_LAUNCHER_URL_SIZE];
    struct nm_drv_vect_s drvs;
  }*gates;                /**< remote nodes, with the url and gate, in the same order as in 'group'. */
};

static struct newmad_launcher_status_s*nm_launcher_status = NULL;

/* ********************************************************* */

static void*newmad_launcher_instantiate(puk_instance_t, puk_context_t);
static void newmad_launcher_destroy(void*_status);

const static struct puk_component_driver_s newmad_launcher_component =
  {
    .instantiate = &newmad_launcher_instantiate,
    .destroy     = &newmad_launcher_destroy
  };

/* ********************************************************* */

static void         newmad_launcher_init(void*_status, int*argc, char**argv, const char*group_name);
static int          newmad_launcher_get_size(void*_status);
static int          newmad_launcher_get_rank(void*_status);
static void         newmad_launcher_get_gates(void*_status, nm_gate_t*gates);
static void         newmad_launcher_abort(void*_status);

const static struct newmad_launcher_driver_s newmad_launcher_driver =
  {
    .init        = &newmad_launcher_init,
    .get_size    = &newmad_launcher_get_size,
    .get_rank    = &newmad_launcher_get_rank,
    .get_gates   = &newmad_launcher_get_gates,
    .abort       = &newmad_launcher_abort
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(NewMad_Launcher_madico,
  puk_component_declare("NewMad_Launcher_madico",
			puk_component_provides("PadicoComponent", "component",  &newmad_launcher_component),
			puk_component_provides("NewMad_Launcher", "launcher", &newmad_launcher_driver ))
			);

/* ********************************************************* */

static void*newmad_launcher_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct newmad_launcher_status_s*status = padico_malloc(sizeof(struct newmad_launcher_status_s));
  status->group = NULL;
  status->gates = NULL;
  status->p_session = NULL;
  assert(nm_launcher_status == NULL);
  nm_launcher_status = status;
  return status;
}

static void newmad_launcher_destroy(void*_status)
{
  struct newmad_launcher_status_s*status = _status;
  if(status->p_session)
    {
      nm_session_destroy(status->p_session);
      status->p_session = NULL;
    }
  if(status->gates)
    {
      padico_free(status->gates);
      status->gates = NULL;
    }
  padico_free(_status);

  struct padico_control_event_s*event = padico_malloc(sizeof(struct padico_control_event_s));
  event->kind = PADICO_CONTROL_EVENT_SHUTDOWN;
  padico_tasklet_schedule(padico_control_get_event_channel(), event);
  
}

/* ********************************************************* */

static nm_drv_vect_t newmad_launcher_selector(const char*url, void*_arg)
{
  struct newmad_launcher_status_s*status = _arg;
  const int size = padico_group_size(status->group);
  int i = 0;
  while(i < size)
    {
      if(strcmp(status->gates[i].url, url) == 0)
	{
	  nm_drv_vect_t p_drvs = nm_drv_vect_copy(&status->gates[i].drvs);
	  return p_drvs;
	}
      i++;
    }
  return NULL;
}

static void newmad_launcher_init(void*_status, int*argc, char**argv, const char*group_name)
{
  struct newmad_launcher_status_s*status = _status;
  status->group = padico_group_lookup(group_name);
  if(!status->group)
    {
      const char*session = padico_na_node_getsession(padico_na_getlocalnode());
      padico_group_t session_group = padico_group_lookup(session);
      const char*distrib = getenv("NM_LAUNCHER_DISTRIB");
      if(distrib == NULL)
	{
	  status->group = session_group;
	}
      else
	{
	  status->group = padico_group_topology(session_group, distrib);
	}
      if(!status->group)
	{
	  padico_fatal("NewMad_Launcher: group %s does not exist. Mad-MPI binaries are supposed to be launched with 'mpirun'.\n", group_name);
	}
    }
  const int size = padico_group_size(status->group);
  const int rank = padico_group_rank(status->group, padico_na_getlocalnode());
  status->gates = padico_malloc(size * sizeof(struct newmad_launcher_gate_s));
  status->gates[rank].p_gate = NM_GATE_NONE;
  /* init session */
  int rc = nm_session_create(&status->p_session, group_name);
  if(rc != NM_ESUCCESS)
    {
      padico_fatal("NewMad_Launcher: nm_session_create()- rc = %d\n", rc);
    }
  const char*nmad_driver = padico_getenv("NMAD_DRIVER");
  if(!nmad_driver)
    {
      puk_hashtable_t loaded_components = puk_hashtable_new_ptr();
      int i;
      for(i = 0; i < size; i++)
	{
	  nm_drv_vect_init(&status->gates[i].drvs);
	  const puk_component_t component_1 =
	    padico_ns_serial_selector(padico_group_node(status->group, i), "trk_small", puk_iface_NewMad_minidriver());
	  const puk_component_t component_2 =
	    padico_ns_serial_selector(padico_group_node(status->group, i), "trk_large", puk_iface_NewMad_minidriver());
	  assert(component_1 != NULL);
	  assert(component_2 != NULL);
	  nm_drv_t p_drv1 = puk_hashtable_lookup(loaded_components, component_1);
	  if(p_drv1 == NULL)
	    {
	      p_drv1 = nm_session_add_driver(component_1);
	      puk_hashtable_insert(loaded_components, component_1, p_drv1);
	    }
	  nm_drv_vect_push_back(&status->gates[i].drvs, p_drv1);
	  nm_drv_t p_drv2 = puk_hashtable_lookup(loaded_components, component_2);
	  if(p_drv2 == NULL)
	    {
	      p_drv2 = nm_session_add_driver(component_2);
	      puk_hashtable_insert(loaded_components, component_2, p_drv2);
	    }
	  nm_drv_vect_push_back(&status->gates[i].drvs, p_drv2);
	}
      nm_session_set_selector(&newmad_launcher_selector, status);
      puk_hashtable_delete(loaded_components);
    }
  rc = nm_session_init(status->p_session, argc, argv, &status->local_url);
  if(rc != NM_ESUCCESS)
    {
      padico_fatal("NewMad_Launcher: nm_session_init()- rc = %d\n", rc);
    }

  padico_print("NewMad_Launcher: nm_session_init done- url = %s; size = %d; rank = %d\n",
	       status->local_url, size, rank);
  if(strlen(status->local_url) >= NM_LAUNCHER_URL_SIZE)
    {
      padico_fatal("NewMad_Launcher: url is too large.\n");
    }

  /* ** connect gates */
  const char*addr_component = padico_module_self_name();
  const char*addr_instance = group_name;
  const int addr_instance_size = strlen(group_name) + 1;
  int i;
  for(i = 0; i < size; i++)
    {
      padico_na_node_t peer = padico_group_node(status->group, i);
      if(peer != padico_na_getlocalnode())
	{
	  padico_out(50, "sending url to node #%d (%p)\n", i, peer);
	  padico_addrdb_publish(peer, addr_component,
				addr_instance, addr_instance_size,
				status->local_url, strlen(status->local_url)+1);
	  padico_req_t req = padico_tm_req_new(NULL, NULL);
	  padico_addrdb_get(peer, addr_component,
			    addr_instance, addr_instance_size,
			    &status->gates[i].url[0], NM_LAUNCHER_URL_SIZE, req);
	  padico_out(50, "waiting for url from node #%d (%p)...\n", i, peer);
	  padico_tm_req_wait(req);
	  padico_out(50, "received url from node #%d- connecting...\n", i);
	}
      else
	{
	  strncpy(&status->gates[i].url[0], status->local_url, NM_LAUNCHER_URL_SIZE);
	  status->gates[i].url[NM_LAUNCHER_URL_SIZE - 1] = '\0';
	}
      padico_print("NewMad_Launcher: connecting- #%d; url = %s\n", i, status->gates[i].url);
      rc = nm_session_connect(status->p_session, &status->gates[i].p_gate, status->gates[i].url);
      if(rc != NM_ESUCCESS)
	{
	  padico_warning("NewMad_Launcher: nm_core_gate_connect()- rc = %d\n", rc);
	}
      padico_out(50, "connection to node #%d done.\n", i);
    }
  padico_print("NewMad_Launcher: connected.\n");
}

static int newmad_launcher_get_size(void*_status)
{
  struct newmad_launcher_status_s*status = _status;
  return padico_group_size(status->group);
}

static int newmad_launcher_get_rank(void*_status)
{
  struct newmad_launcher_status_s*status = _status;
  return padico_group_rank(status->group, padico_na_getlocalnode());
}

static void newmad_launcher_get_gates(void*_status, nm_gate_t*gates)
{
  struct newmad_launcher_status_s*status = _status;
  int i;
  for(i = 0; i < padico_group_size(status->group); i++)
    {
      gates[i] = status->gates[i].p_gate;
      padico_out(50, "get_gates()- i = %d; gate = %p\n", i, gates[i]);
    }  
}

static void newmad_launcher_abort(void*_status)
{
  struct newmad_launcher_status_s*status = _status;
  padico_group_kill(status->group, -1);
  abort();
}

/* ********************************************************* */
/* ** madico launcher extension */

nm_gate_t newmadico_launcher_get_gate(int rank)
{
  struct newmad_launcher_status_s*status = nm_launcher_status;
  const int size = padico_group_size(status->group);
  if(rank < size)
    return status->gates[rank].p_gate;
  else
    return NULL;
}

padico_na_node_t newmadico_launcher_get_node(int rank)
{
  struct newmad_launcher_status_s*status = nm_launcher_status;
  const int size = padico_group_size(status->group);
  if(rank < size)
    return padico_group_node(status->group, rank);
  else
    return NULL;
}


