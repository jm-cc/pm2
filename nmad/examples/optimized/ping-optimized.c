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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <tbx.h>

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

#include "ping-optimized.h"
#include "indexed-optimized.h"
#include "vector-optimized.h"

/*
 * END OF EMULATION OF THE MPI DATATYPE
 */

int
main(int	  argc,
     char	**argv) {
  struct nm_core *p_core   = NULL;
  char		 *r_url	   = NULL;
  char		 *l_url	   = NULL;
  uint8_t	  drv_id   = 0;
  uint8_t	  gate_id  = 0;
  char		 *hostname = "localhost";
  int             err;

  int                   size = 0;
  int                   number_of_blocks = 0;

  /*
    Initialise NewMadeleine
  */
  err = nm_core_init(&argc, argv, &p_core, nm_so_load);
  if (err != NM_ESUCCESS) {
    printf("nm_core_init returned err = %d\n", err);
    goto out;
  }
  err = nm_so_pack_interface_init();
  if(err != NM_ESUCCESS) {
    printf("nm_so_pack_interface_init return err = %d\n", err);
    goto out;
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
    goto out;
  }
  err = nm_core_gate_init(p_core, &gate_id);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out;
  }

  if (argc > 1) {
    /* client */
    argv++;
    r_url = *argv;
    printf("running as client using remote url: %s [%s]\n", hostname, r_url);

    err = nm_core_gate_connect(p_core, gate_id, drv_id, hostname, r_url);
    if (err != NM_ESUCCESS) {
      printf("nm_core_gate_connect returned err = %d\n", err);
      goto out;
    }
  }
  else {
    /* server */
    printf("running as server\n");
    printf("local url: [%s]\n", l_url);
    err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL, NULL);
    if (err != NM_ESUCCESS) {
      printf("nm_core_gate_accept returned err = %d\n", err);
      goto out;
    }

  }

  /*
    Ping-pong
  */
  if (argc > 1) {
    /* client */
    for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
      for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
        if (number_of_blocks > size) continue;

        //pingpong_datatype_indexed(p_core, gate_id, size, number_of_blocks, 1);
        pingpong_datatype_vector(p_core, gate_id, size, number_of_blocks, 1);
      }
    }
  }
  else {
    /* server */
    for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
      for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
        if (number_of_blocks > size) continue;

        pingpong_datatype_vector(p_core, gate_id, size, number_of_blocks, 0);
      }
    }
  }
 out:
  return 0;
}
