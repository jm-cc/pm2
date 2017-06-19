/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
#include <nm_launcher.h>
#include <nm_core_interface.h>
#include <tbx.h>
#ifdef PIOMAN
#include <pioman.h>
#endif

#ifdef PUKABI
#include <Padico/Puk-ABI.h>
#endif /* PUKABI */

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);


static struct
{
  puk_instance_t instance;                   /**< instance of NewMad_Launcher component */
  struct puk_receptacle_NewMad_Launcher_s r; /**< receptacle for launcher component */
  nm_gate_t*gates;         /**< pointer to the gate array, filled by the launcher component */
  int size;                /**< number of gates */
  puk_hashtable_t reverse; /**< reverse table: p_gate -> rank */
  puk_mod_t boot_mod;      /**< the mod that was used for init */
  int puk_init;            /**< whether we initialized Puk ourself */
  int init_level;          /**< number of stacked init */
} launcher =
  {
    .instance = NULL, .gates = NULL, .size = -1, .reverse = NULL,
    .boot_mod = NULL, .puk_init = 0, .init_level = 0
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

int nm_launcher_session_open(nm_session_t*p_session, const char*label)
{
  assert(launcher.instance != NULL);
  int rc = nm_session_open(p_session, label);
  return rc;
}

int nm_launcher_session_close(nm_session_t p_session)
{
  assert(launcher.instance != NULL);
  int rc = nm_session_destroy(p_session);
  return rc;
}

int nm_launcher_get_gate(int dest, nm_gate_t*pp_gate)
{
  assert(launcher.gates != NULL);
  assert(dest >= 0 && dest < launcher.size);
  nm_gate_t p_gate = launcher.gates[dest];
  *pp_gate = p_gate;
  return NM_ESUCCESS;
}

int nm_launcher_get_dest(nm_gate_t p_gate, int*dest)
{
  assert(p_gate != NULL);
  intptr_t rank_as_ptr = (intptr_t)puk_hashtable_lookup(launcher.reverse, p_gate);
  if(rank_as_ptr > 0)
    {
      *dest = (rank_as_ptr - 1);
      return NM_ESUCCESS;
    }
  else
    {
      *dest = -1;
      return -NM_EINVAL;
    }
}

void nm_launcher_abort(void)
{
  assert(launcher.instance != NULL);
  (*launcher.r.driver->abort)(launcher.r._status);
}

int nm_launcher_init_checked(int *argc, char**argv, const struct nm_abi_config_s*p_nm_abi_config)
{
  nm_abi_config_check(p_nm_abi_config);
  launcher.init_level++;
  if(launcher.init_level > 1)
    return NM_ESUCCESS;

  if(!tbx_initialized())
    {
      tbx_init(argc, &argv);
    }
  /*
   * Lazy Puk initialization (it may already have been initialized in PadicoTM)
   */
  if(!padico_puk_initialized())
    {
      padico_puk_init(0, NULL);
      launcher.puk_init = 1;
    }

  if(getenv("PADICO_NOPRELOAD") != NULL)
    {
      padico_rc_t rc = padico_puk_mod_resolve(&launcher.boot_mod, "PadicoBootLib");
      if((launcher.boot_mod == NULL) || padico_rc_iserror(rc))
	{
	  NM_FATAL("launcher: cannot resolve module PadicoBootLib.\n");
	}
      rc = padico_puk_mod_load(launcher.boot_mod);
      if(padico_rc_iserror(rc))
	{
	  NM_FATAL("launcher: cannot load module PadicoBootLib.\n");
	}
    }

  const char*launcher_name =
    (puk_mod_getbyname("PadicoTM") != NULL) ? "NewMad_Launcher_newmadico" : "NewMad_Launcher_cmdline";

  /*
   * NewMad initialization is performed by component 'NewMad_Launcher_*'
   */
  puk_component_t launcher_component = puk_component_resolve(launcher_name);
  launcher.instance = puk_component_instantiate(launcher_component);
  puk_instance_indirect_NewMad_Launcher(launcher.instance, NULL, &launcher.r);
  (*launcher.r.driver->init)(launcher.r._status, argc, argv, "NewMadeleine");

  launcher.size = (*launcher.r.driver->get_size)(launcher.r._status);
  launcher.gates = malloc(launcher.size * sizeof(nm_gate_t));
  launcher.reverse = puk_hashtable_new_ptr();
  (*launcher.r.driver->get_gates)(launcher.r._status, launcher.gates);
  int j;
  for(j = 0; j < launcher.size; j++)
    {
      nm_gate_t p_gate = launcher.gates[j];
      const intptr_t rank_as_ptr = j + 1;
      puk_hashtable_insert(launcher.reverse, p_gate, (void*)rank_as_ptr);
    }
  
  return NM_ESUCCESS;
}

int nm_launcher_exit(void)
{
  launcher.init_level--;
  if(launcher.init_level > 0)
    return NM_ESUCCESS;
  if(launcher.init_level < 0)
    return -NM_EALREADY;

#ifdef PIOMAN_TRACE
  piom_trace_flush();
#endif /* PIOMAN_TRACE */
  
  free(launcher.gates);
  launcher.gates = NULL;
  puk_instance_destroy(launcher.instance);
  launcher.instance = NULL;
  if(launcher.boot_mod)
    {
      padico_puk_mod_unload(launcher.boot_mod);
      launcher.boot_mod = NULL;
    }
  if(launcher.puk_init)
    {
      padico_puk_shutdown();
    }
  return NM_ESUCCESS;
}


