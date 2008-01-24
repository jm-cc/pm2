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

#include <nm_so_sendrecv_interface.h>

#include <nm_drivers.h>
#include "helper.h"

const char *msg	= "hello, world";

int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        char			*r_url1		= NULL;
        char			*r_url2		= NULL;
        char			*l_url		= NULL;
        uint8_t			 drv_id		=    0;
        nm_gate_id_t             gate_id1	=    0;
        nm_gate_id_t		 gate_id2	=    0;
        char			*s_buf		= NULL;
        char			*r_buf		= NULL;
        uint64_t		 len;
	struct nm_so_interface *interface = NULL;
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

        err = nm_so_sr_init(p_core, &interface);
        if(err != NM_ESUCCESS) {
          printf("nm_so_sr_init return err = %d\n", err);
          goto out;
        }

        argc--;
        argv++;

        if (argc) {
          /* client */

            r_url1	= *argv;
            argc--;
            argv++;


            if(argc){
              r_url2	= *argv;
              argc--;
              argv++;
              printf("running as client using remote url: %s %s\n", r_url1, r_url2);
            } else {
              printf("running as client using remote url: %s\n", r_url1);
            }

        } else {
          /* no args: server */
          printf("running as server\n");
        }

//#if defined CONFIG_MX
//        err = nm_core_driver_load_init(p_core, nm_mx_load, &drv_id, &l_url);
//#elif defined CONFIG_GM
//        err = nm_core_driver_load_init(p_core, nm_gm_load, &drv_id, &l_url);
//#elif defined CONFIG_QSNET
//        err = nm_core_driver_load_init(p_core, nm_qsnet_load, &drv_id, &l_url);
//#elif defined CONFIG_SISCI
//	err = nm_core_driver_load_init(p_core, nm_sisci_load, &drv_id, &l_url);
//#else
        err = nm_core_driver_load_init(p_core, load_driver("tcp"), &drv_id, &l_url);
//#endif
        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_load_init returned err = %d\n", err);
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
          err = nm_core_gate_accept(p_core, gate_id1, drv_id, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }

          err = nm_core_gate_accept(p_core, gate_id2, drv_id, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }

        } else if(!r_url2){
          // je fais un connect sur r_url1 dans la gate n°1
          err = nm_core_gate_connect(p_core, gate_id1, drv_id, r_url1);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
          }

          // je fais un accept pour r_url2 dans la gate n°2
          err = nm_core_gate_accept(p_core, gate_id2, drv_id, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }
        } else {
          // connect sur les 2 gates
          err = nm_core_gate_connect(p_core, gate_id1, drv_id, r_url1);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
          }

          err = nm_core_gate_connect(p_core, gate_id2, drv_id, r_url2);
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

        if(!r_url1 && !r_url2){
          nm_so_request request;

          // ping sur mon 1er voisin
          nm_so_sr_isend(interface, gate_id1, 0, s_buf, len, &request);
          nm_so_sr_swait(interface, request);

          nm_so_sr_isend(interface, gate_id2, 0, s_buf, len, &request);
          nm_so_sr_swait(interface, request);

        } else {
          nm_so_request request;
          int i = 0;

          nm_so_sr_irecv(interface, NM_SO_ANY_SRC, 0, r_buf, len, &request);

          while(i++ < 1000)
            ;

          nm_so_sr_rwait(interface, request);

          printf("buffer contents: %s\n", r_buf);
        }

 out:
        return 0;
}

// sur le noeud 1 : pm2-load 3-hello
// sur le noeud 2 : pm2-load 3-hello "url_1"
// sur le neoud 3 : pm2-load 3-hello "url_1" "url_2"

