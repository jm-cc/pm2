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

#include <nm_public.h>
#include <nm_session_interface.h>
#include <nm_session_private.h>
#include <Padico/Puk.h>
#include <pm2_common.h>

#include <string.h>
#include <assert.h>

static struct
{
  struct nm_core*p_core;
  struct nm_drv*p_drv;
  const char*local_url;
  puk_hashtable_t gates;
  int ref_count;
} nm_session =
  {
    .p_core    = NULL,
    .p_drv     = NULL,
    .local_url = NULL,
    .gates     = NULL,
    .ref_count = 0
  };

int nm_session_create(nm_session_t*pp_session, const char*label)
{
  struct nm_session_s*p_session = TBX_MALLOC(sizeof(struct nm_session_s));
  p_session->p_core = NULL;
  p_session->label = strdup(label);
  const int len = strlen(label);
  p_session->session_hash = puk_hash_oneatatime((void*)label, len);
  nm_session.ref_count++;
  *pp_session = p_session;
  return NM_ESUCCESS;
}

int nm_session_init(nm_session_t p_session, int*argc, char**argv, const char**p_local_url)
{
  assert(p_session != NULL);
  if(nm_session.p_core == NULL)
    {
      int err = nm_core_init(argc, argv, &nm_session.p_core);
      assert(err == NM_ESUCCESS);
      puk_component_t driver_assembly = nm_core_component_load("driver", "custom");
      const char*driver_url = NULL;
      err = nm_core_driver_load_init(nm_session.p_core, driver_assembly, &nm_session.p_drv, &driver_url);
      assert(err == NM_ESUCCESS);
      nm_session.local_url = strdup(driver_url);
      /* TODO: add custom railstring support with:
       *   . default driver
       *   . multi-rail
       *   . configuration through argv / environment variable
       */
      nm_session.gates = puk_hashtable_new_string();
    }
  
  p_session->p_core = nm_session.p_core;
  *p_local_url = nm_session.local_url;

  return NM_ESUCCESS;
}

int nm_session_connect(nm_session_t p_session, nm_gate_t*pp_gate, const char*url)
{
  assert(p_session != NULL);
  assert(nm_session.gates != NULL);
  nm_gate_t p_gate = puk_hashtable_lookup(nm_session.gates, url);
  if(!p_gate)
    {
      assert(nm_session.p_core != NULL);
      int err = nm_core_gate_init(nm_session.p_core, &p_gate);
      assert(err == NM_ESUCCESS);
      const int is_server = (strcmp(url, nm_session.local_url) > 0);
      if(is_server)
	{
	  err = nm_core_gate_accept(nm_session.p_core, p_gate, nm_session.p_drv, url);
	  assert(err == NM_ESUCCESS);
	}
      else
	{
	  err = nm_core_gate_connect(nm_session.p_core, p_gate, nm_session.p_drv, url);
	  assert(err == NM_ESUCCESS);
	}
      char*key_url = strdup(url);
      puk_hashtable_insert(nm_session.gates, key_url, p_gate);
    }
  *pp_gate = p_gate;
  return NM_ESUCCESS;
}

int nm_session_destroy(nm_session_t p_session)
{
  TBX_FREE(p_session);
  nm_session.ref_count--;
  if(nm_session.ref_count == 0)
    {
      nm_core_exit(nm_session.p_core);
      nm_session.p_core = NULL;
      nm_session.p_drv = NULL;
      free(nm_session.local_url);
      nm_session.local_url = NULL;
    }
  return NM_ESUCCESS;
}
