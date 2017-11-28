/** @file
 * @brief PadicoControl over minidriver
 * @note usable only as secondary controler, cannot bootstrap
 */

#include <Padico/Puk.h>
#include <Padico/Module.h>
#include <Padico/PadicoTM.h>
#include <Padico/PadicoControl.h>
#include <Padico/Topology.h>

PADICO_MODULE_DECLARE(Control_minidriver);
PADICO_MODULE_INIT(control_minidriver_init);

/* ********************************************************* */

#if defined(NMAD) && defined(PIOMAN)

#include <nm_minidriver.h>
#include <pioman.h>


/* *** PadicoComponent driver ******************************** */

static void*control_minidriver_instantiate(puk_instance_t instance, puk_context_t context);
static void control_minidriver_destroy(void*_status);

/** instanciation facet for Control_minidriver
 */
const static struct puk_component_driver_s control_minidriver_component_driver =
{
  .instantiate = &control_minidriver_instantiate,
  .destroy     = &control_minidriver_destroy
};

/* *** PadicoControl driver ******************************** */

static void control_minidriver_send_common(void*_status, padico_na_node_t node, padico_control_msg_t msg);

/** 'Control' facet for Control/PSP
 */
const static struct padico_na_control_driver_s control_minidriver_control_driver =
{
  .send_common = &control_minidriver_send_common
};

/* *** PadicoConnector driver ****************************** */

static int control_minidriver_connector_connect(void*_status, padico_req_t req, padico_na_node_t node);

/** 'Connector' facet for Control/PSP
 */
const static struct padico_control_connector_driver_s control_minidriver_connector_driver =
{
  .connect = &control_minidriver_connector_connect
};

/* ********************************************************* */

struct control_minidriver_status_s
{
  struct puk_receptacle_NewMad_minidriver_s minidriver;
};

static struct
{
  struct piom_ltask ltask;
} control_minidriver;

/* ********************************************************* */

static void*control_minidriver_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct control_minidriver_status_s*status = padico_malloc(sizeof(struct control_minidriver_status_s));
  puk_context_indirect_NewMad_minidriver(instance, "minidriver", &status->minidriver);
  return status;
}

static void control_minidriver_destroy(void*_status)
{
  padico_free(_status);
}

/* ********************************************************* */

static void control_minidriver_send_common(void*_status, padico_na_node_t node, padico_control_msg_t msg)
{
  struct control_minidriver_status_s*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*r = &status->minidriver;
  const struct iovec*v = padico_control_msg_get_iovec(msg);
  const int n = padico_control_msg_get_iovec_size(msg);
  (*r->driver->send_post)(r->_status, v, n);
  int err = 0;
  do
    {
      err = (*r->driver->send_poll)(r->_status);
    }
  while(err == -NM_EAGAIN);
  if(err != NM_ESUCCESS)
    {
      padico_warning("Contorl_minidriver: error %d while sending data.\n", err);
    }
}

static int control_minidriver_task_poll(void*_status)
{
  return 0;
}

static void*control_minidriver_dispatcher(void*dummy)
{
  struct control_minidriver_status_s*_status = NULL;
  piom_ltask_create(&control_minidriver.ltask, &control_minidriver_task_poll, &_status, PIOM_LTASK_OPTION_REPEAT);
  for(;;)
    {
      piom_ltask_submit(&control_minidriver.ltask);
      piom_ltask_wait(&control_minidriver.ltask);
      struct control_minidriver_status_s*status = (void*)_status;
      if(status)
	{
	}
    }
  return NULL;
}

/* ********************************************************* */

static int control_minidriver_connector_connect(void*_status, padico_req_t req, padico_na_node_t node)
{
  return -1;
}

#endif /* NMAD && PIOMAN */

/* ********************************************************* */

static int control_minidriver_init(void)
{
#if defined(NMAD) && defined(PIOMAN)
  puk_component_declare("Control-minidriver",
			puk_component_provides("PadicoComponent", "component", &control_minidriver_component_driver),
			puk_component_provides("PadicoControl", "control", &control_minidriver_control_driver),
			puk_component_provides("PadicoConnector", "connector", &control_minidriver_connector_driver),
			puk_component_uses("NewMad_minidriver", "minidriver"));
  padico_tm_invoke_later(&control_minidriver_dispatcher, NULL);
#else /* NMAD && PIOMAN */

  padico_warning("Control-minidriver: not initialized- NewMad not included in core.");
  return -1;

#endif /* NMAD && PIOMAN */
  return 0;
}


