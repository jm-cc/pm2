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

#if defined CONFIG_DISK && defined CONFIG_SCHED_NULL
#  include <nm_disk_public.h>
#  include <nm_null_public.h>

const char *msg	= "hello, world";

int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        struct nm_pkt_wrap	*p_pw		= NULL;
        char			*r_url		= NULL;
        uint8_t			 drv_id		=    0;
        nm_gate_id_t		 gate_id	=    0;
        char			*buf		= NULL;
        uint64_t		 len;
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_null_load);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

        argc--;
        argv++;

        if (argc) {
                if (!strcmp(*argv, "-r")) {
                        r_url	= "w#f";
                } else {
                        r_url	= *argv;
                }

                printf("running as client using remote url: [%s]\n", r_url);

                argc--;
                argv++;
        } else {
                r_url	= "w#f";
        }

        err = nm_core_driver_load_init(p_core, nm_disk_load, &drv_id, NULL);

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

        if (r_url[0] == 'r') {
                printf("reading using url: [%s]\n", r_url);

                err = nm_core_gate_connect(p_core, gate_id, drv_id,
                                           "localhost", r_url);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect returned err = %d\n", err);
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
        } else if (r_url[0] == 'a' || r_url[0] == 'w') {
                if (r_url[0] == 'a') {
                        printf("appending using url: [%s]\n", r_url);
                } else {
                        printf("writing using url: [%s]\n", r_url);
                }

                /* client
                 */
                err = nm_core_gate_connect(p_core, gate_id, drv_id,
                                           "localhost", r_url);
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
        } else {
                printf("invalid file access mode in url\n");
        }

        for (;;) {
                err = nm_schedule(p_core);
                if (err != NM_ESUCCESS) {
                        printf("nm_schedule returned err = %d\n", err);
                }

                if (r_url[0] == 'r') {
                        printf("buffer contents: %s\n", buf);
                }

                printf("<schedule loop end>\n\n");
                sleep(2);
        }

 out:
        return 0;
}

#else

int
main(int	  argc,
     char	**argv) {
        printf("this program only works with the disk and null scheduler driver\n");

        return 0;
}

#endif
