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

#include <nm_so_util.h>
#include <Padico/Puk.h>
#include <nm_drivers.h>
#include <nm_launcher.h>

#ifdef CONFIG_PROTO_MAD3

#include <nm_mad3_public.h>

static puk_instance_t launcher_instance = NULL;
static struct puk_receptacle_NewMad_Launcher_s r;
static nm_gate_id_t *gate_ids;

int nm_so_get_rank(int *rank) {
  *rank = (*r.driver->get_rank)(r._status);
  return NM_ESUCCESS;
}

int nm_so_get_size(int *size) {
  *size  = (*r.driver->get_size)(r._status);
  return NM_ESUCCESS;
}

int nm_so_get_sr_if(struct nm_so_interface **sr_if) {
  *sr_if  = (*r.driver->get_so_sr_if)(r._status);
  return NM_ESUCCESS;
}

int nm_so_get_pack_if(nm_so_pack_interface *pack_if) {
  *pack_if = (*r.driver->get_so_pack_if)(r._status);
  return NM_ESUCCESS;
}

int nm_so_get_gate_out_id(int dest, nm_gate_id_t *gate_id) {
  *gate_id = gate_ids[dest];
  return NM_ESUCCESS;
}

int nm_so_get_gate_in_id(int dest, nm_gate_id_t *gate_id) {
  *gate_id = gate_ids[dest];
  return NM_ESUCCESS;
}

int nm_so_init(int *argc, char	**argv) {
  /*
   * Lazy Puk initialization (it may already have been initialized in PadicoTM)
   */
  if(!padico_puk_initialized()) {
    padico_puk_init(*argc, &*argv);
  }

  nm_mad3_launcher_init();

  /*
   * NewMad initialization is performed by component 'NewMad_Launcher'
   * (from proto_mad3 or from PadicoTM)
   */
  puk_adapter_t launcher = puk_adapter_resolve("NewMad_Launcher");
  launcher_instance = puk_adapter_instanciate(launcher);
  puk_instance_indirect_NewMad_Launcher(launcher_instance, NULL, &r);
  (*r.driver->init)(r._status, argc, &*argv, "NewMadeleine", &nm_so_load);

  {
    int size = (*r.driver->get_size)(r._status);
    gate_ids = malloc(size * sizeof(nm_gate_id_t));
    (*r.driver->get_gate_ids)(r._status, &gate_ids[0]);
  }

  return NM_ESUCCESS;
}

int nm_so_exit(void) {
  free(gate_ids);
  puk_instance_destroy(launcher_instance);
  return NM_ESUCCESS;
}

#endif /* CONFIG_PROTO_MAD3 */

