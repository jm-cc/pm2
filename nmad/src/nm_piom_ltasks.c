/* -*- Mode: C; c-basic-offset:2 ; -*- */
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
#include <nm_private.h>
#if PIOMAN_POLL

struct nm_ltask_policy_s
{
  enum
    {
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
    .location = NM_POLICY_ANY,
    .level    = PIOM_TOPO_MACHINE,
    .custom   = -1
  };

/** retrieve the binding policy */
void nm_ltask_set_policy(void)
{
  const char* policy = getenv("PIOM_BINDING_POLICY");
  if(!policy)
    {
      NM_DISPF("# nmad: default binding policy.\n");
    }
  else
    {
#warning TODO- set policy
    }
  return ;
}

static piom_topo_obj_t nm_get_binding_policy(struct nm_drv*p_drv)
{
  piom_topo_obj_t binding = piom_topo_full;

  switch(ltask_policy.location)
    {
    case NM_POLICY_APP:
      binding = piom_get_parent_obj(piom_ltask_current_obj(), ltask_policy.level);
      break;

    case NM_POLICY_DEV:
      binding = piom_get_parent_obj(p_drv->binding, ltask_policy.level);
      break;

    case NM_POLICY_ANY:
      binding = piom_topo_full;
      break;

    case NM_POLICY_CUSTOM:
      /* TODO */
      break;
    default:
      break;
    }
  return binding;
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
  else
    {
      nm_ltask_submit_poll_recv(p_pw);
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
      piom_ltask_destroy(&p_pw->ltask);
      nm_so_process_complete_recv(p_pw->p_gate->p_core, p_pw);
      nmad_unlock();
    }
  else if((p_pw->ltask.state & PIOM_LTASK_STATE_CANCELLED) || (err == -NM_ECLOSED))
    {
      piom_ltask_destroy(&p_pw->ltask);
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
  else
    {
      nm_ltask_submit_poll_send(p_pw);
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
      piom_ltask_destroy(&p_pw->ltask);
      nm_so_process_complete_send(p_pw->p_gate->p_core, p_pw);
      nm_so_pw_free(p_pw);
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
  nm_try_and_commit(p_drv->p_core);
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

void nm_ltask_submit_poll_recv(struct nm_pkt_wrap *p_pw)
{
  piom_topo_obj_t task_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(&p_pw->ltask, &nm_task_poll_recv,  p_pw,
		    PIOM_LTASK_OPTION_ONESHOT | PIOM_LTASK_OPTION_DESTROY | PIOM_LTASK_OPTION_NOWAIT,
		    task_binding);
  const struct nm_drv_iface_s*driver = p_pw->p_drv->driver;
  if(driver->capabilities.is_exportable && (driver->wait_recv_iov != NULL))
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_recv, 2 * p_pw->p_drv->profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_ltask_submit_poll_send(struct nm_pkt_wrap *p_pw)
{
  piom_topo_obj_t task_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(&p_pw->ltask, &nm_task_poll_send, p_pw,
		    PIOM_LTASK_OPTION_ONESHOT | PIOM_LTASK_OPTION_DESTROY | PIOM_LTASK_OPTION_NOWAIT,
		    task_binding);
  assert(p_pw->p_gdrv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  if(r->driver->capabilities.is_exportable && (r->driver->wait_send_iov != NULL))
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_send, 2 * p_pw->p_drv->profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);
}

void nm_ltask_submit_post_drv(struct piom_ltask *task, struct nm_drv*p_drv)
{
  piom_topo_obj_t task_binding = nm_get_binding_policy(p_drv);
  piom_ltask_create(task, 
		    &nm_task_post_on_drv,
		    p_drv,
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT,
		    task_binding);
  piom_ltask_submit(task);  
}

void nm_ltask_submit_offload(struct piom_ltask *task, struct nm_pkt_wrap *p_pw)
{
  piom_topo_obj_t task_binding = nm_get_binding_policy(p_pw->p_drv);
  piom_ltask_create(task, 
		    &nm_task_offload,
		    p_pw,
		    PIOM_LTASK_OPTION_ONESHOT, 
		    task_binding);
  piom_ltask_submit(task);	
}

#else /* PIOMAN_POLL */

#endif /* PIOMAN_POLL */
