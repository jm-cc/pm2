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

/* specify the binding policy of communication requests.
 * All these policies can be specified to NewMadeleine using 
 * the NM_BINDING_POLICY environment variable
 */
enum nm_ltask_policy_e
{
  /* bound to the core from which a request is submitted */
  NM_BIND_ON_LOCAL_CORE,
  /* can be executed by any core that share a L2 cache */
  NM_BIND_ON_LOCAL_L2, 
  /* can be executed by any core that share a L3 cache */
  NM_BIND_ON_LOCAL_L3, 
  /* can be executed by any core that is on the same die */
  NM_BIND_ON_LOCAL_DIE, 
  /* can be executed by any core that is on the NUMA node */
  NM_BIND_ON_LOCAL_NODE, 
  /* can be executed by any core that is on the machine */
  NM_BIND_ON_LOCAL_MACHINE, 
  /* all the communication requests are bound to a specific core 
   * this core is specifyed by the PIOM_BINDING_CORE env variable
   */
  NM_BIND_ON_CORE, 
};

static enum nm_ltask_policy_e __policy = NM_BIND_ON_LOCAL_CORE;
static int __policy_core = 0;

/* retrieve the binding policy */
void nm_ltask_set_policy(void)
{
  const char* policy = getenv("PIOM_BINDING_POLICY");
  if(!policy){
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_LOCAL_CORE\n");
    goto out;
  }
  if(! strcmp(policy, "NM_BIND_ON_LOCAL_CORE")) {
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_LOCAL_CORE\n");
    __policy = NM_BIND_ON_LOCAL_CORE;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_L2")) {
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_LOCAL_L2\n");
    __policy = NM_BIND_ON_LOCAL_L2;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_L3")) {
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_LOCAL_L3\n");
    __policy = NM_BIND_ON_LOCAL_L3;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_DIE" )) {
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_LOCAL_DIE\n");
    __policy = NM_BIND_ON_LOCAL_DIE;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_NODE")) {
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_LOCAL_NODE\n");
    __policy = NM_BIND_ON_LOCAL_NODE;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_MACHINE")) {
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_LOCAL_MACHINE\n");
    __policy = NM_BIND_ON_LOCAL_MACHINE;
  } else if(! strcmp(policy, "NM_BIND_ON_CORE")) {
    __policy = NM_BIND_ON_CORE;
    const char* core = getenv("PIOM_BINDING_CORE");
    if(core)
      __policy_core = atoi(core);
    else
      __policy_core = 0;
    NM_DISPF("# nmad: binding policy = NM_BIND_ON_CORE %d\n", __policy_core);
  } else if(policy) {
    NM_DISPF("# nmad: unknown binding policy %s\n", policy);
  }
 out:
  
  return ;
}

static piom_vpset_t nm_get_binding_policy(void)
{
  switch(__policy) {
  case  NM_BIND_ON_LOCAL_CORE:
    return piom_get_parent_core(PIOM_CURRENT_VP);
  case NM_BIND_ON_LOCAL_L2: 
    return piom_get_parent_l2(PIOM_CURRENT_VP);
  case NM_BIND_ON_LOCAL_L3: 
    return piom_get_parent_l3(PIOM_CURRENT_VP);
  case NM_BIND_ON_LOCAL_DIE: 
    return piom_get_parent_die(PIOM_CURRENT_VP);
  case NM_BIND_ON_LOCAL_NODE: 
    return piom_get_parent_node(PIOM_CURRENT_VP);
  case NM_BIND_ON_LOCAL_MACHINE: 
    return piom_get_parent_machine(PIOM_CURRENT_VP);
  case NM_BIND_ON_CORE:
    return PIOM_VPSET_VP(__policy_core);
  }
  return PIOM_VPSET_VP(PIOM_CURRENT_VP);
}

