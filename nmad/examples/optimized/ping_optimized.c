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

#if defined(CONFIG_MULTI_RAIL)
#if defined CONFIG_MX && defined CONFIG_QSNET
#  include <nm_mx_public.h>
#  include <nm_qsnet_public.h>
#else
#error "MX and QSNET must be available for multi rail"
#endif
#endif /* CONFIG_MULTI_RAIL */

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

#include "ping_optimized.h"
#include "indexed_optimized.h"
#include "vector_optimized.h"
#include "struct_optimized.h"

#if defined(CONFIG_MULTI_RAIL)
#define CLIENT_ARGUMENTS 4
#else
#define CLIENT_ARGUMENTS 3
#endif

int
main(int	  argc,
     char	**argv) {
  struct nm_core *p_core   = NULL;
  char		 *r_url	   = NULL;
  char		 *l_url	   = NULL;
  uint8_t	  drv_id   = 0;
#if defined(CONFIG_MULTI_RAIL)
  char		 *r_url2   = NULL;
  char		 *l_url2   = NULL;
  uint8_t	  drv_id2  = 0;
  int             err2;
#endif /* CONFIG_MULTI_RAIL */
  uint8_t	  gate_id  = 0;
  char		 *hostname = "localhost";
  int             err;

  char		 *datatype = NULL;
  int             size = 0;
  int             number_of_blocks = 0;

  int i=0;
  for(i=0 ; i<argc ; i++) fprintf(stderr, "argument[%d] = %s\n",i,argv[i]);

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
#if defined(CONFIG_MULTI_RAIL)
  err  = nm_core_driver_init(p_core, nm_mx_load, &drv_id, &l_url);
  printf("local url: [%s]\n", l_url);
  err2 = nm_core_driver_init(p_core, nm_qsnet_load, &drv_id2, &l_url2);
  printf("local url2: [%s]\n", l_url2);
  if (err2 != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err2);
  }
#else /* CONFIG_MULTI_RAIL */
#if defined CONFIG_MX
  err = nm_core_driver_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
  err = nm_core_driver_init(p_core, nm_gm_load, &drv_id, &l_url);
#elif defined CONFIG_QSNET
  err = nm_core_driver_init(p_core, nm_qsnet_load, &drv_id, &l_url);
#else
  err = nm_core_driver_init(p_core, nm_tcp_load, &drv_id, &l_url);
#endif
#endif /* CONFIG_MULTI_RAIL */

  printf("local url: [%s]\n", l_url);
  if (err != NM_ESUCCESS) {
    printf("nm_core_driver_init returned err = %d\n", err);
    goto out;
  }

  err = nm_core_gate_init(p_core, &gate_id);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out;
  }

  if (argc == CLIENT_ARGUMENTS) {
    /* client */
    argv++;
    r_url = *argv;
    argv++;
    datatype = *argv;
#if defined(CONFIG_MULTI_RAIL)
    argv++;
    r_url2 = *argv;
    printf("running as client using remote url: %s [%s] [%s]\n", hostname, r_url, r_url2);
#else
    printf("running as client using remote url: %s [%s]\n", hostname, r_url);
#endif /* CONFIG_MULTI_RAIL */

    err = nm_core_gate_connect(p_core, gate_id, drv_id, hostname, r_url);
    if (err != NM_ESUCCESS) {
      printf("nm_core_gate_connect returned err = %d\n", err);
      goto out;
    }
#if defined(CONFIG_MULTI_RAIL)
    err2 = nm_core_gate_connect(p_core, gate_id, drv_id2, hostname, r_url2);
    if (err2 != NM_ESUCCESS) {
      printf("nm_core_gate_connect returned err = %d\n", err2);
      goto out;
    }
#endif /* CONFIG_MULTI_RAIL */
    printf("Connection established\n");
  }
  else if (argc == 2) {
    /* server */
    argv++;
    datatype = *argv;
    printf("running as server\n");
    err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL, NULL);
    if (err != NM_ESUCCESS) {
      printf("nm_core_gate_accept returned err = %d\n", err);
      goto out;
    }
#if defined(CONFIG_MULTI_RAIL)
    err2 = nm_core_gate_accept(p_core, gate_id, drv_id2, NULL, NULL);
    if (err2 != NM_ESUCCESS) {
      printf("nm_core_gate_accept returned err = %d\n", err2);
      goto out;
    }
#endif /* CONFIG_MULTI_RAIL */
    printf("Connection established\n");
  }
  else {
    fprintf(stderr, "usage: argv[0] [<remote url>] <datatype>\n");
    exit(EXIT_FAILURE);
  }

  /*
    Ping-pong
  */
  if (argc == CLIENT_ARGUMENTS) {
    /* client */
    if (!strcmp(datatype, "index")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_indexed(p_core, gate_id, size, number_of_blocks, 1);
        }
      }
    }

    if (!strcmp(datatype, "vector")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_vector(p_core, gate_id, size, number_of_blocks, 1);
        }
      }
    }

    if (!strcmp(datatype, "struct")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        pingpong_datatype_struct(p_core, gate_id, size, 1);
      }
    }
  }
  else {
    /* server */
    if (!strcmp(datatype, "index")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_indexed(p_core, gate_id, size, number_of_blocks, 0);
        }
      }
    }

    if (!strcmp(datatype, "vector")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        for(number_of_blocks = MIN_BLOCKS ; number_of_blocks <= MAX_BLOCKS ; number_of_blocks++) {
          if (number_of_blocks > size) continue;
          pingpong_datatype_vector(p_core, gate_id, size, number_of_blocks, 0);
        }
      }
    }

    if (!strcmp(datatype, "struct")) {
      for(size=MIN_SIZE ; size<=MAX_SIZE ; size*=2) {
        pingpong_datatype_struct(p_core, gate_id, size, 0);
      }
    }
  }
 out:
  return 0;
}
