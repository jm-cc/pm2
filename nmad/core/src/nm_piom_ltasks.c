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
int nm_poll_recv_task(void *args)
{
  struct nm_pkt_wrap * p_pw=args;
  return nm_poll_recv(p_pw);
}

int nm_poll_send_task(void *args)
{
  struct nm_pkt_wrap * p_pw=args;
  return nm_poll_send(p_pw);
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
  return nm_piom_post_all(p_core);
}

int nm_offload_task(void* args)
{
  struct nm_pkt_wrap * p_pw=args;
  nm_post_send(p_pw);
  return 0;
}

#ifndef MARCEL
#define FULL_VP 1
#else
#define VP1 1
#define VP2 1
#define VP3 1
#define VP4 1
#endif

void nm_submit_poll_recv_ltask(struct nm_pkt_wrap *p_pw)
{
  /* For now only poll on vp 0*/
#ifdef FULL_VP
  piom_vpset_t task_vpset = piom_vpset_full;
#else
  piom_vpset_t task_vpset = PIOM_VPSET_VP(VP1);
  piom_vpset_set(&task_vpset, VP2);
  piom_vpset_set(&task_vpset, VP3);
  piom_vpset_set(&task_vpset, VP4);
#endif
  piom_ltask_create(&p_pw->ltask, 
		    &nm_poll_recv_task, 
		    p_pw,
		    PIOM_LTASK_OPTION_REPEAT, 
		    task_vpset);
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_submit_poll_send_ltask(struct nm_pkt_wrap *p_pw)
{
#ifdef FULL_VP
  piom_vpset_t task_vpset = piom_vpset_full;
#else
  piom_vpset_t task_vpset = PIOM_VPSET_VP(VP1);
  piom_vpset_set(&task_vpset, VP2);
  piom_vpset_set(&task_vpset, VP3);
  piom_vpset_set(&task_vpset, VP4);
#endif
  piom_ltask_create(&p_pw->ltask, 
		    &nm_poll_send_task, 
		    p_pw,
		    PIOM_LTASK_OPTION_REPEAT, 
		    task_vpset);
  piom_ltask_submit(&p_pw->ltask);	
}

void nm_submit_post_ltask(struct piom_ltask *task, struct nm_core *p_core)
{
#ifdef FULL_VP
  piom_vpset_t task_vpset = piom_vpset_full;
#else
  piom_vpset_t task_vpset = PIOM_VPSET_VP(VP1);
  piom_vpset_set(&task_vpset, VP2);
  piom_vpset_set(&task_vpset, VP3);
  piom_vpset_set(&task_vpset, VP4);
#endif
  piom_ltask_create(task, 
		    &nm_post_task, 
		    p_core,
		    PIOM_LTASK_OPTION_REPEAT, 
		    task_vpset);
  piom_ltask_submit(task);	
}

void nm_submit_offload_ltask(struct piom_ltask *task, struct nm_pkt_wrap *p_pw)
{
#ifdef FULL_VP
  piom_vpset_t task_vpset = piom_vpset_full;
#else
  piom_vpset_t task_vpset = PIOM_VPSET_VP(VP1);
  piom_vpset_set(&task_vpset, VP2);
  piom_vpset_set(&task_vpset, VP3);
  piom_vpset_set(&task_vpset, VP4);
#endif
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
