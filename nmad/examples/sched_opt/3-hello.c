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

const char *msg	= "hello, world";

static
void
usage(void) {
        fprintf(stderr, "usage: hello [[-h <remote_hostname>] <remote url>]\n");
        exit(EXIT_FAILURE);
}

int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        char			*r_url1		= NULL;
        char			*r_url2		= NULL;
        char			*l_url		= NULL;
        uint8_t			 drv_id		=    0;
        uint8_t			 gate_id1	=    0;
        uint8_t			 gate_id2	=    0;
        char			*s_buf		= NULL;
        char			*r_buf		= NULL;
        char			*hostname	= "localhost";
        uint64_t		 len;
	struct nm_so_cnx         cnx;
	nm_so_pack_interface     interface;
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

	err = nm_so_pack_interface_init(p_core, &interface);
	if(err != NM_ESUCCESS) {
	  printf("nm_so_pack_interface_init return err = %d\n", err);
	  goto out;
	}

        argc--;
        argv++;

        if (argc) {
          /* client */

          if (!strcmp(*argv, "-h")) {
            argc--;
            argv++;

            if (!argc)
              usage();

            hostname = *argv;

          } else {

            r_url1	= *argv;
            argc--;
            argv++;


            if(argc){
              r_url2	= *argv;
              argc--;
              argv++;
              printf("running as client using remote url: %s [%s] [%s]\n", hostname, r_url1, r_url2);
            } else {
              printf("running as client using remote url: %s [%s]\n", hostname, r_url1);
            }
          }

        } else {
          /* no args: server */
          printf("running as server\n");
        }

#if defined CONFIG_MX
        err = nm_core_driver_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
        err = nm_core_driver_init(p_core, nm_gm_load, &drv_id, &l_url);
#elif defined CONFIG_QSNET
        err = nm_core_driver_init(p_core, nm_qsnet_load, &drv_id, &l_url);
#elif defined CONFIG_SISCI
	err = nm_core_driver_init(p_core, nm_sisci_load, &drv_id, &l_url);
#else
        err = nm_core_driver_init(p_core, nm_tcp_load, &drv_id, &l_url);
#endif
        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_init returned err = %d\n", err);
                goto out;
        }


        // ouverture de deux gates
        err = nm_core_gate_init(p_core, &gate_id1);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        err = nm_core_gate_init(p_core, &gate_id2);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        printf("local url: [%s]\n", l_url);

        if(!r_url1 && !r_url2){
          // accept sur les 2 gates
          err = nm_core_gate_accept(p_core, gate_id1, drv_id, NULL, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }

          err = nm_core_gate_accept(p_core, gate_id2, drv_id, NULL, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }

        } else if(!r_url2){
          // je fais un connect sur r_url1 dans la gate n�1
          err = nm_core_gate_connect(p_core, gate_id1, drv_id,
                                     hostname, r_url1);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
          }

          // je fais un accept pour r_url2 dans la gate n�2
          err = nm_core_gate_accept(p_core, gate_id2, drv_id, NULL, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }
        } else {
          // connect sur les 2 gates
          err = nm_core_gate_connect(p_core, gate_id1, drv_id,
                                     hostname, r_url1);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
          }

          err = nm_core_gate_connect(p_core, gate_id2, drv_id,
                                     hostname, r_url2);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
          }
        }

        // Initialisation des buffers d'E/S
        len = 1+strlen(msg);
        s_buf = malloc((size_t)len);
        r_buf = malloc((size_t)len);
        strcpy(s_buf, msg);
        memset(r_buf, 0, len);


        // ping sur mon 1er voisin
        nm_so_begin_packing(interface, gate_id1, 0, &cnx);

        nm_so_pack(&cnx, s_buf, len);

        nm_so_end_packing(&cnx);


        // pong (qui devrait venir du 2)
        nm_so_begin_unpacking(interface, -1, 0, &cnx);

        nm_so_unpack(&cnx, r_buf, len);

        nm_so_end_unpacking(&cnx);


        printf("buffer contents: %s\n", r_buf);

 out:
        return 0;
}

// sur le noeud 1 : pm2load 3-hello
// sur le noeud 2 : pm2load 3-hello "url_1"
// sur le neoud 3 : pm2load 3-hello "url_1" "url_2"

