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


#if defined CONFIG_MX && defined CONFIG_QSNET
#  include <nm_mx_public.h>
#  include <nm_qsnet_public.h>

#define SIZE  64 //(64 * 1024)

const char *msg_beg	= "hello", *msg_end = "world!";

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
        char			*l_url1		= NULL;
        char			*l_url2		= NULL;
        uint8_t			 drv1_id	=    0;
        uint8_t			 drv2_id	=    0;
        nm_gate_id_t		 gate_id	=    0;
        char			*buf		= NULL;
        char			*hostname	= "localhost";
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
                while (argc) {
                        if (!strcmp(*argv, "-h")) {
                                argc--;
                                argv++;

                                if (!argc)
                                        usage();

                                hostname = *argv;
                        } else {
                                r_url1	= r_url2;
                                r_url2	= *argv;
                        }

                        argc--;
                        argv++;
                }

                printf("running as client using remote url: %s [%s] [%s]\n", hostname, r_url1, r_url2);
        } else {
                /* no args: server */
                printf("running as server\n");
        }

        err = nm_core_driver_load_init(p_core, nm_mx_load, &drv1_id, &l_url1);
        printf("local url1: [%s]\n", l_url1);
        err = nm_core_driver_load_init(p_core, nm_qsnet_load, &drv2_id, &l_url2);
        printf("local url2: [%s]\n", l_url2);

        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_load_init returned err = %d\n", err);
                goto out;
        }

        err = nm_core_gate_init(p_core, &gate_id);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        buf = malloc(SIZE+1);
	memset(buf, 0, SIZE+1);

        if (!r_url1 && !r_url2) {
                /* server
                 */

                err = nm_core_gate_accept(p_core, gate_id, drv1_id, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept(drv1) returned err = %d\n", err);
                        goto out;
                }

                err = nm_core_gate_accept(p_core, gate_id, drv2_id, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept(drv2) returned err = %d\n", err);
                        goto out;
                }




		nm_so_begin_unpacking(interface, gate_id, 0, &cnx);

		nm_so_unpack(&cnx, buf, SIZE/2);
                nm_so_unpack(&cnx, buf + SIZE/2, SIZE/2);

		nm_so_end_unpacking(&cnx);

        } else {
                /* client
                 */
                err = nm_core_gate_connect(p_core, gate_id, drv1_id,
                                           r_url1);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect(drv1) returned err = %d\n", err);
                        goto out;
                }

                err = nm_core_gate_connect(p_core, gate_id, drv2_id,
                                           r_url2);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect(drv2) returned err = %d\n", err);
                        goto out;
                }


		{
		  char *src, *dst;

		  memset(buf, ' ', SIZE);
		  dst = buf;
		  src = (char *) msg_beg;
		  while(*src)
		    *dst++ = *src++;

		  dst = buf + SIZE - strlen(msg_end);
		  src = (char *) msg_end;
		  while(*src)
		    *dst++ = *src++;

		  //printf("Here's the message we're going to send : [%s]\n", buf);
		}


		nm_so_begin_packing(interface, gate_id, 0, &cnx);

		nm_so_pack(&cnx, buf, SIZE/2);
                nm_so_pack(&cnx, buf + SIZE/2, SIZE/2);

		nm_so_end_packing(&cnx);
        }

        if (!r_url1 && !r_url2) {
                printf("buffer contents: [%s]\n", buf);
        }

 out:
        return 0;
}

#else
int main(int argc, char **argv) {
        printf("This program requires Myrinet/MX and Quadrics networks subsystems\n");

        return 0;
}
#endif
