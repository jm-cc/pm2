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

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);


static struct
{
  puk_instance_t instance;
  struct puk_receptacle_NewMad_Launcher_s r;
  nm_gate_t *gates;
  int size;
} launcher =
  {
    .instance = NULL, .gates = NULL, .size = -1
  };

int nm_launcher_get_rank(int *rank)
{
  assert(launcher.instance != NULL);
  *rank = (*launcher.r.driver->get_rank)(launcher.r._status);
  return NM_ESUCCESS;
}

int nm_launcher_get_size(int *size)
{
  assert(launcher.instance != NULL);
  *size  = (*launcher.r.driver->get_size)(launcher.r._status);
  return NM_ESUCCESS;
}

int nm_launcher_get_session(nm_session_t *p_session)
{
  assert(launcher.instance != NULL);
  *p_session  = (*launcher.r.driver->get_session)(launcher.r._status);
  return NM_ESUCCESS;
}

int nm_launcher_get_gate(int dest, nm_gate_t*pp_gate)
{
  assert(launcher.gates != NULL);
  assert(dest >= 0 && dest < launcher.size);
  nm_gate_t p_gate = launcher.gates[dest];
  *pp_gate = p_gate;
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
  puk_component_t launcher_component = puk_adapter_resolve(launcher_name);
  launcher.instance = puk_adapter_instanciate(launcher_component);
  puk_instance_indirect_NewMad_Launcher(launcher.instance, NULL, &launcher.r);
  (*launcher.r.driver->init)(launcher.r._status, argc, &*argv, "NewMadeleine");

  nm_sr_init((*launcher.r.driver->get_session)(launcher.r._status));

  launcher.size = (*launcher.r.driver->get_size)(launcher.r._status);
  launcher.gates = TBX_MALLOC(launcher.size * sizeof(nm_gate_t));
  (*launcher.r.driver->get_gates)(launcher.r._status, launcher.gates);
  
  return NM_ESUCCESS;
}

int nm_launcher_exit(void)
{
  static int exit_done = 0;
  if(exit_done)
    return NM_ESUCCESS;
  exit_done = 1;
  free(launcher.gates);
  puk_instance_destroy(launcher.instance);
  padico_puk_shutdown();
  return NM_ESUCCESS;
}


