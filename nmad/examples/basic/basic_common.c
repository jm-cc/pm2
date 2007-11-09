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

#include "basic_common.h"

/* Choose a scheduler
 */
#if defined CONFIG_SCHED_MINI_ALT
#  include <nm_mini_alt_public.h>
#elif defined CONFIG_SCHED_OPT
#  include <nm_so_public.h>
#else
#  include <nm_mini_public.h>
#endif

/* Choose an API
 */
#include <nm_basic_public.h>

/* Choose a driver
 */
#include <nm_drivers.h>

#include <tbx.h>

static
void
usage(void) {
        fprintf(stderr, "usage: <prog> [[-h <remote_hostname>] <remote url>]\n");
        exit(EXIT_FAILURE);
}

/* wrapper to initialize basic point 2 point example sessions
   returns 0 for server, 1 for client
 */
int
bc_core_init(int	  	 *p_argc,
             char		**argv,
             struct nm_core	**pp_core,
             struct nm_proto	**pp_proto,
             nm_gate_id_t	 *p_gate_id) {
        struct nm_core		*p_core		= NULL;
        char			*r_url		= NULL;
        char			*l_url		= NULL;
        uint8_t			 drv_id		=    0;
        char			*hostname	= "localhost";
        int argc;
        int err;

#if defined CONFIG_SCHED_MINI_ALT
        err = nm_core_init(p_argc, argv, pp_core, nm_mini_alt_load);
#elif defined CONFIG_SCHED_OPT
        err = nm_core_init(p_argc, argv, pp_core, nm_so_load);
#else
        err = nm_core_init(p_argc, argv, pp_core, nm_mini_load);
#endif
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                exit(EXIT_FAILURE);
        }

        p_core	= *pp_core;
        argc	= *p_argc;

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

                printf("running as client using remote url: %s\"%s\"\n", hostname, r_url);
        } else {
                /* no args: server */
                printf("running as server\n");
        }

        *p_argc	= argc;

        err = nm_core_proto_init(p_core, nm_basic_load, pp_proto);
        if (err != NM_ESUCCESS) {
                printf("nm_core_proto_init returned err = %d\n", err);
                exit(EXIT_FAILURE);
        }

#if defined CONFIG_IBVERBS
        err = nm_core_driver_load_init(p_core, nm_ibverbs_load, &drv_id, &l_url);
#elif defined CONFIG_MX
        err = nm_core_driver_load_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
        err = nm_core_driver_load_init(p_core, nm_gm_load, &drv_id, &l_url);
#elif defined CONFIG_QSNET
        err = nm_core_driver_load_init(p_core, nm_qsnet_load, &drv_id, &l_url);
#elif defined CONFIG_SISCI
        err = nm_core_driver_load_init(p_core, nm_sisci_load, &drv_id, &l_url);
#elif defined CONFIG_TCP
        err = nm_core_driver_load_init(p_core, nm_tcpdg_load, &drv_id, &l_url);
#endif

        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_load_init returned err = %d\n", err);
                exit(EXIT_FAILURE);
        }

        err = nm_core_gate_init(p_core, p_gate_id);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                exit(EXIT_FAILURE);
        }

        if (!r_url) {
                /* server
                 */
                printf("local url: \"%s\"\n", l_url);

                err = nm_core_gate_accept(p_core, *p_gate_id, drv_id, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept returned err = %d\n", err);
                        exit(EXIT_FAILURE);
                }

        } else {
                /* client
                 */
                err = nm_core_gate_connect(p_core, *p_gate_id, drv_id, r_url);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect returned err = %d\n", err);
                        exit(EXIT_FAILURE);
                }

        }

        return !r_url;
}
