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

#include <nm_so_interface.h>

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

#define MAX     (8*1024)
#define LOOPS   1000

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
        char			*r_url		= NULL;
        char			*l_url		= NULL;
        uint8_t			 drv_id		=    0;
        uint8_t			 gate_id	=    0;
        char			*buf		= NULL;
        char			*hostname	= "localhost";
        uint32_t		 len;
	struct nm_so_cnx        *cnx            = NULL;
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

        argc--;
        argv++;

        if (argc) {
                /* client */
                while (argc) {
                        if (!strcmp(*argv, "-h")) {
                                argc--;
                                argv++;

                                if (!argc)
                                        usage();

                                hostname = *argv;
                        } else {
                                r_url	= *argv;
                        }

                        argc--;
                        argv++;
                }

                printf("running as client using remote url: %s[%s]\n", hostname, r_url);
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

        buf = malloc(MAX);
	memset(buf, 0, MAX);

        if (!r_url) {
	  int k;
                /* server
                 */
                printf("local url: [%s]\n", l_url);

                err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept returned err = %d\n", err);
                        goto out;
                }

		for(len = 8; len <= MAX; len *= 2) {
		  for(k = 0; k < LOOPS; k++) {

		    nm_so_begin_unpacking(p_core, gate_id, 0, &cnx);
		    nm_so_unpack(cnx, buf, len/2);
		    nm_so_unpack(cnx, buf+len/2, len/2);
		    nm_so_end_unpacking(p_core, cnx);

		    nm_so_begin_packing(p_core, gate_id, 0, &cnx);
		    nm_so_pack(cnx, buf, len/2);
		    nm_so_pack(cnx, buf+len/2, len/2);
		    nm_so_end_packing(p_core, cnx);

		  }
		}

        } else {
	  tbx_tick_t t1, t2;
	  int k;
                /* client
                 */
                err = nm_core_gate_connect(p_core, gate_id, drv_id,
                                           hostname, r_url);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect returned err = %d\n", err);
                        goto out;
                }

		for(len = 8; len <= MAX; len *= 2) {

		  TBX_GET_TICK(t1);

		  for(k = 0; k < LOOPS; k++) {
		    nm_so_begin_packing(p_core, gate_id, 0, &cnx);
		    nm_so_pack(cnx, buf, len/2);
		    nm_so_pack(cnx, buf+len/2, len/2);
		    nm_so_end_packing(p_core, cnx);

		    nm_so_begin_unpacking(p_core, gate_id, 0, &cnx);
		    nm_so_unpack(cnx, buf, len/2);
		    nm_so_unpack(cnx, buf+len/2, len/2);
		    nm_so_end_unpacking(p_core, cnx);
		  }

		  TBX_GET_TICK(t2);

		  printf("%d\t%lf\n", len, TBX_TIMING_DELAY(t1, t2)/(2*LOOPS));
		}
        }

 out:
        return 0;
}
