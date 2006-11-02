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

#define MIN_SIZE   4
#define MAX_SIZE   1024 * 4
#define MIN_BLOCKS 2
#define MAX_BLOCKS 10
#define LOOPS      2000

#include "ping-optimized.h"
#include "indexed-optimized.h"

void pingpong_datatype_indexed(struct nm_core        *p_core,
                               uint8_t                gate_id,
                               int                    number_of_elements,
                               int                    number_of_blocks,
                               int                    client) {
  struct MPIR_DATATYPE *datatype_indexed = NULL;
  float                *buffer = NULL;
  float                *r_buffer = NULL;
  int                   i, k;

  /*
    Initialise data
  */
  buffer = malloc(number_of_elements * 2 * sizeof(float));
  memset(buffer, 0, number_of_elements);
  for(i=0 ; i<number_of_elements*2 ; i++) buffer[i] = i;

  datatype_indexed = malloc(sizeof(struct MPIR_DATATYPE));
  init_datatype_indexed(datatype_indexed, number_of_elements, number_of_blocks);
  r_buffer = malloc(number_of_elements * sizeof(float));

  if (client) {
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
    for(k = 0 ; k<LOOPS ; k++) {
      pack_datatype_indexed(p_core, gate_id, datatype_indexed, buffer);
      unpack_datatype_indexed(p_core, gate_id, r_buffer);
    }
    TBX_GET_TICK(t2);

    printf("%d\t%d\t%d\t%lf\n", MPIR_INDEXED, number_of_elements, number_of_blocks, TBX_TIMING_DELAY(t1, t2) / (2 * LOOPS));
    PRINTF("Received value: ");
    for(i=0 ; i<number_of_elements ; i++) PRINTF("%3.2f ", r_buffer[i]);
    PRINTF("\n");
  }
  else { /* server */
    for(k = 0 ; k<LOOPS ; k++) {
      unpack_datatype_indexed(p_core, gate_id, r_buffer);
      pack_datatype_indexed(p_core, gate_id, datatype_indexed, buffer);
    }
  }

  free(buffer);
  free(r_buffer);
  free(datatype_indexed);
}

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

        pingpong_datatype_indexed(p_core, gate_id, size, number_of_blocks, 1);
      }
    }
  }
  else {
    /* server */
    for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
      for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
        if (number_of_blocks > size) continue;

        pingpong_datatype_indexed(p_core, gate_id, size, number_of_blocks, 0);
      }
    }
  }
 out:
  return 0;
}
