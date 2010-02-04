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

#ifdef PIOM_ENABLE_LTASKS

/* specify the binding policy of communication requests
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
    __policy = NM_BIND_ON_LOCAL_CORE;
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

int nm_poll_recv_task(void *args)
{
  struct nm_pkt_wrap * p_pw = args;
  int ret;
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
  int ret;
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
  int ret;
  if(nmad_trylock()){
    ret = nm_piom_post_all(p_core);
    nmad_unlock();
  }
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

#endif /* PIOM_ENABLE_LTASKS */
#else

#endif /* PIOMAN_POLL */
