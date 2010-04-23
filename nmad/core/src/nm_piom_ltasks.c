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

#ifndef PIOM_DISABLE_LTASKS

/* specify the binding policy of communication requests.
 * All these policies can be specified to NewMadeleine using 
 * the NM_BINDING_POLICY environment variable
 */
enum __nm_ltask_policy{
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

static enum __nm_ltask_policy __policy = NM_BIND_ON_LOCAL_CORE;
static int __policy_core = 0;

/* retrieve the binding policy */
void nm_ltask_set_policy()
{
  const char* policy = getenv("PIOM_BINDING_POLICY");
  if(!policy){
    fprintf(stderr, "# Binding policy: NM_BIND_ON_LOCAL_CORE\n");
    goto out;
  }
  if(! strcmp(policy, "NM_BIND_ON_LOCAL_CORE")) {
    fprintf(stderr, "# Binding policy: NM_BIND_ON_LOCAL_CORE\n");
    __policy = NM_BIND_ON_LOCAL_CORE;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_L2")) {
    fprintf(stderr, "# Binding policy: NM_BIND_ON_LOCAL_L2\n");
    __policy = NM_BIND_ON_LOCAL_L2;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_L3")) {
    fprintf(stderr, "# Binding policy: NM_BIND_ON_LOCAL_L3\n");
    __policy = NM_BIND_ON_LOCAL_L3;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_DIE" )) {
    fprintf(stderr, "# Binding policy: NM_BIND_ON_LOCAL_DIE\n");
    __policy = NM_BIND_ON_LOCAL_DIE;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_NODE")) {
    fprintf(stderr, "# Binding policy: NM_BIND_ON_LOCAL_NODE\n");
    __policy = NM_BIND_ON_LOCAL_NODE;
  } else if(! strcmp(policy, "NM_BIND_ON_LOCAL_MACHINE")) {
    fprintf(stderr, "# Binding policy: NM_BIND_ON_LOCAL_MACHINE\n");
    __policy = NM_BIND_ON_LOCAL_MACHINE;
  } else if(! strcmp(policy, "NM_BIND_ON_CORE")) {
    __policy = NM_BIND_ON_CORE;
    const char* core = getenv("PIOM_BINDING_CORE");
    if(core)
      __policy_core = atoi(core);
    else
      __policy_core = 0;
    fprintf(stderr, "# Binding policy: NM_BIND_ON_CORE %d\n", __policy_core);
  } else if(policy) {
    fprintf(stderr, "unknown binding policy: %s\n", policy);
  }
 out:
  
  return ;
}

static piom_vpset_t nm_get_binding_policy()
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
      fprintf(stderr, "PIOM_BINDING_CORE is not set. Please set this environment variable in order to specify a binding core.\n");
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
      fprintf(stderr, "Cannot parse PIOM_BINDING_CORE ('%s'). Falling back to default (%d) for driver #%d\n", policy_backup, 0, drv_id);
      binding_core=0;
      goto out;
    }

 out:
  fprintf(stderr, "# Driver #%d (%s) is bound to core #%d\n",  drv_id, p_drv->driver->name, binding_core);
  p_drv->vpset = piom_get_parent_core(binding_core);
  return p_drv->vpset;
 
}

int nm_poll_recv_task(void *args)
{
  struct nm_pkt_wrap * p_pw = args;
  int ret = -NM_EUNKNOWN;
  /* todo: lock something when using fine-grain locks */
  if(nmad_trylock()) {
    ret = nm_poll_recv(p_pw);
    nmad_unlock();
  }
  return ret;
}

int nm_poll_send_task(void *args)
{
  struct nm_pkt_wrap * p_pw=args;
  int ret = -NM_EUNKNOWN;
  if(nmad_trylock()) {
    ret =  nm_poll_send(p_pw);
    nmad_unlock();
  }
  return ret;
}

int nm_post_recv_task(void *args)
{
  TBX_FAILURE("Not yet implemented!\n");
  return 0;
}

int nm_post_send_task(void *args)
{
  TBX_FAILURE("Not yet implemented!\n");
  return 0;
}

int nm_post_task(void *args)
{
  struct nm_core * p_core=args;
  int ret = -NM_EUNKNOWN;
  if(nmad_trylock()){
    ret = nm_piom_post_all(p_core);
    nmad_unlock();
  }
  return ret;
}

int nm_post_on_drv_task(void *args)
{
  struct nm_drv * p_drv = args;
  int ret;
  nmad_lock();
  ret = nm_piom_post_on_drv(p_drv);
  nmad_unlock();
  return ret;
}

int nm_offload_task(void* args)
{
  struct nm_pkt_wrap * p_pw=args;
  nmad_lock();
  nm_post_send(p_pw);
  nmad_unlock();
  return 0;
}

void nm_submit_poll_recv_ltask(struct nm_pkt_wrap *p_pw)
{
  piom_vpset_t task_vpset = nm_get_binding_policy();

  piom_ltask_create(&p_pw->ltask, 
		    &nm_poll_recv_task, 
		    p_pw,
		    PIOM_LTASK_OPTION_REPEAT, 
		    task_vpset);
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_submit_poll_send_ltask(struct nm_pkt_wrap *p_pw)
{
  piom_vpset_t task_vpset = nm_get_binding_policy();

  piom_ltask_create(&p_pw->ltask, 
		    &nm_poll_send_task, 
		    p_pw,
		    PIOM_LTASK_OPTION_REPEAT, 
		    task_vpset);
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_submit_post_ltask(struct piom_ltask *task, struct nm_core *p_core)
{
  piom_vpset_t task_vpset = nm_get_binding_policy();

  piom_ltask_create(task, 
		    &nm_post_task, 
		    p_core,
		    PIOM_LTASK_OPTION_REPEAT, 
		    task_vpset);
  piom_ltask_submit(task);	
}

void nm_submit_post_drv_ltask(struct piom_ltask *task, struct nm_drv *p_drv)
{
  piom_vpset_t task_vpset = nm_get_binding_policy_drv(p_drv);
 
  piom_ltask_create(task, 
		    &nm_post_on_drv_task, 
		    p_drv,
		    PIOM_LTASK_OPTION_REPEAT, 
		    task_vpset);
  piom_ltask_submit(task);  
}

void nm_submit_offload_ltask(struct piom_ltask *task, struct nm_pkt_wrap *p_pw)
{
  piom_vpset_t task_vpset = nm_get_binding_policy();

  piom_ltask_create(task, 
		    &nm_offload_task, 
		    p_pw,
		    PIOM_LTASK_OPTION_NULL, 
		    task_vpset);
  piom_ltask_submit(task);	
}

#endif /* PIOM_DISABLE_LTASKS */
#else

#endif /* PIOMAN_POLL */