static piom_vpset_t nm_get_binding_policy_drv(struct nm_drv *p_drv)
{
  if(__policy != NM_BIND_ON_CORE)
    return nm_get_binding_policy();

  struct nm_core *p_core = p_drv->p_core;
  struct tbx_fast_list_head *head = &p_core->driver_list;
  struct tbx_fast_list_head *pos = head->next;
  /* Get the driver number */
  int drv_id = -1;
  int i = 0;
  for( i=0; pos != head; i++) 
    {
      if(&p_drv->_link == pos){
	drv_id = i;
	break;
      }
      pos = pos->next;
    }
  
  assert(drv_id >= 0);

  int binding_core = -1;
  char* policy_backup = getenv("PIOM_BINDING_CORE");
  if(!policy_backup)
    {
      NM_DISPF("# nmad: PIOM_BINDING_CORE is not set. Please set this environment variable in order to specify a binding core.\n");
      /* falling back to core #0 */
      binding_core = 0;
      goto out;
    }

  char* policy = strdup(policy_backup);
  char* saveptr = NULL;
  char*token = strtok_r(policy, "+", &saveptr);
  int first_core = -1;

  i=0;
  while(token) {
    first_core = atoi(token);
    if(i == drv_id)
      binding_core=atoi(token);
    i++;
    token = strtok_r(NULL, "+", &saveptr);
  }

  if(binding_core < 0)
    {
      if(first_core >= 0) 
	{
	  /* This means that the user specified one binding core and that we have at least 2 drivers.
	   * Bind all the drivers to a single binding_core.
	   */
	  binding_core = first_core; 
	  goto out;
	}
      /* PIOM_BINDING_CORE is set to something that cannot be parsed */
      NM_DISPF("Cannot parse PIOM_BINDING_CORE ('%s'). Falling back to default (%d) for driver #%d\n", policy_backup, 0, drv_id);
      binding_core=0;
      goto out;
    }

 out:
  NM_DISPF("# nmad: driver #%d (%s) is bound to core #%d\n",  drv_id, p_drv->driver->name, binding_core);
  p_drv->vpset = piom_get_parent_core(binding_core);
  return p_drv->vpset;
 
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
  piom_vpset_t task_vpset = nm_get_binding_policy();
  piom_ltask_create(&p_pw->ltask, &nm_task_poll_recv,  p_pw,
		    PIOM_LTASK_OPTION_ONESHOT | PIOM_LTASK_OPTION_DESTROY | PIOM_LTASK_OPTION_NOWAIT,
		    task_vpset);
  const struct nm_drv_iface_s*driver = p_pw->p_drv->driver;
  if(driver->capabilities.is_exportable && (driver->wait_recv_iov != NULL))
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_recv, 2 * p_pw->p_drv->profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_ltask_submit_poll_send(struct nm_pkt_wrap *p_pw)
{
  piom_vpset_t task_vpset = nm_get_binding_policy();

  piom_ltask_create(&p_pw->ltask, &nm_task_poll_send, p_pw,
		    PIOM_LTASK_OPTION_ONESHOT | PIOM_LTASK_OPTION_DESTROY | PIOM_LTASK_OPTION_NOWAIT,
		    task_vpset);
  assert(p_pw->p_gdrv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  if(r->driver->capabilities.is_exportable && (r->driver->wait_send_iov != NULL))
    {
      piom_ltask_set_blocking(&p_pw->ltask, &nm_task_block_send, 2 * p_pw->p_drv->profile.latency);
    }
  piom_ltask_submit(&p_pw->ltask);
}

void nm_ltask_submit_post_drv(struct piom_ltask *task, struct nm_drv *p_drv)
{
  piom_vpset_t task_vpset = nm_get_binding_policy_drv(p_drv);
 
  piom_ltask_create(task, 
		    &nm_task_post_on_drv,
		    p_drv,
		    PIOM_LTASK_OPTION_REPEAT | PIOM_LTASK_OPTION_NOWAIT,
		    task_vpset);
  piom_ltask_submit(task);  
}

void nm_ltask_submit_offload(struct piom_ltask *task, struct nm_pkt_wrap *p_pw)
{
  piom_vpset_t task_vpset = nm_get_binding_policy();

  piom_ltask_create(task, 
		    &nm_task_offload,
		    p_pw,
		    PIOM_LTASK_OPTION_ONESHOT, 
		    task_vpset);
  piom_ltask_submit(task);	
}

#else /* PIOMAN_POLL */

#endif /* PIOMAN_POLL */
