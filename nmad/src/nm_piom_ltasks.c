/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * NewMadeleine
 * Copyright (C) 2006-2015 (see AUTHORS file)
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
#if PIOMAN_POLL

struct nm_ltask_policy_s
{
  enum
    {
      NM_POLICY_NONE,  /**< not initialized */
      NM_POLICY_APP,   /**< near the application (current location) */
      NM_POLICY_DEV,   /**< near the network device */
      NM_POLICY_ANY,   /**< anywhere, no policy */
      NM_POLICY_CUSTOM /**< custom location, given by user */
    } location;
  enum piom_topo_level_e level;
  int custom;
};

static struct nm_ltask_policy_s ltask_policy =
  {
    .location = NM_POLICY_NONE,
    .level    = PIOM_TOPO_NONE,
    .custom   = -1
  };

/** retrieve the binding policy */
void nm_ltask_set_policy(void)
{
  const char*policy = getenv("PIOM_BINDING_POLICY");
  const char*level = getenv("PIOM_BINDING_LEVEL");
  if(!policy)
    {
      NM_DISPF("# nmad: default pioman binding policy.\n");
      ltask_policy.location = NM_POLICY_DEV;
    }
  else
    {
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
      else if(strcmp(policy, "custom") == 0)
	{
	  ltask_policy.location = NM_POLICY_CUSTOM;
	  padico_fatal("nmad: pioman custom policy not supported yet.\n");
	}
      else
	{
	  padico_fatal("nmad: unknown pioman binding policy %s.\n", policy);
	}
    }
  if(!level)
    {
      ltask_policy.level = PIOM_TOPO_SOCKET;
    }
  else
    {
      NM_DISPF("# nmad: set pioman custom binding level = %s\n", level);
      if(strcmp(level, "machine") == 0)
	{
	  ltask_policy.level = PIOM_TOPO_MACHINE;
	}
      else if(strcmp(level, "node") == 0)
	{
	  ltask_policy.level = PIOM_TOPO_NODE;
	}
      else if(strcmp(level, "socket") == 0)
	{
	  ltask_policy.level = PIOM_TOPO_SOCKET;
	}
      else if(strcmp(level, "core") == 0)
	{
	  ltask_policy.level = PIOM_TOPO_CORE;
	}
      else if(strcmp(level, "pu") == 0)
	{
	  ltask_policy.level = PIOM_TOPO_PU;
	}
      else
	{
	  padico_fatal("nmad: unknown pioman binding level %s.\n", level);
	}
    }
  return ;
}

static piom_topo_obj_t nm_piom_driver_binding(struct nm_drv*p_drv)
{
  piom_topo_obj_t binding = piom_topo_full;
#if defined(PM2_TOPOLOGY) && defined(PIOMAN_TOPOLOGY_HWLOC)
      {
	hwloc_cpuset_t cpuset = p_drv->profile.cpuset;
	if(cpuset == NULL)
	  {
	    cpuset = hwloc_topology_get_complete_cpuset(piom_ltask_topology());
	  }
	hwloc_obj_t o = hwloc_get_obj_covering_cpuset(piom_ltask_topology(), cpuset);
	assert(o != NULL);
	binding = piom_get_parent_obj(o, ltask_policy.level);
	if(binding == NULL)
	  binding = o;
      }
#endif /* PM2_TOPOLOGY */
      return binding;
}

