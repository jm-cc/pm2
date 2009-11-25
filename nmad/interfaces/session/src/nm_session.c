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
#include "tbx.h"
#include <string.h>
#include <assert.h>

static struct
{
  struct nm_core*p_core;
  struct nm_drv*p_drv;
  const char*local_url;
} nm_session =
  {
    .p_core = NULL,
    .p_drv = NULL,
    .local_url = NULL
  };

int nm_session_create(nm_session_t*pp_session, const char*label)
{
  struct nm_session_s*p_session = TBX_MALLOC(sizeof(struct nm_session_s));
  p_session->p_core = NULL;
  p_session->label = strdup(label);
  const int len = strlen(label);
  p_session->session_hash = puk_hash_oneatatime((void*)label, len);

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

    }
  
  p_session->p_core = nm_session.p_core;
  *p_local_url = nm_session.local_url;

  return NM_ESUCCESS;
}

int nm_session_connect(nm_session_t p_session, nm_gate_t*p_gate, const char*url)
{
  return NM_ESUCCESS;
}

int nm_session_accept(nm_session_t p_session, nm_gate_t*p_gate)
{
  return NM_ESUCCESS;
}

int nm_session_destroy(nm_session_t p_session)
{
  return NM_ESUCCESS;
}
