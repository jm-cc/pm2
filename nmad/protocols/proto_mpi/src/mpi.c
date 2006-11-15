/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*
 * mpi.c
 * =====
 */

#include "mpi.h"

#include <stdint.h>

#include <nm_public.h>
#include <nm_so_public.h>
#include <nm_so_pack_interface.h>

#if defined CONFIG_MX
#  include <nm_mx_public.h>
#elif defined CONFIG_GM
#  include <nm_gm_public.h>
#elif defined CONFIG_QSNET
#  include <nm_qsnet_public.h>
#elif defined CONFIG_SISCI
#  include <nm_sisci_public.h>
#else
#  include <nm_tcp_public.h>
#endif

int not_implemented(char *s) 
{
  printf("%s: Not implemented yet\n", s);
  abort();
}

int MPI_Init(int *argc,
             char **argv) {
  nm_so_pack_interface  interface;
  struct nm_core       *p_core  = NULL;
  uint8_t		drv_id	=    0;
  char                 *l_url	= NULL;
  uint8_t		gate_id	=    0;
  int                   err;

  /*
    Initialise NewMadeleine
  */
  err = nm_core_init(argc, argv, &p_core, nm_so_load);
  if (err != NM_ESUCCESS) {
    printf("nm_core_init returned err = %d\n", err);
    return 0;
  }
  err = nm_so_pack_interface_init(p_core, &interface);
  if(err != NM_ESUCCESS) {
    printf("nm_so_pack_interface_init return err = %d\n", err);
    return 0;
  }

#if defined CONFIG_MX
  err = nm_core_driver_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
  err = nm_core_driver_init(p_core, nm_gm_load, &drv_id, &l_url);
#elif defined CONFIG_QSNET
  err = nm_core_driver_init(p_core, nm_qsnet_load, &drv_id, &l_url);
#else
  err = nm_core_driver_init(p_core, nm_tcp_load, &drv_id, &l_url);
#endif
  if (err != NM_ESUCCESS) {
    printf("nm_core_driver_init returned err = %d\n", err);
    return 0;
  }
  printf("local url: [%s]\n", l_url);

  err = nm_core_gate_init(p_core, &gate_id);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    return 0;
  }

  return 1;
}

int MPI_Finalize(void) {
  return not_implemented("MPI_Finalize");
}

int MPI_Comm_size(MPI_Comm comm,
                  int *size) {
  return not_implemented("MPI_Comm_size");
}

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) {
  return not_implemented("MPI_Comm_rank");
}

int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm) {
  return not_implemented("MPI_Send");
}

int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status) {
  return not_implemented("MPI_Recv");
}

