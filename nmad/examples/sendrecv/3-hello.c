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
#include <nm_sendrecv_interface.h>
#include "helper.h"

const char *msg	= "hello, world";

#ifdef CONFIG_PROTO_MAD3
int main(int argc, char **argv) {
        printf("This program does not work with the protocol mad3\n");
	exit(0);
}
#else
int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        char			*r_url1		= NULL;
        char			*r_url2		= NULL;
        char			*l_url		= NULL;
        nm_drv_id_t		 drv_id		=    0;
        nm_gate_t		 gate1   	=    0;
        nm_gate_t		 gate2   	=    0;
        char			*s_buf		= NULL;
        char			*r_buf		= NULL;
        uint64_t		 len;
        int err;

        err = nm_core_init(&argc, argv, &p_core);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
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

#if defined CONFIG_MX
        err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "mx"), &drv_id, &l_url);
#elif defined CONFIG_GM
        err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "gm"), &drv_id, &l_url);
#elif defined CONFIG_QSNET
        err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "qsnet"), &drv_id, &l_url);
#elif defined CONFIG_SISCI
	err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "sisci"), &drv_id, &l_url);
#elif defined CONFIG_IBVERBS
	err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "ibverbs"), &drv_id, &l_url);

#elif defined CONFIG_TCP
        err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "tcp"), &drv_id, &l_url);
#endif
        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_load_init returned err = %d\n", err);
                goto out;
        }


        // ouverture de deux gates
        err = nm_core_gate_init(p_core, &gate1);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        err = nm_core_gate_init(p_core, &gate2);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        printf("local url: [%s]\n", l_url);

        if(!r_url1 && !r_url2){
          // accept sur les 2 gates
          err = nm_core_gate_accept(p_core, gate1, drv_id, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }

          err = nm_core_gate_accept(p_core, gate2, drv_id, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }

        } else if(!r_url2){
          // je fais un connect sur r_url1 dans la gate n�1
          err = nm_core_gate_connect(p_core, gate1, drv_id, r_url1);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
          }

          // je fais un accept pour r_url2 dans la gate n�2
          err = nm_core_gate_accept(p_core, gate2, drv_id, NULL);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
          }
        } else {
          // connect sur les 2 gates
          err = nm_core_gate_connect(p_core, gate1, drv_id, r_url1);
          if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
          }

          err = nm_core_gate_connect(p_core, gate2, drv_id, r_url2);
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
          nm_sr_request_t request;

          // ping vers 1
          nm_sr_isend(p_core, gate1, 0, s_buf, len, &request);
          nm_sr_swait(p_core, &request);

          // pong de 2
          nm_sr_irecv(p_core, NM_ANY_GATE, 0, r_buf, len, &request);
          nm_sr_rwait(p_core, &request);

        } else {
          nm_sr_request_t request;
          nm_gate_t gate = NM_ANY_GATE;

          //pong de 0
          nm_sr_irecv(p_core, NM_ANY_GATE, 0, r_buf, len, &request);
          nm_sr_rwait(p_core, &request);

          nm_sr_recv_source(p_core, &request, &gate);

          if(gate == gate1){
             nm_sr_isend(p_core, gate2, 0, s_buf, len, &request);
             nm_sr_swait(p_core, &request);
          } else {
            nm_sr_isend(p_core, gate1, 0, s_buf, len, &request);
            nm_sr_swait(p_core, &request);
          }
        }

        printf("buffer contents: %s\n", r_buf);

 out:
        return 0;
}

// sur le noeud 1 : pm2-load 3-hello
// sur le noeud 2 : pm2-load 3-hello "url_1"
// sur le neoud 3 : pm2-load 3-hello "url_1" "url_2"

#endif