static piom_topo_obj_t nm_get_binding_policy(struct nm_drv*p_drv)
{
  if(p_drv->ltask_binding == NULL)
    {
      p_drv->ltask_binding = piom_topo_full;
      switch(ltask_policy.location)
	{
	case NM_POLICY_APP:
	  p_drv->ltask_binding = piom_get_parent_obj(piom_ltask_current_obj(), ltask_policy.level);
	  break;
	  
	case NM_POLICY_DEV:
	  p_drv->ltask_binding = nm_piom_driver_binding(p_drv);
	  break;
	  
	case NM_POLICY_ANY:
	  p_drv->ltask_binding = piom_topo_full;
	  break;
	  
	case NM_POLICY_CUSTOM:
	  /* TODO */
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

static int nm_task_poll_recv(void*_pw)
{
  struct nm_pkt_wrap*p_pw = _pw;
  int ret = -NM_EUNKNOWN;
  /* todo: lock something when using fine-grain locks */
  if(nmad_trylock())
    {
      ret = nm_pw_poll_recv(p_pw);
      nmad_unlock();
    }
  return ret;
}

static int nm_task_block_recv(void*_pw)
{
  struct nm_pkt_wrap*p_pw = _pw;
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = r->driver->wait_recv_iov(r->_status, p_pw);
  if(err == NM_ESUCCESS)
    {
      nmad_lock();
      piom_ltask_completed(&p_pw->ltask);
      nm_so_process_complete_recv(p_pw->p_gate->p_core, p_pw);
      nmad_unlock();
    }
  else if((p_pw->ltask.state & PIOM_LTASK_STATE_CANCELLED) || (err == -NM_ECLOSED))
    {
      piom_ltask_completed(&p_pw->ltask);
      return NM_ESUCCESS;
    }
  else if(err == -NM_EAGAIN)
    {
      nm_ltask_submit_poll_recv(p_pw);
    }
  else
    {
      NM_WARN("wait_recv returned err = %d\n", err);
    }
  return NM_ESUCCESS;
}

static int nm_task_poll_send(void*_pw)
{
  struct nm_pkt_wrap*p_pw = _pw;
  if(nmad_trylock())
    {
      nm_pw_poll_send(p_pw);
      nmad_unlock();
    }
  return NM_ESUCCESS;
}

static int nm_task_block_send(void*_pw)
{
  struct nm_pkt_wrap*p_pw = _pw;
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err;
  do
    {
      err = r->driver->wait_send_iov(r->_status, p_pw);
    }
  while(err == -NM_EAGAIN);
  nmad_lock();
  if(err == NM_ESUCCESS)
    {
      piom_ltask_completed(&p_pw->ltask);
      nm_so_process_complete_send(p_pw->p_gate->p_core, p_pw);
    }
  else
    {
      nm_ltask_submit_poll_send(p_pw);
      NM_WARN("wait_send returned %d", err);
    }
  nmad_unlock();
  return NM_ESUCCESS;
}

static int nm_task_post_on_drv(void*_drv)
{
  struct nm_drv*p_drv = _drv;
  int ret = NM_ESUCCESS;
  nmad_lock();
  nm_strat_apply(p_drv->p_core);
  nm_drv_post_all(p_drv);
  nmad_unlock();
  return ret;
}

static int nm_task_offload(void* args)
{
  struct nm_pkt_wrap * p_pw=args;
  nmad_lock();
  nm_pw_post_send(p_pw);
  nmad_unlock();
  return 0;
}

static void nm_ltask_destructor(struct piom_ltask*p_ltask)
{
  struct nm_pkt_wrap*p_pw = tbx_container_of(p_ltask, struct nm_pkt_wrap, ltask);
  nm_pw_ref_dec(p_pw);
}

void nm_ltask_submit_poll_recv(struct nm_pkt_wrap *p_pw)
{
  piom_topo_obj_t ltask_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(&p_pw->ltask, &nm_task_poll_recv,  p_pw,
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT);
  piom_ltask_set_binding(&p_pw->ltask, ltask_binding);
  piom_ltask_set_name(&p_pw->ltask, "nmad: poll_recv");
  piom_ltask_set_destructor(&p_pw->ltask, &nm_ltask_destructor);
  nm_pw_ref_inc(p_pw);
  const struct nm_drv_iface_s*driver = p_pw->p_drv->driver;
  if(driver->capabilities.is_exportable && (driver->wait_recv_iov != NULL))
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_recv, 2 * p_pw->p_drv->profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_ltask_submit_poll_send(struct nm_pkt_wrap *p_pw)
{
  piom_topo_obj_t ltask_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(&p_pw->ltask, &nm_task_poll_send, p_pw, 
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT);
  piom_ltask_set_binding(&p_pw->ltask, ltask_binding);
  piom_ltask_set_name(&p_pw->ltask, "nmad: poll_send");
  piom_ltask_set_destructor(&p_pw->ltask, &nm_ltask_destructor);
  nm_pw_ref_inc(p_pw);
  assert(p_pw->p_gdrv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  if(r->driver->capabilities.is_exportable && (r->driver->wait_send_iov != NULL))
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_send, 2 * p_pw->p_drv->profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);
}

void nm_ltask_submit_post_drv(struct nm_drv*p_drv)
{
  piom_topo_obj_t ltask_binding = nm_get_binding_policy(p_drv);
  piom_ltask_create(&p_drv->p_ltask, &nm_task_post_on_drv, p_drv,
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT);
  piom_ltask_set_binding(&p_drv->p_ltask, ltask_binding);
  piom_ltask_set_name(&p_drv->p_ltask, "nmad: post_on_drv");
  piom_ltask_submit(&p_drv->p_ltask);
}

void nm_ltask_submit_offload(struct piom_ltask*p_ltask, struct nm_pkt_wrap *p_pw)
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

#else /* PIOMAN_POLL */

#endif /* PIOMAN_POLL */
