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

#ifdef CONFIG_PROTO_MAD3
#include <nm_mad3_public.h>
#endif


#if !defined(CONFIG_PROTO_MAD3) && !defined(CONFIG_PADICO)
extern void nm_cmdline_launcher_declare(void);
#endif

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

int nm_launcher_get_core(struct nm_core **p_core)
{
  *p_core  = (*r.driver->get_core)(r._status);
  return NM_ESUCCESS;
}

int nm_launcher_get_gate(int dest, nm_gate_t *gate)
{
  *gate = gates[dest];
  return NM_ESUCCESS;
}

int nm_launcher_init(int *argc, char**argv)
{
  /*
   * Lazy Puk initialization (it may already have been initialized in PadicoTM)
   */
  if(!padico_puk_initialized()) {
    padico_puk_init(*argc, &*argv);
  }

#if defined(CONFIG_PROTO_MAD3)
  nm_mad3_launcher_init();
#elif !defined(CONFIG_PADICO)
  nm_cmdline_launcher_declare();
#endif

  /*
   * NewMad initialization is performed by component 'NewMad_Launcher'
   * (from proto_mad3 or from PadicoTM)
   */
  puk_adapter_t launcher = puk_adapter_resolve("NewMad_Launcher");
  launcher_instance = puk_adapter_instanciate(launcher);
  puk_instance_indirect_NewMad_Launcher(launcher_instance, NULL, &r);
  (*r.driver->init)(r._status, argc, &*argv, "NewMadeleine");

  nm_sr_init((*r.driver->get_core)(r._status));

  {
    int size = (*r.driver->get_size)(r._status);
    gates = TBX_MALLOC(size * sizeof(nm_gate_t));
    (*r.driver->get_gates)(r._status, gates);
  }

  (*r.driver->post_init)(r._status);

  return NM_ESUCCESS;
}

int nm_launcher_exit(void)
{
  free(gates);
  puk_instance_destroy(launcher_instance);
  return NM_ESUCCESS;
}


