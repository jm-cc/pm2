/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
#include <nm_private.h>
#if PIOMAN

struct nm_ltask_policy_s
{
  enum
    {
      NM_POLICY_NONE = 0, /**< not initialized */
      NM_POLICY_APP,      /**< near the application (current location) */
      NM_POLICY_DEV,      /**< near the network device */
      NM_POLICY_ANY       /**< anywhere, no policy */
    } location;
};

static struct nm_ltask_policy_s ltask_policy =
  {
    .location = NM_POLICY_NONE
  };

/** retrieve the binding policy */
void nm_ltask_set_policy(void)
{
  const char*policy = getenv("PIOM_BINDING_POLICY");
  if(!policy)
    {
      const char*padico_host_count = getenv("PADICO_HOST_COUNT");
      int mcount = padico_host_count ? atoi(padico_host_count) : 1;
      policy = (mcount > 1) ? "app" : "dev";
      NM_DISPF("# nmad: use default pioman binding policy for %d process/node.\n", mcount);
    }

  NM_DISPF("# nmad: set pioman binding policy = %s\n", policy);
  if(strcmp(policy, "app") == 0)
    {
      ltask_policy.location = NM_POLICY_APP;
    }
  else if(strcmp(policy, "dev") == 0)
    {
      ltask_policy.location = NM_POLICY_DEV;
    }
  else if(strcmp(policy, "any") == 0)
    {
      ltask_policy.location = NM_POLICY_ANY;
    }
  else
    {
      padico_fatal("nmad: unknown pioman binding policy %s.\n", policy);
    }
}

static piom_topo_obj_t nm_piom_driver_binding(nm_drv_t p_drv)
{
  piom_topo_obj_t binding = NULL;
#if defined(PIOMAN_TOPOLOGY_HWLOC)
  hwloc_cpuset_t cpuset = p_drv->props.profile.cpuset;
  if(cpuset != NULL)
    {
      char s_binding[64];
      hwloc_obj_t o = hwloc_get_obj_covering_cpuset(piom_topo_get(), cpuset);
      assert(o != NULL);
      binding = o;
      hwloc_obj_snprintf(s_binding, sizeof(s_binding), piom_topo_get(), binding, "#", 0);
      NM_DISPF("# nmad: network %s binding: %s.\n", p_drv->assembly->name, s_binding);
    }
  if(binding == NULL)
    {
      binding = NULL;
    }
#else /* PIOMAN_TOPOLOGY_HWLOC */
  binding = piom_topo_full;
#endif /* PIOMAN_TOPOLOGY_HWLOC */
  return binding;
}

static piom_topo_obj_t nm_get_binding_policy(nm_drv_t p_drv)
{
  if(p_drv->ltask_binding == NULL)
    {
      p_drv->ltask_binding = NULL;
      switch(ltask_policy.location)
	{
	case NM_POLICY_APP:
	  p_drv->ltask_binding = piom_topo_current_obj();
	  break;
	  
	case NM_POLICY_DEV:
	  p_drv->ltask_binding = nm_piom_driver_binding(p_drv);
	  break;
	  
	case NM_POLICY_ANY:
	  p_drv->ltask_binding = NULL;
	  break;
	  
	default:
	  padico_fatal("nmad: pioman binding policy not defined.\n");
	  break;
	}
    }
  return p_drv->ltask_binding;
}


/* ********************************************************* */
/* ** tasks */

static int nm_ltask_pw_recv(void*_pw)
{
  struct nm_pkt_wrap_s*p_pw = _pw;
  nm_pw_recv_progress(p_pw);
  return NM_ESUCCESS;
}

static int nm_task_block_recv(void*_pw)
{
  struct nm_pkt_wrap_s*p_pw = _pw;
  /*
  struct nm_core*p_core = p_pw->p_gate->p_core;
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = r->driver->wait_recv_iov(r->_status, p_pw);
  if(err == NM_ESUCCESS)
    {
      nm_core_lock(p_core);
      piom_ltask_completed(&p_pw->ltask);
      nm_pw_process_complete_recv(p_core, p_pw);
      nm_core_unlock(p_core);
    }
  else if((p_pw->ltask.state & PIOM_LTASK_STATE_CANCELLED) || (err == -NM_ECLOSED))
    {
      piom_ltask_completed(&p_pw->ltask);
      return NM_ESUCCESS;
    }
  else if(err == -NM_EAGAIN)
    {
      p_pw->flags |= NM_PW_POSTED;
      nm_ltask_submit_pw_recv(p_pw);
    }
  else
    {
      NM_WARN("wait_recv returned err = %d\n", err);
    }
  */
  NM_FATAL("blocking recv not supported with minidriver- p_pw = %p.\n", p_pw);
  return NM_ESUCCESS;
}

