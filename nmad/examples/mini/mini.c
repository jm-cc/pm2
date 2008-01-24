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
#if defined CONFIG_SCHED_MINI_ALT
#  include <nm_mini_alt_public.h>
#elif defined CONFIG_SCHED_OPT
#  include <nm_so_public.h>
#else
#  include <nm_mini_public.h>
#endif

#include <nm_drivers.h>

const char *msg	= "hello, world";

int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        struct nm_pkt_wrap	*p_pw		= NULL;
        char			*r_url		= NULL;
        char			*l_url		= NULL;
        uint8_t			 drv_id		=    0;
        nm_gate_id_t             gate_id	=    0;
        char			*buf		= NULL;
        uint64_t		 len;
        int err;

#if defined CONFIG_SCHED_MINI_ALT
        err = nm_core_init(&argc, argv, &p_core, nm_mini_alt_load);
#elif defined CONFIG_SCHED_OPT
        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
#else
        err = nm_core_init(&argc, argv, &p_core, nm_mini_load);
#endif
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

        argc--;
        argv++;

        if (argc) {
                r_url	= *argv;
                printf("running as client using remote url: [%s]\n", r_url);

                argc--;
                argv++;
        } else {
                printf("running as server\n");
        }

#if defined CONFIG_MX
        printf("MX\n");
        err = nm_core_driver_load_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
        printf("GM\n");
        err = nm_core_driver_load_init(p_core, nm_gm_load, &drv_id, &l_url);
#elif defined CONFIG_TCP
        printf("TCP\n");
        err = nm_core_driver_load_init(p_core, nm_tcp_load, &drv_id, &l_url);
#endif
        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_load_init returned err = %d\n", err);
                goto out;
        }

        err = nm_core_gate_init(p_core, &gate_id);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        len = 1+strlen(msg);
        buf = malloc((size_t)len);

        if (!r_url) {
            /* server
             */
                printf("local url: [%s]\n", l_url);

                err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept returned err = %d\n", err);
                        goto out;
                }

                memset(buf, 0, len);

                err	= nm_core_wrap_buffer(p_core,
                                              0,
                                              128,
                                              0,
                                              buf,
                                              len,
                                              &p_pw);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_wrap_buf returned err = %d\n", err);
                        goto out;
                }

                err	= nm_core_post_recv(p_core, p_pw);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_post_recv returned err = %d\n", err);
                        goto out;
                }
        } else {
                /* client
                 */
                err = nm_core_gate_connect(p_core, gate_id, drv_id, r_url);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect returned err = %d\n", err);
                        goto out;
                }

                strcpy(buf, msg);

                err	= nm_core_wrap_buffer(p_core,
                                              0,
                                              128,
                                              0,
                                              buf,
                                              len,
                                              &p_pw);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_wrap_buf returned err = %d\n", err);
                        goto out;
                }

                err	= nm_core_post_send(p_core, p_pw);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_post_send returned err = %d\n", err);
                        goto out;
                }
        }

        for (;;) {
                err = nm_schedule(p_core);
                if (err != NM_ESUCCESS) {
                        printf("nm_schedule returned err = %d\n", err);
                }

                if (!r_url) {
                        printf("buffer contents: %s\n", buf);
                }

                printf("<schedule loop end>\n\n");
                sleep(2);
        }

 out:
        return 0;
}

