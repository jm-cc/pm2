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
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <nm_core_interface.h>
#include <pm2_common.h>


static puk_instance_t launcher_instance = NULL;
static struct puk_receptacle_NewMad_Launcher_s r;
static nm_gate_t *gates = NULL;

int nm_launcher_get_rank(int *rank)
{
  *rank = (*r.driver->get_rank)(r._status);
  return NM_ESUCCESS;
}

int nm_launcher_get_size(int *size)
{
  *size  = (*r.driver->get_size)(r._status);
  return NM_ESUCCESS;
}

int nm_launcher_get_session(nm_session_t *p_session)
{
  *p_session  = (*r.driver->get_session)(r._status);
  return NM_ESUCCESS;
}

int nm_launcher_get_gate(int dest, nm_gate_t *gate)
{
  *gate = gates[dest];
  return NM_ESUCCESS;
}

int nm_launcher_init(int *argc, char**argv)
{
  static int init_done = 0;
  if(init_done)
    return NM_ESUCCESS;
  init_done = 1;

  common_pre_init(argc, argv, NULL);
  common_post_init(argc, argv, NULL);

  /*
   * Lazy Puk initialization (it may already have been initialized in PadicoTM)
   */
  if(!padico_puk_initialized()) {
    padico_puk_init(*argc, &*argv);
  }

#if defined(CONFIG_PROTO_MAD3)
  const char launcher_name[] = "NewMad_Launcher_mad3";
#elif defined(CONFIG_PADICO)
  const char launcher_name[] = "NewMad_Launcher_newmadico";
#else 
  const char launcher_name[] = "NewMad_Launcher_cmdline";
#endif

  /*
   * NewMad initialization is performed by component 'NewMad_Launcher_*'
   */
  puk_component_t launcher = puk_adapter_resolve(launcher_name);
  launcher_instance = puk_adapter_instanciate(launcher);
  puk_instance_indirect_NewMad_Launcher(launcher_instance, NULL, &r);
  (*r.driver->init)(r._status, argc, &*argv, "NewMadeleine");

  nm_sr_init((*r.driver->get_session)(r._status));

  int size = (*r.driver->get_size)(r._status);
  gates = TBX_MALLOC(size * sizeof(nm_gate_t));
  (*r.driver->get_gates)(r._status, gates);
  
  return NM_ESUCCESS;
}

int nm_launcher_exit(void)
{
  static int exit_done = 0;
  if(exit_done)
    return NM_ESUCCESS;
  exit_done = 1;
  free(gates);
  puk_instance_destroy(launcher_instance);
  padico_puk_shutdown();
  return NM_ESUCCESS;
}