static int nm_ltask_pw_send(void*_pw)
{
  struct nm_pkt_wrap_s*p_pw = _pw;
  if(p_pw->flags & NM_PW_POSTED)
    nm_pw_poll_send(p_pw);
  else
    nm_pw_post_send(p_pw);
  return NM_ESUCCESS;
}

static int nm_task_block_send(void*_pw)
{
  struct nm_pkt_wrap_s*p_pw = _pw;
  /*
  struct nm_core*p_core = p_pw->p_gate->p_core;
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err;
  do
    {
      err = r->driver->wait_send_iov(r->_status, p_pw);
    }
  while(err == -NM_EAGAIN);
  nm_core_lock(p_core);
  if(err == NM_ESUCCESS)
    {
      piom_ltask_completed(&p_pw->ltask);
      nm_pw_process_complete_send(p_core, p_pw);
    }
  else
    {
      nm_ltask_submit_pw_send(p_pw);
      NM_WARN("wait_send returned %d", err);
    }
  nm_core_unlock(p_core);
  */
  NM_FATAL("blocking send not supported with minidriver- p_pw = %p\n", p_pw);
  return NM_ESUCCESS;
}

static int nm_ltask_core_progress(void*_p_core)
{
  struct nm_core*p_core = _p_core;
  int ret = NM_ESUCCESS;
  if(nm_core_trylock(p_core))
    {
      nm_core_progress(p_core);
      nm_core_unlock(p_core);
    }
  nm_core_events_dispatch(p_core);
  return ret;
}

static int nm_task_offload(void*_pw)
{
  struct nm_pkt_wrap_s*p_pw = _pw;
  struct nm_core*p_core = p_pw->p_gate->p_core;
  nm_core_lock(p_core);
  nm_pw_post_send(p_pw);
  nm_core_unlock(p_core);
  return 0;
}

static void nm_ltask_destructor(struct piom_ltask*p_ltask)
{
  struct nm_pkt_wrap_s*p_pw = tbx_container_of(p_ltask, struct nm_pkt_wrap_s, ltask);
  nm_pw_ref_dec(p_pw);
}

void nm_ltask_submit_pw_recv(struct nm_pkt_wrap_s*p_pw)
{
  piom_topo_obj_t ltask_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(&p_pw->ltask, &nm_ltask_pw_recv,  p_pw,
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT);
  piom_ltask_set_binding(&p_pw->ltask, ltask_binding);
  piom_ltask_set_name(&p_pw->ltask, "nmad: poll_recv");
  piom_ltask_set_destructor(&p_pw->ltask, &nm_ltask_destructor);
  nm_pw_ref_inc(p_pw);
  if(p_pw->p_drv->props.capabilities.is_exportable)
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_recv, 2 * p_pw->p_drv->props.profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_ltask_submit_pw_send(struct nm_pkt_wrap_s*p_pw)
{
  piom_topo_obj_t ltask_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(&p_pw->ltask, &nm_ltask_pw_send, p_pw, 
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT);
  piom_ltask_set_binding(&p_pw->ltask, ltask_binding);
  piom_ltask_set_name(&p_pw->ltask, "nmad: pw_send");
  piom_ltask_set_destructor(&p_pw->ltask, &nm_ltask_destructor);
  nm_pw_ref_inc(p_pw);
  assert(p_pw->p_trk);
  if(p_pw->p_drv->props.capabilities.is_exportable)
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_send, 2 * p_pw->p_drv->props.profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);
}

void nm_ltask_submit_core_progress(struct nm_core*p_core)
{
  piom_topo_obj_t ltask_binding = NULL;
  piom_ltask_create(&p_core->ltask, &nm_ltask_core_progress, p_core,
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT);
  piom_ltask_set_binding(&p_core->ltask, ltask_binding);
  piom_ltask_set_name(&p_core->ltask, "nmad: core_progress");
  piom_ltask_submit(&p_core->ltask);
}

void nm_ltask_submit_offload(struct piom_ltask*p_ltask, struct nm_pkt_wrap_s *p_pw)
{
  piom_topo_obj_t ltask_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(p_ltask, &nm_task_offload, p_pw,
		    PIOM_LTASK_OPTION_ONESHOT | PIOM_LTASK_OPTION_NOWAIT);
  piom_ltask_set_binding(p_ltask, ltask_binding);
  piom_ltask_set_name(p_ltask, "nmad: offload");
  piom_ltask_set_destructor(&p_pw->ltask, &nm_ltask_destructor);
  nm_pw_ref_inc(p_pw);
  piom_ltask_submit(p_ltask);
}

#else /* PIOMAN */

#endif /* PIOMAN */
